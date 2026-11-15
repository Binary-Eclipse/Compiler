# First-time setup for ShakilScript on Windows.
# Installs MSYS2 (if missing) and the build tools: gcc (UCRT64), make, flex, bison.
# Run automatically by build.bat; can also be run by hand:
#   powershell -ExecutionPolicy Bypass -File scripts\setup_windows.ps1
$ErrorActionPreference = 'Stop'

function Say($m) { Write-Host "[ShakilScript setup] $m" }

$root = $env:SHAKIL_MSYS2
if (-not $root) { $root = 'C:\msys64' }
$bash = Join-Path $root 'usr\bin\bash.exe'

try {
    if (-not (Test-Path $bash)) {
        Say 'MSYS2 not found. Installing it (about 100 MB download)...'
        if (Get-Command winget -ErrorAction SilentlyContinue) {
            Say 'Using winget (accept the Windows prompt if one appears).'
            & winget install --id MSYS2.MSYS2 -e --silent --accept-package-agreements --accept-source-agreements
        }
        if (-not (Test-Path $bash)) {
            Say 'Downloading the official MSYS2 installer from repo.msys2.org ...'
            $installer = Join-Path $env:TEMP 'msys2-x86_64-latest.exe'
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -UseBasicParsing -Uri 'https://repo.msys2.org/distrib/msys2-x86_64-latest.exe' -OutFile $installer
            $rootFwd = $root.Replace('\', '/')
            Start-Process -FilePath $installer -ArgumentList @('in', '--confirm-command', '--accept-messages', '--root', $rootFwd) -Wait
        }
        if (-not (Test-Path $bash)) { throw "MSYS2 installation did not produce $bash" }
    } else {
        Say "MSYS2 found at $root"
    }

    Say 'Updating MSYS2 and installing gcc, make, flex, bison (first time: a few minutes)...'
    # The first core update may stop early on a brand-new install, so it is run twice.
    & $bash -lc 'pacman -Syu --noconfirm' | Out-Host
    & $bash -lc 'pacman -Syu --noconfirm' | Out-Host
    & $bash -lc 'pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc make flex bison' | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'pacman could not install the build tools' }

    foreach ($f in @('ucrt64\bin\gcc.exe', 'usr\bin\make.exe', 'usr\bin\flex.exe', 'usr\bin\bison.exe')) {
        if (-not (Test-Path (Join-Path $root $f))) { throw "Expected tool is missing after install: $f" }
    }
    Say 'Done. Build tools are ready.'
    exit 0
}
catch {
    Write-Host "[ShakilScript setup] FAILED: $_" -ForegroundColor Red
    Write-Host 'Manual alternative: install MSYS2 from https://www.msys2.org, open "MSYS2 UCRT64", run:'
    Write-Host '  pacman -S --needed mingw-w64-ucrt-x86_64-gcc make flex bison'
    exit 1
}
