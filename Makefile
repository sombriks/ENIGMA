
enigma: enigma.o
	gcc enigma.o -o enigma

enigma.o: enigma.c
	gcc -g -c enigma.c

clean:
	rm -rf enigma
	rm -rf *.o
	rm -rf *.d
	rm -rf *.p1
	rm -rf *.as
	rm -rf *.pre
	rm -rf *.cmf
	rm -rf *.cof
	rm -rf *.obj
	rm -rf *.hex
	rm -rf *.hxl
	rm -rf *.sdb
	rm -rf *.sym
	rm -rf startup.*
	rm -rf doprnt.*
	
all: enigma
	
# o target pra pic depende da correta instalação do xc8
# o codeoffset é usado para não sobrescrevermos o bootloader da placa de testes
# a placa de testes é essa: 
# http://www.afeletronica.com.br/pd-296167-pic-18f4550-afsmart-modulo-de-desenvolvimento.html
#
# pra usar a usb da placa (quando ela não estiver em modo de bootloader) 
# precisaremos disso aqui:
# http://www.microchip.com/mplab/microchip-libraries-for-applications
#

piscaled: piscaled.c
	xc8 --chip=18F4550 --codeoffset=0x1000 piscaled.c

picusb: picusb.c
	xc8 --chip=18F4550 --codeoffset=0x1000 picusb.c

enigmapic: enigmapic.c
	xc8 --chip=18F4550 --codeoffset=0x1000 enigmapic.c
