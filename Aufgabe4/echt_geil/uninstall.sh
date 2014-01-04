#!/bin/sh
module="translate"
device="translate"

# Remove translate modules (Abort on failure)
/sbin/rmmod $module $* || exit 1

# Remove corresponding nodes
rm -f /dev/${device}[0-1] 

# Remove compilations files
make clean
