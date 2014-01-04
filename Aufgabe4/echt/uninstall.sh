#!/bin/sh
module="translate"
device="translate"

# Remove translate modules
/sbin/rmmod $module $*

# Remove corresponding nodes
rm -f /dev/${device}[0-1] 

# Remove compilations files
make clean
