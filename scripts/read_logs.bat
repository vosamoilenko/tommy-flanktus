@echo off
echo FLANKTUS - Reading live debug logs...
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0read_logs.ps1" %*
