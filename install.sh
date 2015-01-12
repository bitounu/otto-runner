#!/bin/bash

git submodule init
git submodule update

./wifi.sh

cp python-init /etc/init.d/python
cp otto-demo-init /etc/init.d/otto-demo
cp fastcamd-init /etc/init.d/fastcamd
cp wifi-init /etc/init.d/wifi

update-rc.d python defaults
update-rc.d wifi defaults
update-rc.d otto-demo defaults
update-rc.d fastcamd defaults
cp -r fastcmd/ ~/fastcmd
apt-get -y install python-pip
pip install bottle
