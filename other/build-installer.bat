rd /s /q C:\BSA-Analytics
cd C:\
git clone https://github.com/vtyulb/BSA-Analytics.git
cd C:\BSA-Analytics
mkdir build
cd build

C:\Qt\qt-5.5.0-x64-mingw510r0-seh-rev0\qt-5.5.0-x64-mingw510r0-seh-rev0\bin\qmake.exe ..\src
C:\Qt\qt-5.5.0-x64-mingw510r0-seh-rev0\mingw64\bin\mingw32-make.exe -f Makefile.release

move release\BSA-Analytics.exe E:\work\bsa\BSA-Analytics-x64\
"C:\Program Files (x86)\Inno Setup 5\ISCC.exe" C:\bsa-installation-script-x64.iss
move C:\Output\BSA-Analytics-x64.exe E:\work\bsa\

pause