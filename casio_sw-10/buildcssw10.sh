export PATH=/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/:$PATH
rm -rf casio_sw-10
git clone https://github.com/M-HT/casio_sw-10
cp Makefile.sw10_alsadrv casio_sw-10/sw10_alsadrv/Makefile
cd casio_sw-10/sw10_alsadrv
make
cp sw10_alsadrv /mnt/c/MiSTer_MidiLink/casio_sw-10/sbin/sw10_alsadrv.new





