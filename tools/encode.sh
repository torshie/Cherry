#!/bin/sh
if [ "$1" = "" ]; then
	echo "Usage: $0 <.ppm image>"
	exit 1
fi

ENCODER=$HOME/workspace/libvpx/out/vpxenc
TMP_FILE=/tmp/tmpfile.y4m

ppmtoy4m -S 420mpeg2 < "$1" > $TMP_FILE

"$ENCODER" $TMP_FILE -o ~/bbb.ivf --ivf --limit=1

