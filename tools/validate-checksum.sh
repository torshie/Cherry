#!/bin/sh
if [ -z "$1" -o -z "$2" ]; then
	echo "Usage: $0 <md5 file> <.y4m file>"
	exit 1
fi

md5File="$1"
y4mFile="$2"

sum=`md5sum "$y4mFile" | cut -d' ' -f1`
line=`grep $sum "$md5File"`
if [ -z "$line" ]; then
	echo "Cannot find such a checksum: $sum $y4mFile"
	exit 1
fi
n1=`echo "$line" | cut -d' ' -f2`
n1=`basename "$n1"`
n2=`basename "$y4mFile"`
if [ ! "$n1" = $n2 ]; then
	echo "File name mistach"
	exit 1
fi
exit 0
