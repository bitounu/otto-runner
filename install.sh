#!/bin/bash

git submodule init
git submodule update

./wifi.sh

apt-get -y install python-pip dnsmasq device-tree-compiler
pip install bottle

dtc -I dts -O dtb -o /boot/dt-blob.bin ces.dts

cp python-init /etc/init.d/python
cp otto-demo-init /etc/init.d/otto-demo
cp fastcamd-init /etc/init.d/fastcamd
cp wifi-init /etc/init.d/wifi

update-rc.d python defaults
update-rc.d wifi defaults
update-rc.d otto-demo defaults
update-rc.d fastcamd defaults
update-rc.d dnsmasq remove
cp -r fastcmd/ ~/fastcmd

echo "options 8192cu rtw_power_mgnt=0" > /etc/modprobe.d/8192cu.conf

cat <<EOF > /etc/network/interfaces
auto lo

iface lo inet loopback
iface eth0 inet dhcp
iface eth1 inet dhcp

allow-hotplug eth0
allow-hotplug eth1
allow-hotplug wlan0
EOF

cat <<EOF > /etc/modprobe.d/raspi-blacklist.conf
blacklist snd-soc-pcm512x
blacklist snd-soc-wm8804
EOF
