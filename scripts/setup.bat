@echo off
echo FLANKTUS v6.0 - First-time Windows Setup
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0setup.ps1" %*
