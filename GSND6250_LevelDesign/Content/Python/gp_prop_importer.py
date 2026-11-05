"""Batch-import downloaded props (.glb / .gltf / .obj; loose, in folders or zipped) from an inbox folder.

Menu: GP Tools > Import Props from Inbox (registered by init_unreal.py).

- Inbox: ~/Downloads/props_inbox. Drop model files, zips or folders there.
- Target: <current Content Browser folder>/Props/ (the current folder itself if it is a "Props" folder).
  - a loose model file   -> Props/<file name>/
  - a zip or a folder    -> Props/<pack name>/, sharing materials and textures
- Packs open a tick-box dialog first (the editor's details view, with search): tick the models to import.
  A folder you made starts all ticked (you already picked its contents); a zip starts all ticked if it
  has <= SMALL_PACK models, else all unticked. The last choice for a pack is remembered in
  <inbox>/_picks/<pack>.txt and ticked again next time. Cancel skips that pack.
- A model whose Static Mesh already exists in the target folder is not offered again and never
  overwritten, so a big pack can be picked from in several rounds. A pack stays in the inbox until
  every model in it has been imported.
- Same model in several formats (packs often ship glTF + OBJ + FBX): one is imported, glb > gltf > obj.
- Material per file, decided from its contents:
  - glTF with vertex colours (COLOR_0) and no textures: the file's materials are not imported;
    every slot gets M_GLB_VertexColor (Props/Materials, created on first use).
  - anything else (textured glTF, obj + mtl): materials and textures imported as usual.
- Each model file becomes one combined Static Mesh.
- Imported sources move to <inbox>/_imported/<date>/; a report goes to <inbox>/_imported/last_report.json.

Pipelines are project copies of the engine's default Interchange pipelines, in /Game/Tools/PropImport
(IP_Props_VertexColor, IP_Props_GLTF, IP_Props_OBJ), so their settings can be checked in the editor.
"""
import datetime
import json
import os
import shutil
import struct
import zipfile

import unreal

INBOX = os.path.join(os.path.expanduser("~"), "Downloads", "props_inbox")
MODEL_EXTS = (".glb", ".gltf", ".obj")          # preference order when a model ships in several formats
PIPELINE_DIR = "/Game/Tools/PropImport"
PIPELINES = {   # mode: (engine pipeline to copy, project copy, import materials)
    "vertexcolor": ("/Interchange/Pipelines/DefaultGLTFAssetsPipeline", "IP_Props_VertexColor", False),
    "gltf": ("/Interchange/Pipelines/DefaultGLTFAssetsPipeline", "IP_Props_GLTF", True),
    "obj": ("/Interchange/Pipelines/DefaultFBXOBJAssetsPipeline", "IP_Props_OBJ", True),
}
# Engine pipelines that run after ours, as in the engine's default glTF stack (BaseEngine.ini):
# DefaultGLTFPipeline turns glTF materials into instances with their colours and textures.
# Without it imported glTF materials keep no parameters and render black.
EXTRA_PIPELINES = {"gltf": ["/Interchange/Pipelines/DefaultGLTFPipeline.DefaultGLTFPipeline"]}
VERTEX_COLOR_MATERIAL = "M_GLB_VertexColor"
SMALL_PACK = 10                                 # packs up to this size start with every model ticked

EAL = unreal.EditorAssetLibrary


def log(msg):
    unreal.log(f"[PropImport] {msg}")


# ------------------------------------------------------------------ what kind of file is it
def gltf_json(path):
    with open(path, "rb") as f:
        data = f.read()
    if path.lower().endswith(".glb"):
        length = struct.unpack("<I", data[12:16])[0]          # first chunk of a .glb is the JSON
        return json.loads(data[20:20 + length])
    return json.loads(data.decode("utf-8"))


def mode_for(model):
    """'vertexcolor' for glTF coloured per vertex without textures, else 'gltf' / 'obj'."""
    if model.lower().endswith(".obj"):
        return "obj"
    try:
        j = gltf_json(model)
    except Exception:
        return "gltf"
    has_vc = any("COLOR_0" in p.get("attributes", {}) for m in j.get("meshes", []) for p in m.get("primitives", []))
    has_tex = bool(j.get("images")) or any("baseColorTexture" in m.get("pbrMetallicRoughness", {})
                                           for m in j.get("materials", []))
    return "vertexcolor" if has_vc and not has_tex else "gltf"


# ------------------------------------------------------------------ pipelines
def _configure(pipeline, import_materials):
    """Settings on top of the engine defaults (property names from the 5.8 Interchange pipeline headers)."""
    common = pipeline.get_editor_property("common_meshes_properties")
    common.set_editor_property("force_all_mesh_as_type", unreal.InterchangeForceMeshType.IFMT_STATIC_MESH)
    common.set_editor_property("vertex_color_import_option", unreal.InterchangeVertexColorImportOption.IVCIO_REPLACE)
    meshes = pipeline.get_editor_property("mesh_pipeline")
    meshes.set_editor_property("combine_static_meshes_behavior", unreal.InterchangeCombineStaticMeshesBehavior.ALL)
    pipeline.get_editor_property("material_pipeline").set_editor_property("import_materials", import_materials)


def pipeline_path(mode):
    """Project copy of the engine pipeline, created and configured on first use."""
    src, name, import_materials = PIPELINES[mode]
    path = f"{PIPELINE_DIR}/{name}"
    if not EAL.does_asset_exist(path):
        if not EAL.duplicate_asset(src, path):
            raise RuntimeError(f"could not copy {src} to {path}")
        _configure(EAL.load_asset(path), import_materials)
        EAL.save_asset(path, only_if_is_dirty=False)
        log(f"created pipeline {path}")
    return unreal.SoftObjectPath(f"{path}.{name}")


# ------------------------------------------------------------------ vertex-colour material
def vertex_color_material(props_root):
    """Lit material, Base Colour = Vertex Color; roughness/metallic of the low-poly GLBs' own material."""
    folder = f"{props_root}/Materials"
    path = f"{folder}/{VERTEX_COLOR_MATERIAL}"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    mel = unreal.MaterialEditingLibrary
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        VERTEX_COLOR_MATERIAL, folder, unreal.Material, unreal.MaterialFactoryNew())
    vc = mel.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -400, 0)
    mel.connect_material_property(vc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for value, prop, y in ((0.92, unreal.MaterialProperty.MP_ROUGHNESS, 200),
                           (0.02, unreal.MaterialProperty.MP_METALLIC, 300)):
        c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, y)
        c.set_editor_property("r", value)
        mel.connect_material_property(c, "", prop)
    mel.recompile_material(mat)
    EAL.save_asset(path, only_if_is_dirty=False)
    log(f"created {path}")
    return mat


# ------------------------------------------------------------------ inbox
def _models_in(folder):
    """One file per model name, preferring glb > gltf > obj."""
    best = {}
    for root, _, files in os.walk(folder):
        for f in files:
            stem, ext = os.path.splitext(f)
            ext = ext.lower()
            if ext in MODEL_EXTS:
                rank = MODEL_EXTS.index(ext)
                if stem not in best or rank < best[stem][0]:
                    best[stem] = (rank, os.path.join(root, f))
    return [best[k][1] for k in sorted(best)]


def _unzip(zip_path, out):
    with zipfile.ZipFile(zip_path) as z:
        base = os.path.realpath(out) + os.sep
        for member in z.namelist():                       # data only; refuse paths that escape the folder
            if not os.path.realpath(os.path.join(out, member)).startswith(base):
                raise RuntimeError(f"unsafe path in {os.path.basename(zip_path)}: {member}")
        z.extractall(out)


def collect_items():
    """[(name, [model files], [paths to move when done], is_pack)] from the top level of the inbox."""
    items = []
    for entry in sorted(os.listdir(INBOX)):
        full = os.path.join(INBOX, entry)
        stem, ext = os.path.splitext(entry)
        if entry.startswith("_"):
            continue
        if os.path.isfile(full) and ext.lower() in MODEL_EXTS:
            moves = [full] + [os.path.join(INBOX, stem + e) for e in (".mtl", ".bin")
                              if os.path.exists(os.path.join(INBOX, stem + e))]
            items.append((stem, [full], moves, False))
        elif os.path.isfile(full) and ext.lower() == ".zip":
            out = os.path.join(INBOX, "_unzipped", stem)
            if not os.path.isdir(out):
                _unzip(full, out)
            items.append((stem, _models_in(out), [full, out], True))
        elif os.path.isdir(full):
            items.append((entry, _models_in(full), [full], True))
    return [i for i in items if i[1]]


def model_name(path):
    return os.path.splitext(os.path.basename(path))[0]


def _norm(name):
    """Compare model and asset names loosely: the importer may change case or punctuation."""
    return "".join(c for c in name.lower() if c.isalnum())


def pick_file(pack):
    return os.path.join(INBOX, "_picks", _asset_name(pack) + ".txt")


def read_pick(pack):
    try:
        with open(pick_file(pack), encoding="utf-8") as f:
            return {line.strip() for line in f if line.strip() and not line.startswith("#")}
    except OSError:
        return None


def write_pick(pack, names):
    os.makedirs(os.path.dirname(pick_file(pack)), exist_ok=True)
    with open(pick_file(pack), "w", encoding="utf-8") as f:
        f.write("# models ticked for this pack the last time it was imported\n")
        f.write("".join(n + "\n" for n in sorted(names)))


def default_ticks(names, previous, from_zip=True):
    """{name: ticked}: the last choice if there is one; else a folder you made is all ticked,
    a zip is all ticked when small and all unticked when big."""
    if previous is not None:
        return {n: n in previous for n in names}
    return {n: (not from_zip) or len(names) <= SMALL_PACK for n in names}


def chosen(names, ticked):
    """Keep only real model names: the dialog lets the user edit map keys, unknown ones are ignored."""
    return [n for n in names if n in ticked]


def _free_path(path):
    """path, or path_2, path_3 ... if it exists: importing the same pack twice in a day must not nest
    the second copy inside the first or fail on the move."""
    stem, ext = os.path.splitext(path)
    n = 2
    while os.path.exists(path):
        path = f"{stem}_{n}{ext}"
        n += 1
    return path


def _asset_name(name):
    return "".join(c if c.isalnum() or c in "-_" else "_" for c in name).strip("_") or "Prop"


# ------------------------------------------------------------------ import
def import_model(model, folder, mode):
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated", True)
    params.set_editor_property("replace_existing", False)
    params.set_editor_property("override_pipelines", [pipeline_path(mode)] +
                               [unreal.SoftObjectPath(p) for p in EXTRA_PIPELINES.get(mode, [])])
    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    before = set(EAL.list_assets(folder, recursive=True)) if EAL.does_directory_exist(folder) else set()
    result = manager.import_asset(folder, unreal.InterchangeManager.create_source_data(model), params)
    ok = result[0] if isinstance(result, tuple) else result
    new = [p for p in EAL.list_assets(folder, recursive=True) if p not in before]
    meshes = [m for m in (EAL.load_asset(p) for p in new) if isinstance(m, unreal.StaticMesh)]
    return bool(ok), meshes


def mesh_info(mesh):
    box = mesh.get_bounding_box()
    size = box.max - box.min
    return {"asset": mesh.get_path_name().split(".")[0],
            "size_cm": [round(size.x), round(size.y), round(size.z)],
            "pivot_above_bottom_cm": round(-box.min.z),
            "triangles": mesh.get_num_triangles(0),
            "materials": [m.material_interface.get_name() if m.material_interface else None
                          for m in mesh.get_editor_property("static_materials")]}


def existing_meshes(folder):
    """Normalised names of the Static Meshes already in the target folder."""
    if not EAL.does_directory_exist(folder):
        return set()
    names = set()
    for p in EAL.list_assets(folder, recursive=True):
        data = EAL.find_asset_data(p)
        if str(data.asset_class_path.asset_name) == "StaticMesh":
            names.add(_norm(str(data.asset_name)))
    return names


def plan(items, root):
    """Decide what to import before any import starts (dialogs first, then one progress bar).
    Returns [(name, folder, [models to import], moves, done_already)]; Cancel on a pack skips it."""
    import gp_prop_pick_ui
    jobs = []
    for name, models, moves, is_pack in items:
        folder = f"{root}/{_asset_name(name)}"
        have = existing_meshes(folder)
        todo = [m for m in models if _norm(model_name(m)) not in have]
        done = [model_name(m) for m in models if _norm(model_name(m)) in have]
        if todo and is_pack:
            names = [model_name(m) for m in todo]
            ticked = gp_prop_pick_ui.ask(name, default_ticks(names, read_pick(name), moves[0].lower().endswith(".zip")), done)
            if ticked is None:
                log(f"{name}: cancelled")
                continue
            keep = set(chosen(names, ticked))
            write_pick(name, keep)
            todo = [m for m in todo if model_name(m) in keep]
        jobs.append((name, folder, todo, moves, done, len(models)))
    return jobs


def props_root():
    res = unreal.EditorUtilityLibrary.get_current_content_browser_path()
    ok, path = res if isinstance(res, tuple) else (bool(res), res)
    if not ok or not str(path).strip("/"):
        return None                                  # no Content Browser folder: never guess a target
    return props_root_for(str(path))


def props_root_for(path):
    """Content Browser folder -> Props folder. Inside a Props folder (e.g. browsing Props/Saferoom/bed)
    that Props folder is used, so the target does not depend on how deep you happen to be."""
    path = "/" + path.strip("/")
    if path.startswith("/All/"):                      # virtual path the Content Browser can report
        path = path[len("/All"):]
    if not path.startswith("/Game"):
        path = "/Game" + path
    parts = path.split("/")
    if "Props" in parts:
        return "/".join(parts[:parts.index("Props") + 1])
    return path + "/Props"


def run():
    os.makedirs(INBOX, exist_ok=True)
    root = props_root()
    if root is None:
        unreal.EditorDialog.show_message(
            "Import Props", "Could not read the Content Browser folder.\n"
            "Open the Content Browser at the folder to import into (e.g. GP5_FromTheShadows), click inside it, "
            "then run Import Props again.", unreal.AppMsgType.OK)
        return
    log(f"target {root}")
    items = collect_items()
    if not items:
        unreal.EditorDialog.show_message("Import Props", f"No .glb/.gltf/.obj in the inbox:\n{INBOX}",
                                         unreal.AppMsgType.OK)
        return
    jobs = plan(items, root)
    done_dir = os.path.join(INBOX, "_imported", datetime.date.today().isoformat())
    total = max(1, sum(len(todo) for _, _, todo, _, _, _ in jobs))
    report, vc_mat = [], None
    with unreal.ScopedSlowTask(total, "Importing props") as task:
        task.make_dialog(True)
        for name, folder, todo, moves, done, count in jobs:
            row = {"name": name, "folder": folder, "already_imported": done, "models": []}
            report.append(row)
            imported = len(done)
            for model in todo:
                if task.should_cancel():
                    break
                task.enter_progress_frame(1, os.path.basename(model))
                mode = mode_for(model)
                entry = {"source": os.path.relpath(model, INBOX), "mode": mode}
                try:
                    ok, meshes = import_model(model, folder, mode)
                    if not ok or not meshes:
                        raise RuntimeError("no static mesh imported")
                    for mesh in meshes:
                        if mode == "vertexcolor":
                            vc_mat = vc_mat or vertex_color_material(root)
                            for i in range(len(mesh.get_editor_property("static_materials"))):
                                mesh.set_material(i, vc_mat)
                        EAL.save_asset(mesh.get_path_name().split(".")[0], only_if_is_dirty=False)
                    entry["meshes"] = [mesh_info(m) for m in meshes]
                    entry["status"] = "ok"
                    imported += 1
                except Exception as e:                   # keep going; the report says what failed
                    entry["status"] = f"failed: {e}"
                row["models"].append(entry)
                log(f"{name}/{os.path.basename(model)} [{mode}]: {entry['status']}")
            if EAL.does_directory_exist(folder):
                EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)   # shared materials / textures
            failed = sum(1 for e in row["models"] if e["status"] != "ok")
            row["status"] = (f"{len(row['models']) - failed} imported, {failed} failed, "
                             f"{imported}/{count} of the pack in UE")
            if imported == count:                        # everything in UE: the source is done with
                os.makedirs(done_dir, exist_ok=True)
                for p in moves:
                    if os.path.exists(p):
                        shutil.move(p, _free_path(os.path.join(done_dir, os.path.basename(p))))
                row["status"] += ", source moved to _imported"
    os.makedirs(os.path.join(INBOX, "_imported"), exist_ok=True)
    with open(os.path.join(INBOX, "_imported", "last_report.json"), "w", encoding="utf-8") as f:
        json.dump({"target": root, "time": datetime.datetime.now().isoformat(timespec="seconds"), "items": report},
                  f, ensure_ascii=False, indent=1)
    lines = [f"{r['name']}: {r['status']}" for r in report] or ["nothing imported (cancelled)"]
    unreal.EditorDialog.show_message("Import Props", f"Target: {root}\n\n" + "\n".join(lines), unreal.AppMsgType.OK)


def open_inbox():
    os.makedirs(INBOX, exist_ok=True)
    os.startfile(INBOX)
