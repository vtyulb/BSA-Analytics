#!/bin/bash

VBoxHeadless -s "Windows 10" &
sleep 60s
VBoxManage --nologo guestcontrol "Windows 10" --username "Влад" run --exe "E:\work\bsa\BSA-Analytics\other\installer\build-installer.bat"
VBoxManage --nologo guestcontrol "Windows 10" --username "Влад" run --exe "E:\work\bsa\BSA-Analytics\other\installer\shutdown.bat"
scp -P 105 -c aes256-cbc  /home/vlad/work/bsa/BSA-Analytics-x64.exe root@vtyulb.ru:/var/www/public_html/downloads
