# Note-card metadata layout

Primary portable application: `C:\RELEASE\Notes_ZHKVSKY_Portable\Notes.exe`.
Branch: `feature/note-card-metadata`.

Changes:
- Note-list delete buttons use an explicit icon foreground for the selected theme, independent of inherited QPushButton palette overrides. Existing Trash/deletion handlers are unchanged.
- Times use HH:mm in both note-card dates and the editor date label. Existing relative labels for earlier days remain unchanged.
- A right-side metadata column reserves space for date/delete and colored tag circles below. Title, preview and folder text are elided within the left column, with the same geometry for normal, pinned and animated rows.
- Only note-card tag delegates use dots mode: 10 px colored circles in 16 px cells. Names remain available in tooltips; sidebar tags and editor chips retain their existing labels.
- Dot rows wrap within the reserved column; larger collections scroll in a bounded 80 px area. Card height fits the larger of text/folder content and dot content. A two-pixel layout allowance avoids QListView wrapping a cell at the exact viewport boundary.
- Heading rules in the list and editor use a subtle translucent gray line.

No database schema, tag relationships, note content or user settings formats change. Release remains NOTES_PORTABLE=ON, PRO_VERSION=ON, UPDATE_CHECKER=OFF.

Verification artifacts live outside Git under `D:\01_PROJECTS\NOTES_ZHKVSKY\TEST`. The native suite verifies 24-hour formatting, circle-only rendering and colors, right alignment, compact two-tag layout, 21-tag wrapping, dark icon strokes, pinned row geometry, NO TAG, Markdown/Kanban and saved headings. A dedicated probe verifies the two-item horizontal row, and the feedback suite covers dark-heading contrast and NO TAG hover/drag/cancellation.

Pre-update full backup: `D:\01_PROJECTS\NOTES_ZHKVSKY\Backups\Notes_C_before_card_metadata_20261117_154653`.
Installation must exclude Data and Settings, then compare their full file set and SHA-256 hashes with this backup manifest. No test data is deployed.

Clean Windows testing remains unavailable. The previously documented inherited folder-move path-formatting issue remains outside this UI change.

Acceptance evidence:
- `TEST/card-metadata-verified/ui-test-report.txt`: 82 PASS, FAILURES=0.
- `card-metadata-verified.log`: separate-process heading and edited Kanban-body persistence PASS.
- `TEST/card-feedback-confirmed/ui-test-report.txt`: 12 PASS, FAILURES=0.
- `TEST/card-dot-confirmed/ui-test-report.txt`: two-item row probe PASS; viewport 34x16, item rectangles (0,0,16,16) and (16,0,16,16).
- Ordinary final Notes.exe was launched with synthetic data in `TEST/Карточки заметок`. The selected pinned note was checked visually: light delete icon, HH:mm dates in list/editor, dots in horizontal rows at right, and subtle gray heading lines. Screenshot: `card-metadata-verified.jpg`. Sidebar and editor tag labels remain visible. Dot view background is transparent.

## Continuous row hover and background compositing

Branch: `fix/note-row-hover-compositing`.
Row hover is determined by the full viewport row rectangle, including child widgets, rather than a card-only enter/leave flag. The existing action timer invalidates the previous and current row when this target changes. Delegate and tag-card painters share one opaque background obtained by blending the existing theme layer onto its base exactly once. The floating delete button fills the same background and omits its separate hover backplate; icon contrast and destructive hover color remain unchanged. Other sidebar action buttons retain their previous hover treatment.

Evidence: `TEST/hover-halo-after/ui-test-report.txt` has 30 PASS, FAILURES=0, covering selected/unselected rows in dark/light/sepia, actual unselected hover highlighting, visible delete controls, equal background pixels under the delete button and dots, and pointer crossing onto the dots. `TEST/hover-halo-regression/ui-test-report.txt` has 82 PASS, FAILURES=0; its runner also passed separate-process heading/Kanban persistence. Ordinary Notes.exe was launched in `TEST/Проверка фона карточек` with synthetic data. No database/settings format changes.

Full pre-update backup: `D:\01_PROJECTS\NOTES_ZHKVSKY\Backups\Notes_C_before_hover_halo_20261117_161051`.
