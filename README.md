OTTO SDK testbed
----------------

Dependencies
------------
	sudo apt-get install build-essential inotify-tools

	bcm2835 - http://www.airspayce.com/mikem/bcm2835/

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