@echo off
setlocal EnableExtensions
if not exist build mkdir build
echo --- Building x64 Version (Sigma64.exe) using MSVC ---

rem Check for MSVC
where cl.exe >nul 2>&1
if %errorlevel% neq 0 goto no_msvc

set CL_PATH=
for /f "delims=" %%I in ('where cl.exe') do (
	set "CL_PATH=%%~fI"
	goto got_cl_path
)

:got_cl_path
echo Using C compiler: %CL_PATH%

echo %CL_PATH% | find /I "WATCOM" >nul
if %errorlevel% equ 0 goto wrong_cl

set CL_IS_MSVC=
for /f "delims=" %%L in ('cl 2^>^&1') do (
	echo %%L | findstr /I /C:"Microsoft" >nul
	if not errorlevel 1 (
		set CL_IS_MSVC=1
		goto cl_checked
	)
)

:cl_checked
if not defined CL_IS_MSVC goto wrong_cl

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

:wrong_cl
echo Error: cl.exe is not the Microsoft compiler in current PATH.
echo Detected: %CL_PATH%
echo Please open "x64 Native Tools Command Prompt for VS" and run build_x64.bat again.
goto end

:error
echo Build Failed!

:end
if exist build\*.obj del build\*.obj
if exist build\*.res del build\*.res
endlocal
pause
