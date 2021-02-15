#!/bin/sh
LC_ALL=C ls -m /bin | sed -e's/,$//g' -e's/, /,/g' | table -c 160 -n
