#!/bin/bash

VBoxHeadless -s 7 &
sleep 20s
VBoxManage --nologo guestcontrol "7" --username void run --exe "C:\Users\void\Desktop\build-installer.bat"
VBoxManage --nologo guestcontrol "7" --username void run --exe "C:\Users\void\Desktop\stop.bat"
scp /home/vlad/work/bsa/BSA-Analytics-x64.exe root@vtyulb.ru:/var/www/bsa/   
