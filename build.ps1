param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [string]$CMakeExe = '',
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$xcRoot = $PSScriptRoot
$xcVsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $xcVsWhere -PathType Leaf)) {
    throw 'Install Visual Studio or Build Tools with Desktop development with C++, a Windows SDK and C++ CMake tools for Windows.'
}
$xcInstancesText = (& $xcVsWhere -all -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json -utf8) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Visual Studio discovery failed.' }
$xcInstances = @($xcInstancesText | ConvertFrom-Json | Where-Object { $_.isComplete } | Sort-Object { [version]$_.installationVersion } -Descending)
if ($xcInstances.Count -eq 0) { throw 'No complete MSVC C++ installation was found. Add Desktop development with C++ in Visual Studio Installer.' }

$xcCandidates = [Collections.Generic.List[string]]::new()
if (-not [string]::IsNullOrWhiteSpace($CMakeExe)) { $xcCandidates.Add($CMakeExe) }
$xcPathCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($xcPathCommand) { $xcCandidates.Add($xcPathCommand.Source) }
foreach ($xcInstance in $xcInstances) {
    $xcCandidates.Add((Join-Path $xcInstance.installationPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'))
}
$xcProgramFilesCmake = Join-Path $env:ProgramFiles 'CMake\bin\cmake.exe'
$xcCandidates.Add($xcProgramFilesCmake)
$xcSelectedCmake = $null
$xcGenerator = $null
$xcInstallation = $null
$xcNinja = $null
$xcDevCmd = $null
foreach ($xcCandidate in ($xcCandidates | Select-Object -Unique)) {
    if (-not (Test-Path -LiteralPath $xcCandidate -PathType Leaf)) { continue }
    $xcHelp = (& $xcCandidate --help 2>$null) -join "`n"
    if ($LASTEXITCODE -ne 0) { continue }
    foreach ($xcInstance in $xcInstances) {
        $xcCandidateNinja = Join-Path $xcInstance.installationPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
        $xcCandidateDevCmd = Join-Path $xcInstance.installationPath 'Common7\Tools\VsDevCmd.bat'
        if ($xcHelp.Contains('Ninja') -and (Test-Path -LiteralPath $xcCandidateNinja -PathType Leaf) -and
            (Test-Path -LiteralPath $xcCandidateDevCmd -PathType Leaf)) {
            $xcSelectedCmake = $xcCandidate
            $xcGenerator = 'Ninja'
            $xcInstallation = $xcInstance.installationPath
            $xcNinja = $xcCandidateNinja
            $xcDevCmd = $xcCandidateDevCmd
            break
        }
        $xcMajor = ([version]$xcInstance.installationVersion).Major
        $xcCandidateGenerator = switch ($xcMajor) { 18 { 'Visual Studio 18 2026' } 17 { 'Visual Studio 17 2022' } default { $null } }
        if ($xcCandidateGenerator -and $xcHelp.Contains($xcCandidateGenerator)) {
            $xcSelectedCmake = $xcCandidate
            $xcGenerator = $xcCandidateGenerator
            $xcInstallation = $xcInstance.installationPath
            break
        }
    }
    if ($xcSelectedCmake) { break }
}
if (-not $xcSelectedCmake) { throw 'No compatible CMake was found. Add C++ CMake tools for Windows in Visual Studio Installer, or pass -CMakeExe with a current cmake.exe path.' }
$xcBuild = Join-Path $xcRoot ('build\' + $Configuration + '-' + $(if ($xcGenerator -eq 'Ninja') { 'ninja' } else { 'vs' }))
$xcOutput = Join-Path $xcRoot 'dist'
$xcBuildTests = if ($Check) { 'ON' } else { 'OFF' }
Write-Host "CMake: $xcSelectedCmake"
Write-Host "Compiler: $xcInstallation"
if ($xcGenerator -eq 'Ninja') {
    & (Join-Path $xcRoot 'tools\native-build.cmd') $xcDevCmd $xcSelectedCmake $xcRoot $xcBuild $xcNinja $Configuration $xcBuildTests
} else {
    & $xcSelectedCmake -S $xcRoot -B $xcBuild -G $xcGenerator -A x64 "-DCMAKE_GENERATOR_INSTANCE=$xcInstallation" "-DXC3_BUILD_TESTS=$xcBuildTests"
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $xcSelectedCmake --build $xcBuild --config $Configuration --parallel
}
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed; the previous distributable was not replaced.' }
if ($Check) {
    $xcCtest = Join-Path (Split-Path -Parent $xcSelectedCmake) 'ctest.exe'
    & $xcCtest --test-dir $xcBuild -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Launcher safety checks failed; the previous distributable was not replaced.' }
}
New-Item -ItemType Directory -Path $xcOutput -Force | Out-Null
$xcExe = Join-Path $xcOutput 'Nostale-Xigncode-Disabler.exe'
$xcBuiltExe = if ($xcGenerator -eq 'Ninja') { Join-Path $xcBuild 'Nostale-Xigncode-Disabler.exe' } else { Join-Path $xcBuild "$Configuration\Nostale-Xigncode-Disabler.exe" }
Copy-Item -LiteralPath $xcBuiltExe -Destination $xcExe
Write-Host "Ready: $xcExe"
Write-Host "SHA256: $((Get-FileHash -LiteralPath $xcExe -Algorithm SHA256).Hash)"
