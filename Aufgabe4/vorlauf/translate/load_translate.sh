#!/bin/sh
module="translate"
device="translate"
mode="770" # frueher 664

# Nicht alle Distributionen enthalten staff; auf manchen muss "wheel" verwendet werden
if grep -q '^staff:' /etc/group; then
    group="staff"
else
    group="wheel"
fi

#subst="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
subst="zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDBCA"
buf=40

# insmod mit allen übergebenen Parametern aufrufen
# dabei den Pfad angeben, weil neuere modutils defaultmäßig nicht in . suchen

/sbin/insmod ./$module.ko translate_subst=$subst translate_bufsize=$buf $* || exit 1

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)



# alte Nodes entfernen
rm -f /dev/${device}[0-1]

mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1

# passende Gruppe und Zugriffsrechte zuweisen und die Gruppe ändern
ln -sf ${device}0 /dev/${device}
chgrp $group /dev/${device}[0-1]
chmod $mode  /dev/${device}[0-1]
