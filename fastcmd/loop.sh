#!/bin/bash

pid_file=/tmp/raspifastcamd.pid
count=1
while [[ $count -le 100 ]]; do
	kill -s USR1 $(cat $pid_file)
	sleep 0.01
	(( count++ ))
done
