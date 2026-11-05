"""Tick-box dialog for choosing which models of a pack to import (used by gp_prop_importer).

Kept in its own module because it defines an Unreal class: gp_prop_importer is reloaded on every
menu click, this one is imported once per editor session.
"""
import unreal


@unreal.uclass()
class GPPropPick(unreal.Object):
    """Shown with the editor's own details view: one tick box per model, with the search box on top."""
    pack = unreal.uproperty(str, meta={"DisplayName": "Pack", "Category": "Import"})
    already_imported = unreal.uproperty(str, meta={"DisplayName": "Already imported (not listed)", "Category": "Import"})
    models = unreal.uproperty(unreal.Map(str, bool), meta={"DisplayName": "Tick the models to import", "Category": "Import"})


def ask(pack, choices, already):
    """choices: {model name: ticked by default}. Returns the ticked names, or None if cancelled."""
    form = GPPropPick()
    form.set_editor_property("pack", pack)
    form.set_editor_property("already_imported", ", ".join(sorted(already)) or "-")
    m = unreal.Map(str, bool)
    for name in sorted(choices):
        m[name] = bool(choices[name])
    form.set_editor_property("models", m)
    options = unreal.EditorDialogLibraryObjectDetailsViewOptions()
    options.set_editor_property("show_object_name", False)
    options.set_editor_property("allow_search", True)
    options.set_editor_property("allow_resizing", True)
    options.set_editor_property("min_width", 520)
    options.set_editor_property("min_height", 640)
    if not unreal.EditorDialog.show_object_details_view(f"Import Props: {pack}", form, options):
        return None
    edited = form.get_editor_property("models")
    return {k for k in edited.keys() if edited[k]}
