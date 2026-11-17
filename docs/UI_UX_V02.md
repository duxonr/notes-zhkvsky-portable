# Notes ZHKVSKY Portable — UI/UX v0.2

## Scope

Based on upstream Notes 2.3.1 and local v0.1.0. C++/Qt, editor algorithms,
database schema and the existing move/delete/tag handlers are retained.

* Folder hover controls: create subfolder and existing confirmed delete. Rename
  remains available by double-clicking the name or through the context menu.
* Tag hover controls: existing palette and existing tag removal. Rename remains
  available by double-clicking the name or through the context menu.
* Hovering a folder/tag counter replaces its number with three dots in the same
  32 px area. Clicking opens the original context menu for that exact row.
* Action callbacks use persistent model indexes and reject invalid indexes after resets.
* Fixed 32 px counter column, 8 px right margin, original row heights. Below 160 px
  of row width, actions give space to the title and counter; the context menu remains.
* First-level folders use DemiBold; nested folders use Normal and a corner-marked icon.
* Folder icon double-click opens a color dialog with a **Use theme color** reset.
  Colors are stored in `Settings/Settings.ini`, group `folderIconColors`, keyed by
  stable node ID. No SQLite migration is needed. Selected custom icons have a
  contrasting contour while retaining the chosen color.
* Action/folder icons are small Qt vectors: the bundled subset fonts do not contain
  the requested folder-plus, pencil, palette or alternate-folder glyphs.
* Ordinary tag clicks clear previous selection. Ctrl/Shift retain explicit multiple
  selection behavior. Filtering still flows through the existing selection handler.
* Note dragging uses a transparent 32 px icon (48 px with a group count), hotspot
  `(8,8)`. MIME data, move action, drop restrictions and database moves are unchanged.
* Windows native caption follows the application theme using DWM. Unsupported
  attributes are left to Windows. No replacement frameless title bar was introduced.
* Portable IPC includes a hash of the canonical program directory so testing a copy
  does not intercept a launch of the working portable instance.

## Build

Use the existing MSVC/Qt environment and a separate CMake build directory:

```text
cmake -S . -B <build> -G Ninja -DCMAKE_BUILD_TYPE=Release -DPRO_VERSION=ON -DUPDATE_CHECKER=OFF -DNOTES_PORTABLE=ON -DCMAKE_PREFIX_PATH=<Qt>/msvc2022_64
cmake --build <build> --parallel 4
windeployqt --release --qmldir src/qml <package>/Notes.exe
```

Validated with Qt 6.8.3, MSVC 19.44 and Windows 11. The upstream application version
stays 2.3.1; v0.2 identifies these local UI changes.

## Tests

`tests/run_ui_ux_v02.py` links a Qt integration driver to the already built application
objects, excluding its production main. Pass a **new** output directory. Runtime
files are copied without Data or Settings. The driver creates only synthetic data.

```text
python tests/run_ui_ux_v02.py --build <build> --runtime <portable-package> --qt <Qt>/msvc2022_64 --out <new-test-directory>
```

Covered by executable checks:

* Root, child and third-level folder creation; targeted subfolder button; inline
  rename by double-click; original confirmed removal, verified in SQLite.
* Counter geometry, unchanged row height and a narrowed sidebar; arrow expand/collapse.
* Folder color persistence in settings, stability across Light/Dark/Sepia, move to
  root and back, and reset. DWM accepts all three caption colors on the test OS.
* Tag palette and inline rename, single selection, explicit Ctrl selection, selection
  replacement, and original tag removal.
* Pro is enabled, Kanban is editable, and its QML handlers check and rename a task.
* Separate notes are created via the editor. Real drop events move one note to a
  nested folder and multiple notes to a folder; SQLite checks confirm their parents
  and retained records.

Native OLE drag previews require real mouse input rather than a synthetic Qt drag
loop. These were separately observed with computer input: single preview 32×32,
group preview 48×32 with count 2, target hover and Escape cancellation. The dark
native Windows caption and the folder-icon double-click dialog were also observed.

Limitations: no clean-Windows/other-machine run, no large-data performance benchmark,
and no automated pixel comparison for every theme/hover combination. Shift is
preserved as explicit tag multiselection; a contiguous Shift range was not added.

## Working installation and update

Working installation: `C:\RELEASE\Notes_ZHKVSKY_Portable`.
The D: packages and tests are development artifacts, not the primary installation.

1. Fully quit the working application and copy its entire folder to a fresh backup.
2. Verify the backup, including the database and settings.
3. Copy the new executable and runtime libraries/plugins into the C: installation,
   excluding **Data** and **Settings**. Never delete either directory and never copy
   test databases/settings into the working installation.
4. Compare hashes of all Data/Settings files before and after the update.
5. Roll back binaries from the backup if required; preserve the current Data and
   Settings unless explicitly restoring a separate user-data backup.

The initial backup for this task is
`D:\01_PROJECTS\NOTES_ZHKVSKY\Backups\Notes_C_before_v02_20261117_001149`.

DWM reference: https://learn.microsoft.com/windows/apps/desktop/modernize/apply-windows-themes

## Row-control fixes after v0.2 feedback

The rename icon buttons were removed from both folders and tags. Non-destructive
action icons retain contrasting text color on hover instead of changing to the
blue selection background. Only the delete action gains a red hover accent.

The folder color dialog now owns a copy of its persistent index and protects its
initiating hover editor while the dialog is open. Previously, leaving the tree
could destroy that editor and invalidate the reference passed to the dialog.
Deferred cleanup restores hover-only controls after the modal event returns.

Regression checks enter the dialog via an actual icon double-click, send a tree
leave event and process deferred destruction, accept the chosen color and reload
it in a separate process. The old implementation failed this reproduction.
Additional checks cover both counter menus, hover-dot rendering without geometry
changes, removal of rename buttons, and icon contrast on the selected blue row.
