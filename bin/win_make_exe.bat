@echo off
setlocal EnableDelayedExpansion

REM ------------------------------------------------------------
REM Get Windows SDK root from registry (safe parsing)
REM ------------------------------------------------------------

for /f "skip=2 tokens=2,*" %%A in (
  'reg query "HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots" /v KitsRoot10 2^>nul'
) do (
  set "SDKROOT=%%B"
)

if not defined SDKROOT (
  echo ERROR: Windows 10 SDK not found.
  exit /b 1
)

REM Remove trailing backslash
if "!SDKROOT:~-1!"=="\" set "SDKROOT=!SDKROOT:~0,-1!"

set "LIBBASE=!SDKROOT!\Lib"

REM ------------------------------------------------------------
REM Find latest SDK version (reverse sorted)
REM ------------------------------------------------------------

for /f %%V in ('dir /b /ad "!LIBBASE!" ^| sort /r') do (
  set "SDKVER=%%V"
  goto :foundver
)

:foundver

if not defined SDKVER (
  echo ERROR: No SDK versions found.
  exit /b 1
)

set "LIBPATH=!LIBBASE!\!SDKVER!\um\x64"

if not exist "!LIBPATH!\kernel32.lib" (
  echo ERROR: kernel32.lib not found in !LIBPATH!
  exit /b 1
)

REM ----------------------FOR DEBUGGING-------------------------
REM ------------------------------------------------------------
REM echo Using SDK version: !SDKVER!
REM echo Using LIBPATH: !LIBPATH!

set "PATH_TO_OBJECT_FILE=%~1\%2.o"

lld-link %PATH_TO_OBJECT_FILE% ^
  /LIBPATH:"!LIBPATH!" ^
  kernel32.lib ^
  /SUBSYSTEM:CONSOLE ^
  /ENTRY:_start ^
  /NODEFAULTLIB ^
  /OUT:%2.exe

if errorlevel 1 (
  echo ERROR: Linking failed.
  exit /b 1
)

REM -----------------------
REM echo Linking successful.
endlocal
