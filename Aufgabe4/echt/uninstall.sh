#!/bin/sh
module="translate"
device="translate"

# invoke rmmod with all arguments we got
/sbin/rmmod $module $* || exit 1

# Remove stale nodes
rm -f /dev/${device} /dev/${device}[0-1] 

# remove compilations
make clean
