#!/bin/bash

make clean
make all

manage_map()
{
  if [ "$1" == "1" ] ; then
    ./mmanage &
    ./vmappl
    killall -9 mmanage
  fi
}

if [ $# -gt 0 ] ; then
  manage_map
else
  ./mmanage
fi

echo "process finished!"
