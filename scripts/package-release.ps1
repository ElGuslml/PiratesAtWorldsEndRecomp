# Packages a release of PiratesAtWorldsEndRecomp from a Release build into a folder and a zip:
#   - the two executables and the DLLs next to them;
#   - README.md, LICENSE, THIRD_PARTY_NOTICES.md;
#   - assets\README.md (the user copies the game files there);
#   - licenses\: the license text of every component in the binaries (the ReXGlue SDK and its components,
#     NativeXRenderer, DiligentCore, SPIRV-Cross, volk, SMAA, NVIDIA DLSS, and the launcher's RecompLauncher,
#     Barlow fonts, nanosvg and JSON for Modern C++);
#   - the zip's SHA-256 next to it (<zip>.sha256), for the launcher's updater and anyone checking the download.
# It never takes userdata\, logs\, *.toml or *.pdb from the build folder: those are the builder's saves,
# caches (the game's shaders among them), settings and debug data.
#
#   .\scripts\package-release.ps1 [-Build out\build\win-amd64-release] [-Out out\release]
[CmdletBinding()]
param(
  [string]$Build = "$PSScriptRoot\..\out\build\win-amd64-release",
  [string]$Out = "$PSScriptRoot\..\out\release"
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path "$PSScriptRoot\..").Path
$Build = (Resolve-Path $Build).Path
$Out = [System.IO.Path]::GetFullPath($Out)

$versionLine = Select-String -Path "$repo\res\version.h" -Pattern 'POTCAWERECOMP_VERSION_STR "([^"]+)"'
$version = $versionLine.Matches[0].Groups[1].Value
$name = "PiratesAtWorldsEndRecomp-v$version-windows-x64"
$dest = Join-Path $Out $name
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
New-Item -ItemType Directory -Force "$dest\assets", "$dest\licenses" | Out-Null

# The program.
foreach ($exe in @("PiratesAtWorldsEndRecomp.exe", "PiratesAtWorldsEnd Launcher.exe")) {
  Copy-Item -LiteralPath "$Build\$exe" $dest
}
Get-ChildItem -LiteralPath $Build -Filter *.dll | ForEach-Object { Copy-Item -LiteralPath $_.FullName $dest }

# Documents.
Copy-Item "$repo\README.md", "$repo\LICENSE", "$repo\THIRD_PARTY_NOTICES.md" $dest
Copy-Item "$repo\assets\README.md" "$dest\assets\README.md"

# Licenses of the components in the binaries.
function Add-License([string]$component, [string]$path) {
  if (-not (Test-Path -LiteralPath $path)) { throw "license file not found: $path" }
  $file = Split-Path $path -Leaf
  Copy-Item -LiteralPath $path "$dest\licenses\$component-$file"
}
$nxr = "$repo\native_x_renderer"
$diligent = "$nxr\thirdparty\diligent-core"
Add-License "NativeXRenderer" "$nxr\LICENSE"
Add-License "NativeXRenderer" "$nxr\THIRD_PARTY_NOTICES.md"
Add-License "DiligentCore" "$diligent\License.txt"
Add-License "SPIRV-Cross" "$diligent\ThirdParty\SPIRV-Cross\LICENSE"
Add-License "volk" "$diligent\ThirdParty\volk\LICENSE.md"
Add-License "SMAA" "$nxr\thirdparty\smaa\LICENSE.txt"
Add-License "NVIDIA-DLSS" "$Build\_deps\dlss-headers-src\LICENSE.txt"
Add-License "ReXGlue-SDK" "$repo\thirdparty\rexglue-sdk\LICENSE"
# The launcher's common code, its fonts and its SVG rasterizer (the RecompLauncher submodule, recomp_launcher\).
Add-License "RecompLauncher" "$repo\recomp_launcher\LICENSE"
Add-License "Barlow-fonts" "$repo\recomp_launcher\fonts\OFL.txt"
Add-License "nanosvg" "$repo\recomp_launcher\thirdparty\nanosvg\LICENSE.txt"
# JSON for Modern C++, which the launcher's updater reads GitHub's answer with (from the SDK's inja).
Add-License "nlohmann-json" "$repo\thirdparty\rexglue-sdk\thirdparty\inja\third_party\include\nlohmann\LICENSE.MIT"
# The SDK's components, as THIRD_PARTY_NOTICES.md lists them (license files under
# thirdparty\rexglue-sdk\thirdparty\; licenses given in a header are in that header).
$sdkThirdParty = "$repo\thirdparty\rexglue-sdk\thirdparty"
$notices = Get-Content "$repo\THIRD_PARTY_NOTICES.md"
foreach ($line in $notices) {
  if ($line -notmatch '^\| (?<component>[^|]+?) \| [^|]+ \| (?<files>[^|]+) \|$') { continue }
  $component = (($Matches.component -replace '\(.*\)', '').Trim() -replace '[^A-Za-z0-9+.-]+', '-').Trim('-')
  foreach ($m in [regex]::Matches($Matches.files, '`([^`]+)`')) {
    $relative = $m.Groups[1].Value
    if ($relative -like "*.h") { continue }
    $path = Join-Path $sdkThirdParty ($relative -replace '/', '\')
    if (Test-Path -LiteralPath $path) { Add-License $component $path }
  }
}

# Nothing of the builder's own.
$forbidden = Get-ChildItem -Recurse -LiteralPath $dest | Where-Object {
  $_.Name -like "*.pdb" -or $_.Name -like "*.toml" -or $_.Name -like "*.log" -or
  $_.FullName -match '\\(userdata|logs|cache)(\\|$)'
}
if ($forbidden) { throw "the package contains files it must not: $($forbidden.FullName -join ', ')" }

# The zip holds the folder itself, with "/" separators (Compress-Archive in Windows PowerShell 5.1 writes "\",
# which extractors outside Windows take as part of the file name).
$zip = Join-Path $Out "$name.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($zip, [System.IO.Compression.ZipArchiveMode]::Create)
try {
  Get-ChildItem -Recurse -LiteralPath $dest -File | ForEach-Object {
    $entry = "$name/" + ($_.FullName.Substring($dest.Length + 1) -replace '\\', '/')
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $_.FullName, $entry,
      [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
  }
} finally {
  $archive.Dispose()
}
Get-ChildItem -Recurse -LiteralPath $dest -File | ForEach-Object {
  "{0,10:N0}  {1}" -f $_.Length, $_.FullName.Substring($dest.Length + 1)
}
"zip: $zip ({0:N1} MB)" -f ((Get-Item $zip).Length / 1MB)

# Its SHA-256, as sha256sum writes it. The launcher's updater checks the zip against GitHub's digest of the asset,
# or against this file when it is attached to the release too (<zip>.sha256).
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $zip).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText("$zip.sha256", "$hash  $name.zip`n", (New-Object System.Text.UTF8Encoding($false)))
"sha256: $hash"

# The installer (installer\potcawe.iss) from the same folder, when Inno Setup 6.6 or later is installed
# (winget install JRSoftware.InnoSetup).
$iscc = @((Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source,
          "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
          "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
          "$env:ProgramFiles\Inno Setup 6\ISCC.exe") | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if ($iscc) {
  $setup = Join-Path $Out "PiratesAtWorldsEndRecomp-v$version-setup.exe"
  & $iscc /Q "/DAppVersion=$version" "/DSourceDir=$dest" "/DOutputDir=$Out" "$repo\installer\potcawe.iss"
  if ($LASTEXITCODE -ne 0) { throw "ISCC failed ($LASTEXITCODE)" }
  $setupHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $setup).Hash.ToLowerInvariant()
  [System.IO.File]::WriteAllText("$setup.sha256", "$setupHash  $(Split-Path $setup -Leaf)`n",
                                 (New-Object System.Text.UTF8Encoding($false)))
  "installer: $setup ({0:N1} MB)" -f ((Get-Item $setup).Length / 1MB)
} else {
  Write-Warning "Inno Setup not found: no installer (winget install JRSoftware.InnoSetup)"
}
