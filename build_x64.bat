@echo off
if not exist build mkdir build
echo --- Building x64 Version (Sigma64.exe) using MSVC ---

rem Check for MSVC
where cl.exe >nul 2>&1
if %errorlevel% neq 0 goto no_msvc

rem Compile Resources
echo Compiling Resources...
rc.exe /nologo /fo build\sigma64.res sigmaz.rc
if %errorlevel% neq 0 goto error

rem Compile & Link
echo Compiling Code...
rem Removed /arch:AVX2 to keep precision
cl.exe /nologo /O2 /W3 /D_CRT_SECURE_NO_WARNINGS main.c detect.c bench.c bench_float.c bench_mem.c bench_crypto.c bench_compress.c bench_matrix.c timer.c build\sigma64.res user32.lib kernel32.lib gdi32.lib /Fe:build\sigma64.exe /Fo:build\ /link /SUBSYSTEM:WINDOWS
if %errorlevel% neq 0 goto error

echo Build Successful: build\sigma64.exe
goto end

:no_msvc
echo Error: MSVC (cl.exe) not found in PATH.
echo Please run this from a "x64 Native Tools Command Prompt" for VS.
goto end

:error
echo Build Failed!

:end
if exist build\*.obj del build\*.obj
if exist build\*.res del build\*.res
pause
