OTTO SDK testbed
----------------
For Ubuntu 14.04LTS

Vagrant Build Image - <tbd>


Building(on Pi)
-------
make  
sudo build/main build/particles.so  

sudo build/main build/&lt;mode name&gt;.so  

Dependencies
------------
build-essential inotify-tools
***
	sudo apt-get install build-essential inotify-tools

bcm2835 - http://www.airspayce.com/mikem/bcm2835/  
***
	wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.38.tar.gz  
	tar zxvf bcm2835-1.xx.tar.gz  
	cd bcm2835-1.xx  
	./configure  
	make  
	sudo make check  
	sudo make install  



To build
--------
	cd src
	make


To setup automatic builds, run the following in the top level directory
	./watch_source.sh

I suggest running a tmux session with 3 or 4 panes.
Pane 1
------
	cd src/build
	./main ./inputd.so

Pane 2
------
	cd src/build
	./main

Pane 3
------
	./watch_source.sh

This allows faster development using live building/reloading of code
