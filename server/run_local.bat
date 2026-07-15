@echo off
cd /d %~dp0
echo Starting local CVM server on :9000
where go >nul 2>nul
if %ERRORLEVEL%==0 (
  go run server.go
) else (
  python server_local.py
)
