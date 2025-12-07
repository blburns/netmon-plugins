# scripts/download_vendor.ps1
# PowerShell script to manually download vendor protocol headers (Windows)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$ThirdPartyDir = Join-Path $ProjectRoot "vendor"
$IncludeDir = Join-Path $ThirdPartyDir "include"

# Create directories
New-Item -ItemType Directory -Force -Path $IncludeDir | Out-Null

Write-Host "Downloading vendor protocol headers..." -ForegroundColor Green

# Example: Download a protocol header
# Uncomment and modify as needed

# Example 1: Download a single header file
# Write-Host "Downloading example_protocol.h..." -ForegroundColor Yellow
# Invoke-WebRequest -Uri "https://raw.githubusercontent.com/example/protocol/master/include/protocol.h" `
#     -OutFile (Join-Path $IncludeDir "example_protocol.h")

# Example 2: Download from a Git repository
# $RepoDir = Join-Path $ThirdPartyDir "src\example_protocol"
# if (-not (Test-Path $RepoDir)) {
#     Write-Host "Cloning example_protocol repository..." -ForegroundColor Yellow
#     git clone --depth 1 --branch v1.0.0 `
#         https://github.com/example/protocol.git `
#         $RepoDir
#     Copy-Item (Join-Path $RepoDir "include\*.h") $IncludeDir
# }

# Example 3: Download and extract a tarball/zip
# $ZipFile = Join-Path $env:TEMP "example_protocol.zip"
# if (-not (Test-Path (Join-Path $IncludeDir "example_protocol.h"))) {
#     Write-Host "Downloading example_protocol archive..." -ForegroundColor Yellow
#     Invoke-WebRequest -Uri "https://example.com/protocol-1.0.0.zip" -OutFile $ZipFile
#     Expand-Archive -Path $ZipFile -DestinationPath $env:TEMP -Force
#     Copy-Item (Join-Path $env:TEMP "protocol-1.0.0\include\*.h") $IncludeDir
#     Remove-Item $ZipFile -Force
# }

Write-Host "Third-party headers downloaded to: $IncludeDir" -ForegroundColor Green
Write-Host ""
Write-Host "To use these headers in your plugins, include them like:" -ForegroundColor Cyan
Write-Host "  #include `"vendor/example_protocol.h`"" -ForegroundColor Cyan
Write-Host ""
Write-Host "Note: Make sure to add the include directory in CMakeLists.txt" -ForegroundColor Yellow

