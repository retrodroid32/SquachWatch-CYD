@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
set "PIO_MODE="
set "PIO_EXE="
set "DEFAULT_PORT="
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
echo Detected serial ports:
call :run_pio device list
echo.
set /p "UPLOAD_PORT=COM port (for example COM17): "
if not defined UPLOAD_PORT (
  echo ERROR: No COM port selected.
  pause
  exit /b 1
)
echo(!UPLOAD_PORT!| findstr /R /I "^COM[0-9][0-9]*$" >nul
if errorlevel 1 (
  echo(!UPLOAD_PORT!| findstr /R "^[0-9][0-9]*$" >nul
  if not errorlevel 1 set "UPLOAD_PORT=COM!UPLOAD_PORT!"
)
echo Selected upload port: !UPLOAD_PORT!
choice /C YN /N /M "Flash SquachWatch to !UPLOAD_PORT!? [Y/N]: "
if errorlevel 2 exit /b 0
call :run_pio run -e cyd32-st7798 -t upload --upload-port !UPLOAD_PORT!
if errorlevel 1 (
  echo FLASH FAILED on !UPLOAD_PORT!.
  pause
  exit /b 1
)
echo FLASH OK - uploaded to !UPLOAD_PORT!
pause
exit /b 0

:run_pio
if /I "%PIO_MODE%"=="PIO" pio %*
if /I "%PIO_MODE%"=="PLATFORMIO" platformio %*
if /I "%PIO_MODE%"=="EXE" "%PIO_EXE%" %*
if /I "%PIO_MODE%"=="PY" py -m platformio %*
if /I "%PIO_MODE%"=="PYTHON" python -m platformio %*
exit /b %ERRORLEVEL%
