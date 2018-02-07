#!/bin/bash

VBoxHeadless -s 7 &
sleep 20s
VBoxManage --nologo guestcontrol "7" --username void run --exe "C:\Users\void\Desktop\build-installer.bat"
VBoxManage --nologo guestcontrol "7" --username void run --exe "C:\Users\void\Desktop\stop.bat"
scp -P 105 -c aes256-cbc  /home/vlad/work/bsa/BSA-Analytics-x64.exe root@vtyulb.ru:/var/www/public_html/downloads
