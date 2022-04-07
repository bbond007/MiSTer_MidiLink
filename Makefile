CC=arm-linux-gnueabihf-gcc 
STRIP=arm-linux-gnueabihf-strip 
CCFLAGS=-Ialsa/include -Lalsa/lib -Ofast -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -funsafe-math-optimizations
LDFLAGS=-lasound -lm -pthread 
SRC=main.c modem.c serial.c serial2.c misc.c udpsock.c tcpsock.c alsa.c ini.c directory.c modem_snd.c
SRC_UTL=mlinkutil.c misc.c serial2.c tcpsock.c
all : 
	$(CC) $(CCFLAGS) $(LDFLAGS) $(SRC) -o midilink
	$(STRIP) midilink
	$(CC) $(CCFLAGS) $(SRC_UTL) -o mlinkutil
	$(STRIP) mlinkutil
	
x86 : 
	gcc $(LDFLAGS)$(SRC)  -o midilink_x86
	strip midilink_x86
	gcc $(SRC_UTL) -o mlinkutil_x86
	strip mlinkutil_x86
	
clean:
	rm -f midilink mlinkutil mlinkutil_x86 *~ *.orig DEADJOE
	
	


