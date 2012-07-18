#!/bin/sh

while [ "$#" -gt 0 ]; do
	case "$1" in
		-c)
			chroma=yes
			shift
			;;
		-*)
			echo "Unknown option $1"
			exit 1
			;;
		*)
			if [ -z "$first" ]; then
				first="$1"
			else
				second="$1"
			fi
			shift
			;;
	esac
done

if [ -z "$first" -o -z "$second" ]; then
	echo "Usage: diff-plane.sh [-c] <first> <second>"
	exit 1
fi

blockscan="$HOME/workspace/Cherry/out/debug/tools/block-scan"

if [ "$chroma" = yes ]; then
	$blockscan 960 540 8 < $first > /tmp/_first.block
	$blockscan 960 540 8 < $second > /tmp/_second.block
	hexdump -v -e '"%08_ax "' -e '8/1 "%02X ""    "" "' \
		-e '8/1 "%_p""\n"' /tmp/_first.block > /tmp/_first.txt

	hexdump -v -e '"%08_ax "' -e '8/1 "%02X ""    "" "' \
		-e '8/1 "%_p""\n"' /tmp/_second.block > /tmp/_second.txt
else
	$blockscan 1920 1080 16 < $first > /tmp/_first.block
	$blockscan 1920 1080 16 < $second > /tmp/_second.block
	hexdump -Cv /tmp/_first.block > /tmp/_first.txt
	hexdump -Cv /tmp/_second.block > /tmp/_second.txt
fi
diff -ru /tmp/_first.txt /tmp/_second.txt
