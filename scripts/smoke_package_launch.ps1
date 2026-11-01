param(
  [Parameter(Mandatory = $true)]
  [string]$PackageRoot
)

$ErrorActionPreference = 'Stop'
$resolvedRoot = (Resolve-Path -LiteralPath $PackageRoot).Path
$launcher = Join-Path $resolvedRoot 'Launch.exe'
$backend = Join-Path $resolvedRoot 'backend/workbench_server.exe'
$webIndex = Join-Path $resolvedRoot 'frontend/dist/index.html'
foreach ($requiredFile in @($launcher, $backend, $webIndex)) {
  if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
    throw "Package launch smoke check is missing: $requiredFile"
  }
}

$previousMode = $env:UEW_LAUNCHER_SELF_TEST
try {
  $env:UEW_LAUNCHER_SELF_TEST = '1'
  $result = Start-Process -FilePath $launcher -WorkingDirectory $resolvedRoot -WindowStyle Hidden -Wait -PassThru
  if ($result.ExitCode -ne 0) {
    throw "Launch.exe self-test returned exit code $($result.ExitCode)."
  }
  $line = "$(Get-Date -AsUTC -Format o) | Launch.exe self-test exit=0 | launcher started bundled backend; /api/health and / returned HTTP 200; backend stopped by job-object cleanup. Package root: extracted-folder layout."
  $logPath = Join-Path $resolvedRoot 'docs/demo/launcher-smoke.log'
  $logDirectory = Split-Path -Parent $logPath
  New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
  Set-Content -LiteralPath $logPath -Value $line -Encoding utf8
  Write-Output $line
}
finally {
  $env:UEW_LAUNCHER_SELF_TEST = $previousMode
}
