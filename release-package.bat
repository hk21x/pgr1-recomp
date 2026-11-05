@echo off
setlocal
rem This disc's package for a release, from build.bat's output (run that first, with the disc in assets): the runtime and
rem the compiler, no game code. Each disc is its own compile, so run it once per disc, each in a checkout of its own, and
rem attach every zip and its .json. The disc is told by assets\default.xbe's SHA-1 (every disc's xbe has that name).
cd /d "%~dp0"
if not exist assets\default.xbe (echo assets\default.xbe is missing: put the disc's files in assets first. & exit /b 1)
set "SHA="
for /f "skip=1 delims=" %%h in ('certutil -hashfile assets\default.xbe SHA1') do if not defined SHA set "SHA=%%h"
set "SHA=%SHA: =%"
set "ZIP="
if /i "%SHA%"=="4878c3bdbd44663b64ce64986b313e8c6b7f8917" set "ZIP=PGR1-E11-package.zip"
if /i "%SHA%"=="ba7763e6e946ab40aed6706f2c9bc1df3a990fa0" set "ZIP=PGR1-E10-package.zip"
if /i "%SHA%"=="0d55fff29ceb34ab4dfcfc4820e823d967ca1469" set "ZIP=PGR1-USA-package.zip"
rem Asia: USA's code (every section the same), so USA's package
if /i "%SHA%"=="c14b35767fddc5a7af4b3aa13b53e374be23fe57" set "ZIP=PGR1-USA-package.zip"
if /i "%SHA%"=="4c38cf630f7dc92a845d2141020157e78e682e3d" set "ZIP=PGR1-JPN-package.zip"
if not defined ZIP (echo assets\default.xbe ^(SHA-1 %SHA%^) is not a supported disc. & exit /b 1)
echo %ZIP%, for the disc with default.xbe %SHA%
call sdk\scripts\release-package.bat %ZIP% PGR1
endlocal
