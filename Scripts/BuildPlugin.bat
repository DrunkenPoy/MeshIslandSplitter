@echo off
REM Usage: BuildPlugin.bat [EngineRoot]
REM Default EngineRoot: C:\Program Files\Epic Games\UE_5.8
set ENGINE=%~1
if "%ENGINE%"=="" set ENGINE=C:\Program Files\Epic Games\UE_5.8
set REPO=%~dp0..
set OUT=%REPO%\Packaged

"%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin ^
  -Plugin="%REPO%\MeshIslandSplitter.uplugin" ^
  -Package="%OUT%" ^
  -TargetPlatforms=Win64 ^
  -Rocket
