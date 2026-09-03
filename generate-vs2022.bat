@echo off
setlocal
cd /d "%~dp0"

if not "%~1"=="" set "SDL3_DIR=%~f1"
if not defined SDL3_DIR set "SDL3_DIR=%~dp0dependencies\SDL3"

if exist "%~dp0premake5.exe" (
	set "PREMAKE=%~dp0premake5.exe"
) else (
	where premake5.exe >nul 2>nul
	if errorlevel 1 (
		echo premake5.exe was not found in this directory or PATH.
		exit /b 1
	)
	set "PREMAKE=premake5.exe"
)

"%PREMAKE%" vs2022 --sdl3-dir="%SDL3_DIR%"
if errorlevel 1 exit /b %errorlevel%

echo Generated build\premake\OpenGromada.sln
