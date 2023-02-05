@echo off

TITLE MoeTag Windows Deploy

del /S "..\deploy\*"

set /p "id=Enter Input Path: "

C:\Qt\6.4.0\msvc2019_64\bin\windeployqt.exe --dir ../deploy %id%/MoeTagCMake.exe --release

xcopy /s "%id%\MoeTagCMake.exe" "..\deploy\MoeTagCMake.exe*"
xcopy /s "..\ffmpegwindows64/dependencies" "..\deploy"
xcopy /s "%id%\x64\bin\libqtadvanceddocking.dll" "..\deploy\libqtadvanceddocking.dll*"
rename "..\deploy\MoeTagCMake.exe" "MoeTag.exe"

PAUSE
