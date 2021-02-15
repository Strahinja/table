#!/bin/sh
LC_ALL=C ls -l | tail -n +2 | sed -e's/ \+/ /g' | cut -d' ' -f1,9 \
	| table -d " " -c 30 -n
