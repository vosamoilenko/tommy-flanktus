@echo off
echo FLANKTUS - Exporting sensor log...
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0dump_log.ps1" %*
