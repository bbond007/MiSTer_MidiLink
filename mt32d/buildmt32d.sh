#export PATH=/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin:$PATH
export PATH=/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin:$PATH
rm -rf munt
git clone https://github.com/munt/munt
cp toolchain.cmake munt
cp Makefile.mt32d munt/mt32emu_alsadrv/Makefile
cd munt
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain.mt32emu -DBUILD_TESTING=OFF -Dmunt_WITH_MT32EMU_QT=OFF -DCMAKE_BUILD_TYPE=Release
cd mt32emu && make
cd ../../mt32emu_alsadrv
make
cp mt32d /mnt/c/MiSTer_MidiLink/mt32d/sbin/mt32d.new
