@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup\setup.ps1" %*
exit /b %errorlevel%
