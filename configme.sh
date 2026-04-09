#!/bin/sh


dirs="/usr/share/mbrola \
/usr/share/festival \
/usr/local/share/mbrola \
/usr/local/share/festival \
$HOME"

voice=""
for d in $dirs; do
        if [ -d $d ] ; then
                v=`find $d -path "*/pl1" -and -type f`
                if [ "$v" != "" ] ; then
                        voice=$v
                        break
                fi
        fi
done
voice=`echo $voice | awk '{print $1}' | head -n 1`

if [ "$voice" = "" ] ; then
	echo Brak mbrola-pl1
	exit 1
fi


if ! sve=`sox --version` ; then
	echo A gdzie sox?
	exit 1
fi
contrast=""
if play -n -q synth 0.01 sine 440 vol 0 contrast >/dev/null 2>/dev/null; then
        contrast=contrast
fi

if ! mbrola=`which mbrola` ; then
	echo A mbrola gdzie?
	exit 1
fi

echo "mbrola=$mbrola"
echo "mbrola_voice=$voice"
echo "contrast=$contrast"

echo "mbrola=$mbrola" > ./config.dat
echo "mbrola_voice=$voice" >> ./config.dat
echo "contrast=$contrast" >> ./config.dat


