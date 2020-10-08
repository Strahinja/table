#!/bin/sh
file ~/Downloads/* | sed -e's/ \+/ /g' | table -d : -c 110
