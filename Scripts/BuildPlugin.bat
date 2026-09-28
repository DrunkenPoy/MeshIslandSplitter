@echo off
setlocal EnableDelayedExpansion
REM Usage: BuildPlugin.bat [Version ...]
REM Builds the plugin for each engine version and writes Packaged\<Major>_<Minor>.zip
REM Default versions: 5.7 5.8 (engines under C:\Program Files\Epic Games\UE_<Version>)
REM Override the engines folder with EPIC_ROOT.
REM
REM The build runs in %TEMP%\mis to stay under the 260-char path limit,
REM which long repo/worktree paths otherwise exceed.

set VERSIONS=%*
if "%VERSIONS%"=="" set VERSIONS=5.7 5.8
if "%EPIC_ROOT%"=="" set EPIC_ROOT=C:\Program Files\Epic Games
set REPO=%~dp0..
set OUT=%REPO%\Packaged
set WORK=%TEMP%\mis
if not exist "%OUT%" mkdir "%OUT%"

set FAILED=
for %%V in (%VERSIONS%) do (
  set VER=%%V
  set TAG=!VER:.=_!
  set PKG=%WORK%\!TAG!\MeshIslandSplitter
  echo ==== Building for UE %%V ====
  if exist "%WORK%\!TAG!" rmdir /s /q "%WORK%\!TAG!"
  call "%EPIC_ROOT%\UE_%%V\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin ^
    -Plugin="%REPO%\MeshIslandSplitter.uplugin" ^
    -Package="!PKG!" ^
    -TargetPlatforms=Win64 ^
    -Rocket
  if errorlevel 1 (
    set FAILED=!FAILED! %%V
  ) else (
    if exist "!PKG!\Intermediate" rmdir /s /q "!PKG!\Intermediate"
    if exist "!PKG!\HostProject" rmdir /s /q "!PKG!\HostProject"
    powershell -NoProfile -Command "Compress-Archive -Path '!PKG!' -DestinationPath '%OUT%\!TAG!.zip' -Force"
    echo Created %OUT%\!TAG!.zip
  )
)

if not "%FAILED%"=="" (
  echo Build failed for:%FAILED%
  exit /b 1
)
exit /b 0
