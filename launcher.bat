@echo off
setlocal
rem the "short" name in launcher\game.json: the launcher is <short>Launcher.exe, its work folder <short>
set "GAME_SHORT=PGR1"
rem Builds <short>Launcher.exe (sdk\launcher, set up by launcher\game.json): the one exe a player runs to build and play
rem the game from their own disc. Needs what build.bat needs (Visual Studio with clang, CMake, Ninja, Git).
cd /d "%~dp0"
echo.
echo   %GAME_SHORT% launcher - build
echo   ================================================
echo.
if exist .git (
    where git >nul 2>nul && git submodule update --init sdk
)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
    echo   Visual Studio with the C++ workload was not found: run build.bat once, it sets it up.
    goto :fail
)
call "%VSDIR%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
set "VSCMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%VSCMAKE%\CMake\bin;%VSCMAKE%\Ninja;%VSDIR%\VC\Tools\Llvm\x64\bin;%PATH%"

cmake -S sdk\launcher -B out\launcher -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLAUNCHER_GAME="%CD%\launcher" || goto :fail
cmake --build out\launcher || goto :fail
copy /y out\launcher\%GAME_SHORT%Launcher.exe . >nul || goto :fail
echo.
echo   Built: %CD%\%GAME_SHORT%Launcher.exe
echo   Copy it into an empty folder anywhere (it works beside itself, in a %GAME_SHORT% folder) and run it.
echo.
pause
exit /b 0

:fail
echo.
echo   LAUNCHER BUILD FAILED. Read the last message above.
echo.
pause
exit /b 1
