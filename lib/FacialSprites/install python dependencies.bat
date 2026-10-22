@echo off
cd /d "%~dp0"
echo Installing Python dependencies (Pillow)...
python -m ensurepip --upgrade
python -m pip install --upgrade pip
python -m pip install pillow
echo.
echo Done! If you see "Successfully installed pillow" or "Requirement already satisfied", you are ready to go.
PAUSE
