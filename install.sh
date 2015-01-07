#!/bin/bash
cp python-init /etc/init.d/python
cp otto-demo-init /etc/init.d/otto-demo
cp fastcamd-init /etc/init.d/fastcamd

update-rc.d python defaults
update-rc.d otto-demo defaults
update-rc.d fastcamd defaults
cp -r fastcmd/ ~/fastcmd
apt-get -y install python-pip
pip install bottle
