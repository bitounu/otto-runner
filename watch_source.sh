#!/bin/bash
while true; do
	inotifywait -r -m -e moved_to ./ |
	while read dir ev file; do
		filename=$(basename "$file")
		extension="${filename##*.}"
		if [ "$extension" == "c" ]; then
			make
		fi
	done

done
