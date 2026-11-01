param(
  [ValidatePattern('^[A-Za-z0-9_-]+$')]
  [string]$CandidateId = 'CandidateID',
  [switch]$UseExistingNpmDependencies
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$packageName = "${CandidateId}_Workbench"
$packageOutput = Join-Path $repo $packageName
$archive = Join-Path $repo "$packageName.zip"
$stagingRoot = Join-Path $repo 'build/package-staging'
$package = Join-Path $stagingRoot $packageName

if (Test-Path -LiteralPath $packageOutput) { throw "Refusing to overwrite existing package directory: $packageOutput" }
if (Test-Path -LiteralPath $archive) { throw "Refusing to overwrite existing archive: $archive" }
if (Test-Path -LiteralPath $package) { throw "Refusing to overwrite existing staging directory: $package" }

Push-Location $repo
try {
  cmake --preset windows-msvc-x64-package
  if ($LASTEXITCODE -ne 0) { throw 'CMake package configuration failed.' }
  cmake --build --preset windows-msvc-x64-package --target workbench_server workbench_launcher --parallel
  if ($LASTEXITCODE -ne 0) { throw 'CMake package build failed.' }
  if ($UseExistingNpmDependencies) {
    npm ls --prefix frontend --depth=0
    if ($LASTEXITCODE -ne 0) { throw 'Existing npm dependencies do not match the direct package requirements.' }
  }
  else {
    npm --prefix frontend ci
    if ($LASTEXITCODE -ne 0) { throw 'npm dependency installation failed.' }
  }
  npm --prefix frontend run build
  if ($LASTEXITCODE -ne 0) { throw 'Frontend production build failed.' }
  $previousCandidateId = $env:UEW_CANDIDATE_ID
  $env:UEW_CANDIDATE_ID = $CandidateId
  try {
    py -3.12 (Join-Path $repo 'scripts/build_final_report.py')
    if ($LASTEXITCODE -ne 0) { throw 'Final report generation failed. Install requirements-report.txt with Python 3.12.' }
  }
  finally { $env:UEW_CANDIDATE_ID = $previousCandidateId }

  New-Item -ItemType Directory -Path $package -Force | Out-Null
  foreach ($directory in @('backend','frontend','include','src','data','tests','results','docs','scripts','.vscode')) {
    New-Item -ItemType Directory -Path (Join-Path $package $directory) -Force | Out-Null
  }
  Copy-Item (Join-Path $repo 'build/package-vs18-x64/Release/Launch.exe') (Join-Path $package 'Launch.exe')
  Copy-Item (Join-Path $repo 'build/package-vs18-x64/Release/workbench_server.exe') (Join-Path $package 'backend/workbench_server.exe')
  foreach ($file in @('CMakeLists.txt','CMakePresets.json','README.md','AI_USE_LOG.md','OWNERSHIP.md','requirements.txt','requirements-report.txt')) {
    Copy-Item (Join-Path $repo $file) (Join-Path $package $file)
  }
  foreach ($directory in @('include','src','data','tests','results','docs','scripts')) {
    Get-ChildItem -LiteralPath (Join-Path $repo $directory) -Force |
      Copy-Item -Destination (Join-Path $package $directory) -Recurse
  }
  Copy-Item (Join-Path $repo '.vscode/launch.json') (Join-Path $package '.vscode/launch.json')
  Copy-Item (Join-Path $repo '.vscode/tasks.json') (Join-Path $package '.vscode/tasks.json')
  $launchConfigPath = Join-Path $package '.vscode/launch.json'
  $launchConfig = Get-Content -LiteralPath $launchConfigPath -Raw
  $launchConfig = $launchConfig.Replace('CandidateID_Workbench', "${CandidateId}_Workbench")
  $launchConfig = $launchConfig.Replace('${workspaceFolder}/' + "${CandidateId}_Workbench/Launch.exe", '${workspaceFolder}/Launch.exe')
  $launchConfig = $launchConfig.Replace('${workspaceFolder}/' + "${CandidateId}_Workbench", '${workspaceFolder}')
  Set-Content -LiteralPath $launchConfigPath -Value $launchConfig -Encoding utf8

  $frontend = Join-Path $package 'frontend'
  foreach ($path in @('frontend/src','frontend/dist')) {
    $frontendSubdirectory = Split-Path -Leaf $path
    $frontendDestination = Join-Path $frontend $frontendSubdirectory
    New-Item -ItemType Directory -Path $frontendDestination -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $repo $path) -Force |
      Copy-Item -Destination $frontendDestination -Recurse
  }
  foreach ($file in @('index.html','package.json','package-lock.json','vite.config.js')) {
    Copy-Item (Join-Path $repo "frontend/$file") $frontend
  }

  $notice = [System.Collections.Generic.List[string]]::new()
  $notice.Add('Third-party software notices for the Unified Engineering Workbench package')
  $notice.Add('')
  $lock = Get-Content (Join-Path $repo 'frontend/package-lock.json') -Raw | ConvertFrom-Json -AsHashtable
  foreach ($entry in $lock['packages'].GetEnumerator()) {
    if (-not $entry.Key.StartsWith('node_modules/')) { continue }
    $modulePath = Join-Path (Join-Path $repo 'frontend') $entry.Key
    $metadataPath = Join-Path $modulePath 'package.json'
    if (-not (Test-Path -LiteralPath $metadataPath)) { continue }
    $metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
    $notice.Add("## $($metadata.name) $($metadata.version) - $($metadata.license)")
    $licenseFile = Get-ChildItem -LiteralPath $modulePath -File | Where-Object { $_.Name -match '^(LICENSE|LICENCE|COPYING|NOTICE)' } | Select-Object -First 1
    if (-not $licenseFile -and $metadata.name.StartsWith('@esbuild/')) {
      $licensePath = Join-Path $repo 'frontend/node_modules/esbuild/LICENSE.md'
      if (Test-Path -LiteralPath $licensePath) { $licenseFile = Get-Item -LiteralPath $licensePath }
    }
    if (-not $licenseFile -and $metadata.name.StartsWith('@rollup/rollup-')) {
      $licensePath = Join-Path $repo 'frontend/node_modules/rollup/LICENSE.md'
      if (Test-Path -LiteralPath $licensePath) { $licenseFile = Get-Item -LiteralPath $licensePath }
    }
    if ($licenseFile) { $notice.Add((Get-Content -LiteralPath $licenseFile.FullName -Raw).Trim()) }
    else { $notice.Add('License text was not present in the installed package. See the package metadata and upstream project.') }
    $notice.Add('')
  }
  Set-Content -LiteralPath (Join-Path $frontend 'THIRD_PARTY_NOTICES.txt') -Value $notice -Encoding utf8

  $cppNotice = [System.Collections.Generic.List[string]]::new()
  $cppNotice.Add('C++ dependency license notices. Versions and hashes are recorded in CMakeLists.txt.')
  $cppNotice.Add('')
  foreach ($dependency in @(
      @{Name='cpp-httplib 0.18.3'; Path='build/vs18-x64/_deps/httplib-src/LICENSE'},
      @{Name='nlohmann/json 3.12.0'; Path='build/vs18-x64/_deps/nlohmann_json-3.12.0-src/LICENSE.MIT'},
      @{Name='GoogleTest 1.15.2 (build/test dependency; not in runtime binaries)'; Path='build/vs18-x64/_deps/googletest-src/LICENSE'})) {
    $licensePath = Join-Path $repo $dependency.Path
    if (-not (Test-Path -LiteralPath $licensePath)) { throw "Missing dependency license file: $licensePath" }
    $cppNotice.Add("## $($dependency.Name)")
    $cppNotice.Add((Get-Content -LiteralPath $licensePath -Raw).Trim())
    $cppNotice.Add('')
  }
  Set-Content -LiteralPath (Join-Path $package 'THIRD_PARTY_NOTICES.txt') -Value $cppNotice -Encoding utf8

  $packageReadmePath = Join-Path $package 'README.md'
  $packageReadme = Get-Content -LiteralPath $packageReadmePath -Raw
  $packageReadme = $packageReadme.Replace('Open the repository root, place the generated', 'For source-workspace use, open the repository root and place the generated')
  $packageReadme += "`r`n`r`nFor the extracted package, open this directory itself in VS Code; its F5 configuration starts the `Launch.exe` in this folder.`r`n"
  $packageReadme = $packageReadme.Replace('${workspaceFolder}/' + "${CandidateId}_Workbench/Launch.exe", '${workspaceFolder}/Launch.exe')
  Set-Content -LiteralPath $packageReadmePath -Value $packageReadme -Encoding utf8

  foreach ($textFile in Get-ChildItem -LiteralPath $package -Recurse -File | Where-Object { $_.Extension -in @('.md','.txt','.json') }) {
    $text = Get-Content -LiteralPath $textFile.FullName -Raw
    if ($CandidateId -ne 'CandidateID' -and $text.Contains('CandidateID')) {
      Set-Content -LiteralPath $textFile.FullName -Value $text.Replace('CandidateID', $CandidateId) -Encoding utf8
    }
    if ($CandidateId -ne 'CandidateID' -and $text.Contains('<CandidateId>')) {
      $text = Get-Content -LiteralPath $textFile.FullName -Raw
      Set-Content -LiteralPath $textFile.FullName -Value $text.Replace('<CandidateId>', $CandidateId) -Encoding utf8
    }
  }

  $required = @('Launch.exe','backend/workbench_server.exe','frontend/dist/index.html','data/examples/unit_tetrahedron_ascii.stl','data/examples/synthetic_temperature_points.csv','AI_USE_LOG.md','OWNERSHIP.md','docs/FINAL_TECHNICAL_REPORT.pdf','docs/submission/REQUIREMENT_EVIDENCE.md','THIRD_PARTY_NOTICES.txt','frontend/THIRD_PARTY_NOTICES.txt')
  foreach ($relativePath in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $package $relativePath))) { throw "Required package file is missing: $relativePath" }
  }
  & (Join-Path $package 'scripts/smoke_package_launch.ps1') -PackageRoot $package
  if ($LASTEXITCODE -ne 0) { throw 'Packaged Launch.exe self-test failed.' }
  Compress-Archive -LiteralPath $package -DestinationPath $archive -CompressionLevel Optimal
  Move-Item -LiteralPath $package -Destination $packageOutput
  Write-Output "Package directory: $packageOutput"
  Write-Output "ZIP archive: $archive"
  Get-FileHash -Algorithm SHA256 -LiteralPath $archive | Format-List
}
finally {
  Pop-Location
  if ((Test-Path -LiteralPath $package) -and -not (Test-Path -LiteralPath $packageOutput)) {
    Remove-Item -LiteralPath $package -Recurse -Force
  }
}
