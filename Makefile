CC=arm-linux-gnueabihf-gcc 
STRIP=arm-linux-gnueabihf-strip 
CCFLAGS=-Ofast -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -funsafe-math-optimizations

all : 
	$(CC) $(CCFLAGS) main.c serial.c setbaud.c misc.c udpsock.c tcpsock.c alsa.c ini.c directory.c modem_snd.c -pthread -lasound -lm -o midilink
	$(STRIP) midilink
clean:
	rm -f main.o serial.o setbaud.o misc.o udpsock.o alsa.o ini.0 midilink *~
	
	


