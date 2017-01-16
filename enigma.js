
// http://www.counton.org/explorer/codebreaking/enigma-cipher.php
// TODO traduzir isso aqui pra C

/******************************** Wheel(ORDER) :: Constructor ******************************/
function makeWheel(order)
 {
 this.letter_order = new Array(51);
 this.letters = new Array(51);
 this.enc_position = 0;
 this.dec_position = 0;
 for(i=0;i<order.length;i++)
  {
  this.letter_order[i] = charToNumber(order.charAt(i));
  this.letters[i] = order.charAt(i);
  }
 }

var key_order = "abcdefghijklmnopqrstuvwxyz0123456789!\"$\'()+\\,-./?@ "
//Gets the number of the letter ie. a=0,b=1,c=2 etc
function charToNumber(c)
 {
 var v = -1;
 v = key_order.indexOf(c)
 return v;
 }
//Gets the letter of the number ie. 0=a,1=b,2=c etc
function numberToChar(b)
 {
 var a='*';
 a = key_order.charAt(b)
 return a;
}

/******************************** Encryption and Decryption ******************************/

var no_debug = 1

//The Wheels
var wheel = new Array(4);
wheel[0] = new makeWheel("bcagdefhilkjomnrqpu vstwzyx.94/3,20!\\?@81\"5'+$(6)-7");
wheel[1] = new makeWheel("chtzwefdbyiqljuvskgaxorpnm\"6-(1$873,04 /.!25'\\+?)9@");
wheel[2] = new makeWheel("x6pr8g7+2!n0$dw\\z?@4lhya5mo.v)9-,1 (3sqiu'etb\"jcfk/");
wheel[3] = new makeWheel("j\"kbcefpl?/,v6gw(2!0o.5yamh1 -7r3s8x)9u$i+t\\z'qdn4@");
var total_wheels = 4

var letter_order = new Array(51);
var l=0;
var letter_limit = 51;

//Encrytes the Message given as parameter
function Encryptor(input_text)
 {
 getInputs()

 input_text = input_text.toLowerCase(); //Convert to lower case
 l = input_text.length;

 var c="";
 var output_text = "";
 for(i=0;i<l;i++)
  {
  c = input_text.charAt(i);
  letters_number = charToNumber(c); //Get the numarical value
  pos = letters_number;

  if(letters_number==-1) c='*';	//If the inputed char is not a alphabet
  else
   {
   	  					 	   if(!no_debug) info(2,"Char:"+c+",No:"+fill(letters_number)+"->")
   //To this for all the wheels
   for(k=total_wheels-1;k>=0;k--)
    {
	pos = pos + wheel[k].enc_position; //Turns the wheel to its 'enc_position'
    if(pos >= letter_limit) pos = pos - letter_limit; //Makes the corrections. The wheel is a circle. So if it is after 25 it must be corrected
	if(k > 0) pos = wheel[k].letter_order[pos]; //Finds the number at 'pos' and give it for the next wheel. This is needed for all wheel execpt the last (0'th) Wheel
	 						   if(!no_debug) info(2,"|"+fill(pos)+"("+fill(wheel[k].enc_position)+"-"+wheel[k].letters[pos]+") in W "+k);
	}

   c = numberToChar(wheel[0].letter_order[pos]);
  								if(!no_debug) info(0," -> "+c);
   }
  wheel[0].enc_position++; //Turns the wheel one time

  for(k=0;k<total_wheels;k++)
   {
   if(wheel[k].enc_position>letter_limit) //One full turn of any wheel is completed
    {
    wheel[k].enc_position=0;
	if(k+1 != total_wheels) wheel[k+1].enc_position++; //Do this if the current wheel is not the last wheel
    }
   }

  output_text = output_text + c; //Get the final result, letter by letter
  }
   			  				  	if(!no_debug) info(0,"The Encrypted text is \""+output_text+"\"\n");
 return output_text
 }

//Decrypts the Message
function Decryptor(input_text)
 {
 getInputs()

 input_text = input_text.toLowerCase();
 l = input_text.length;

 var c=' ';
 var output_text = "";

 for(j=0;j<l;j++)
  {
  c = input_text.charAt(j);

  var decrypt = ' ';
  var to = -1;
  var from = charToNumber(c); //Gets the 'from' of the encrypted charecter
  var ch = from
   	  					 	   if(!no_debug) info(2,"Char:"+c+",No:"+fill(from)+"->")
  for(k=0;k<total_wheels;k++)
   {
   for(i=0;i<letter_limit;i++) //Go thru every letter in the wheel,
    if(wheel[k].letter_order[i] == ch) {ch = i;break;} //Searching the char 'c'(as a number) in the 'letter_order'. If found, the position is sorted in 'ch'.

   ch = ch - wheel[k].dec_position;	//Adjusts the wheel position
   if(ch < 0) ch = ch + letter_limit; //If the given 'to' is before a, find its original char from end
   		   	  	   	  		   if(!no_debug) info(2,"|"+fill(ch)+"("+fill(wheel[k].dec_position)+"-"+wheel[k].letters[ch]+") in W "+k);
   }
  to=ch
  wheel[0].dec_position++; //Turns the wheel one step

  decrypt=numberToChar(to); //Find the letter in 'to'
  if(c=='*') decrypt = '*'; //If the inputed text is a non-recoganizable charector, output '*'
 	 	  	   			 	   if(!no_debug) info(0," -> "+decrypt);
  for(k=0;k<total_wheels;k++)
   {
   if(wheel[k].dec_position > letter_limit)	//If dec_position exeeds the limit, start at top
    {
    wheel[k].dec_position=0;
    if(k+1 != total_wheels) wheel[k+1].dec_position++; //Do this if the current wheel is not the last wheel
    }
   }
  output_text = output_text + decrypt;
  }
   			  				  if(!no_debug) info(0,"The Decrypted Text is \""+output_text+"\"");
 return output_text;
 }

/************ getInputs() :: Find the user set variables from forms ************/
function getInputs()
 {
 //Wheel starts at given positions
 wheel[0].enc_position = validate(f.one.value);
 wheel[1].enc_position = validate(f.two.value);
 wheel[2].enc_position = validate(f.three.value);
 wheel[3].enc_position = validate(f.four.value);

 wheel[0].dec_position = validate(f.one.value);
 wheel[1].dec_position = validate(f.two.value);
 wheel[2].dec_position = validate(f.three.value);
 wheel[3].dec_position = validate(f.four.value);

 if(f.debug.checked) no_debug=0;
 else no_debug=1;
 }
function validate(no)
 {
 no=no.toLowerCase()
 no = charToNumber(no)
 if(no > letter_limit-1) no = letter_limit-1;
 else if(no < 0) no = 0;
 return no
 }

/************ info() :: Displays the debuging info ************/
function info(cls,text)
 {
 if(cls==1) txt.value = ""
 else if(cls==0) txt.value = txt.value + text + "\n"
 else if(cls==2) txt.value = txt.value + text
 }
/************ fill() :: Fills spaces in numbers ************/
//"2" becomes "2 " - done for making order in the debug textarea
function fill(no)
 {
 if(no<10) no=no+" "
 return no}