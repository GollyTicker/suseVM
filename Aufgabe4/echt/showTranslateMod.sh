#!/bin/sh
echo " =========== \"translate\" in Modules ==========="
lsmod | grep translate

echo " =========== last Kernel-logs ==========="
dmesg | tail -f