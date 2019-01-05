#!/bin/bash

MIDILINK=/media/fat/MidiLink.tar.gz
KERNEL=/media/fat/linux/zImage_dtb
KERNELOLD=/media/fat/linux/zImage_dtb.old
SF_FILEID="1hO88em6bmTklHWo_WwEFnOgSFhlMEhws"
SF_FILENAME="/media/fat/soundfonts/SC-55.sf2"
GD_COOKIE="/tmp/goole_drive_cookie"
MT32_ROM_DIR="/media/fat/mt32-rom-data/"
MT32_ROM_ZIP="$MT32_ROM_DIR/mt32_roms.zip"
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
  echo "Removing old MidiLink archive"
  rm $MIDILINK
fi
echo "Downloading MidiLink archive"
curl -kL https://github.com/bbond007/MiSTer_MidiLink/blob/master/INSTALL/MidiLink.tar.gz?raw=true -o $MIDILINK
cd /
echo "Remounting Linux Filesystem"
/bin/mount -o remount,rw /
echo "Extracting MidiLink Archive"
tar -xvzf $MIDILINK
rm $MIDILINK
echo "Downloading sondfont cookie"
curl -kc $GD_COOKIE -s -L "https://drive.google.com/uc?export=download&id=${SF_FILEID}" > /dev/null
echo "Downloading SC-55 soundfont"
curl -kLb $GD_COOKIE "https://drive.google.com/uc?export=download&confirm=`awk '/download/ {print $NF}' $GD_COOKIE`&id=${SF_FILEID}" -o ${SF_FILENAME}
echo "Downloading MT-32/CM-32 ROMS"
curl -k "https://www.deceifermedia.com/files/applications/Roland_MT32_ROMs.zip" -o $MT32_ROM_ZIP
echo "Unzipping CM-32 ROMS"
unzip -o $MT32_ROM_ZIP CM32L* -d $MT32_ROM_DIR
echo "Backing up Kernel"
if [ -f $KERNELOLD ]; 
then 
  echo "Removing old backup Kernel"
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



