$ErrorActionPreference = "Stop"

function Resolve-GitExe {
    # 1) Git already available in PATH.
    $cmd = Get-Command git.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # 2) Standard Git for Windows locations.
    $candidates = @(
        "$env:ProgramFiles\Git\cmd\git.exe",
        "${env:ProgramFiles(x86)}\Git\cmd\git.exe",
        "$env:LOCALAPPDATA\Programs\Git\cmd\git.exe"
    ) | Where-Object { $_ -and (Test-Path $_) }

    if ($candidates.Count -gt 0) { return $candidates[0] }

    # 3) GitHub Desktop includes its own Git even when git.exe is not in PATH.
    $desktopRoot = Join-Path $env:LOCALAPPDATA "GitHubDesktop"
    if (Test-Path $desktopRoot) {
        $desktopCandidates = Get-ChildItem -Path $desktopRoot -Directory -Filter "app-*" -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object {
                @(
                    (Join-Path $_.FullName "resources\app\git\cmd\git.exe"),
                    (Join-Path $_.FullName "resources\app\git\mingw64\bin\git.exe")
                )
            } | Where-Object { Test-Path $_ }

        if ($desktopCandidates.Count -gt 0) { return $desktopCandidates[0] }
    }

    return $null
}

$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $RepoRoot

if (-not (Test-Path ".git")) {
    Write-Host "ERROR: This script must be inside the root Git repository folder (the same folder as .git and Thesis.uproject)." -ForegroundColor Red
    Write-Host "Current folder: $RepoRoot" -ForegroundColor Yellow
    exit 1
}

$Git = Resolve-GitExe
if (-not $Git) {
    Write-Host "ERROR: Git executable was not found." -ForegroundColor Red
    Write-Host "GitHub Desktop is installed, but its bundled Git could not be located automatically." -ForegroundColor Yellow
    Write-Host "Install Git for Windows, then reopen this script:" -ForegroundColor Yellow
    Write-Host "https://git-scm.com/download/win" -ForegroundColor Cyan
    exit 1
}

Write-Host "Using Git: $Git" -ForegroundColor DarkGray
Write-Host "Cleaning generated Unreal / IDE files from Git tracking..." -ForegroundColor Cyan

$generatedPaths = @(
    "Binaries",
    "DerivedDataCache",
    "Intermediate",
    "Saved",
    ".idea",
    ".vs",
    ".vscode"
)

foreach ($path in $generatedPaths) {
    if (Test-Path $path) {
        Write-Host "  Untracking $path" -ForegroundColor DarkGray
    }
    & $Git rm -r --cached --ignore-unmatch -- "$path" 2>$null | Out-Null
}

# Ensure ignore rules and project source/assets are staged.
& $Git add -- ".gitignore" ".gitattributes"

$keepPaths = @("Thesis.uproject", "Config", "Content", "Source")
foreach ($path in $keepPaths) {
    if (Test-Path $path) {
        & $Git add -- "$path"
    }
}

Write-Host ""
Write-Host "Done. Generated folders remain on your PC but are removed from Git tracking." -ForegroundColor Green
Write-Host "GitHub Desktop should now stop showing Binaries / Intermediate / .idea / Saved." -ForegroundColor Green
Write-Host ""
Write-Host "Remaining Git changes:" -ForegroundColor Cyan
& $Git status --short
Write-Host ""
Write-Host "If no file above is over 100 MB, return to GitHub Desktop and commit normally." -ForegroundColor Green
