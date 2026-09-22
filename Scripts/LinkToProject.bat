@echo off
REM Usage: LinkToProject.bat "D:\UE\MyHostProject"
REM Creates a directory junction <HostProject>\Plugins\MeshIslandSplitter -> this repo.
if "%~1"=="" (
  echo Usage: %~nx0 "path\to\HostProject"
  exit /b 1
)
set REPO=%~dp0..
if not exist "%~1\Plugins" mkdir "%~1\Plugins"
mklink /J "%~1\Plugins\MeshIslandSplitter" "%REPO%"
