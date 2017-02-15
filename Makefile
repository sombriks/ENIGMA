
enigma: enigma.o
	gcc enigma.o -o enigma

enigma.o: enigma.c
	gcc -g -c enigma.c

clean:
	rm -rf enigma.o enigma
	
all: enigma
	
# o target pra pic depende da correta instalação do xc8
# o codeoffset é usado para não sobrescrevermos o bootloader da placa de testes
# a placa de testes é essa: 
# http://www.afeletronica.com.br/pd-296167-pic-18f4550-afsmart-modulo-de-desenvolvimento.html
pic: enigmapic.c
	xc8 --chip=18F4550 --codeoffset=0x1000 enigmapic.c
