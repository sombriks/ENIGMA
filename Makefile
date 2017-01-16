
enigma: enigma.o
	gcc enigma.o -o enigma

enigma.o:
	gcc -c enigma.c

clean:
	rm -rf enigma.o enigma
	
all:
	enigma

 