CC=arm-linux-gnueabihf-gcc 
STRIP=arm-linux-gnueabihf-strip 
CCFLAGS=-Ialsa/include -Lalsa/lib -Ofast -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -funsafe-math-optimizations
LDFLAGS=-lasound -lm -pthread 
all : 

	$(CC) $(CCFLAGS) $(LDFLAGS) main.c serial.c setbaud.c misc.c udpsock.c tcpsock.c alsa.c ini.c directory.c modem_snd.c -o midilink
	$(STRIP) midilink
clean:
	rm -f midilink *~ *.orig DEADJOE
	
	


