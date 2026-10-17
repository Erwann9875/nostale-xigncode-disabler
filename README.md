# NosTale Xigncode3 Disabler

Standalone Windows launcher that temporarily skips Xigncode3 initialization in a
newly created, verified client process. No gameplay plugins, packet hooks, SDK
agent or EWS loader are included.

## Quick start

1. Use a supported client and a server where you have explicit permission to use
   this tool.
2. Copy `dist/Nostale-Xigncode-Disabler.exe` beside `NostaleX.dat` or `NostaleX.exe`.
   No additional project DLL is needed. Keep the normal game files intact.
3. In the client's `Config.ini`, set your actual login server address and port:

   ```ini
   [NosTale_Network]
   IP=login.example.test
   Port=4000
   ```

   Replace the example hostname and port with your server's actual settings.
   Localhost, LAN/public IP addresses and hostnames are allowed; there is no
   `127.0.0.1`-only restriction. An explicit non-empty address and a numeric port
   from 1 to 65535 are still required. Do not change unrelated INI settings.
4. Double-click the EXE. **Yes** temporarily disables Xigncode3 for that launch,
   **No** leaves it unchanged, and **Cancel** does nothing. The dialog shows the
   selected client; `NostaleX.dat` takes precedence when both filenames exist.
5. Accept the normal Windows administrator-consent prompt if required by the
   original client. Close the game and launch normally to restore stock startup.

## Command line

```powershell
.\Nostale-Xigncode-Disabler.exe --launch-client "C:\Games\NosTale\NostaleX.dat" --disable-xigncode3
```

### Suspended-only check

```powershell
.\Nostale-Xigncode-Disabler.exe --launch-client "C:\Games\NosTale\NostaleX.dat" --disable-xigncode3 --validate-only
```

Run this check from an Administrator terminal when the original client requires
elevation. It verifies the memory change, then discards the suspended child; the
game is never resumed and no login is attempted. It does not certify login or
gameplay. Omit `--disable-xigncode3` to validate unchanged startup instead.

## What it does

- Verifies the client size and SHA-256 against `contracts/client-profiles.json`.
- Reads the configured login address and validates the explicit port.
- Creates the client suspended and checks its mapped PE layout, original startup
  instructions and not-yet-initialized Xigncode module state.
- Changes only the initialization call in that new process's memory.
- Restores page protection, flushes the instruction cache, verifies readback,
  then resumes the new client. Errors abort the child instead of continuing.

It does not patch executable files on disk, delete XIGNCODE files, stop services,
change drivers, attach to running games, emulate challenge responses or alter the
server. Removing the address restriction does not add support for other client
builds or guarantee that a server will accept the client. No client downloads or
unsupported-build fallback are included.

## Supported clients

32-bit NosTale **0.9.3.3257**, **3,681,648 bytes**, with one of these hashes:

```text
537ABBCBED3661AB7CA9989B0C0274DA65DAC662F2049D0171503A82216E764B
C46116A3D1618E9DB6085BCA4A3B8BA4C71E38D5A3071A1A6B5AD9CEA52B096B
```

Game executables and vendor anti-cheat binaries are not distributed here. A new
client needs a fresh code/layout review; merely adding its hash is not enough.

## Build

The included release EXE uses a static C++ runtime. Users do not need .NET or
Visual Studio to run it. Building requires Windows, MSVC C++20, a Windows SDK
and compatible CMake from Visual Studio/Build Tools 2022 or 2026.

Install **Desktop development with C++** and **C++ CMake tools for Windows**
through Visual Studio Installer. Then double-click `build.cmd`, or run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Build with the focused argument and endpoint checks:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Check
```

The script searches complete Community, Professional, Enterprise and Build Tools
installations, including bundled CMake that is not on PATH. It launches no package
manager, installer or download. Bundled Ninja is preferred, with the Visual Studio
generator as a fallback. Output: `dist/Nostale-Xigncode-Disabler.exe`.

## Source map

- `src/main.cpp`: standalone entry point.
- `src/launcher/adjacent_client_launcher.cpp`: double-click launch dialog.
- `src/launcher/local_client_launcher.cpp`: verification, suspended launch and cleanup.
- `src/launcher/xigncode_startup.cpp` and `.hpp`: process-memory startup change.
- `src/launcher/launch_options.cpp` and `.hpp`: options and endpoint settings.
- `src/platform/sha256.cpp`: file fingerprinting through Windows BCrypt.
- `contracts/client-profiles.json`: reviewed client allowlist.
- `cmake/`: build-time generation of the compiled profiles.
- `tests/`: focused option and endpoint checks.
- `VALIDATION.txt`: completed checks and untested release limitations.

## Misuse disclaimer

Use this tool only on systems and game servers you own or where you have explicit
permission. The author does not endorse or authorize cheating, unauthorized use,
or violations of third-party rules or terms.

You are solely responsible for how you use this software and for any misuse.
To the extent permitted by applicable law, the author disclaims responsibility
and liability for misuse and its consequences, including account bans, loss of
data, damage or other losses. The software is provided **AS IS**, without warranty,
as described in `LICENSE`. This disclaimer does not override non-waivable legal
obligations.

## License

The original copyright notice and all-rights-reserved license are retained.
See `LICENSE`. Public source availability does not change those terms.
