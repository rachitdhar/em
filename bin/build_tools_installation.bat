@echo off
setlocal

echo Checking for Visual Studio Build Tools installer...

set INSTALLER=vs_BuildTools.exe

:: Step 1: Check if installer exists
if exist "%INSTALLER%" (
    echo Installer already present.
) else (
    echo Installer not found. Downloading...
)

curl -L -o "%INSTALLER%" https://aka.ms/vs/stable/vs_BuildTools.exe

if not exist "%INSTALLER%" (
    echo ERROR: Failed to download installer.
    exit /b 1
)

echo Download complete.

:: Step 2: Launch installer with UI
echo Launching Visual Studio Build Tools installer...

start "" "%INSTALLER%" ^
  --add Microsoft.VisualStudio.Workload.VCTools ^
  --includeRecommended

echo Installer launched. Please follow the on-screen instructions.
echo Make sure to select "C++ build tools".

endlocal
