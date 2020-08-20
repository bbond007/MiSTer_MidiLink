CC=arm-linux-gnueabihf-gcc 
STRIP=arm-linux-gnueabihf-strip 
CCFLAGS=-Ialsa/include -Lalsa/lib -Ofast -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -funsafe-math-optimizations
LDFLAGS=-lasound -lm -pthread 
all : 
	$(CC) $(CCFLAGS) $(LDFLAGS) main.c modem.c serial.c serial2.c misc.c udpsock.c tcpsock.c alsa.c ini.c directory.c modem_snd.c -o midilink
	$(STRIP) midilink
	$(CC) $(CCFLAGS) mlinkutil.c misc.c serial2.c tcpsock.c -o mlinkutil
	$(STRIP) mlinkutil
clean:
	rm -f midilink *~ *.orig DEADJOE
	
	


