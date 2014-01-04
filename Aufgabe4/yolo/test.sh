#!/bin/sh

echo "01[]abcABC" > /dev/translate0
echo "01[]ABCabc" > /dev/translate0
cat /dev/translate0 > /dev/translate1

echo "Expected:"
echo "01[]abcABC"
echo "01[]ABCabc"
echo "Was:"
cat /dev/translate1