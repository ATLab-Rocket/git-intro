param(
    [Parameter(Mandatory=$true)][ValidateSet('F407','L476')][string]$Chip,
    [string]$CubeMX = "$env:LOCALAPPDATA\Programs\STM32CubeMX"
)
$ErrorActionPreference = 'Stop'
$canRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$project = Join-Path $canRoot "ATlab_CAN_$Chip"
$buildRoot = Join-Path $canRoot 'build'
New-Item -ItemType Directory -Force $buildRoot | Out-Null
$script = Join-Path $buildRoot "generate-$Chip.mx"
$metadata = @{}
foreach ($file in @('.project', '.cproject')) {
    $metadata[$file] = [System.IO.File]::ReadAllBytes((Join-Path $project $file))
}
# Java file.encoding must match UTF-8 script paths, including Chinese workspace names.
@"
config load "$project\ATlab_CAN_$Chip.ioc"
project path "$project"
project toolchain STM32CubeIDE
project generateunderroot 1
project generate
config save "$project\ATlab_CAN_$Chip.ioc"
exit
"@ | Set-Content -LiteralPath $script -Encoding utf8NoBOM
# Some CubeMX/CubeIDE service combinations stall during metadata generation.
# Bound that wait and preserve the reviewed Normal/Loopback configurations.
$java = Join-Path $CubeMX 'jre\bin\java.exe'
$mxJar = Join-Path $CubeMX 'STM32CubeMX.exe'
$arguments = @('-Dfile.encoding=UTF-8', '--add-opens', 'java.desktop/java.awt=ALL-UNNAMED', '--add-exports', 'java.desktop/sun.awt=ALL-UNNAMED', '-jar', "`"$mxJar`"", '-q', "`"$script`"")
try {
    $process = Start-Process -FilePath $java -ArgumentList $arguments -PassThru -WindowStyle Hidden
    $finished = $process.WaitForExit(180000)
    if (!$finished) {
        Stop-Process -Id $process.Id
        throw 'CubeMX timed out; generated files may be partial. Review diff and rebuild.'
    }
    if ($process.ExitCode -ne 0) { throw 'CubeMX generation failed' }
} finally {
    foreach ($file in $metadata.Keys) {
        [System.IO.File]::WriteAllBytes((Join-Path $project $file), $metadata[$file])
    }
}
python (Join-Path $PSScriptRoot 'sync_sources.py')
Write-Output 'Review metadata and re-run build.ps1 after regeneration; see README for generation limitations.'
