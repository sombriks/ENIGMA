#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

const char *key_order = "abcdefghijklmnopqrstuvwxyz0123456789!\"$\'()+\\,-./?@ ";
const int letter_limit = 51;

int no_debug = 0;

struct wheel {
  int letter_order[51];
  char letters[51];
  int enc_position;
  int dec_position;
};

int charToNumber(char c){
  int v = -1;
  int i = letter_limit;
  while(i--)
    if(c == key_order[i])
      v = i;
    return v;
}

struct wheel makeWheel(char *order,int len){
  struct wheel wh;
  for(int i = 0 ; i < len ; i++){
    wh.letter_order[i] = charToNumber(order[i]);
    wh.letters[i] = order[i];
  }  
  return wh;
}

struct wheel wheels[4];

const int total_wheels = 4;

struct form {
  char one;
  char two;
  char three;
  char four;
  int debug;
};

struct form f;

int validate(char no) {
  no = tolower(no); // ctype.h
  int nu = charToNumber(no);
  if (nu > letter_limit - 1) nu = letter_limit - 1;
  else if (nu < 0) nu = 0;
  return nu;
}

/************ getInputs() :: Find the user set variables from forms ************/
void getInputs() {
  //Wheel starts at given positions
  wheels[0].enc_position = validate(f.one);
  wheels[1].enc_position = validate(f.two);
  wheels[2].enc_position = validate(f.three);
  wheels[3].enc_position = validate(f.four);
  
  wheels[0].dec_position = validate(f.one);
  wheels[1].dec_position = validate(f.two);
  wheels[2].dec_position = validate(f.three);
  wheels[3].dec_position = validate(f.four);
  
  if (f.debug) no_debug = 0;
  else no_debug = 1;
}

char *toLowerCase(char *input, int len) {
  
  char *ret = malloc(len * sizeof(char));
  
  while(len--){
    ret[len] = tolower(input[len]);
  }
  
  return ret;
}

struct txt {
  char *value;
};

struct txt enigmalog; 

void info(int op, char *msg) {
  // TODO implement 
}

char numberToChar(int b) {
  char a = '*';
  if(b >= 0 && b < letter_limit -1)
    a = key_order[b];
  return a;
}

//Encrytes the Message given as parameter
char *Encryptor(char *raw_input_text,int input_len) {
    
  // setup wheels 
  getInputs();
    
  // input_text = input_text.toLowerCase(); //Convert to lower case
  char *input_text = toLowerCase(input_text,input_len);
  int l = input_len;
    
  char c;
  char *output_text = malloc(input_len * sizeof(char));
  
  for (int i = 0; i < l; i++) {
    c = input_text[i];
    int letters_number = charToNumber(c); //Get the numarical value
    int pos = letters_number;
    
    if (letters_number == -1) c = '*'; //If the inputed char is not presente in the alphabet
    else {
      if (!no_debug) {
        char buf[14];
        sprintf(buf,"Char: %c,No:%d ->",c,letters_number);
        info(2, buf);// "Char:" + c + ",No:" + fill(letters_number) + "->");
      }//To this for all the wheels
      for (int k = total_wheels - 1; k >= 0; k--) {
        pos = pos + wheels[k].enc_position; //Turns the wheel to its 'enc_position'
        if (pos >= letter_limit) pos = pos - letter_limit; //Makes the corrections. The wheel is a circle. So if it is after 25 it must be corrected
        if (k > 0) pos = wheels[k].letter_order[pos]; //Finds the number at 'pos' and give it for the next wheel. This is needed for all wheel execpt the last (0'th) Wheel
        if (!no_debug) {
          char buf[25];
          sprintf(buf,"| %d (%d - %c) in W %d",pos,wheels[k].enc_position,wheels[k].letters[pos],k);
          info(2,buf);
//           info(2, "|" + fill(pos) + "(" + fill(wheels[k].enc_position) + "-" + wheels[k].letters[pos] + ") in W " + k);
        }
      }      
      c = numberToChar(wheels[0].letter_order[pos]);
      if (!no_debug) {
        char buf[5];
        sprintf(buf," -> %c",c);
        info(0, buf);
      }
    }
    wheels[0].enc_position++; //Turns the wheel one time
    
    for (int k = 0; k < total_wheels; k++) {
      if (wheels[k].enc_position > letter_limit) //One full turn of any wheel is completed
      {
        wheels[k].enc_position = 0;
        if (k + 1 != total_wheels) wheels[k + 1].enc_position++; //Do this if the current wheel is not the last wheel
      }
    }
    
//     output_text = output_text + c; //Get the final result, letter by letter
    output_text[i]=c;
  }
  if (!no_debug) {
    char buf[100];
    sprintf(buf,"The Encrypted text is \"%s\"\n",output_text);
    info(0, buf);
  }
  return output_text;
}

//Decrypts the Message
char *Decryptor(char *raw_input_text,int input_len) {
  
  // setup wheels 
  getInputs();
  
  char *input_text = toLowerCase(raw_input_text,input_len);
  int l = input_len;
  
  char c;
  char *output_text = malloc(input_len * sizeof(char));
  
  for (int j = 0; j < l; j++) {
    c = input_text[j];
    
    char decrypt = ' ';
    int to = -1;
    int from = charToNumber(c); //Gets the 'from' of the encrypted charecter
    int ch = from;
    if (!no_debug) {      
      char buf[14];
      sprintf(buf,"Char: %c,No:%d ->",c,from);
      info(2, buf);// info(2, "Char:" + c + ",No:" + fill(from) + "->")
    }
    for (int k = 0; k < total_wheels; k++) {
      for (int i = 0; i < letter_limit; i++) //Go thru every letter in the wheel,
        if (wheels[k].letter_order[i] == ch) { ch = i; break; } //Searching the char 'c'(as a number) in the 'letter_order'. If found, the position is sorted in 'ch'.
        
        ch = ch - wheels[k].dec_position;	//Adjusts the wheel position
        if (ch < 0) ch = ch + letter_limit; //If the given 'to' is before a, find its original char from end
        if (!no_debug) {
//           info(2, "|" + fill(ch) + "(" + fill(wheels[k].dec_position) + "-" + wheels[k].letters[ch] + ") in W " + k);
          char buf[25];
          sprintf(buf,"| %d (%d - %c) in W %d",ch,wheels[k].dec_position,wheels[k].letters[ch],k);
          info(2,buf);
          
        }
    }
    to = ch;
    wheels[0].dec_position++; //Turns the wheel one step
    
    decrypt = numberToChar(to); //Find the letter in 'to'
    if (c == '*') decrypt = '*'; //If the inputed text is a non-recoganizable charector, output '*'
    if (!no_debug) {
      //       info(0, " -> " + decrypt);
      char buf[5];
      sprintf(buf," -> %c",decrypt);
      info(0, buf);      
    }
    for (int k = 0; k < total_wheels; k++) {
      if (wheels[k].dec_position > letter_limit)	//If dec_position exeeds the limit, start at top
      {
        wheels[k].dec_position = 0;
        if (k + 1 != total_wheels) wheels[k + 1].dec_position++; //Do this if the current wheel is not the last wheel
      }
    }
    output_text[j] = decrypt;
  }
  if (!no_debug) {
//     info(0, "The Decrypted Text is \"" + output_text + "\"");
//     console.log(txt.value);
//     txt.value = "";
    char buf[100];
    sprintf(buf,"The Decrypted text is \"%s\"\n",output_text);
    info(0, buf);
  }
  return output_text;
}

int main(int argc, char **argv) {
  
  // set up wheels
  wheels[0] = makeWheel("bcagdefhilkjomnrqpu vstwzyx.94/3,20!\\?@81\"5'+$(6)-7",letter_limit);
  wheels[1] = makeWheel("chtzwefdbyiqljuvskgaxorpnm\"6-(1$873,04 /.!25'\\+?)9@",letter_limit);
  wheels[2] = makeWheel("x6pr8g7+2!n0$dw\\z?@4lhya5mo.v)9-,1 (3sqiu'etb\"jcfk/",letter_limit);
  wheels[3] = makeWheel("j\"kbcefpl?/,v6gw(2!0o.5yamh1 -7r3s8x)9u$i+t\\z'qdn4@",letter_limit);
  
  // teste TODO implementar a interface de linha de comando que nem na versão do nodejs
  
  f.one = 'A';
  f.two = 'A';
  f.three = 'A';
  f.four = 'A';
  f.debug = 1;
  
  
  char *in = strdup("xUxa");// tem que dar dup porque a string é const
  char *out = Encryptor(in,4);
  
  printf("Encrypt:\n< %s\n> %s\n",in,out);
  
  in = strdup("@u14");
  out = Decryptor(in,4);
  
  printf("Decrypt:\n< %s\n> %s\n",in,out);
  
  return 0;
}
