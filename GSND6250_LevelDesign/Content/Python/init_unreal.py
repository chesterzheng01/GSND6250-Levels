"""Runs when the editor starts (Content/Python/init_unreal.py). Adds the GP Tools menu.

To load it without restarting: in the editor console type
    py "<project>/Content/Python/init_unreal.py"
"""
import os
import sys

import unreal

# UE puts Content/Python on sys.path only if it existed when the editor started; make sure it is there
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.append(_HERE)


@unreal.uclass()
class GPImportPropsEntry(unreal.ToolMenuEntryScript):
    @unreal.ufunction(override=True)
    def execute(self, context):
        import importlib
        import gp_prop_importer
        importlib.reload(gp_prop_importer)          # pick up edits without restarting the editor
        gp_prop_importer.run()


@unreal.uclass()
class GPOpenInboxEntry(unreal.ToolMenuEntryScript):
    @unreal.ufunction(override=True)
    def execute(self, context):
        import gp_prop_importer
        gp_prop_importer.open_inbox()


def register_menu():
    menus = unreal.ToolMenus.get()
    main = menus.find_menu("LevelEditor.MainMenu")
    if main is None:
        return
    menu = menus.find_menu("LevelEditor.MainMenu.GPTools") or main.add_sub_menu(
        "GPTools", "", "GPTools", "GP Tools", "GSND 6250 helper tools")
    for cls, name, label, tip in (
            (GPImportPropsEntry, "ImportProps", "Import Props from Inbox",
             "Import every .glb/.gltf/.obj (or zip) in Downloads/props_inbox into <current Content Browser folder>/Props"),
            (GPOpenInboxEntry, "OpenInbox", "Open Props Inbox", "Open Downloads/props_inbox in Explorer")):
        entry = cls()
        entry.init_entry("GPTools", "LevelEditor.MainMenu.GPTools", "Props", name, label, tip)
        menu.add_menu_entry_object(entry)
    menus.refresh_all_widgets()


# This file runs for everyone who opens the project, including headless commandlets
# (-run=pythonscript, which have no menus): a failure here must only warn, never break startup.
try:
    register_menu()
except Exception as e:
    unreal.log_warning(f"[GP Tools] menu not registered: {e}")
