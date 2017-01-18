
enigma: enigma.o
	gcc enigma.o -o enigma

enigma.o: enigma.c
	gcc -g -c enigma.c

clean:
	rm -rf enigma.o enigma
	
all: enigma
	

 
