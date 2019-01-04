#!/bin/bash

MIDILINK=/media/fat/MidiLink.tar.gz
KERNEL=/media/fat/linux/zImage_dtb
KERNELOLD=/media/fat/linux/zImage_dtb.old

echo "Checking Internet..."
if ! ping -q -w1 -c1 google.com &>/dev/null
then
  echo "No Internet connection"
  exit 1
fi
echo "Installing MidiLink"
uartmode 0
sleep 1
if [ -f $MIDILINK ]; 
then
  echo "Removing old ML archive"
  rm $MIDILINK
fi
echo "Downloading ML archive"
curl -kL https://github.com/bbond007/MiSTer_MidiLink/blob/master/INSTALL/MidiLink.tar.gz?raw=true -o $MIDILINK
cd /
echo "Extracting ML Archive"
tar -xvzf $MIDILINK
rm $MIDILINK
echo "Backing up Kernel"
if [ -f $KERNELOLD ]; 
then 
  echo "RM old backup Kernel"
  rm $KERNELOLD
fi 
mv $KERNEL $KERNELOLD
echo "Installing Kernel"
curl -kL https://github.com/bbond007/MiSTer_MidiLink/blob/master/INSTALL/zImage_dtb?raw=true? -o $KERNEL
echo "Rebooting in:"
sleep 1
echo "3"
sleep 1 
echo "2"
sleep 1 
echo "1"
sleep 1
reboot



