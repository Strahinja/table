#!/bin/sh
file * | sed -e's/ \+/ /g' | table -d : -c 110 -n
