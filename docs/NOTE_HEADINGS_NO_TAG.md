# Note headings and NO TAG

Primary portable application: `C:\RELEASE\Notes_ZHKVSKY_Portable\Notes.exe`.
Source branch: `feature/note-headings-and-no-tag`.
Release configuration: NOTES_PORTABLE=ON, PRO_VERSION=ON, UPDATE_CHECKER=OFF.

## Behavior

- The note list places the existing modification date at the right of the title line. A reserved target beside it shows a delete icon on hover. The original deletion/Trash/confirmation handlers are retained, including discarding a temporary empty note.
- A short blue underline distinguishes the list heading. The editor has an optional, larger heading input above the unchanged Markdown body. Creating a note focuses the body.
- Empty heading input retains automatic first-line titles. Entering a heading overrides that title without changing the body; clearing the input restores automatic naming. Existing notes retain their original content and automatic behavior.
- Manual-title flags use existing SQLite metadata rows named `zhkvsky_manual_title_<id>` with integer value 1. The displayed title remains in node_table.title. There is no schema migration. Title/content/flag saves commit together.
- NO TAG is a protected row above ordinary tags. It selects notes without tags and accepts note drops to clear all tags. It cannot be edited or reordered. Its sentinel ID is never written as a real tag relationship.
- Clearing tags commits relationship removal and tag counters in one transaction before updating the visible note. It leaves content, title and folder unchanged.
- Card height follows actual wrapped tag geometry, including a bounded scrolling area for many tags. Title/date/body/tag positions share a common layout.

## Build and verification

Final build directory: `D:\01_PROJECTS\NOTES_ZHKVSKY\build-notes-headings-release`.
The generated CMake compiler file originally contained a CP866/UTF-8 mojibake MSVC include prefix. Correcting only that generated prefix and rebuilding all objects restored Ninja header dependencies (nodedata.cpp.obj: 121 dependencies). This is a local toolchain configuration repair, not an application source or dependency change. On a newly configured build verify the include prefix and `ninja -t deps` before relying on incremental builds.

Integration runner: `tests/run_ui_ux_v02.py`; native driver: `tests/ui_ux_v02.cpp`.
Every run uses a new synthetic portable folder, never the primary Data or Settings. The suite exercises heading precedence and clearing, body focus, two and 21 tags, NO TAG multi-note drops, row deletion including empty notes, Pro Kanban edits, existing sidebar controls and DB persistence. The runner also opens a separate process to verify the saved manual heading and edited Kanban body.

Pre-update full backup: `D:\01_PROJECTS\NOTES_ZHKVSKY\Backups\Notes_C_preinstall_headings_20261117_151246`.
Updates copy application/runtime/documentation files only; Data and Settings are excluded and their complete file set and SHA-256 hashes must match before and after copying.

A clean Windows installation is not available for testing.

## Acceptance evidence

- `D:\01_PROJECTS\NOTES_ZHKVSKY\TEST\headings-layout-accepted\ui-test-report.txt`: 75 PASS, FAILURES=0.
- `D:\01_PROJECTS\NOTES_ZHKVSKY\headings-layout-accepted.log`: separate-process heading/Kanban-body restart PASS.
- Ordinary Notes.exe was launched, edited, closed and reopened in `D:\01_PROJECTS\NOTES_ZHKVSKY\TEST\Проверка заголовков Portable`. Cyrillic title/content, Markdown formatting and a real Kanban checkbox persisted; SQLite integrity_check returned ok. The heading was visually verified above the board. Screenshot: `kanban-verified.jpg` in that fixture.
- windeployqt Release packaging completed with the existing Qt 6.8.3 runtime.

## Pre-existing issue outside this change

Synthetic folder-move fixtures exposed an inherited path-formatting problem: DBManager::moveNode uses a char separator as the second QString::arg argument, which can be interpreted as a field width. The parent_id movement checks pass, but absolute_path can contain spaces/placeholders. This also appears in the prior row-fix fixture. This update does not repair existing user folder paths or broaden the requested change into a data migration. This limitation remains open for a separate fix.
