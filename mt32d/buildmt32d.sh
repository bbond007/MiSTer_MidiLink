#export PATH=/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin:$PATH
export PATH=/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin:$PATH
#export PATH=/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin:$PATH
#export PATH=/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf/bin/:$PATH
rm -rf munt
git clone --depth 1 https://github.com/munt/munt
cp toolchain.cmake munt
cp Makefile.mt32d munt/mt32emu_alsadrv/Makefile
cd munt
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain.mt32emu -DBUILD_TESTING=OFF -Dmunt_WITH_MT32EMU_QT=OFF -DCMAKE_BUILD_TYPE=Release
cd mt32emu && make
cd ../../mt32emu_alsadrv
make
cp mt32d ../../sbin/mt32d.new

