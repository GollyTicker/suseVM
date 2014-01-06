#!/bin/sh
modulename="translate"
deviceprefix="translate"

#Entfernen des Moduls. Exit mit Fehler, falls nicht erfolgreich.
/sbin/rmmod $module $* || exit 1

#Entfernen der Devicenodes
rm -f /dev/${device}[0-1] 

#Clean
make clean
