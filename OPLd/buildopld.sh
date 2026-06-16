#export PATH=/opt/gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf/bin:$PATH
#export PATH=/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin:$PATH
export PATH=/opt/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf/bin:$PATH
#export PATH=/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-linux-gnueabihf/bin/:$PATH
rm -rf opld
tar -xvf opld.tar.gz
cp Makefile.opld opld/src/Makefile
cd opld/src
make
cd ..
cp opld ../sbin/opld.new

