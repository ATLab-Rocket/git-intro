param(
    [string]$CubeIDE = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\stm32cubeidec.exe'
)
$ErrorActionPreference = 'Stop'
$canRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildRoot = Join-Path $canRoot 'build'
New-Item -ItemType Directory -Force $buildRoot | Out-Null
python (Join-Path $PSScriptRoot 'sync_sources.py')
if ($LASTEXITCODE -ne 0) { throw 'Source synchronization failed' }
foreach ($chip in @('F407', 'L476')) {
    foreach ($mode in @('Loopback', 'Normal')) {
        $name = "ATlab_CAN_$chip"
        $project = Join-Path $canRoot $name
        $workspace = Join-Path $buildRoot "ide-$chip-$mode"
        $log = Join-Path $buildRoot "$chip-$mode.log"
        # Never reuse a GUI workspace. Explicit configuration avoids active-only builds.
        & $CubeIDE --launcher.suppressErrors -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data $workspace -import $project -cleanBuild "$name/$mode" *> $log
        $exitCode = $LASTEXITCODE
        $output = Get-Content -LiteralPath $log -Raw
        $elf = Join-Path $project "$mode\$name.elf"
        if ($exitCode -ne 0 -or $output -match 'Build Failed|error:|warning:|Skipping' -or !(Test-Path -LiteralPath $elf) -or $output -notmatch 'Build Finished\. 0 errors, 0 warnings') {
            throw "Build verification failed: $chip $mode; inspect $log"
        }
        Write-Output "$chip $mode PASS (0 errors, 0 warnings)"
        $hash = (Get-FileHash -LiteralPath $elf -Algorithm SHA256).Hash
        Write-Output "ELF SHA256=$hash"
    }
}
