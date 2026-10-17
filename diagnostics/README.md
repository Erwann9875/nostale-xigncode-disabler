# Read-only file diagnostics

This independent tool reports a local file's SHA-256 fingerprint, size and basic
Windows PE headers. Use it to compare file versions and identify x86, x64 or ARM
images without running them. Executable `.dat` files are supported too; ordinary
data archives are not PE images and will be rejected.

It uses only the Python standard library. No build, launcher, third-party Python
package, game installation or vendor binary is required.

## Run it

1. Install Python 3.10 or newer if needed.
2. Open PowerShell in the repository folder.
3. Replace the example path with the file you want to inspect:

   ```powershell
   py -3 .\diagnostics\inspect_file.py "C:\Games\Example\client.dat"
   ```

   Use `python` instead of `py -3` if that is how Python is installed on your PC.
   The result is JSON in the terminal. Exit code `0` means a report was produced;
   `2` means an unreadable, malformed or unsupported input, or incorrect arguments.

The report contains the basename, file size, SHA-256, PE32/PE32+ format, machine
identifier, architecture, section count and DLL flag. Successful reports contain
no absolute paths, file contents, extracted resources or disassembled code.
Errors may include the input path; review messages before sharing them.

For a consistent fingerprint, inspect a stable file after closing anything that
could modify it. Matching hashes identify matching bytes, not a safe or trusted
program. Basic header checks are not a complete Windows-loader validation,
digital-signature check, malware scan or guarantee of client/server compatibility.

## Test it

From the repository folder:

```powershell
py -3 -m unittest discover -s diagnostics -p "test_*.py" -v
```

The tests create synthetic headers and inert data. They check both PE formats,
architecture reporting, malformed/truncated files, bounds, Unicode filenames,
fingerprints, read-only behavior and command-line errors. No test file is loaded
as an executable, and no real game or vendor files are needed.

## Scope

This directory is separate from the repository's launcher. Running these scripts
does not invoke it. This diagnostic tool does not launch or inject into a process,
patch files or memory, use the network, emulate an anti-cheat protocol, generate
verification replies or include vendor binaries. It is not an anti-cheat emulator
or a fix for server disconnections. No new launcher EXE is built by these commands.

Header definitions follow the
[Microsoft PE/COFF specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).
