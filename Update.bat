@echo off
setlocal

rem ============================================================================
rem -- UNTESTED ----------------------------------------------------------------
rem ============================================================================
rem
rem CARLA Duckietown is developed and tested on Ubuntu only. The Duckietown
rem section of this script mirrors Update.sh but has NEVER been run on Windows.
rem Treat it as a starting point rather than a supported path, and expect to fix
rem something. If you get it working, please open a pull request.
rem
rem It needs, beyond the requirements of the CARLA section:
rem   * tar.exe, which ships with Windows 10 1803 and newer (7-Zip is used as a
rem     fallback if tar is missing)
rem   * curl.exe, same vintage (PowerShell is used as a fallback)
rem
rem ============================================================================
rem -- Set up environment ------------------------------------------------------
rem ============================================================================

set FILE_N=[Update.bat]
set SCRIPT_DIR=%~dp0

set CONTENT_FOLDER=%SCRIPT_DIR%Unreal/CarlaUE4/Content/Carla
set VERSION_FILE=%CONTENT_FOLDER%/.version
set CONTENT_VERSIONS=%SCRIPT_DIR%Util/ContentVersions.txt

set UE4_CONTENT_ROOT=%SCRIPT_DIR%Unreal\CarlaUE4\Content
set DUCKIETOWN_CONTENT_FOLDER=%UE4_CONTENT_ROOT%\Duckietown
set DUCKIETOWN_VERSION_FILE=%DUCKIETOWN_CONTENT_FOLDER%\.version
set DUCKIETOWN_VERSIONS=%SCRIPT_DIR%Util\DuckietownContentVersions.txt
set DUCKIETOWN_ARCHIVE=%SCRIPT_DIR%DuckietownContent.tar.gz

set SKIP_DOWNLOAD=false
set SKIP_DUCKIETOWN=false

rem ============================================================================
rem -- Parse arguments ---------------------------------------------------------
rem ============================================================================

:arg_loop
if "%~1"=="" goto arg_done
if /I "%~1"=="-h" goto show_help
if /I "%~1"=="--help" goto show_help
if /I "%~1"=="-s" set SKIP_DOWNLOAD=true& shift& goto arg_loop
if /I "%~1"=="--skip-download" set SKIP_DOWNLOAD=true& shift& goto arg_loop
if /I "%~1"=="-d" set SKIP_DUCKIETOWN=true& shift& goto arg_loop
if /I "%~1"=="--skip-duckietown" set SKIP_DUCKIETOWN=true& shift& goto arg_loop
echo %FILE_N% Unknown argument "%~1".
goto show_help
:arg_done

if "%SKIP_DOWNLOAD%"=="true" if "%SKIP_DUCKIETOWN%"=="true" (
  echo %FILE_N% Both downloads skipped, nothing to do.
  goto good_exit
)

rem ============================================================================
rem -- Download the CARLA content ----------------------------------------------
rem ============================================================================

if "%SKIP_DOWNLOAD%"=="true" (
  echo %FILE_N% Skipping the CARLA content update.
  goto duckietown
)

if not exist "%CONTENT_FOLDER%" mkdir "%CONTENT_FOLDER%"

for /F "delims=" %%a in ('type "%CONTENT_VERSIONS%"') do (
   set "lastLine=%%a"
)
set CONTENT_ID=%lastLine:~-16,16%
set CONTENT_LINK=https://carla-assets.s3.us-east-005.backblazeb2.com/%CONTENT_ID%.tar.gz
if "%CONTENT_ID:~0,2%"=="20" (
  set CONTENT_FILE=%CONTENT_FOLDER%/%CONTENT_ID%.tar.gz
  set CONTENT_FILE_TAR=%CONTENT_FOLDER%/%CONTENT_ID%.tar
  echo %CONTENT_ID%
  echo %CONTENT_LINK%
) else (
  echo Error reading the latest version from ContentVersions.txt, check last line of file %CONTENT_VERSIONS%'
  goto error_download
)

echo Downloading "%CONTENT_LINK%"...
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%CONTENT_LINK%', '%CONTENT_FILE%')"
if %errorlevel% neq 0 goto error_download

echo %FILE_N% Extracting content from "%CONTENT_FILE%", this can take a while...
if exist "%ProgramW6432%/7-Zip/7z.exe" (
    "%ProgramW6432%/7-Zip/7z.exe" x "%CONTENT_FILE%" -o"%CONTENT_FOLDER%" -y
    if %errorlevel% neq 0 goto error_download
    echo Deleting %CONTENT_FILE:/=\%
    del %CONTENT_FILE:/=\%
    "%ProgramW6432%/7-Zip/7z.exe" x "%CONTENT_FILE_TAR%" -o"%CONTENT_FOLDER%" -y
    if %errorlevel% neq 0 goto error_download
    echo Deleting %CONTENT_FILE_TAR:/=\%
    del %CONTENT_FILE_TAR:/=\%
) else (
    powershell -Command "Expand-Archive '%CONTENT_FILE%' -DestinationPath '%CONTENT_FOLDER%'"
    if %errorlevel% neq 0 goto error_download
    del %CONTENT_FILE%
)

echo %CONTENT_ID%> "%VERSION_FILE:/=\%"
echo %FILE_N% CARLA content has been installed in "%CONTENT_FOLDER%".

rem ============================================================================
rem -- Download the Duckietown content -----------------------------------------
rem ============================================================================

:duckietown

if "%SKIP_DUCKIETOWN%"=="true" (
  echo %FILE_N% Skipping the Duckietown content update.
  goto success
)

if not exist "%DUCKIETOWN_VERSIONS%" (
  echo %FILE_N% Cannot find "%DUCKIETOWN_VERSIONS%".
  goto error_duckietown
)

rem The file ends with a "Latest: <url>" line. Split on space only and take the
rem remainder: the URL contains colons, so ':' must not be a delimiter, and the
rem string cannot be sliced at a fixed length either.
set DUCKIETOWN_CONTENT_LINK=
for /F "tokens=1,* delims= " %%a in ('findstr /B "Latest:" "%DUCKIETOWN_VERSIONS%"') do set DUCKIETOWN_CONTENT_LINK=%%b

if "%DUCKIETOWN_CONTENT_LINK%"=="" (
  echo %FILE_N% Could not read the "Latest:" entry from "%DUCKIETOWN_VERSIONS%".
  goto error_duckietown
)

echo %FILE_N% Duckietown content: %DUCKIETOWN_CONTENT_LINK%

rem Skip the download when the installed version already matches.
if not exist "%DUCKIETOWN_VERSION_FILE%" goto duckietown_download
set INSTALLED_ID=
for /F "usebackq delims=" %%a in ("%DUCKIETOWN_VERSION_FILE%") do set INSTALLED_ID=%%a
if "%INSTALLED_ID%"=="%DUCKIETOWN_CONTENT_LINK%" (
  echo %FILE_N% Duckietown content is up-to-date.
  goto success
)

:duckietown_download

rem Back up any existing folder rather than overwriting it, matching Update.sh.
if exist "%DUCKIETOWN_CONTENT_FOLDER%" (
  for /F %%t in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMddHHmmss"') do set TIMESTAMP=%%t
  echo %FILE_N% Backing up existing Duckietown content...
  move "%DUCKIETOWN_CONTENT_FOLDER%" "%DUCKIETOWN_CONTENT_FOLDER%_%TIMESTAMP%"
  if %errorlevel% neq 0 goto error_duckietown
)

rem curl ships with Windows 10 1803 and newer; fall back to PowerShell if absent.
echo %FILE_N% Downloading Duckietown content from %DUCKIETOWN_CONTENT_LINK%
where curl >nul 2>nul
if %errorlevel%==0 (
  curl -fL -C - -o "%DUCKIETOWN_ARCHIVE%" "%DUCKIETOWN_CONTENT_LINK%"
) else (
  powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%DUCKIETOWN_CONTENT_LINK%', '%DUCKIETOWN_ARCHIVE%')"
)
if %errorlevel% neq 0 (
  echo %FILE_N% Download failed.
  goto error_duckietown
)

echo %FILE_N% Extracting Duckietown content, this can take a while...
where tar >nul 2>nul
if %errorlevel%==0 (
  tar -xzf "%DUCKIETOWN_ARCHIVE%" -C "%UE4_CONTENT_ROOT%"
  if %errorlevel% neq 0 goto error_duckietown
) else (
  if not exist "%ProgramW6432%/7-Zip/7z.exe" (
    echo %FILE_N% Neither tar.exe nor 7-Zip found; cannot extract the archive.
    goto error_duckietown
  )
  "%ProgramW6432%/7-Zip/7z.exe" x "%DUCKIETOWN_ARCHIVE%" -o"%SCRIPT_DIR%" -y
  if %errorlevel% neq 0 goto error_duckietown
  "%ProgramW6432%/7-Zip/7z.exe" x "%SCRIPT_DIR%DuckietownContent.tar" -o"%UE4_CONTENT_ROOT%" -y
  if %errorlevel% neq 0 goto error_duckietown
  del "%SCRIPT_DIR%DuckietownContent.tar"
)

del "%DUCKIETOWN_ARCHIVE%"
echo %DUCKIETOWN_CONTENT_LINK%> "%DUCKIETOWN_VERSION_FILE%"
echo %FILE_N% Duckietown content has been installed in "%DUCKIETOWN_CONTENT_FOLDER%".

goto success

rem ============================================================================
rem -- Messages and errors -----------------------------------------------------
rem ============================================================================

:show_help
    echo.
    echo %FILE_N% Update CARLA and Duckietown content, to be run after "git pull".
    echo.
    echo     Usage: Update.bat [-h^|--help] [-s^|--skip-download] [-d^|--skip-duckietown]
    echo.
    echo         -s, --skip-download      Skip the CARLA content.
    echo         -d, --skip-duckietown    Skip the Duckietown content.
    echo.
    echo %FILE_N% NOTE: the Duckietown half of this script is untested on Windows.
    echo.
    goto good_exit

:success
    echo.
    echo %FILE_N% Done.
    goto good_exit

:error_download
    echo %FILE_N% Failed to update the CARLA content.
    goto bad_exit

:error_duckietown
    echo %FILE_N% Failed to update the Duckietown content.
    echo %FILE_N% This path is untested on Windows -- see the notes at the top of
    echo %FILE_N% this script, or fetch the archive by hand from the id above and
    echo %FILE_N% extract it into Unreal\CarlaUE4\Content.
    if exist "%DUCKIETOWN_ARCHIVE%" del "%DUCKIETOWN_ARCHIVE%"
    goto bad_exit

:good_exit
    echo %FILE_N% Exiting...
    endlocal
    exit /b 0

:bad_exit
    echo %FILE_N% Exiting with error...
    endlocal
    exit /b 1
