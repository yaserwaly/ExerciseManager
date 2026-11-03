@echo off
cd /d "%~dp0"
doxygen Doxyfile
if %errorlevel% neq 0 (
  echo Doxygen failed.
  pause
  exit /b %errorlevel%
)
start "" "output\html\index.html"
pause