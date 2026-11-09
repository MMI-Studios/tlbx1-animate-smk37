# Owner-authorized installation of the exact reviewed Animate candidate.
$ErrorActionPreference = 'Stop'
$firmwareRoot = Split-Path $PSScriptRoot -Parent
$utility = Join-Path $firmwareRoot 'build/recovery-tools/m-flasher-0.9.3/M-Flasher.exe'
$package = Join-Path $firmwareRoot 'build/smk/Animate-SMK-37-Pro_982.fwsc'
$backup = Join-Path $firmwareRoot 'recovery-backups/read-20261107-164044/smk37-20261107-164052.bin'
if ((Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash -ne 'fcbf8adc2615cb5accdc24065a2f41cc95a9d77913a360f05a38b2fb20968b4c') { throw 'Reviewed package hash mismatch' }
if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -ne 'ac25ddb8bd2ef4f2ead1e403fcc074646e34725c548fe7e963261a81c394e013') { throw 'Verified backup hash mismatch' }
$session = Join-Path $firmwareRoot ('recovery-backups/install-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $session -ErrorAction Stop | Out-Null
try {
    # CLI preserves bootloader and settings unless their explicit flags are passed.
    $process = Start-Process -FilePath $utility -ArgumentList @('firmware', $package, '--backup-dir', $session, '--yes', '--restart') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $session 'install.log') -RedirectStandardError (Join-Path $session 'stderr.log')
    "Exit code: $($process.ExitCode)" | Out-File -LiteralPath (Join-Path $session 'result.txt')
    if ($process.ExitCode -ne 0) { throw "Firmware utility exited $($process.ExitCode); inspect logs before retrying" }
} catch {
    $_ | Out-String | Out-File -LiteralPath (Join-Path $session 'error.txt')
    exit 1
}
