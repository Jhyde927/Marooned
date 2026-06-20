@echo off
if exist Marooned rd /S /Q Marooned
if exist Marooned_Win64.zip del /S /Q Marooned_Win64.zip

mkdir Marooned
mkdir Marooned\assets
make

xcopy /s /e dlls\win64 Marooned\
xcopy /s /e /i /y assets Marooned\assets\
copy Marooned.exe Marooned\
copy credits.txt Marooned\

powershell Compress-Archive -Path Marooned -DestinationPath Marooned_Win64.zip -Force
rd /S /Q Marooned
