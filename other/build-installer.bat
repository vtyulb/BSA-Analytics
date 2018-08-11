C:
cd C:\
rd /s /q C:\BSA-Analytics
git clone https://github.com/vtyulb/BSA-Analytics.git
cd C:\BSA-Analytics
mkdir build
cd build

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
C:\Qt\5.11.1\msvc2017_64\bin\qmake.exe ..\src -spec win32-msvc
nmake

move release\BSA-Analytics.exe E:\work\bsa\BSA-Analytics-x64\
"C:\Program Files (x86)\Inno Setup 5\ISCC.exe" C:\bsa-installation-script-x64.iss
move C:\Output\BSA-Analytics-x64.exe E:\work\bsa\

pause
