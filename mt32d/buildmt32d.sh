export PATH=/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/:$PATH
sudo mount lxde_linux.img MrRoot
#sudo mount linux.img MrRoot
rm -rf munt
git clone https://github.com/munt/munt
cp toolchain.cmake munt
cp Makefile.mt32d munt/mt32emu_alsadrv/Makefile
cd munt
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -DBUILD_TESTING=OFF -Dmunt_WITH_MT32EMU_QT=OFF -DCMAKE_BUILD_TYPE=Release
cd mt32emu && make
cd ../../mt32emu_alsadrv
make
arm-linux-gnueabihf-strip mt32d
cp mt32d ../../
