param([string]$VcVarsPath)
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $PSScriptRoot 'bin'
New-Item -ItemType Directory -Force -Path $bin | Out-Null
if (-not $VcVarsPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio 2022 Build Tools with Desktop development with C++.' }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installation) { throw 'MSVC x86 compiler tools are unavailable.' }
    $VcVarsPath = Join-Path $installation 'VC\Auxiliary\Build\vcvars32.bat'
}
if (-not (Test-Path -LiteralPath $VcVarsPath)) { throw 'vcvars32.bat was not found.' }
$source = Join-Path $project 'native\launcher.cpp'
$output = Join-Path $bin 'FTL-Ultrawide.exe'
$batch = Join-Path $bin 'build.cmd'
@"
@echo off
call "$VcVarsPath" >nul
if errorlevel 1 exit /b 1
cd /d "$bin"
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT /DUNICODE /D_UNICODE "$source" /Fe:"$output" /link /SUBSYSTEM:WINDOWS /MACHINE:X86 advapi32.lib shell32.lib user32.lib
"@ | Set-Content -LiteralPath $batch -Encoding ascii
& cmd.exe /d /c $batch
if ($LASTEXITCODE -ne 0) { throw 'Native launcher build failed.' }
Write-Output $output
