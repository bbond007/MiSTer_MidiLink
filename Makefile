CC=arm-linux-gnueabihf-gcc 
STRIP=arm-linux-gnueabihf-strip 

all : 
	$(CC) main.c serial.c setbaud.c misc.c socket.c alsa.c ini.c -pthread -lasound -o midilink
	$(STRIP) midilink
clean:
	rm -f main.o serial.o setbaud.o misc.o socket.o alsa.o ini.0 midilink *~
	
	


