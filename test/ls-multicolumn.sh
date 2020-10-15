#!/bin/sh
ls -m /bin | sed -e's/,$//g' | table -c 160
