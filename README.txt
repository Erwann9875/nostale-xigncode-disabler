NOSTALE XIGNCODE3 DISABLER

Standalone Windows launcher for local NosTale development. It can temporarily
skip Xigncode3 initialization in a newly created, verified client process.
It contains no gameplay plugins, packet hooks, SDK agent or EWS loader.

QUICK START
1. Use a supported client and a local development server you control.
2. Copy dist/Nostale-Xigncode-Disabler.exe beside NostaleX.dat or NostaleX.exe.
   No extra DLL from this project is needed. Keep the normal game files intact.
3. Beside that client, Config.ini must have an explicit local login endpoint:

   [NosTale_Network]
   IP=127.0.0.1
   Port=4000

   Replace 4000 with your actual local LOGIN port. Do not change unrelated INI
   settings. Missing values, hostnames and remote endpoints are refused.
4. Double-click the EXE. The dialog shows the selected client. Yes enables the
   temporary change, No uses normal startup, and Cancel does nothing.
   NostaleX.dat takes precedence when both supported filenames are present.
5. Accept the normal Windows administrator-consent prompt if required by the
   original client. Close the client and launch normally to restore stock startup.

COMMAND LINE
Nostale-Xigncode-Disabler.exe --launch-client "C:\Games\NosTale\NostaleX.dat" --disable-xigncode3

SUSPENDED-ONLY CHECK
Nostale-Xigncode-Disabler.exe --launch-client "C:\Games\NosTale\NostaleX.dat" --disable-xigncode3 --validate-only

Run that check from an Administrator terminal when the original client requires
elevation. It verifies the memory change, then discards the suspended child;
the game is never resumed. It does not certify in-game login or gameplay.
Omit --disable-xigncode3 to validate the unchanged startup instead.

WHAT IT DOES
- Verifies the client size and SHA-256 against contracts/client-profiles.json.
- Requires an explicit 127.0.0.1 Config.ini endpoint and valid port.
- Creates the client suspended and validates its mapped PE layout, original
  startup instructions and not-yet-initialized Xigncode module state.
- Changes only the initialization call in that new process's memory.
- Restores page protection, flushes the instruction cache, verifies readback,
  then resumes the new client. Errors abort the child instead of continuing.

It does not patch executable files on disk, delete XIGNCODE files, stop services,
change drivers, attach to running games, emulate challenge responses or alter
the server. This is not a universal bypass for other clients or remote servers.
No automatic client downloads or unsupported-build fallback are included.

SUPPORTED CLIENTS
32-bit NosTale 0.9.3.3257, 3,681,648 bytes, with one of these reviewed hashes:
537ABBCBED3661AB7CA9989B0C0274DA65DAC662F2049D0171503A82216E764B
C46116A3D1618E9DB6085BCA4A3B8BA4C71E38D5A3071A1A6B5AD9CEA52B096B

Game executables and vendor anti-cheat binaries are NOT distributed here. A new
client needs a fresh code/layout review; merely adding its hash is not sufficient.

BUILD
The included release EXE uses a static C++ runtime; users do not need .NET or
Visual Studio to run it. Building requires Windows, MSVC C++20, a Windows SDK,
and a compatible CMake (Visual Studio/Build Tools 2022 or 2026).

Install Desktop development with C++ and C++ CMake tools for Windows through
Visual Studio Installer. Then double-click build.cmd, or run:

powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1

Build with the focused argument/loopback safety checks:
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Check

The script searches complete Community, Professional, Enterprise and Build Tools
installations, including bundled CMake that is not on PATH. No package manager,
installer or download is launched by the build script.
Bundled Ninja is preferred; the Visual Studio generator is used as a fallback.
Output: dist/Nostale-Xigncode-Disabler.exe

SOURCE MAP
src/main.cpp: standalone program entry point.
src/launcher/adjacent_client_launcher.cpp: double-click launch dialog.
src/launcher/local_client_launcher.cpp: verification, suspended launch and cleanup.
src/launcher/xigncode_startup.cpp and .hpp: process-local startup change.
src/launcher/launch_options.cpp and .hpp: opt-in switch and loopback validation.
src/platform/sha256.cpp: file fingerprinting through Windows BCrypt.
contracts/client-profiles.json: reviewed client allowlist.
cmake/: build-time generation of the compiled client profiles.
tests/: focused launcher option and local-endpoint guard checks.

LICENSE
The original copyright notice and all-rights-reserved license are retained.
See LICENSE. Public source availability does not change those license terms.
