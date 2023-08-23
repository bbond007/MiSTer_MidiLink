#!/bin/bash
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#SF_FILEID="1hO88em6bmTklHWo_WwEFnOgSFhlMEhws"
SF_FILEID="1G53wKnIBMONgOVx0gCOWrBlJaXsyaKml"
SOUNDFONT_DIR="/media/fat/linux/soundfonts/"
SF_FILENAME="$SOUNDFONT_DIR/SC-55.sf2"
GD_COOKIE="/tmp/google_drive_cookie"
MT32_ROM_DIR="/media/fat/linux/mt32-rom-data/"
MT32_ROM_ZIP="$MT32_ROM_DIR/mt32_roms.zip"
MP3_DIR="/media/fat/MP3"
MIDI_DIR="/media/fat/MIDI"
MIDILINK_BIN="/sbin/midilink"
echo "Checking Internet..."
if ! ping -q -w1 -c1 google.com &>/dev/null
then
  echo "No Internet connection"
  exit 1
fi
if [ -d "$MT32_ROM_DIR" ]; then
  echo "Found MT-32 ROM directory"
else
  echo "Creating MT-32 ROM directory"
  mkdir $MT32_ROM_DIR
fi
if [ -d "$SOUNDFONT_DIR" ]; then
  echo "Found SoundFont directory"
else
  echo "Creating SoundFont directory"
  mkdir $SOUNDFONT_DIR
fi
if [ -d "$MP3_DIR" ]; then
  echo "Found MP3 directory"
else
  echo "Creating MP3 directory"
  mkdir $MP3_DIR
fi
if [ -d "$MIDI_DIR" ]; then
  echo "Found MIDI directory"
else
  echo "Creating MIDI directory"
  mkdir $MIDI_DIR
fi
if [ -f $SF_FILENAME ];
then 
  echo "SC-55 Soundfont found"
else
  echo "Downloading sondfont cookie"
  curl -kc $GD_COOKIE -s -L "https://drive.google.com/uc?export=download&id=${SF_FILEID}" > /dev/null
  echo "Downloading SC-55 soundfont"
  curl -kLb $GD_COOKIE "https://drive.google.com/uc?export=download&confirm=`awk '/download/ {print $NF}' $GD_COOKIE`&id=${SF_FILEID}" -o ${SF_FILENAME}
fi
if [ -f $MT32_ROM_ZIP ];
then
  echo "MT-32/CM-32 ROMS found"
else
  echo "Downloading MT-32/CM-32 ROMS"
  curl -Lk "https://archive.org/download/mame-versioned-roland-mt-32-and-cm-32l-rom-files/mt32-roms.zip" -o $MT32_ROM_ZIP
  echo "Unzipping CM-32 ROMS"
  unzip -o $MT32_ROM_ZIP cm32l* -d $MT32_ROM_DIR
fi
echo "Done in:"
sleep 1
echo "3"
sleep 1 
echo "2"
sleep 1 
echo "1"
sleep 1
