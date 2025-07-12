@echo off
REM This script runs a series of network diagnostic commands using NetAn
REM and saves the output to a log file.
REM It requires NetAn to be built and located in the 'build' directory.
REM For 'ping', 'trace', and 'mtr' commands, this script must be run as Administrator.

set "LOG_FILE=netan_diagnostic_%DATE:~10,4%%DATE:~4,2%%DATE:~7,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%.log"
set "NETAN_PATH=.\build\netan.exe"

REM Check if NetAn executable exists
if not exist "%NETAN_PATH%" (
    echo Error: NetAn executable not found at %NETAN_PATH%.
    echo Please build the project first (cd build ^&^& cmake .. ^&^& cmake --build .).
    goto :eof
)

echo Starting NetAn network diagnostic. Output will be saved to %LOG_FILE%
echo ------------------------------------------------------------------- >> "%LOG_FILE%"

REM Function to run a command and log its output
REM This function assumes the script itself is run as Administrator for trace/mtr
:run_command
set "COMMAND_DESC=%~1"
set "COMMAND=%~2"
echo.
echo --- Running: %COMMAND_DESC% ---
echo Command: %COMMAND%

REM Execute the command and pipe output to console and log file
%COMMAND% 2>&1 | tee -a "%LOG_FILE%"

echo --- End of %COMMAND_DESC% ---
goto :eof

REM --- Diagnostic Commands ---

REM 1. Ping to Google (Requires Admin)
call :run_command "Ping to Google (Requires Admin)" "%NETAN_PATH% ping google.com"

REM 2. Traceroute to Google (Requires Admin)
call :run_command "Traceroute to Google (Requires Admin)" "%NETAN_PATH% trace google.com"

REM 3. MTR to Google (Requires Admin)
call :run_command "MTR to Google (Requires Admin)" "%NETAN_PATH% mtr google.com --count 5"

REM 4. DNS Lookup for Google (no admin needed)
call :run_command "DNS Lookup for Google (no admin needed)" "%NETAN_PATH% dns google.com"

REM 5. Port Scan on a common service (no admin needed)
call :run_command "Port Scan on example.com (Port 80) (no admin needed)" "%NETAN_PATH% scan example.com 80 80"

echo.
echo Diagnostic complete. Check %LOG_FILE% for full details.