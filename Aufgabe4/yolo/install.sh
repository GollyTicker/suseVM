#!/bin/sh
modulename="translate"
deviceprefix="translate"
mode="664"
buffersize=200

#Chiffren
#Identity:
chiffre="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#Vorgabe:
#chiffre="zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDBCA"
#Rot13 (selbst-invers):
#chiffre="nopqrstuvwxyzabcdefghijklmNOPQRSTUVWXYZABCDEFGHIJKLM"

#Build des Moduls. Exit mit Fehler, falls nicht erfolgreich.
make || exit 1

#Einsetzen des Moduls. Exit mit Fehler, falls nicht erfolgreich.
/sbin/insmod ./$modulename.ko translate_subst=$chiffre translate_bufsize=$buffersize $* || exit 1
#/sbin/insmod ./$modulename.ko $* || exit 1

#Lese die major number
majornumber=$(awk "\$2==\"$modulename\" {print \$1}" /proc/devices)

#Ersetzen der alten Devicenodes durch neue
rm -f /dev/${device}[0-1]
mknod /dev/${device}0 c $majornumber 0
mknod /dev/${device}1 c $majornumber 1

#Setze Gruppen und Rechte
group="staff"
grep "^staff:" /etc/group > /dev/null || group="wheel"
chgrp $group /dev/${device}[0-1]
chmod $mode  /dev/${device}[0-1]
