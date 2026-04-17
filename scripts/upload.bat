@echo off
echo FLANKTUS - Compile and upload...
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0upload.ps1" %*
