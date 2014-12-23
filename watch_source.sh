#!/bin/bash
IP=`vagrant ssh-config | grep "HostName" | awk '{print $2}'`
IDENTITY=`vagrant ssh-config | grep "IdentityFile" | awk '{print $2}'`

#echo "ssh -i $IDENTITY vagrant@$IP  -o \"ForwardAgent yes\" \"make -f /vagrant/otto-sdk/src/Makefile\""
#while true; do
	#inotifywait -r -m -e modify,moved_to,close_write ./ |
	fswatch -k -r ./ |
	while read file; do
		filename=$(basename "$file")
		extension="${filename##*.}"
		if [ "$extension" == "c" ]; then
			ssh -i $IDENTITY vagrant@$IP  -o "ForwardAgent yes" "cd /vagrant/otto-sdk/src/ ; make"
		fi
	done

#done
