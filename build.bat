@echo off
if not exist build mkdir build
set WATCOM=C:\WATCOM
set PATH=%WATCOM%\binnt;%WATCOM%\binw;%PATH%
set INCLUDE=%WATCOM%\h;%WATCOM%\h\nt;%WATCOM%\h\win

echo --- Building Win16 Version (Sigma16.exe) ---
rem Compile Resources
%WATCOM%\binnt\wrc -q -r -bt=windows -i=%WATCOM%\h -i=%WATCOM%\h\win -fo=build\sigma16.res sigmaz.rc
if errorlevel 1 goto error

rem Compile & Link (Large Model needed for >64k data in benchmarks)
%WATCOM%\binnt\wcl -q -w4 -ml -bt=windows -i=%WATCOM%\h -i=%WATCOM%\h\win -l=windows main.c detect.c bench.c bench_float.c bench_mem.c bench_crypto.c bench_compress.c bench_matrix.c timer.c -fe=build\sigma16.exe -fo=build\
if errorlevel 1 goto error

rem Bind Resource
%WATCOM%\binnt\wrc -q build\sigma16.res build\sigma16.exe
if errorlevel 1 goto error


echo --- Building Win32 Version (Sigma32.exe) ---
rem Compile Resources
%WATCOM%\binnt\wrc -q -r -bt=nt -i=%WATCOM%\h -i=%WATCOM%\h\nt -fo=build\sigma32.res sigmaz.rc
if errorlevel 1 goto error

rem Compile & Link
%WATCOM%\binnt\wcl386 -q -w4 -bt=nt -l=nt_win -i=%WATCOM%\h -i=%WATCOM%\h\nt main.c detect.c bench.c bench_float.c bench_mem.c bench_crypto.c bench_compress.c bench_matrix.c timer.c -fe=build\sigma32.exe -fo=build\
if errorlevel 1 goto error

rem Bind Resource
%WATCOM%\binnt\wrc -q build\sigma32.res build\sigma32.exe
if errorlevel 1 goto error

echo Build Successful.
goto end

:error
echo Build Failed!
pause

:end
del build\*.obj
del build\*.res
if exist build\*.err del build\*.err
if exist *.err del *.err
if exist build\*.map del build\*.map
if exist *.map del *.map
