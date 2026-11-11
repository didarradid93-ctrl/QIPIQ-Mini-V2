param(
    [Parameter(Mandatory)]
    [ValidatePattern('^COM[1-9][0-9]*$')]
    [string]$Port,

    [string]$Python = 'python',
    [string]$BackupPath
)

$ErrorActionPreference = 'Stop'
$backupSize = 16MB
$requiredFiles = @(
    'flash_args',
    'bootloader/bootloader.bin',
    'partition_table/partition-table.bin',
    'qipiq_mini_v2.bin'
)
foreach ($name in $requiredFiles) {
    if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf)) {
        throw "Missing firmware file: $name"
    }
}

& $Python -m esptool version
if ($LASTEXITCODE -ne 0) {
    throw 'Install esptool with python -m pip install esptool and try again.'
}

$prefix = @('-m', 'esptool', '--chip', 'esp32s3', '--port', $Port, '--baud', '460800')
if ($BackupPath) {
    $backup = Get-Item -LiteralPath $BackupPath
    if ($backup.PSIsContainer -or $backup.Length -ne $backupSize) {
        throw 'Provide a full 16 MiB flash backup from this board.'
    }
} else {
    $name = "QIPIQ-$Port-$(Get-Date -Format 'yyyyMMdd-HHmmss')-backup.bin"
    $target = Join-Path (Get-Location).Path $name
    $partial = "$target.part"
    if ((Test-Path -LiteralPath $target) -or (Test-Path -LiteralPath $partial)) {
        throw "Backup already exists: $target"
    }
    & $Python @prefix read-flash 0 ALL $partial
    if ($LASTEXITCODE -ne 0) {
        throw "Backup failed. Review $partial before retrying."
    }
    if ((Get-Item -LiteralPath $partial).Length -ne $backupSize) {
        throw 'The board did not report 16 MiB of flash.'
    }
    Move-Item -LiteralPath $partial -Destination $target
    $backup = Get-Item -LiteralPath $target
}
Write-Host "Backup saved at $($backup.FullName)"
Get-FileHash -LiteralPath $backup.FullName -Algorithm SHA256

Push-Location -LiteralPath $PSScriptRoot
try {
    & $Python @prefix write-flash '@flash_args'
    if ($LASTEXITCODE -ne 0) {
        throw 'Writing firmware failed. Keep the backup and consult the recovery instructions.'
    }
} finally {
    Pop-Location
}
