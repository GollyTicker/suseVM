#!/bin/bash

make clean
make all
./mmanage

if [ $# -gt 0 ] ; then
  if [ $1 -eq 1 ] ; then
    ./vmappl
    echo "Killing mmanage!"
    killall -9 mmanage
    echo "mmanage Killed!"
  fi
fi

echo "process finished!"
