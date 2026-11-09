# Read-only device backup. No firmware/restore/bootloader commands permitted.
$ErrorActionPreference = 'Stop'
$firmwareRoot = Split-Path $PSScriptRoot -Parent
$utility = Join-Path $firmwareRoot 'build/recovery-tools/m-flasher-0.9.3/M-Flasher.exe'
$backupRoot = Join-Path $firmwareRoot 'recovery-backups'
$session = Join-Path $backupRoot ('read-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $session -ErrorAction Stop | Out-Null
try {
    $process = Start-Process -FilePath $utility -ArgumentList @('backup', $session) -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $session 'backup.log') -RedirectStandardError (Join-Path $session 'stderr.log')
    "Exit code: $($process.ExitCode)" | Out-File -LiteralPath (Join-Path $session 'result.txt')
    if ($process.ExitCode -ne 0) { throw "Backup utility exited $($process.ExitCode)" }
    $images = @(Get-ChildItem -LiteralPath $session -File -Filter '*.bin')
    if (!$images.Count) { throw 'No backup image produced' }
    $images | Get-FileHash -Algorithm SHA256 | Format-List |
        Out-File -LiteralPath (Join-Path $session 'sha256.txt')
} catch {
    $_ | Out-String | Out-File -LiteralPath (Join-Path $session 'error.txt')
    exit 1
}
