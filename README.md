EntityHero - Doom Eternal entity editor tool.

Cloning:
1. git clone https://github.com/nopjne/EntityHero
3. Download prebuilt wxWidget binaries: https://github.com/nopjne/EntityHero/releases/download/v0.4/wxWidget.zip (Or build yourself from https://www.wxwidgets.org/)
4. mkdir ./wxWidget
5. Extract wxWidget.zip into ./wxWidget
6. start wxGUI/wxGUI.sln
7. build solution

Contributing:
Create a pull request and it will be looked through.

Features:
1. Displays .entities files in a treeview that can be modified
2. Supports opening .entities files and exporting/importing to/from text
3. Supports opening and browsing .resources files.
4. Supports reinjecting entities files into resources files.
5. Auto spawn resolution of spawn groups and AI2 class reference - When changing a SPAWN_ENCOUNTER to a different type the tool automatically checks whether the spawngroup is able to support the new type and can adjust the spawn group automatically.
6. Automatic array numbering and renumbering - EntityHero removes the need to number arrays any "item[x]" nodes are hidden from the user and get auto renumbered during save.
7. Supports copying nodes to clipboard which is converted to text on the fly.
8. Supports pasting nodes in text form from clipboard directly.
9. Supports opening .resources files and can automatically find the .entities file - no need to extract .resources files manually anymore.
10. Interfaces with oodle to support compressed .entities files.
11. Automatically searches and correlates values to possible key names to make editing easier and faster - with hints during value modification.
12. Meathook interface:
13. Get spawnPosition and spawnRotation from meathook just by right clicking spawnPosition or rotation and selecting "Get from MH"
14. Open currently loaded level from meathook. Open from MH - allows a level to be loaded into EntityHero directly - no need to browse .resources or find out which update# the file is in.
15. Reload level directly while the game is running - allows EntityHero to push the current open entities file back into Meathook 
16. Goto current encounter - retreives the currently ongoing encounter from meathook and finds the appropriate entityDef in the currently open .entities file.

Usage:
1. File -> Open - Allows the user to open a .resources or .entities (both compressed or decompressed) file. (NOTE Saving after loading an uncompressed .entities file will also save uncompressed)
2. File -> Open from MH - Uses the meathook interface to open the currently loaded .entities file 
3. File -> Export/import from text - Allows forced uncompressed .entities file write
4. Edit -> Undo/Redo last operation.
5. Edit -> Navigate back/forward to previously selected node.
6. Edit -> Copy/Paste from clipboard.
7. QuickFind -> Useful entityDef names.
8. ResourceView -> Browse and search a .resources file without having to extract it.