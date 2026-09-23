@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "PIO_MODE="
set "PIO_EXE="
where pio.exe >nul 2>nul && set "PIO_MODE=PIO" && goto :pio_found
where platformio.exe >nul 2>nul && set "PIO_MODE=PLATFORMIO" && goto :pio_found
if exist "%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" (
  set "PIO_MODE=EXE"
  set "PIO_EXE=%USERPROFILE%\.platformio\penv\Scripts\platformio.exe"
  goto :pio_found
)
where py.exe >nul 2>nul
if not errorlevel 1 (
  py -m platformio --version >nul 2>nul
  if not errorlevel 1 set "PIO_MODE=PY" & goto :pio_found
)
where python.exe >nul 2>nul
if not errorlevel 1 (
  python -m platformio --version >nul 2>nul
  if not errorlevel 1 set "PIO_MODE=PYTHON" & goto :pio_found
)
echo ERROR: PlatformIO Core could not be found.
pause
exit /b 1

:pio_found
echo PlatformIO found using: %PIO_MODE%
if /I "%PIO_MODE%"=="PIO" pio run -e cyd32-st7798
if /I "%PIO_MODE%"=="PLATFORMIO" platformio run -e cyd32-st7798
if /I "%PIO_MODE%"=="EXE" "%PIO_EXE%" run -e cyd32-st7798
if /I "%PIO_MODE%"=="PY" py -m platformio run -e cyd32-st7798
if /I "%PIO_MODE%"=="PYTHON" python -m platformio run -e cyd32-st7798
if errorlevel 1 (
  echo BUILD FAILED.
  pause
  exit /b 1
)
echo BUILD OK
echo Firmware: .pio\build\cyd32-st7798\firmware.bin
pause
