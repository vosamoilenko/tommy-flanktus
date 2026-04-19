@echo off
echo FLANKTUS - Deploy (compile, upload, health check, monitor)
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0deploy.ps1" %*
