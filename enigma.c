#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define FORWARD 1
#define BACKWARD 2

char r1[27][2],r2[27][2],r3[27][2],ref[14][2],plug[27][2];
char map1[]="QWERTYUIOPASDFGHJKLZXCVBNM";
char map2[]="MLPOKIJNBHUYGVCFTRDXSEWQAZ";
char map3[]="LKJHGFDSAMPOIUNBYTVCREXZWQ";
char plugmap[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char refmap[]="NOPQRSTUVWXYZ";

int index1,index2,index3;

void init(){
	int i,j;
	for(i=0;i<27;i++){
		r1[i][0]=(char)(((int)'A')+(i));
		r2[i][0]=(char)(((int)'A')+(i));
		r3[i][0]=(char)(((int)'A')+(i));

		r1[i][1]=map1[i];
		r2[i][1]=map2[i];
		r3[i][1]=map3[i];

		if(i<14){
			ref[i][0]=(char)(((int)'A')+(i));
			ref[i][1]=refmap[i-1];
		}

		plug[i][0]=(char)(((int)'A')+(i));
		plug[i][1]=plugmap[i];
	}
	index3=index2=index1=0;
}

int findInArray(char c, char arr[], int n){
	int i;
	for(i=0;i<n;i++){
		if(arr[i]==c)
			return i;
	}
	return -1;
}

char passThroughRotors(char ch, int direction){
	int in,i;
	char c[27];
	if(direction==FORWARD){
		for(i=0;i<27;i++)
			c[i]=r1[i][0];
		in=findInArray(ch,c,27);
		for(i=0;i<27;i++)
			c[i]=r2[i][0];
		in=findInArray(r1[in][1],c,27);
		for(i=0;i<27;i++)
			c[i]=r3[i][0];
		in=findInArray(r2[in][1],c,27);
		
		return r3[in][1];
	}
	else{
		for(i=0;i<27;i++)
			c[i]=r3[i][1];
		in=findInArray(ch,c,27);
		for(i=0;i<27;i++)
			c[i]=r2[i][1];
		in=findInArray(r1[in][0],c,27);
		for(i=0;i<27;i++)
			c[i]=r1[i][1];
		in=findInArray(r2[in][0],c,27);
		
		return r1[in][0];
	}
}

char passThroughReflectors(char ch){
	char c[14];
	int i,in;
	for(i=0;i<14;i++)
		c[i]=ref[i][0];
	in=findInArray(ch,c,14);
	if(in==-1){
		for(i=0;i<14;i++)
			c[i]=ref[i][1];
		in=findInArray(ch,c,14);
		return ref[in][0];
	}
	else{
		return ref[in][1];
	}
}

char passThroughPlugBoard(char ch,int direction){
	int i;
	char c[27];
	if(direction==FORWARD){
		for(i=0;i<27;i++)
			c[i]=plug[i][0];
		return(plug[findInArray(ch,c,27)][1]);
	}
	else{
		for(i=0;i<27;i++)
			c[i]=plug[i][1];
		return(plug[findInArray(ch,c,27)][0]);
	}

}

void rotateRotors(){
	char c0,c1;
	int i;
	c0=r1[0][0];
	c1=r1[0][1];
	for(i=0;i<25;i++){
		r1[i][0]=r1[i+1][0];
	}
	r1[24][0]=c0;
	r1[24][1]=c1;
	index1++;
	if(index1==26){
		index1=0;
		c0=r2[0][0];
		c1=r2[0][1];
		for(i=0;i<25;i++){
			r2[i][0]=r2[i+1][0];
		}
		r2[24][0]=c0;
		r2[24][1]=c1;
		index2++;
		if(index2==26){
			index2=0;
			c0=r3[0][0];
			c1=r3[0][1];
			for(i=0;i<25;i++){
				r3[i][0]=r3[i+1][0];
			}
			r3[24][0]=c0;
			r3[24][1]=c1;
			index3++;
		}
	}
}

// void main(){
//   char c;
//   init();
//   scanf("%c",&c);
//   c=toupper(c);
//   while(c!=' '){
//     c=passThroughPlugBoard(c,FORWARD);
//     c=passThroughRotors(c,FORWARD);
//     c=passThroughReflectors(c);
//     c=passThroughRotors(c,BACKWARD);
//     c=passThroughPlugBoard(c,FORWARD);
//     printf("%c",c);
//     rotateRotors();
//
//     scanf("%c",&c);
//     c=toupper(c);
//   }
//   printf("\n");
// }

int main(int argc, char **argv){  
  
  if(argc < 2){
    printf("usage: enigma <message>\nmessage: the message being transformed\n");
    return 1;
  }
  
  int len = strlen(argv[1]);
  
  printf("input size: %d\n", len);
  
  int i = -1;
  
  init();
  
  while(++i<len){
    char c;
    c = argv[1][i];
    c=toupper(c);
    c=passThroughPlugBoard(c,FORWARD);
    c=passThroughRotors(c,FORWARD);
    c=passThroughReflectors(c);
    c=passThroughRotors(c,BACKWARD);
    c=passThroughPlugBoard(c,BACKWARD);
    printf("%c",c);
    rotateRotors();
  }
  printf("\n");
  
  return 0;
}
