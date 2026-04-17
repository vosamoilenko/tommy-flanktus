@echo off
echo FLANKTUS - Cleaning CSV...
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0clean_csv.ps1" %*
