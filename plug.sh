#!/bin/bash
# $Header$
#
# Poll for a new card:
#	plug /dev/sda [cbreak]
# In case the card is already plugged, it requires a card change.
#
# This needs keypressed in PATH:
#	http://www.scylla-charybdis.com/tool.php/keypressed
#
# This Works is placed under the terms of the Copyright Less License,
# see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.
#
# $Log$
# Revision 1.1  2011-09-18 13:05:13  tino
# added
#

delay=250
MILL='|/-\'
millc=0
BS="`echo -ne '\b'`"
BELL="`echo -ne '\a'`"

# leave STDIN alone
exec <&1

mill()
{
let millc=(millc+1)%${#MILL}
echo -n "${MILL:$millc:1}$BS"
}

doplug()
{
echo -n "$BELL$3plug $1: "
while eval $2 : \<\"\$1\"
do
	mill
	keypressed $delay && { echo "aborted by $e"; exit 1; }
done 2>/dev/null
}

kbd_clear()
{
while	keypressed
do
	# This is not perfect on CBREAK mode, as it delays 1s due to missing CR
	read -t1
done
}

case "$#" in
1)	;;
2)	ostty="`stty -g`"
	trap 'stty "$ostty"' 0
	stty $2 >/dev/null
	kbd_clear
	;;
*)	echo "Usage: `basename "$0"` device [cbreak] >/dev/tty
	To detect other keystrokes than ENTER use the cbreak option.
	To only clear the input give '' instead of 'cbreak'" >&2
	exit 1;;
esac

e=ENTER
case "`stty`" in
*icanon*)	e=key;;
esac

echo -n "Press $e to interrupt. $BELL"
doplug "$1" '' un
doplug "$1" '!' 'ok, '
echo ok
exit 0

# On abort, the pressed key/line still is in the TTY buffer!
