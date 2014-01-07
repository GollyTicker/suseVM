#!/bin/sh

if [ "$1" == "-m" ]; then
    echo "~~ Modules containg \"translate\":"
    lsmod | grep translate
    echo "~~ Kernel-Log (keeps updating):"
    dmesg | tail -f
    exit 0
fi

testLine1="The quick brown fox jumps over the lazy dog."
testLine2="Jackdaws love my big sphinx of quartz."

echo "Choosing input string:"
echo "\"0: ${testLine1}"
echo " 1: ${testLine2}\""

echo $testLine1 > /dev/translate0
echo $testLine2 > /dev/translate0
cat /dev/translate0 > /dev/translate1

echo ""
echo "Result after coding in both devices:"
cat /dev/translate1