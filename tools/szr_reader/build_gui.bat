@echo off
setlocal EnableExtensions

if "%WATCOM%"=="" (
  set WATCOM=C:\WATCOM
)

if not exist "%WATCOM%\binnt\wcl.exe" (
  echo [ERROR] Open Watcom not found. Set WATCOM env first.
  exit /b 1
)

set PATH=%WATCOM%\binnt;%WATCOM%\binw;%PATH%
set INCLUDE=%WATCOM%\h;%WATCOM%\h\nt;%WATCOM%\h\win

echo --- Build GUI Win16 ---
%WATCOM%\binnt\wcl -q -w4 -ml -bt=windows -i=%WATCOM%\h -i=%WATCOM%\h\win szr_reader.c -l=windows -fe=szr_reader16.exe
if errorlevel 1 goto error

echo --- Build GUI Win32 ---
%WATCOM%\binnt\wcl386 -q -w4 -bt=nt -i=%WATCOM%\h -i=%WATCOM%\h\nt szr_reader.c -l=nt_win -fe=szr_reader32.exe
if errorlevel 1 goto error

echo Build success.
goto end

:error
echo Build failed.
exit /b 1

:end
if exist *.obj del *.obj
if exist *.err del *.err
if exist *.map del *.map
endlocal
