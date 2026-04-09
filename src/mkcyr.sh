#!/bin/bash
cat milena_cyrillic.in | (

	while read a; do
		if [ "$a" != "%CRULES%" ] ; then
			echo $a
			continue
		fi
		cat milena_cyrillic.rules | (
	t=""
	while read a;do	
		if [ "$a" = "" ]; then
			continue
		fi
		cyr=`echo $a | awk '{print $1}'`;
		lat=`echo $a | awk '{print $2}'`;
		if [ "$lat" != "" ] ; then
			lat=`echo $lat | iconv -f UTF-8 -t ISO-8859-2`
		fi
		cyr=`echo -n "$cyr" | iconv -f UTF-8 -t UTF-16LE | hexdump | head -n 1 |\
		awk '{
			c=$2;
			d=$3;
			if (d == "") d="0";
			print "{0x"c",0x"d",";
		}'`
		
		echo $t$cyr'"'$lat'"}'
		t=','
	done
)
	done
) > milena_cyrillic.h


