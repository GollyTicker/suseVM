#!/bin/sh
module="translate"	# Name des Moduls
device="translate"	# Prefix der beiden Devices
mode="664"

# compillieren des Moduls (Abbruch, falls nicht erfolgreich)
make || exit 1

# Translate String
# Neutral:
#subst= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

# Groß <-> Klein:
#subst="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

# Fohl:
subst="zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDBCA"

# Rot 5:
#subst="fghijklmnopqrstuvwxyzabcdeFGHIJKLMNOPQRSTUVWXYZABCDE"

# Buffersize
buf=40

# Das meiste in dieser Datei kommt aus dem dritten Kapitel,
# indem die Installation erklärt wird.

# Hinzufuegen des Kernelmoduls
/sbin/insmod ./$module.ko translate_subst=$subst translate_bufsize=$buf $* || exit 1
# Aufruf ohne Parameter (ein oder auskommentieren)
# /sbin/insmod ./$module.ko $* || exit 1

# major nummer holen
major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)



# alte Device-Nodes entfernen
rm -f /dev/${device}[0-1]

# neue Devide-Nodes erstellen
mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1

# Gruppen und Zugriffsrechte zuweisen
group="staff"
grep "^staff:" /etc/group > /dev/null || group="wheel"
chgrp $group /dev/${device}[0-1]
chmod $mode  /dev/${device}[0-1]
