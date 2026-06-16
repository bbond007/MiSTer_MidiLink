#export PATH=/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin:$PATH
#export PATH=/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin:$PATH
export PATH=/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin:$PATH
#export PATH=/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf/bin/:$PATH
rm -rf casio_sw-10
git clone --depth 1 https://github.com/M-HT/casio_sw-10
cp Makefile.sw10_alsadrv casio_sw-10/sw10_alsadrv/Makefile
cd casio_sw-10/sw10_alsadrv
make
cp sw10_alsadrv ../../sbin/sw10_alsadrv.new





