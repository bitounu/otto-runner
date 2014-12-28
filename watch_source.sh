#!/bin/bash

RPI=192.168.2.2

if [ `uname` == "Darwin" ]; then
	IP=`vagrant ssh-config | grep "HostName" | awk '{print $2}'`
	IDENTITY=`vagrant ssh-config | grep "IdentityFile" | awk '{print $2}'`
	#echo "ssh -i $IDENTITY vagrant@$IP  -o \"ForwardAgent yes\" \"make -f /vagrant/otto-sdk/src/Makefile\""
	WATCH="fswatch -k -r ./"
else
	WATCH="inotifywait -r -m -e modify,moved_to,close_write ./"
fi
#while true; do
	$WATCH |
	while read file; do
		filename=$(basename "$file")
		extension="${filename##*.}"
		if [ "$extension" == "c" ]; then
			cd src ; make
			if [ `uname` == "Darwin" ]; then
				#ssh -i $IDENTITY vagrant@$IP  -o "ForwardAgent yes" "cd /vagrant/otto-sdk/ ; make PREFIX=/opt/cross CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm"
				#if [ -n $RPI ]; then
				#	scp -c blowfish vagrant@$IP:build/main vagrant@$IP:build/test.so build/ipc.so 
				#fi
			fi
		fi
	done

#done
