;**************************************************************************
; FILE:      enigma.asm                                                   *
; CONTENTS:  The Enigma Machine                                           *
; COPYRIGHT: MadLab Ltd. 2004                                             *
; AUTHOR:    James Hutchby                                                *
; UPDATED:   11/08/04                                                     *
;**************************************************************************

     list p=18F4550

     include "p18f4550.inc"

     __config _INTRC_OSC_NOCLKOUT & _WDT_ON & _PWRTE_ON & _BODEN_ON & _MCLRE_OFF & _LVP_OFF & _CP_ALL

     __idlocs h'EC10'

     errorlevel -207,-302,-305,-306


;**************************************************************************
;                                                                         *
; Specification                                                           *
;                                                                         *
;**************************************************************************

; power-up self-test:
; flashes all display LEDs twice (after a few seconds if keyboard missing)
; waits for keyboard assurance test, displays '!' if failure
; otherwise displays "ENIGMA" repeatedly until key pressed or timeout

; M3 Naval Enigma - machine setup characterised by:
; left, centre and right rotor numbers (0 to 7 = I to VIII)
; left, centre and right rotor rings (0 to 25 = A to Z)
; left, centre and right rotor start positions (0 to 25 = A to Z)
; reflector (0 or 1 = B or C)
; plug board - up to 13 wires connecting pairs of letters (0 to 25 = A to Z)

; default settings - rotors 1, 2, 3; rings A, A, A; start A, A, A; reflector B; no plugs

; F1 displays help message

; F2 sets rotors, accepts 3 digits (left-centre-right) '1' to '8'
; resets all rings and start positions to 'A'
; error if invalid key or same digit entered twice
; shift F2 displays current rotors, 3 digits '1' to '8'

; F3 sets rings, accepts 3 letters 'A' to 'Z'
; error if invalid key
; shift F3 displays current ring settings, 3 letters 'A' to 'Z'

; F4 sets rotor start positions, accepts 3 letters 'A' to 'Z'
; error if invalid key
; shift F4 displays rotor start positions, 3 letters 'A' to 'Z'

; shift F5 displays current rotor positions, 3 letters 'A' to 'Z'

; F6 sets reflector, accepts 1 letter 'B' or 'C'
; error if invalid key
; shift F6 displays current reflector, 1 letter 'B' or 'C'

; F7 sets plug board, accepts pairs of letters 'A' to 'Z'
; error if invalid key, letter already used, or link to self
; finish by ENTER, or after 13 pairs entered
; existing plugs not retained
; F7,ENTER will clear all plugs
; shift F7 displays plug board, up to 13 pairs of letters 'A' to 'Z'

; displays '?' when waiting for settings input, displays '!' if error

; letters 'A' to 'Z' encoded and displayed, other keys ignored
; rotors stepped before each letter encoded

; ESC/HOME resets rotors to start positions (i.e. clears message)
; display flashes twice to acknowledge

; BACKSPACE/DELETE deletes last letter entered (steps rotors back one position)
; keyboard LEDs flash twice to acknowledge
; single level of delete only


;**************************************************************************
;                                                                         *
; Port assignments                                                        *
;                                                                         *
;**************************************************************************

PORTA_IO  equ  b'00110000'         ; port A I/O status
PORTB_IO  equ  b'11000000'         ; port B I/O status

COLUMN1_BIT    equ  0              ; LED columns
COLUMN2_BIT    equ  4
COLUMN3_BIT    equ  1
COLUMN4_BIT    equ  5
COLUMN5_BIT    equ  0
COLUMN6_BIT    equ  7
COLUMN7_BIT    equ  6

COLUMN1_PORT   equ  PORTB
COLUMN2_PORT   equ  PORTB
COLUMN3_PORT   equ  PORTA
COLUMN4_PORT   equ  PORTB
COLUMN5_PORT   equ  PORTA
COLUMN6_PORT   equ  PORTA
COLUMN7_PORT   equ  PORTA

ROW1_BIT       equ  3              ; LED rows
ROW2_BIT       equ  2
ROW3_BIT       equ  2
ROW4_BIT       equ  1
ROW5_BIT       equ  3

ROW1_PORT      equ  PORTA
ROW2_PORT      equ  PORTA
ROW3_PORT      equ  PORTB
ROW4_PORT      equ  PORTB
ROW5_PORT      equ  PORTB

PIXELS_MASKA   equ  b'00001100'    ; pixel masks
PIXELS_MASKB   equ  b'00001110'

#define   KBD_CLK   PORTB,7        ; keyboard clock
#define   KBD_DATA  PORTB,6        ; keyboard data

kbd       macro c,d
          movlw PORTB_IO
          if c == 0
          bcf KBD_CLK
          andlw ~b'10000000'
          endif
          if d == 0
          bcf KBD_DATA
          andlw ~b'01000000'
          endif
          tris_B
          endm


;**************************************************************************
;                                                                         *
; Constants and timings                                                   *
;                                                                         *
;**************************************************************************

CLOCK     equ  d'4000000'     ; processor clock frequency in Hz

PRESCALE  equ  b'00000000'    ; prescale TMR0 1:2, weak pull-ups enabled

CR        equ  d'13'          ; carriage return

; delays in 1/100 s
SHOW_DELAY     equ  d'60'
BLANK_DELAY    equ  d'3'
FLASH_DELAY    equ  d'20'
ROTOR_DELAY    equ  d'4'


;--------------------------------------------------------------------------
; keyboard codes
;--------------------------------------------------------------------------

; keyboard command codes
KBD_BAT             equ  h'AA'     ; Basic Assurance Test successful
KBD_ERROR           equ  h'FC'     ; keyboard error
KBD_EXTENDED        equ  h'E0'     ; extended key code (+h'80' in scan code)
KBD_LEDS            equ  h'ED'     ; set/reset LEDs (0 = scroll, 1 = num, 2 = caps)
KBD_BREAK           equ  h'F0'     ; key break code
KBD_TYPEMATIC       equ  h'F3'     ; set typematic rate and delay (h'7f' = slowest)
KBD_ACK             equ  h'FA'     ; acknowledge
KBD_RESEND          equ  h'FE'     ; resend last byte
KBD_RESET           equ  h'FF'     ; reset keyboard

; keyboard scan codes
KBD_A               equ  h'1C'
KBD_B               equ  h'32'
KBD_C               equ  h'21'
KBD_D               equ  h'23'
KBD_E               equ  h'24'
KBD_F               equ  h'2B'
KBD_G               equ  h'34'
KBD_H               equ  h'33'
KBD_I               equ  h'43'
KBD_J               equ  h'3B'
KBD_K               equ  h'42'
KBD_L               equ  h'4B'
KBD_M               equ  h'3A'
KBD_N               equ  h'31'
KBD_O               equ  h'44'
KBD_P               equ  h'4D'
KBD_Q               equ  h'15'
KBD_R               equ  h'2D'
KBD_S               equ  h'1B'
KBD_T               equ  h'2C'
KBD_U               equ  h'3C'
KBD_V               equ  h'2A'
KBD_W               equ  h'1D'
KBD_X               equ  h'22'
KBD_Y               equ  h'35'
KBD_Z               equ  h'1A'
KBD_0               equ  h'45'
KBD_1               equ  h'16'
KBD_2               equ  h'1E'
KBD_3               equ  h'26'
KBD_4               equ  h'25'
KBD_5               equ  h'2E'
KBD_6               equ  h'36'
KBD_7               equ  h'3D'
KBD_8               equ  h'3E'
KBD_9               equ  h'46'
KBD_QUOTE           equ  h'0E'
KBD_MINUS           equ  h'4E'
KBD_EQUALS          equ  h'55'
KBD_BACKSLASH       equ  h'5D'
KBD_BACKSPACE       equ  h'66'
KBD_SPACE           equ  h'29'
KBD_TAB             equ  h'0D'
KBD_CAPS_LOCK       equ  h'58'
KBD_LEFT_SHFT       equ  h'12'
KBD_LEFT_CTRL       equ  h'14'
KBD_LEFT_GUI        equ  h'1F'+h'80'
KBD_LEFT_ALT        equ  h'11'
KBD_RIGHT_SHFT      equ  h'59'
KBD_RIGHT_CTRL      equ  h'14'+h'80'
KBD_RIGHT_GUI       equ  h'27'+h'80'
KBD_RIGHT_ALT       equ  h'11'+h'80'
KBD_APPS            equ  h'2F'+h'80'
KBD_ENTER           equ  h'5A'
KBD_ESC             equ  h'76'
KBD_F1              equ  h'05'
KBD_F2              equ  h'06'
KBD_F3              equ  h'04'
KBD_F4              equ  h'0C'
KBD_F5              equ  h'03'
KBD_F6              equ  h'0B'
KBD_F7              equ  h'83'
KBD_F8              equ  h'0A'
KBD_F9              equ  h'01'
KBD_F10             equ  h'09'
KBD_F11             equ  h'78'
KBD_F12             equ  h'07'
KBD_PRNT_SCRN1      equ  h'12'+h'80'         ; make = E0,12,E0,7C; break = E0,F0,7C,E0,F0,12
KBD_PRNT_SCRN2      equ  h'7C'+h'80'
KBD_SCROLL_LOCK     equ  h'7E'
KBD_PAUSE           equ  h'E1'               ; make = E1,14,77,E1,F0,14,F0,77; break = none
KBD_OPEN_SQUARE     equ  h'54'
KBD_INSERT          equ  h'70'+h'80'
KBD_HOME            equ  h'6C'+h'80'
KBD_PAGE_UP         equ  h'7D'+h'80'
KBD_DELETE          equ  h'71'+h'80'
KBD_END             equ  h'69'+h'80'
KBD_PAGE_DOWN       equ  h'7A'+h'80'
KBD_UP_ARROW        equ  h'75'+h'80'
KBD_LEFT_ARROW      equ  h'6B'+h'80'
KBD_DOWN_ARROW      equ  h'72'+h'80'
KBD_RIGHT_ARROW     equ  h'74'+h'80'
KBD_NUM_LOCK        equ  h'77'
KBD_KP_SLASH        equ  h'4A'+h'80'
KBD_KP_ASTERISK     equ  h'7C'
KBD_KP_MINUS        equ  h'7B'
KBD_KP_PLUS         equ  h'79'
KBD_KP_ENTER        equ  h'5A'+h'80'
KBD_KP_DOT          equ  h'71'
KBD_KP_0            equ  h'70'
KBD_KP_1            equ  h'69'
KBD_KP_2            equ  h'72'
KBD_KP_3            equ  h'7A'
KBD_KP_4            equ  h'6B'
KBD_KP_5            equ  h'73'
KBD_KP_6            equ  h'74'
KBD_KP_7            equ  h'6C'
KBD_KP_8            equ  h'75'
KBD_KP_9            equ  h'7D'
KBD_CLOSE_SQUARE    equ  h'5B'
KBD_SEMICOLON       equ  h'4C'
KBD_APOSTROPHE      equ  h'52'
KBD_COMMA           equ  h'41'
KBD_DOT             equ  h'49'
KBD_SLASH           equ  h'4A'

; ACPI
KDB_POWER           equ  h'37'+h'80'
KBD_SLEEP           equ  h'3F'+h'80'
KBD_WAKE            equ  h'5E'+h'80'

; Windows multimedia
KDB_NEXT_TRACK      equ  h'4D'+h'80'
KDB_PREV_TRACK      equ  h'15'+h'80'
KDB_STOP            equ  h'3B'+h'80'
KDB_PLAY_PAUSE      equ  h'34'+h'80'
KDB_MUTE            equ  h'23'+h'80'
KDB_VOLUME_UP       equ  h'32'+h'80'
KDB_VOLUME_DOWN     equ  h'21'+h'80'
KDB_MEDIA_SELECT    equ  h'50'+h'80'
KDB_EMAIL           equ  h'48'+h'80'
KDB_CALCULATOR      equ  h'2B'+h'80'
KDB_MY_COMPUTER     equ  h'40'+h'80'
KDB_WWW_SEARCH      equ  h'10'+h'80'
KDB_WWW_HOME        equ  h'3A'+h'80'
KDB_WWW_BACK        equ  h'38'+h'80'
KDB_WWW_FORWARD     equ  h'30'+h'80'
KDB_WWW_STOP        equ  h'28'+h'80'
KDB_WWW_REFRESH     equ  h'20'+h'80'
KDB_WWW_FAVOURITES  equ  h'18'+h'80'


;**************************************************************************
;                                                                         *
; File register usage                                                     *
;                                                                         *
;**************************************************************************

RAM  set  h'20'
MAX  set  h'80'

     cblock RAM
     flags                    ; various flags
     pixels:5                 ; LED pixels
     row, col                 ; row and column
     count, loop, repeat      ; counters
     bits                     ; bits count
     key                      ; key pressed, 0 if none
     kbd_code                 ; keyboard code
     kbd_command              ; keyboard command
     letter                   ; current letter, 0 to 25
     letter1, letter2         ; plug board letters
     leds                     ; keyboard LEDs
     ignore                   ; number of scan codes to ignore
     rotor_L:5                ; left rotor - number, ring, start, position, saved
     rotor_C:5                ; centre rotor - number, ring, start, position, saved
     rotor_R:5                ; right rotor - number, ring, start, position, saved
     reflector                ; reflector, 0 or 1
     plugs:d'26'              ; plug board, 0 to 25
     pairs                    ; number of plug board pairs
     setting:2                ; setting limits
     pnt                      ; pointer
     timeout                  ; timeout timer
     work1, work2             ; work registers
     RAM_
     endc

     if RAM_ > MAX
     error "File register usage overflow"
     endif

; flags
RESET     equ  0         ; set if keyboard reset
INHIBIT   equ  1         ; set if keyboard inhibited
EXTENDED  equ  2         ; set if extended key code received
BREAK     equ  3         ; set if key break code received
SHIFT1    equ  4         ; set if left shift key pressed
SHIFT2    equ  5         ; set if right shift key pressed

SHIFT_MASK     equ  (1<<SHIFT1)|(1<<SHIFT2)


;**************************************************************************
;                                                                         *
; Macros                                                                  *
;                                                                         *
;**************************************************************************

routine   macro label              ; routine
label
          endm

table     macro label              ; define lookup table
label     addwf PCL
          endm

entry     macro value              ; define table entry
          retlw value
          endm

index     macro label              ; index lookup table
          call label
          endm

jump      macro label              ; jump through table
          goto label
          endm

tstw      macro                    ; test w register
          iorlw 0
          endm

movff     macro f1,f2              ; move file to file
          movfw f1
          movwf f2
          endm

movlf     macro n,f                ; move literal to file
          movlw n
          movwf f
          endm

tris_A    macro                    ; tristate port A
          bsf STATUS,RP0
          movwf TRISA&h'7f'
          bcf STATUS,RP0
          endm

tris_B    macro                    ; tristate port B
          bsf STATUS,RP0
          movwf TRISB&h'7f'
          bcf STATUS,RP0
          endm

option_   macro                    ; set options
          bsf STATUS,RP0
          movwf OPTION_REG&h'7f'
          bcf STATUS,RP0
          endm


;--------------------------------------------------------------------------
; reset vector
;--------------------------------------------------------------------------

          org 0

          goto main_entry


;**************************************************************************
;                                                                         *
; Lookup tables                                                           *
;                                                                         *
;**************************************************************************

;  ***  ****   ***  ****  ***** *****  ***  *   *  ***   ****
; *   * *   * *   * *   * *     *     *   * *   *   *      * 
; *   * *   * *     *   * *     *     *     *   *   *      * 
; ***** ****  *     *   * ****  ****  *     *****   *      * 
; *   * *   * *     *   * *     *     *  ** *   *   *      * 
; *   * *   * *   * *   * *     *     *   * *   *   *   *  * 
; *   * ****   ***  ****  ***** *      ***  *   *  ***   **  

; *  *  *     *   * *   *  ***  ****   ***  ****   ***  *****
; * *   *     ** ** **  * *   * *   * *   * *   * *   *   *  
; **    *     * * * **  * *   * *   * *   * *   * *       *  
; **    *     *   * * * * *   * ****  *   * ****   ***    *  
; * *   *     *   * *  ** *   * *     *   * * *       *   *  
; *  *  *     *   * *  ** *   * *     * * * *  *  *   *   *  
; *   * ***** *   * *   *  ***  *      ***  *   *  ***    *  

; *   * *   * *   * *   * *   * *****
; *   * *   * *   * *   * *   *     *
; *   * *   * *   *  * *  *   *    * 
; *   * *   * *   *   *    * *    *  
; *   * *   * * * *  * *    *    *   
; *   *  * *  ** ** *   *   *   *    
;  ***    *   *   * *   *   *   *****

;  ***    *    ***   ***  *     *****  ***  *****  ***   *** 
; *   *  **   *   * *   * *     *     *   *     * *   * *   *
; *   *   *       *     * *     *     *        *  *   * *   *
; *   *   *    ***   ***  * *   ****  ****    *    ***   ****
; *   *   *   *         * *****     * *   *  *    *   *     *
; *   *   *   *     *   *   *       * *   * *     *   * *   *
;  ***   ***  *****  ***    *   ****   ***  *      ***   *** 

;        ***   *   
;       *   *  *   
; *****     *  *   
;          *   *   
; *****   *    *   
;                  
;         *    *   


          table font

CHAR_SPACE     equ  d'0'

          entry b'0000000'              ; space
          entry b'0000000'
          entry b'0000000'
          entry b'0000000'
          entry b'0000000'


CHAR_A         equ  d'1'

          entry b'1111110'              ; A
          entry b'0001001'
          entry b'0001001'
          entry b'0001001'
          entry b'1111110'

          entry b'1111111'              ; B
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'0110110'

          entry b'0111110'              ; C
          entry b'1000001'
          entry b'1000001'
          entry b'1000001'
          entry b'0100010'

          entry b'1111111'              ; D
          entry b'1000001'
          entry b'1000001'
          entry b'1000001'
          entry b'0111110'

          entry b'1111111'              ; E
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'1000001'

          entry b'1111111'              ; F
          entry b'0001001'
          entry b'0001001'
          entry b'0001001'
          entry b'0000001'

          entry b'0111110'              ; G
          entry b'1000001'
          entry b'1000001'
          entry b'1010001'
          entry b'0110010'

          entry b'1111111'              ; H
          entry b'0001000'
          entry b'0001000'
          entry b'0001000'
          entry b'1111111'

          entry b'0000000'              ; I
          entry b'1000001'
          entry b'1111111'
          entry b'1000001'
          entry b'0000000'

          entry b'0100000'              ; J
          entry b'1000001'
          entry b'1000001'
          entry b'0111111'
          entry b'0000001'

          entry b'1111111'              ; K
          entry b'0001100'
          entry b'0010010'
          entry b'0100001'
          entry b'1000000'

          entry b'1111111'              ; L
          entry b'1000000'
          entry b'1000000'
          entry b'1000000'
          entry b'1000000'

          entry b'1111111'              ; M
          entry b'0000010'
          entry b'0000100'
          entry b'0000010'
          entry b'1111111'

          entry b'1111111'              ; N
          entry b'0000110'
          entry b'0001000'
          entry b'0110000'
          entry b'1111111'

          entry b'0111110'              ; O
          entry b'1000001'
          entry b'1000001'
          entry b'1000001'
          entry b'0111110'

          entry b'1111111'              ; P
          entry b'0001001'
          entry b'0001001'
          entry b'0001001'
          entry b'0000110'

          entry b'0111110'              ; Q
          entry b'1000001'
          entry b'1100001'
          entry b'1000001'
          entry b'0111110'

          entry b'1111111'              ; R
          entry b'0001001'
          entry b'0011001'
          entry b'0101001'
          entry b'1000110'

          entry b'0100110'              ; S
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'0110010'

          entry b'0000001'              ; T
          entry b'0000001'
          entry b'1111111'
          entry b'0000001'
          entry b'0000001'

          entry b'0111111'              ; U
          entry b'1000000'
          entry b'1000000'
          entry b'1000000'
          entry b'0111111'

          entry b'0011111'              ; V
          entry b'0100000'
          entry b'1000000'
          entry b'0100000'
          entry b'0011111'

          entry b'1111111'              ; W
          entry b'0100000'
          entry b'0010000'
          entry b'0100000'
          entry b'1111111'

          entry b'1100011'              ; X
          entry b'0010100'
          entry b'0001000'
          entry b'0010100'
          entry b'1100011'

          entry b'0000111'              ; Y
          entry b'0001000'
          entry b'1110000'
          entry b'0001000'
          entry b'0000111'

          entry b'1100001'              ; Z
          entry b'1010001'
          entry b'1001001'
          entry b'1000101'
          entry b'1000011'


CHAR_0         equ  CHAR_A+d'26'

          entry b'0111110'              ; 0
          entry b'1000001'
          entry b'1000001'
          entry b'1000001'
          entry b'0111110'

          entry b'0000000'              ; 1
          entry b'1000010'
          entry b'1111111'
          entry b'1000000'
          entry b'0000000'

          entry b'1110010'              ; 2
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'1000110'

          entry b'0100010'              ; 3
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'0110110'

          entry b'0011111'              ; 4
          entry b'0010000'
          entry b'1111000'
          entry b'0010000'
          entry b'0010000'

          entry b'1001111'              ; 5
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'0110001'

          entry b'0111110'              ; 6
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'0110010'

          entry b'1100001'              ; 7
          entry b'0010001'
          entry b'0001001'
          entry b'0000101'
          entry b'0000011'

          entry b'0110110'              ; 8
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'0110110'

          entry b'0100110'              ; 9
          entry b'1001001'
          entry b'1001001'
          entry b'1001001'
          entry b'0111110'


CHAR_EQUALS    equ  CHAR_0+d'10'

          entry b'0010100'              ; =
          entry b'0010100'
          entry b'0010100'
          entry b'0010100'
          entry b'0010100'


CHAR_QUERY     equ  CHAR_EQUALS+1

          entry b'0000010'              ; ?
          entry b'0000001'
          entry b'1010001'
          entry b'0001001'
          entry b'0000110'


CHAR_ERROR     equ  CHAR_QUERY+1

          entry b'0000000'              ; !
          entry b'0000000'
          entry b'1011111'
          entry b'0000000'
          entry b'0000000'


          table reflectors

;             ABCDEFGHIJKLMNOPQRSTUVWXYZ
          dt "YRUHQSLDPXNGOKMIEBFZCWVJAT"    ; reflector B
          dt "FVPJIAOYEDRZXWGCTKUQSBNMHL"    ; reflector C
;         dt "ENKQAUYWJICOPBLMDXZVFTHRGS"    ; 'thin' reflector B
;         dt "RDOBJNTKVEHMLFCWZAXGYIPSUQ"    ; 'thin' reflector C


          org h'100'

          table rotors

;             ABCDEFGHIJKLMNOPQRSTUVWXYZ
          dt "EKMFLGDQVZNTOWYHXUSPAIBRCJ"    ; rotor I
          dt "AJDKSIRUXBLHWTMCQGZNPYFVOE"    ; rotor II
          dt "BDFHJLCPRTXVZNYEIWGAKMUSQO"    ; rotor III
          dt "ESOVPZJAYQUIRHXLNFTGKDCMWB"    ; rotor IV
          dt "VZBRGITYUPSDNHLXAWMJQOFECK"    ; rotor V
          dt "JPGVOUMFYQBENHZRDKASXLICTW"    ; rotor VI
          dt "NZJHGRCXMYSWBOUFAIVLPEKQDT"    ; rotor VII
          dt "FKQHTLXOCBJSPDZRAMEWNIUYGV"    ; rotor VIII
;         dt "LEYJVCNIXWPBQMDRTAKZGFUHOS"    ; beta rotor - 'thin' 4th rotor used in leftmost position
;         dt "FSOKANUERHMBTIYCWLQPZXVGJD"    ; gamma rotor - 'thin' 4th rotor used in leftmost position


          table notches

          dt "QQ"                            ; rotor I
          dt "EE"                            ; rotor II
          dt "VV"                            ; rotor III
          dt "JJ"                            ; rotor IV
          dt "ZZ"                            ; rotor V
          dt "ZM"                            ; rotor VI
          dt "ZM"                            ; rotor VII
          dt "ZM"                            ; rotor VIII
;         dt ".."                            ; beta rotor - no notch as only used in 4th position
;         dt ".."                            ; gamma rotor - no notch as only used in 4th position


          table row_enable                   ; display row enable

AorB      equ  0

          dt (1<<ROW1_BIT)|((ROW1_PORT==PORTB)<<AorB)
          dt (1<<ROW2_BIT)|((ROW2_PORT==PORTB)<<AorB)
          dt (1<<ROW3_BIT)|((ROW3_PORT==PORTB)<<AorB)
          dt (1<<ROW4_BIT)|((ROW4_PORT==PORTB)<<AorB)
          dt (1<<ROW5_BIT)|((ROW5_PORT==PORTB)<<AorB)


          org h'200'

          table keyboard                     ; scan codes and ASCII

          dt KBD_A,'A'
          dt KBD_B,'B'
          dt KBD_C,'C'
          dt KBD_D,'D'
          dt KBD_E,'E'
          dt KBD_F,'F'
          dt KBD_G,'G'
          dt KBD_H,'H'
          dt KBD_I,'I'
          dt KBD_J,'J'
          dt KBD_K,'K'
          dt KBD_L,'L'
          dt KBD_M,'M'
          dt KBD_N,'N'
          dt KBD_O,'O'
          dt KBD_P,'P'
          dt KBD_Q,'Q'
          dt KBD_R,'R'
          dt KBD_S,'S'
          dt KBD_T,'T'
          dt KBD_U,'U'
          dt KBD_V,'V'
          dt KBD_W,'W'
          dt KBD_X,'X'
          dt KBD_Y,'Y'
          dt KBD_Z,'Z'

          dt KBD_0,'0'
          dt KBD_1,'1'
          dt KBD_2,'2'
          dt KBD_3,'3'
          dt KBD_4,'4'
          dt KBD_5,'5'
          dt KBD_6,'6'
          dt KBD_7,'7'
          dt KBD_8,'8'
          dt KBD_9,'9'

          dt KBD_KP_0,'0'
          dt KBD_KP_1,'1'
          dt KBD_KP_2,'2'
          dt KBD_KP_3,'3'
          dt KBD_KP_4,'4'
          dt KBD_KP_5,'5'
          dt KBD_KP_6,'6'
          dt KBD_KP_7,'7'
          dt KBD_KP_8,'8'
          dt KBD_KP_9,'9'

          dt KBD_ENTER,CR
          dt KBD_KP_ENTER,CR

          dt 0


          table vectors                      ; vectors

vector    macro scan,shift,routine
          entry scan
          entry shift
          goto routine
          endm

          vector KBD_BACKSPACE,-1,step_undo
          vector KBD_DELETE,-1,step_undo
          vector KBD_ESC,-1,reset_rotors
          vector KBD_HOME,-1,reset_rotors
          vector KBD_F1,-1,display_help
          vector KBD_F2,0,set_rotors
          vector KBD_F2,1,show_rotors
          vector KBD_F3,0,set_rings
          vector KBD_F3,1,show_rings
          vector KBD_F4,0,set_starts
          vector KBD_F4,1,show_starts
          vector KBD_F5,1,show_positions
          vector KBD_F6,0,set_reflector
          vector KBD_F6,1,show_reflector
          vector KBD_F7,0,set_plugs
          vector KBD_F7,1,show_plugs

          entry 0


;**************************************************************************
;                                                                         *
; Procedures                                                              *
;                                                                         *
;**************************************************************************

;**************************************************************************
; PS/2 keyboard routines                                                  *
;**************************************************************************

;--------------------------------------------------------------------------
; delay
;--------------------------------------------------------------------------

          nop                           ; [4]
          nop                           ; [4]
          nop                           ; [4]
          nop                           ; [4]
          nop                           ; [4]
          nop                           ; [4]
delay16   return                        ; [8]

ldelay    movwf work1                   ; [4]
ldel1     decfsz work1                  ; [4/8]
          goto ldel1                    ; [8/0]
          return                        ; [8]

delay     macro cycles                  ; delay for a number of cycles

          if (cycles) < 0
          error "Delay negative cycles"
          endif

          variable i = cycles

          if i > d'40'

          variable n = (i-d'20')/d'12'
          i -= (n*d'12')+d'20'

          movlw n                       ; [4]
          call ldelay                   ; [8]

          else
          if i >= d'16'

          variable n = (i-d'16')/d'4'
          i -= (n*d'4')+d'16'

          call delay16-n                ; [8]

          endif
          endif

          while i > 0
          nop                           ; [4]
          i -= d'4'
          endw

          endm


;--------------------------------------------------------------------------
; waits, fed with the wait in ms in w reg
;--------------------------------------------------------------------------

          routine wait_ms

          movwf count

WAIT      equ  CLOCK/(d'1000'*d'80')

wait1     movlf WAIT,loop
wait2     clrwdt                        ; [4]
          delay d'64'                   ; [64]
          decfsz loop                   ; [4]
          goto wait2                    ; [8]

          decfsz count
          goto wait1

          return


;--------------------------------------------------------------------------
; waits for a clock transition from the keyboard, returns the Z flag set
; if timeout
;--------------------------------------------------------------------------

          routine wait_kbd

; timeout period in ms
TIMEOUT   equ  (d'256'*d'80')/(CLOCK/d'1000')

          clrf timeout                  ; wait for clock to go high
wait3     clrwdt                        ; [4]
          btfsc KBD_CLK                 ; [8]
          goto wait4
          btfsc KBD_CLK                 ; [8]
          goto wait4
          btfsc KBD_CLK                 ; [8]
          goto wait4
          btfsc KBD_CLK                 ; [8]
          goto wait4
          btfsc KBD_CLK                 ; [8]
          goto wait4
          btfsc KBD_CLK                 ; [8]
          goto wait4
          btfsc KBD_CLK                 ; [8]
          goto wait4
          btfsc KBD_CLK                 ; [8]
          goto wait4
          decfsz timeout                ; [4]
          goto wait3                    ; [8]

          goto wait7                    ; timeout

wait4     clrf timeout                  ; wait for clock to go low
wait5     clrwdt                        ; [4]
          btfss KBD_CLK                 ; [8]
          goto wait6
          btfss KBD_CLK                 ; [8]
          goto wait6
          btfss KBD_CLK                 ; [8]
          goto wait6
          btfss KBD_CLK                 ; [8]
          goto wait6
          btfss KBD_CLK                 ; [8]
          goto wait6
          btfss KBD_CLK                 ; [8]
          goto wait6
          btfss KBD_CLK                 ; [8]
          goto wait6
          btfss KBD_CLK                 ; [8]
          goto wait6
          decfsz timeout                ; [4]
          goto wait5                    ; [8]

          goto wait7                    ; timeout

wait6     delay d'10'*(CLOCK/d'1000000')

          clrz                          ; signal ok

wait7     return


;--------------------------------------------------------------------------
; transmits the byte in w reg to the keyboard, returns the Z flag set if
; timeout
;--------------------------------------------------------------------------

          routine tx_kbd

          movwf kbd_code

          kbd 0,1                       ; request to send
          movlw 1
          call wait_ms

          kbd 0,0                       ; start bit
          nop
          kbd 1,0

          call wait_kbd
          bz tx6                        ; branch if timeout

          movlf 8,count
          clrf bits

tx1       btfsc kbd_code,0
          incf bits

          rrf kbd_code                  ; transmit data bits
          skpnc
          bsf KBD_DATA
          skpc
          bcf KBD_DATA

          call wait_kbd
          bz tx6                        ; branch if timeout

          decfsz count
          goto tx1

          btfss bits,0                  ; parity bit (odd parity)
          bsf KBD_DATA
          btfsc bits,0
          bcf KBD_DATA

          call wait_kbd
          bz tx6                        ; branch if timeout

          kbd 1,1                       ; stop bit

          clrf timeout                  ; wait for acknowledge bit
tx2       clrwdt
          decf timeout
          bz tx6                        ; branch if timeout
          btfsc KBD_DATA
          goto tx2

          clrf timeout
tx3       clrwdt
          decf timeout
          bz tx6                        ; branch if timeout
          btfsc KBD_CLK
          goto tx3

          clrf timeout
tx4       clrwdt
          decf timeout
          bz tx6                        ; branch if timeout
          btfss KBD_DATA
          goto tx4

          clrf timeout
tx5       clrwdt
          decf timeout
          bz tx6                        ; branch if timeout
          btfss KBD_CLK
          goto tx5

          clrz                          ; signal ok

          return

tx6       kbd 1,1

          setz                          ; signal timeout

          return


;--------------------------------------------------------------------------
; polls the keyboard
;--------------------------------------------------------------------------

          routine poll_kbd

;         delay d'10'*(CLOCK/d'1000000')

          call rx_kbd                   ; receive byte
          bz pollk7                     ; branch if timeout
          bc pollk7                     ; branch if error

          tstf ignore                   ; ignore scan codes ?
          bz pollk1                     ; branch if not

          decf ignore
          goto pollk7

pollk1    movlw KBD_PAUSE               ; pause key ?
          subwf kbd_code,w
          bnz pollk2                    ; branch if not

          movlf d'7',ignore             ; ignore the next scan codes
          goto pollk7

pollk2    movlw KBD_EXTENDED            ; extended key code ?
          subwf kbd_code,w
          bnz pollk3                    ; branch if not

          bsf flags,EXTENDED            ; signal received

          goto pollk8

pollk3    movlw KBD_BREAK               ; key break code ?
          subwf kbd_code,w
          bnz pollk4                    ; branch if not

          bsf flags,BREAK               ; signal received

          goto pollk8

pollk4    btfsc flags,EXTENDED          ; extended key ?
          goto pollk6                   ; branch if yes

          movlw KBD_LEFT_SHFT           ; left shift key ?
          subwf kbd_code,w
          bnz pollk5                    ; branch if not

          bsf flags,SHIFT1              ; shift status
          btfsc flags,BREAK
          bcf flags,SHIFT1

          goto pollk7

pollk5    movlw KBD_RIGHT_SHFT          ; right shift key ?
          subwf kbd_code,w
          bnz pollk6                    ; branch if not

          bsf flags,SHIFT2              ; shift status
          btfsc flags,BREAK
          bcf flags,SHIFT2

          goto pollk7

pollk6    btfsc flags,BREAK             ; key break ?
          goto pollk7                   ; branch if yes

          movff kbd_code,key            ; store key pressed

          btfsc flags,EXTENDED
          bsf key,7                     ; extended scan code

pollk7    bcf flags,EXTENDED
          bcf flags,BREAK

pollk8    return


;--------------------------------------------------------------------------
; receives a byte from the keyboard into w reg (and kbd_code), returns
; the Z flag set if timeout and the C flag set if error
;--------------------------------------------------------------------------

          routine get_kbd

          kbd 1,1

          call wait_kbd
          bz rx4                        ; branch if timeout


          routine rx_kbd

          btfsc KBD_DATA                ; test start bit
          goto rx5                      ; branch if error

          movlf 8,count
          clrf bits

rx1       call wait_kbd
          bz rx4                        ; branch if timeout

          clrc                          ; receive data bits
          btfsc KBD_DATA
          setc
          rrf kbd_code

          btfsc kbd_code,7
          incf bits

          decfsz count
          goto rx1

          call wait_kbd
          bz rx4                        ; branch if timeout

          btfsc KBD_DATA                ; test parity bit
          incf bits
          btfss bits,0
          goto rx5                      ; branch if error

          call wait_kbd
          bz rx4                        ; branch if timeout

          btfss KBD_DATA                ; test stop bit
          goto rx5                      ; branch if error

          clrf timeout                  ; wait for clock to go high
rx2       clrwdt
          btfsc KBD_CLK
          goto rx3
          decfsz timeout
          goto rx2
          goto rx4                      ; timeout

rx3       movfw kbd_code

          clrz                          ; signal ok
          clrc

          return

rx4       clrw
          clrf kbd_code

          setz                          ; signal timeout
          clrc

          return

rx5       clrw
          clrf kbd_code

          clrz                          ; signal error
          setc

          return


;--------------------------------------------------------------------------
; transmits the command in w reg to the keyboard, returns the Z flag set
; if error
;--------------------------------------------------------------------------

          routine tx_command

          movwf kbd_command

txc1      movfw kbd_command             ; transmit command
          call tx_kbd
          bz txc4                       ; branch if timeout

          call get_kbd                  ; wait for acknowledge
          bz txc2                       ; branch if timeout
          bc txc2                       ; branch if error

          xorlw KBD_RESEND              ; resend ?
          bz txc1                       ; branch if yes

          xorlw KBD_RESEND              ; acknowledge ?
          xorlw KBD_ACK
          bnz txc4                      ; branch if not

          goto txc3

txc2      movlw KBD_RESEND              ; transmit command
          call tx_kbd
          bz txc4                       ; branch if timeout

          call get_kbd                  ; wait for acknowledge
          bz txc4                       ; branch if timeout
          bc txc4                       ; branch if error

          xorlw KBD_RESEND              ; resend ?
          bz txc1                       ; branch if yes

          xorlw KBD_RESEND              ; acknowledge ?
          xorlw KBD_ACK
          bnz txc4                      ; branch if not

txc3      clrz                          ; signal ok

          return

txc4      setz                          ; signal error

          return


;--------------------------------------------------------------------------
; converts a keyboard scan code in w reg into ASCII, returns the ASCII
; code in w reg, or the Z flag set if not found
;--------------------------------------------------------------------------

          routine convert_key

          movwf work1

          movlf high keyboard,PCLATH

          clrf work2

conv1     movfw work2                   ; get scan code
          index keyboard
          incf work2

          tstw                          ; end of table ?
          bz conv3                      ; branch if yes

          subwf work1,w                 ; match ?
          bnz conv2                     ; branch if not

          movfw work2                   ; get ASCII
          index keyboard

          clrz                          ; signal found

          goto conv3

conv2     incf work2

          goto conv1

conv3     return


;--------------------------------------------------------------------------
; initialises the keyboard, returns the Z flag set if error
;--------------------------------------------------------------------------

          routine initialise_keyboard

          bcf flags,RESET
          bcf flags,INHIBIT

REPEAT    equ  d'1000'/TIMEOUT

initk1    movlf REPEAT,repeat           ; wait for keyboard BAT
initk2    call get_kbd
          bz initk3                     ; branch if timeout
          bc initk4                     ; branch if error
          xorlw KBD_BAT
          bnz initk4
          goto initk5                   ; branch if ok
initk3    decfsz repeat
          goto initk2

initk4    btfsc flags,RESET             ; not received or error
          goto initk6
          bsf flags,RESET

          movlw d'250'
          call wait_ms
          movlw d'250'
          call wait_ms
          movlw d'250'
          call wait_ms
          movlw d'250'
          call wait_ms

          movlw KBD_RESET               ; reset keyboard
          call tx_command
          bz initk6                     ; branch if error

          goto initk1

initk5    movlw KBD_TYPEMATIC           ; set typematic to slowest
          call tx_command
          bz initk6
          movlw h'7f'
          call tx_command
          bz initk6                     ; branch if error

          clrz                          ; signal ok

          return

initk6    setz                          ; signal error

          return


;--------------------------------------------------------------------------
; inhibits communication from the keyboard
;--------------------------------------------------------------------------

          routine inhibit_keyboard

          kbd 0,1

          movlw 1
          call wait_ms

          bsf flags,INHIBIT             ; keyboard inhibited

          return


;--------------------------------------------------------------------------
; releases communication from the keyboard
;--------------------------------------------------------------------------

          routine release_keyboard

          kbd 1,1

          delay d'75'*(CLOCK/d'1000000')

          bcf flags,INHIBIT             ; keyboard not inhibited

          return


;--------------------------------------------------------------------------
; flashes the keyboard LEDs
;--------------------------------------------------------------------------

          routine flash_keyboard

          movlf d'4',repeat

          clrf leds

flash1    movlw KBD_LEDS                ; set/reset LEDs
          call tx_command
          bz flash2                     ; branch if error

          comf leds                     ; toggle LEDS
          movfw leds
          andlw 7
          call tx_command
          bz flash2                     ; branch if error

          decf repeat,w
          movlw FLASH_DELAY
          skpnz
          movlw d'1'
          call display

          decfsz repeat
          goto flash1

flash2    return


;**************************************************************************
; display routines                                                        *
;**************************************************************************

;--------------------------------------------------------------------------
; display delay, fed with the period in 1/100 s in w reg
;--------------------------------------------------------------------------

          routine display

poll      macro

          local poll1,poll2

          btfss flags,INHIBIT           ; clock active and keyboard [8/4]
          goto poll1                    ; not inhibited ? [0/8]
          nop                           ; [4/0]
          goto poll2                    ; [8/0]
poll1     btfss KBD_CLK                 ; [0/8]
          call poll_kbd                 ; poll keyboard if yes
poll2
          endm

          movwf loop

          movlf high row_enable,PCLATH

disp1     movlf pixels,FSR

          clrf row

disp2     movlf PIXELS_MASKA,PORTA
          movlf PIXELS_MASKB,PORTB

          poll

          movfw row
          index row_enable

          poll

          movwf work1
          andlw ~(1<<AorB)
          btfss work1,AorB
          xorwf PORTA
          btfsc work1,AorB
          xorwf PORTB

pixel     macro port,bit,n

          local pix1

          btfsc 0,n
          bsf port,bit

LOOP      set  CLOCK/(d'100'*d'32'*d'5'*d'7')

          movlf LOOP,col
          clrwdt
pix1      poll                          ; [20]
          decfsz col                    ; [4]
          goto pix1                     ; [8]

          bcf port,bit

          endm

          pixel COLUMN1_PORT,COLUMN1_BIT,0
          pixel COLUMN2_PORT,COLUMN2_BIT,1
          pixel COLUMN3_PORT,COLUMN3_BIT,2
          pixel COLUMN4_PORT,COLUMN4_BIT,3
          pixel COLUMN5_PORT,COLUMN5_BIT,4
          pixel COLUMN6_PORT,COLUMN6_BIT,5
          pixel COLUMN7_PORT,COLUMN7_BIT,6

          incf FSR

          incf row
          movlw 5
          subwf row,w
          bnz disp2

          poll

          decfsz loop
          goto disp1

          clrf PORTA                    ; display LEDs off
          clrf PORTB

          return


;--------------------------------------------------------------------------
; clears the display
;--------------------------------------------------------------------------

          routine clear_display

          clrf pixels+0
          clrf pixels+1
          clrf pixels+2
          clrf pixels+3
          clrf pixels+4

          return


;--------------------------------------------------------------------------
; flashes the display
;--------------------------------------------------------------------------

          routine flash_display

          call clear_display

          movlf d'4',repeat

flash3    comf pixels+0
          comf pixels+1
          comf pixels+2
          comf pixels+3
          comf pixels+4

          decf repeat,w
          movlw FLASH_DELAY
          skpnz
          movlw d'1'
          call display

          decfsz repeat
          goto flash3

          return


;--------------------------------------------------------------------------
; shows the character in w reg
;--------------------------------------------------------------------------

          routine show_character

          movwf work1
          addwf work1
          clrc
          rlf work1
          addwf work1                   ; * 5

          call clear_display            ; clear pixels

          movlf high font,PCLATH

          movlf 5,work2
          movlf pixels,FSR

showc1    movfw work1                   ; index into the font
          index font
          incf work1

          movwf 0                       ; set pixels

          incf FSR

          decfsz work2
          goto showc1

          return


;--------------------------------------------------------------------------
; reads a byte from data memory, fed with the address in w reg, returns
; the byte in w reg
;--------------------------------------------------------------------------

read_EEPROM    macro

          bsf STATUS,RP0
          movwf EEADR

          bsf EECON1,RD                 ; read byte
          movfw EEDATA
          bcf STATUS,RP0

          endm


;--------------------------------------------------------------------------
; displays a message, fed with the message EEPROM address in w reg
;--------------------------------------------------------------------------

          routine display_message

          movwf pnt

dispm1    tstf key                      ; key pressed ?
          bnz dispm3                    ; branch if yes

          movfw pnt                     ; get current character
          read_EEPROM
          movwf work1

          tstw                          ; message finished ?
          bz dispm3                     ; branch if yes

          movlw ' '                     ; convert to font index
          subwf work1,w
          movlw CHAR_SPACE
          bz dispm2

          movlw '='
          subwf work1,w
          movlw CHAR_EQUALS
          bz dispm2

          movfw work1
          addlw -'A'+CHAR_A
          skpc
          addlw +'A'-CHAR_A-'0'+CHAR_0

dispm2    call show_character           ; show the character
          movlw SHOW_DELAY
          call display

          call clear_display            ; blank between characters
          movlw BLANK_DELAY
          call display

          incf pnt                      ; next character

          goto dispm1

dispm3    call clear_display

          return


;**************************************************************************
; Enigma encoding/decoding routines                                       *
;**************************************************************************

;--------------------------------------------------------------------------
; reduces modulo 26
;--------------------------------------------------------------------------

mod26     macro f

          movlw d'26'
          btfsc f,7
          addwf f
          subwf f,w
          skpnc
          movwf f

          endm


;--------------------------------------------------------------------------
; multiplies w reg by 26
;--------------------------------------------------------------------------

          routine mult26

          movwf work1
          swapf work1                   ; * 16
          subwf work1
          subwf work1
          subwf work1                   ; * 13
          movfw work1
          addwf work1,w                 ; * 26

          return


;--------------------------------------------------------------------------
; steps the rotors forwards
;--------------------------------------------------------------------------

          routine step_rotors

          movff rotor_L+3,rotor_L+4     ; save current position
          movff rotor_C+3,rotor_C+4
          movff rotor_R+3,rotor_R+4

          movlf high notches,PCLATH

          movfw rotor_C+0               ; centre rotor at notch ?
          addwf rotor_C+0,w
          index notches
          addlw -'A'
          subwf rotor_C+3,w
          bz step1
          incf rotor_C+0,w
          addwf rotor_C+0,w
          index notches
          addlw -'A'
          subwf rotor_C+3,w
          bnz step2                     ; branch if not

step1     incf rotor_L+3                ; rotate left rotor
          mod26 rotor_L+3

          goto step3

step2     movfw rotor_R+0               ; right rotor at notch ?
          addwf rotor_R+0,w
          index notches
          addlw -'A'
          subwf rotor_R+3,w
          bz step3
          incf rotor_R+0,w
          addwf rotor_R+0,w
          index notches
          addlw -'A'
          subwf rotor_R+3,w
          bnz step4                     ; branch if not

step3     incf rotor_C+3                ; rotate centre rotor
          mod26 rotor_C+3

step4     incf rotor_R+3                ; rotate right rotor
          mod26 rotor_R+3

          return


;--------------------------------------------------------------------------
; steps the rotors back one position
;--------------------------------------------------------------------------

          routine step_undo

          movff rotor_L+4,rotor_L+3     ; restore saved position
          movff rotor_C+4,rotor_C+3
          movff rotor_R+4,rotor_R+3

          call clear_display            ; clear display

          call flash_keyboard           ; acknowledge

          call inhibit_keyboard         ; disable keyboard

          return


;--------------------------------------------------------------------------
; transposes a letter at the plug board
;--------------------------------------------------------------------------

          routine do_plugs

          movlw plugs
          addwf letter,w
          movwf FSR
          movff 0,letter

          return


;--------------------------------------------------------------------------
; transposes a letter through a rotor in the forward (right-to-left)
; direction, fed with a pointer to the rotor in w reg
;--------------------------------------------------------------------------

          routine do_rotor_forward

          movwf FSR

          movlf high rotors,PCLATH

          movfw 0                       ; rotor number
          call mult26
          movwf work1

          incf FSR                      ; adjust for ring position
          movfw 0
          subwf letter
          mod26 letter

          incf FSR                      ; adjust for rotor position
          incf FSR
          movfw 0
          addwf letter
          mod26 letter

          movfw work1                   ; transpose letter
          addwf letter,w
          index rotors
          addlw -'A'
          movwf letter

          movfw 0                       ; adjust for rotor position
          subwf letter
          mod26 letter

          decf FSR                      ; adjust for ring position
          decf FSR
          movfw 0
          addwf letter
          mod26 letter

          return


;--------------------------------------------------------------------------
; transposes a letter through a rotor in the reverse (left-to-right)
; direction, fed with a pointer to the rotor in w reg
;--------------------------------------------------------------------------

          routine do_rotor_reverse

          movwf FSR

          movlf high rotors,PCLATH

          movfw 0                       ; rotor number
          call mult26
          movwf work1

          incf FSR                      ; adjust for ring position
          movfw 0
          subwf letter
          mod26 letter

          incf FSR                      ; adjust for rotor position
          incf FSR
          movfw 0
          addwf letter
          mod26 letter

          clrf work2                    ; transpose letter
rev1      movfw work1
          index rotors
          addlw -'A'
          subwf letter,w
          bz rev2
          incf work1
          incf work2
          goto rev1
rev2      movff work2,letter

          movfw 0                       ; adjust for rotor position
          subwf letter
          mod26 letter

          decf FSR                      ; adjust for ring position
          decf FSR
          movfw 0
          addwf letter
          mod26 letter

          return


;--------------------------------------------------------------------------
; transposes a letter at the reflector
;--------------------------------------------------------------------------

          routine do_reflector

          movlf high reflectors,PCLATH

          movfw reflector
          call mult26
          addwf letter,w
          index reflectors
          addlw -'A'
          movwf letter

          return


;--------------------------------------------------------------------------
; encodes the letter in w reg, returns the encoded letter in w reg
;--------------------------------------------------------------------------

          routine encode_letter

          movwf letter

          call step_rotors              ; step the rotors

          call do_plugs                 ; plug board

          movlw rotor_R                 ; right rotor forward
          call do_rotor_forward

          movlw rotor_C                 ; centre rotor forward
          call do_rotor_forward

          movlw rotor_L                 ; left rotor forward
          call do_rotor_forward

          call do_reflector             ; reflector

          movlw rotor_L                 ; left rotor reverse
          call do_rotor_reverse

          movlw rotor_C                 ; centre rotor reverse
          call do_rotor_reverse

          movlw rotor_R                 ; right rotor reverse
          call do_rotor_reverse

          call do_plugs                 ; plug board

          movfw letter

          return


;**************************************************************************
; Enigma settings routines                                                *
;**************************************************************************

;--------------------------------------------------------------------------
; initialises the rotors and reflector
;--------------------------------------------------------------------------

          routine initialise_rotors

          movlf 0,rotor_L+0             ; default left rotor I
          clrf rotor_L+1
          clrf rotor_L+2
          clrf rotor_L+3
          clrf rotor_L+4

          movlf 1,rotor_C+0             ; default centre rotor II
          clrf rotor_C+1
          clrf rotor_C+2
          clrf rotor_C+3
          clrf rotor_C+4

          movlf 2,rotor_R+0             ; default right rotor III
          clrf rotor_R+1
          clrf rotor_R+2
          clrf rotor_R+3
          clrf rotor_R+4

          clrf reflector                ; default reflector B

          return


;--------------------------------------------------------------------------
; initialises the plug board
;--------------------------------------------------------------------------

          routine initialise_plugs

          movlf plugs,FSR               ; clear plug board
          clrf work1
initp1    movff work1,0
          incf FSR
          incf work1
          movlw d'26'
          subwf work1,w
          bnz initp1

          return


;--------------------------------------------------------------------------
; resets the rotors
;--------------------------------------------------------------------------

          routine reset_rotors

          movfw rotor_L+2               ; reset rotor positions
          movwf rotor_L+3
          movwf rotor_L+4

          movfw rotor_C+2
          movwf rotor_C+3
          movwf rotor_C+4

          movfw rotor_R+2
          movwf rotor_R+3
          movwf rotor_R+4

          call flash_display            ; acknowledge

          return


;--------------------------------------------------------------------------
; shows a setting, fed with the setting in w reg
;--------------------------------------------------------------------------

          routine show_setting

          call show_character
          movlw SHOW_DELAY
          call display

          call clear_display
          movlw BLANK_DELAY
          call display

          return


          routine show_digit

          addlw CHAR_0+1
          goto show_setting


          routine show_letter

          addlw CHAR_A
          goto show_setting


          routine show_error

          movlw CHAR_ERROR
          goto show_setting


;--------------------------------------------------------------------------
; shows the current reflector
;--------------------------------------------------------------------------

          routine show_reflector

          incf reflector,w              ; 'B' or 'C'
          call show_letter

          return


;--------------------------------------------------------------------------
; shows the current rotors
;--------------------------------------------------------------------------

          routine show_rotors

          movfw rotor_L+0               ; left - '1' to '8'
          call show_digit

          movfw rotor_C+0               ; centre - '1' to '8'
          call show_digit

          movfw rotor_R+0               ; right - '1' to '8'
          call show_digit

          return


;--------------------------------------------------------------------------
; shows the current ring settings
;--------------------------------------------------------------------------

          routine show_rings

          movfw rotor_L+1               ; left - 'A' to 'Z'
          call show_letter

          movfw rotor_C+1               ; centre - 'A' to 'Z'
          call show_letter

          movfw rotor_R+1               ; right - 'A' to 'Z'
          call show_letter

          return


;--------------------------------------------------------------------------
; shows the rotor start positions
;--------------------------------------------------------------------------

          routine show_starts

          movfw rotor_L+2               ; left - 'A' to 'Z'
          call show_letter

          movfw rotor_C+2               ; centre - 'A' to 'Z'
          call show_letter

          movfw rotor_R+2               ; right - 'A' to 'Z'
          call show_letter

          return


;--------------------------------------------------------------------------
; shows the current rotor positions
;--------------------------------------------------------------------------

          routine show_positions

          movfw rotor_L+3               ; left - 'A' to 'Z'
          call show_letter

          movfw rotor_C+3               ; centre - 'A' to 'Z'
          call show_letter

          movfw rotor_R+3               ; right - 'A' to 'Z'
          call show_letter

          return


;--------------------------------------------------------------------------
; shows the current plug board settings
;--------------------------------------------------------------------------

          routine show_plugs

          movlf plugs,pnt
          clrf letter

showp1    movff pnt,FSR                 ; plug in socket ?
          movfw 0
          subwf letter,w
          bz showp2                     ; branch if not

          bc showp2                     ; display each pair once only

          movfw letter                  ; 'A' to 'Z'
          call show_letter

          movff pnt,FSR
          movfw 0
          call show_letter

          movlw SHOW_DELAY
          call display

showp2    incf pnt
          incf letter

          movlw d'26'                   ; up to 13 pairs
          subwf letter,w
          bnz showp1

          return


;--------------------------------------------------------------------------
; gets a setting, returns the setting in w reg, the C flag set if ENTER
; pressed, or the Z flag set if error
;--------------------------------------------------------------------------

          routine get_setting

          movlw CHAR_QUERY              ; prompt for input
          call show_character

          call release_keyboard         ; enable keyboard

          clrf repeat                   ; wait for key press
gets1     movlw d'2'
          call display
          tstf key
          bnz gets2
          decfsz repeat
          goto gets1

          call inhibit_keyboard         ; disable keyboard

          goto gets5                    ; timeout

gets2     call inhibit_keyboard         ; disable keyboard

          movfw key

          clrf key                      ; clear key

          call convert_key              ; convert scan code to ASCII
          bz gets4                      ; branch if not found

          xorlw CR                      ; ENTER pressed ?
          bz gets3                      ; branch if yes
          xorlw CR

          movwf work1                   ; check in range
          movfw setting+0
          subwf work1,w
          bnc gets4
          incf setting+1,w
          subwf work1,w
          bc gets4                      ; branch if not

          movfw setting+0
          subwf work1,w

          clrz                          ; signal ok
          clrc

          return

gets3     call clear_display

          clrw

          clrz                          ; signal ENTER
          setc

          return

gets4     call show_error               ; error

gets5     call clear_display

          clrw

          setz                          ; signal error
          clrc

          return


;--------------------------------------------------------------------------
; displays a setting, fed with the setting in w reg
;--------------------------------------------------------------------------

          routine display_setting

          call show_character

          call release_keyboard         ; enable keyboard

          movlf SHOW_DELAY,repeat
disps1    movlw 1
          call display
          tstf key                      ; key pressed ?
          bnz disps2                    ; branch if yes
          decfsz repeat
          goto disps1

disps2    call clear_display

          movlf BLANK_DELAY,repeat
disps3    movlw 1
          call display
          tstf key                      ; key pressed ?
          bnz disps4                    ; branch if yes
          decfsz repeat
          goto disps3

disps4    call inhibit_keyboard         ; disable keyboard

          return


          routine display_digit

          addlw CHAR_0+1
          goto display_setting


          routine display_letter

          addlw CHAR_A
          goto display_setting


;--------------------------------------------------------------------------
; sets the current reflector
;--------------------------------------------------------------------------

          routine set_reflector

          movlf 'B',setting+0
          movlf 'C',setting+1

setre1    call get_setting              ; get setting
          bz setre2                     ; branch if error
          bc setre1                     ; branch if ENTER

          movwf reflector               ; change setting
          addlw 'B'-'A'
          call display_letter           ; display new setting

setre2    return


;--------------------------------------------------------------------------
; sets the current rotors
;--------------------------------------------------------------------------

          routine set_rotors

          movlf '1',setting+0
          movlf '8',setting+1

setro1    call get_setting              ; get setting
          bz setro5                     ; branch if error
          bc setro1                     ; branch if ENTER

          movwf rotor_L+0               ; change setting
          call display_digit            ; display new setting

setro2    call get_setting              ; get setting
          bz setro5                     ; branch if error
          bc setro2                     ; branch if ENTER

          xorwf rotor_L+0,w             ; can't use same rotor twice
          bz setro4
          xorwf rotor_L+0,w

          movwf rotor_C+0               ; change setting
          call display_digit            ; display new setting

setro3    call get_setting              ; get setting
          bz setro5                     ; branch if error
          bc setro3                     ; branch if ENTER

          xorwf rotor_L+0,w             ; can't use same rotor twice
          bz setro4
          xorwf rotor_L+0,w
          xorwf rotor_C+0,w
          bz setro4
          xorwf rotor_C+0,w

          movwf rotor_R+0               ; change setting
          call display_digit            ; display new setting

          goto setro5

setro4    call show_error               ; error

setro5    return


;--------------------------------------------------------------------------
; sets the current rings
;--------------------------------------------------------------------------

          routine set_rings

          movlf 'A',setting+0
          movlf 'Z',setting+1

setri1    call get_setting              ; get setting
          bz setri4                     ; branch if error
          bc setri1                     ; branch if ENTER

          movwf rotor_L+1               ; change setting
          call display_letter           ; display new setting

setri2    call get_setting              ; get setting
          bz setri4                     ; branch if error
          bc setri2                     ; branch if ENTER

          movwf rotor_C+1               ; change setting
          call display_letter           ; display new setting

setri3    call get_setting              ; get setting
          bz setri4                     ; branch if error
          bc setri3                     ; branch if ENTER

          movwf rotor_R+1               ; change setting
          call display_letter           ; display new setting

setri4    return


;--------------------------------------------------------------------------
; sets the rotor start positions
;--------------------------------------------------------------------------

          routine set_starts

          movlf 'A',setting+0
          movlf 'Z',setting+1

setst1    call get_setting              ; get setting
          bz setst4                     ; branch if error
          bc setst1                     ; branch if ENTER

          movwf rotor_L+2               ; change setting
          movwf rotor_L+3
          movwf rotor_L+4
          call display_letter           ; display new setting

setst2    call get_setting              ; get setting
          bz setst4                     ; branch if error
          bc setst2                     ; branch if ENTER

          movwf rotor_C+2               ; change setting
          movwf rotor_C+3
          movwf rotor_C+4
          call display_letter           ; display new setting

setst3    call get_setting              ; get setting
          bz setst4                     ; branch if error
          bc setst3                     ; branch if ENTER

          movwf rotor_R+2               ; change setting
          movwf rotor_R+3
          movwf rotor_R+4
          call display_letter           ; display new setting

setst4    return


;--------------------------------------------------------------------------
; sets the current plug board
;--------------------------------------------------------------------------

          routine set_plugs

          movlf 'A',setting+0
          movlf 'Z',setting+1

          call initialise_plugs         ; clear plug board

          clrf pairs

setpl1    call get_setting              ; get setting
          bz setpl3                     ; branch if error
          bc setpl3                     ; branch if ENTER
          movwf letter1

          movfw letter1                 ; display 1st letter in pair
          call display_letter

          call get_setting              ; get setting
          bz setpl3                     ; branch if error
          bc setpl3                     ; branch if ENTER
          movwf letter2

          subwf letter1,w               ; can't connect to itself
          bz setpl2

          movfw letter1                 ; check not already used
          addlw plugs
          movwf FSR
          movfw 0
          subwf letter1,w
          bnz setpl2

          movfw letter2
          addlw plugs
          movwf FSR
          movfw 0
          subwf letter2,w
          bnz setpl2

          movfw letter1                 ; connect letters
          addlw plugs
          movwf FSR
          movff letter2,0

          movfw letter2
          addlw plugs
          movwf FSR
          movff letter1,0

          movfw letter2                 ; display 2nd letter in pair
          call display_letter

          incf pairs                    ; up to 13 pairs
          movlw d'13'
          subwf pairs,w
          bnz setpl1

          goto setpl3

setpl2    call show_error               ; error

setpl3    return


;--------------------------------------------------------------------------
; displays the help message
;--------------------------------------------------------------------------

          routine display_help

          call release_keyboard         ; enable keyboard

          clrf key

          movlw help-h'2100'
          call display_message

          call inhibit_keyboard         ; disable keyboard

          clrf key

          return


;**************************************************************************
; Executive routines                                                      *
;**************************************************************************

;--------------------------------------------------------------------------
; main entry point
;--------------------------------------------------------------------------

          routine main_entry

          clrf PORTA                    ; initialise ports
          movlw PORTA_IO
          tris_A
          clrf PORTB
          movlw PORTB_IO
          tris_B

          movlw PRESCALE
          option_

          bcf INTCON,GIE

          movlf h'07',CMCON             ; disable comparators

          movfw STATUS                  ; power-up ?
          andlw 1<<NOT_PD
          bz main3                      ; branch if not

          clrf flags                    ; clear flags

          clrf key                      ; no key pressed

          clrf ignore                   ; don't ignore scan codes

          call initialise_keyboard      ; initialise the keyboard
          bz main4                      ; branch if error

          call flash_display            ; test display LEDs

          movlw SHOW_DELAY
          call display

          clrf key

          movlf d'20',repeat            ; display sign-on message
main1     movlw signon-h'2100'
          call display_message
          tstf key                      ; key pressed ?
          bnz main2                     ; branch if yes
          movlw SHOW_DELAY*2
          call display
          decfsz repeat
          goto main1

main2     clrf key

          call initialise_rotors        ; initialise the rotors etc.
          call initialise_plugs

main3     goto main_loop

main4     call flash_display            ; test display LEDs

main5     movlw CHAR_ERROR              ; signal keyboard error
          call show_character
          movlw 1
          call display
          goto main5


;--------------------------------------------------------------------------
; main loop
;--------------------------------------------------------------------------

          routine main_loop

          movlw PORTA_IO                ; re-initialise ports
          tris_A
          movlw PORTB_IO
          tris_B

          movlw PRESCALE
          option_

          bcf INTCON,GIE

          movlf h'07',CMCON

          call clear_display

loop1     call release_keyboard         ; enable keyboard

          clrf repeat                   ; wait for key press
loop2     movlw d'5'
          call display
          tstf key
          bnz loop3
          decfsz repeat
          goto loop2
          call clear_display
          goto loop2

loop3     call inhibit_keyboard         ; disable keyboard

          movlf high vectors,PCLATH

          clrf work1

loop4     movfw work1
          index vectors
          incf work1

          tstw                          ; end of table ?
          bz loop9                      ; branch if yes

          subwf key,w                   ; scan code found ?
          bnz loop7                     ; branch if not

          movfw work1                   ; get shift status
          index vectors
          incf work1

          tstw
          bnz loop5

          movfw flags                   ; shift key must not be pressed
          andlw SHIFT_MASK
          bnz loop8

          goto loop6

loop5     andlw h'80'
          bnz loop6

          movfw flags                   ; shift key must be pressed
          andlw SHIFT_MASK
          bz loop8

loop6     clrf key                      ; clear key

          movfw work1
          call vectors

          goto main_loop

loop7     incf work1
loop8     incf work1

          goto loop4

loop9     movfw key

          clrf key                      ; clear key

          call convert_key              ; convert scan code to ASCII
          bz main_loop                  ; branch if not found

          addlw -'A'                    ; must be 'A' to 'Z'
          movwf work1
          movlw d'26'
          subwf work1,w
          bc main_loop                  ; branch if not
          movfw work1

          call encode_letter            ; encode letter

          addlw CHAR_A                  ; display encoded letter
          call show_character

          goto loop1


;**************************************************************************
; EEPROM data                                                             *
;**************************************************************************

          org h'2100'

; sign-on message
signon    de "ENIGMA",0

; help message
help      de "F2=ROTORS F3=RINGS F4=START F5=CURRENT F6=REFLECTOR F7=PLUGS",0


          end
