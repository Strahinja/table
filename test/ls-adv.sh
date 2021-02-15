#!/bin/sh
LC_ALL=C ls -l | tail -n 15 | sed -e's/ \+/ /g' -e's/\t/ /g' \
	| table -d " " -c 120 -s ss -n
