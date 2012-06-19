#!/bin/sh
while [ "$#" -gt 0 ]; do
	case "$1" in
		-ppm)
			PPM_IMAGE=yes
			shift
			;;
		-*)
			echo "Unknown option $1"
			exit 1
			;;
		*)
			if [ "$INPUT" = "" ]; then
				INPUT="$1"
			else
				BASE="$1"
			fi
			shift
			;;
	esac
done

if [ "$INPUT" = "" -o "$BASE" = "" ]; then
	echo "Usage: $1 [-ppm] <.y4m> <base>"
	exit 1
fi

TMP_FILE=/tmp/output.yuv
BLOCK=`expr 960 '*' 540`
for p in `head -n 1 "$INPUT"`; do
	case "$p" in
		W*)
			WIDTH=`echo "$p" | sed s/W//`
			;;
		H*)
			HEIGHT=`echo "$p" | sed s/H//`
	esac
done

y4mtoyuv < "$INPUT" > $TMP_FILE

if [ "$PPM_IMAGE" = yes ]; then
	cat > "$BASE".Y.ppm <<EOF
P5
$WIDTH $HEIGHT
255
EOF
	dd if=$TMP_FILE bs=$BLOCK count=4 >> "$BASE".Y.ppm
else
	dd if=$TMP_FILE bs=$BLOCK count=4 > "$BASE".Y
fi

if [ "$PPM_IMAGE" = yes ]; then
	cat > "$BASE".U.ppm <<EOF
P5
`expr $WIDTH / 2` `expr $HEIGHT / 2`
255
EOF
	dd if=$TMP_FILE bs=$BLOCK count=1 skip=4 >> "$BASE".U.ppm
else
	dd if=$TMP_FILE bs=$BLOCK count=1 skip=4 > "$BASE".U
fi

if [ "$PPM_IMAGE" = yes ]; then
	cat > "$BASE".V.ppm <<EOF
P5
`expr $WIDTH / 2` `expr $HEIGHT / 2`
255
EOF
	dd if=$TMP_FILE bs=$BLOCK count=1 skip=5 >> "$BASE".V.ppm
else
	dd if=$TMP_FILE bs=$BLOCK count=1 skip=5 > "$BASE".V
fi

