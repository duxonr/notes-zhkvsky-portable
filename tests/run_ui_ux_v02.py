"""Windows integration tests against an already built portable Notes object set.

Use a NEW output directory: the runner copies binaries only and creates its own DB.
Requires the same Qt/MSVC versions as the supplied CMake build.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--feedback-only', action='store_true')
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--runtime', type=Path, required=True)
parser.add_argument('--qt', type=Path, required=True)
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
build, runtime, qt, out = (p.resolve() for p in (args.build, args.runtime, args.qt, args.out))
if out.exists():
    parser.error('--out must not exist; test data is never overwritten')
entry = next(e for e in json.loads((build / 'compile_commands.json').read_text())
             if e['file'].endswith('/src/main.cpp'))
command = entry['command']
if '-DNOTES_PORTABLE' not in command or '-DPRO_VERSION' not in command or '-DUPDATE_CHECKER' in command:
    parser.error('Build must use NOTES_PORTABLE=ON, PRO_VERSION=ON, UPDATE_CHECKER=OFF')
out.mkdir(parents=True)
for item in runtime.iterdir():
    if item.name.casefold() in ('data', 'settings'):
        continue
    if item.is_dir():
        shutil.copytree(item, out / item.name)
    else:
        shutil.copy2(item, out / item.name)
shutil.copy2(qt / 'bin/Qt6Test.dll', out / 'Qt6Test.dll')
source = Path(__file__).with_name('ui_ux_v02.cpp').resolve()
command = re.sub(r'/Fo\S+', '/Foui_v02_test.obj', command)
command = command[:command.rfind(' -c ')] + f' -I"{qt / "include/QtTest"}" -c "{source}"'
objects = [p for p in (build / 'CMakeFiles/Notes.dir').rglob('*.obj') if p.name != 'main.cpp.obj']
libraries = ['Core', 'Gui', 'Widgets', 'Network', 'Sql', 'Concurrent', 'QuickWidgets',
             'Quick', 'Qml', 'OpenGL', 'Test']
response = build / 'ui_v02_link.rsp'
response.write_text('\n'.join(['/NOLOGO', '/SUBSYSTEM:CONSOLE', f'/OUT:"{out / "UiDriver.exe"}"',
                                'ui_v02_test.obj'] + [f'"{p}"' for p in objects] +
                               [f'/LIBPATH:"{qt / "lib"}"'] + ['Qt6' + x + '.lib' for x in libraries] +
                               ['user32.lib', 'dwmapi.lib']))
vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                              'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                              '-property', 'installationPath'], text=True).strip()
vcvars = Path(vs) / 'VC/Auxiliary/Build/vcvars64.bat'
script = build / 'compile_ui_v02.cmd'
script.write_text(f'@call "{vcvars}" >nul\n{command}\n@if errorlevel 1 exit /b 1\nlink @"{response}"\n')
subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=build, check=True)
result = subprocess.run([str(out / 'UiDriver.exe')] + (['--feedback-only'] if args.feedback_only else []), cwd=out)
print((out / 'ui-test-report.txt').read_text(encoding='utf-8'))
if result.returncode == 0:
    report_text = (out / 'ui-test-report.txt').read_text(encoding='utf-8')
    heading_id = re.search(r'RESTART_HEADING_ID=(\d+)', report_text)
    if heading_id:
        restarted = subprocess.run([str(out / 'UiDriver.exe'), '--verify-heading', heading_id[1]], cwd=out)
        print('PASS heading and edited Kanban body survive process restart' if restarted.returncode == 0 else 'FAIL heading restart', flush=True)
        raise SystemExit(restarted.returncode)
raise SystemExit(result.returncode)
