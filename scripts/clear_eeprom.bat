@echo off
echo FLANKTUS - Clearing EEPROM log...
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0clear_eeprom.ps1" %*
