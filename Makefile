
enigma: enigma.o
	gcc enigma.o -o enigma

enigma.o: enigma.c
	gcc -g -c enigma.c

clean:
	rm -rf enigma
	rm -rf enigma.o
	rm -rf enigmapic.d
	rm -rf enigmapic.p1
	rm -rf enigmapic.as
	rm -rf enigmapic.pre
	rm -rf enigmapic.cmf
	rm -rf enigmapic.cof
	rm -rf enigmapic.obj
	rm -rf enigmapic.hex
	rm -rf enigmapic.hxl
	rm -rf enigmapic.sdb
	rm -rf enigmapic.sym
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
pic: enigmapic.c
	xc8 --chip=18F4550 --codeoffset=0x1000 enigmapic.c
