@echo off
title Velocity Tower - Windows EXE Builder
echo ============================================================================
echo   VELOCITY TOWER // VICE ^& ICE APEX - WINDOWS EXE PACKAGING
echo ============================================================================
echo.
set "UAT_PATH=O:\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat"
set "PROJECT_PATH=%~dp0VelocityTower.uproject"
set "OUTPUT_DIR=%~dp0Build\Windows_Release"

echo Pruefe ob LiveCodingConsole aktiv ist...
taskkill /F /IM LiveCodingConsole.exe >nul 2>&1

echo Starte Packaging nach: "%OUTPUT_DIR%"
echo Bitte warten (dauert beim ersten Mal ca. 2-5 Minuten)...
echo.

call "%UAT_PATH%" BuildCookRun -project="%PROJECT_PATH%" -noP4 -platform=Win64 -clientconfig=Development -serverconfig=Development -cook -allmaps -build -nocompileeditor -skipbuildeditor -stage -pak -archive -archivedirectory="%OUTPUT_DIR%" -utf8output
if errorlevel 1 (
    echo.
    echo [FEHLER] Build fehlgeschlagen! Falls der Unreal Editor noch offen ist, bitte kurz schliessen und erneut starten.
    pause
    exit /b 1
)

if exist "%~dp0RawAssets" (
    echo Kopiere RawAssets in alle relevanten Build-Verzeichnisse...
    xcopy /E /I /Y "%~dp0RawAssets" "%OUTPUT_DIR%\VelocityTower\RawAssets" >nul
    xcopy /E /I /Y "%~dp0RawAssets" "%OUTPUT_DIR%\Windows\VelocityTower\RawAssets" >nul
    xcopy /E /I /Y "%~dp0RawAssets" "%OUTPUT_DIR%\RawAssets" >nul
)

echo.
echo ============================================================================
echo   ERFOLGREICH! Deine fertige Version fuer deinen Kumpel liegt hier:
echo   %OUTPUT_DIR%\VelocityTower.exe
echo   (WICHTIG: Immer den gesamten Ordner "Windows_Release" als .zip verschicken!)
echo ============================================================================
pause
