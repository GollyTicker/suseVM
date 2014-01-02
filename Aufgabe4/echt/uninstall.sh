#!/bin/sh
module="translate"
device="translate"

# Remove translate modules
/sbin/rmmod $module $* || exit 1

# Remove corresponding nodes
rm -f /dev/${device} /dev/${device}[0-1] 

# Remove compilations files
make clean
