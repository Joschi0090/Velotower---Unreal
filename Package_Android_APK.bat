@echo off
title Velocity Tower - Android APK Builder
echo ============================================================================
echo   VELOCITY TOWER // VICE ^& ICE APEX - ANDROID APK PACKAGING
echo ============================================================================
echo.
set "UAT_PATH=O:\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat"
set "PROJECT_PATH=%~dp0VelocityTower.uproject"
set "OUTPUT_DIR=%~dp0Build\Android_Release"
set "ANDROID_HOME=C:\Users\highn\AppData\Local\Android\Sdk"
set "NDKROOT=C:\Users\highn\AppData\Local\Android\Sdk\ndk\27.2.12479018"
set "NDK_ROOT=C:\Users\highn\AppData\Local\Android\Sdk\ndk\27.2.12479018"
set "JAVA_HOME=C:\Program Files\Microsoft\jdk-21.0.12.101-hotspot"
set "PATH=%JAVA_HOME%\bin;%ANDROID_HOME%\platform-tools;%PATH%"

echo Pruefe ob LiveCodingConsole aktiv ist...
taskkill /F /IM LiveCodingConsole.exe >nul 2>&1

echo Starte Android APK Packaging nach: "%OUTPUT_DIR%"
echo Bitte warten (dauert ca. 2-5 Minuten)...
echo.

call "%UAT_PATH%" BuildCookRun -project="%PROJECT_PATH%" -noP4 -platform=Android -cookflavor=ASTC -clientconfig=Development -cook -allmaps -build -stage -package -pak -archive -archivedirectory="%OUTPUT_DIR%" -utf8output
if errorlevel 1 (
    echo.
    echo [FEHLER] Android Build fehlgeschlagen! Falls der Unreal Editor noch offen ist, bitte kurz schliessen und erneut starten.
    pause
    exit /b 1
)

echo.
echo ============================================================================
echo   ERFOLGREICH! Deine fertige Android APK liegt hier:
echo   %OUTPUT_DIR%\VelocityTower-arm64.apk
echo ============================================================================
pause
