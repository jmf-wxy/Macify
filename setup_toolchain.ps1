# setup_toolchain.ps1
$toolPath = "D:\tools\w64devkit.7z.exe"
$targetDir = "D:\tools"

if (Test-Path $toolPath) {
    Write-Host "[1/3] Extracting w64devkit MinGW toolchain..." -ForegroundColor Cyan
    Start-Process -FilePath $toolPath -ArgumentList "-y -o`"$targetDir`"" -Wait -NoNewWindow

    $binPath = "D:\tools\w64devkit\bin"
    if (Test-Path "$binPath\g++.exe") {
        Write-Host "[2/3] Adding $binPath to User Environment PATH..." -ForegroundColor Cyan
        
        $currentPath = [System.Environment]::GetEnvironmentVariable("Path", "User")
        if ($currentPath -notlike "*$binPath*") {
            [System.Environment]::SetEnvironmentVariable("Path", "$currentPath;$binPath", "User")
        }
        $env:Path = "$binPath;$env:Path"

        Write-Host "[3/3] MinGW GCC/G++ successfully configured!" -ForegroundColor Green
        & "$binPath\g++.exe" --version
    } else {
        Write-Host "[Error] g++.exe not found after extraction." -ForegroundColor Red
    }
} else {
    Write-Host "[Error] $toolPath not found." -ForegroundColor Red
}
