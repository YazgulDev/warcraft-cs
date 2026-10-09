@echo off
rem Reuse the standard launcher so RoC keeps fullscreen DPI handling and optional map/window arguments.
call "%~dp0play.cmd" -Edition ReignOfChaos %*
exit /b %errorlevel%
