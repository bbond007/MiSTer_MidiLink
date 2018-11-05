CC=arm-linux-gnueabihf-gcc 
STRIP=arm-linux-gnueabihf-strip 

all : 
	$(CC) main.c serial.c setbaud.c misc.c -pthread -o midilink
	$(STRIP) midilink
clean:
	rm -f main.o serial.o setbaud.o misc.o midilink *~
	
	


