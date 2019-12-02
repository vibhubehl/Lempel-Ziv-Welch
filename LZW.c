#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<ctype.h>

#define TRUE 1
#define FALSE 0

#define DICTSIZE 4096                     /* allow 4096 entries in the dict  */
#define ENTRYSIZE 32
void file_checker(char* filename);
int current=0;

unsigned char dict[DICTSIZE][ENTRYSIZE];  /* of 30 chars max; the first byte */
                                          /* is string length; index 0xFFF   */
                                          /* will be reserved for padding    */
                          
                                          /* the last byte (if necessary)    */

// These are provided below
int read12(FILE *infil);
int write12(FILE *outfil, int int12);
void strip_lzw_ext(char *fname);
void flush12(FILE *outfil);


void print_str(unsigned char w[]){
    int i;
    for( i=1;i<=w[0]; i++){
        printf("%c",w[i]);
    }
    printf("\n");
}

void print_table(){
    int i=0;
    int j=0;
    for(i=0;i<current;i++){
        printf("\n %d| %d ",i, (dict[i][0]));
        for(j=0;j<dict[i][0];j++){
            printf("%c",dict[i][j+1]);
        }
    }
}

void intitial(){
    unsigned char n=0;
    int i;
    for(i=0;i<256;i++,n++,current++){
        dict[i][0]=1;
        dict[i][1]=n;
    }
    //print_table();
}


void file_checker(char* filename){//checks whether file exists or not

    FILE *ifp= fopen(filename,"r");
    if(ifp==NULL){
        printf("no filename specified:");
        exit(2);
    }
    
}


int finder_index(unsigned char value[]){//finds the index for a particular value in dict
    int n=0;
    int flag;
    int len = value[0];
   
    for(n=0; n<current; n++){// looping dict to find value
    
        int j;
        flag=0;
    
        for(j=0; j<=len;j++){
            if(value[j]!=dict[n][j]){
                flag=-1;
                break;
            }
        }

        if(flag==0){//found
            return n;
        }
    }
    return -1;//not found
}


void add_dict(unsigned char nentry[]){//adds new char to dictionary
    //static int n=256;
    int len=nentry[0];
    if(current==4095){

        return;
    }
    int i=0;
    for(i=0; i<=len; i++){
        dict[current][i]=nentry[i];//adding new entry char by char
    }
    printf("%d-",current);
    for(i=1; i<=dict[current][0]; i++){
        printf("%d ", dict[current][i]);//adding new entry char by char
    }
    printf("\n");
    current++;
}

/*adds kth index of dict to w*/
void add_str(unsigned char w[], int k){
    int len=dict[k][0];
    int i;
    for(i=0;i<=ENTRYSIZE-1;i++){
        w[i]=dict[k][i];;
    }
}

/*****************************************************************************/
/* encode() performs the Lempel Ziv Welch compression from the algorithm in  */
/* the assignment specification. The strings in the dictionary have to be    */
/* handled carefully since 0 may be a valid character in a string (we can't  */
/* use the standard C string handling functions, since they will interpret   */
/* the 0 as the end of string marker). Again, writing the codes is handled   */
/* by a separate function, just so I don't have to worry about writing 12    */
/* bit numbers inside this algorithm.                                        */
void encode(FILE *in, FILE *out) {
    // TODO implemen
    unsigned char input;
    int index;
    unsigned char w[ENTRYSIZE];
    w[0]=0;

    while(fread(&input, 1, 1, in)){

        if(w[0]>=ENTRYSIZE-1){//condition overflow
            index=finder_index(w);//finding w in dictionary //!
            write12(out,index);//putting index in file
            w[0]=0;//emptying w
        }
     
        w[0]++;
        w[w[0]]=input;//copy temp to w
        index=finder_index(w);//finding w in dictionary 


        if(index==-1){//ww not found in dictionay
            add_dict(w);//adding w
            w[0]--;
            index=finder_index(w);
            w[0]++;
            write12(out,index);
            w[0]=1;
            w[1]=input;
            
        }
    
    }

    index=finder_index(w);//finding w in dictionary //!
    write12(out,index);//putting index in file
    flush12(out);
    return;
    
}


/*****************************************************************************/
/* decode() performs the Lempel Ziv Welch decompression from the algorithm   */
/* in the assignment specification.                                          */
void decode(FILE *in, FILE *out) {
    // TODO implement
    unsigned char w[ENTRYSIZE];

    int n=read12(in);//reading the input file

    add_str(w,n);//adding n'th index from dict to w

    int len1=w[0];//length of w
    int j=0;
    for(j=0;j<len1;j++){
        putc(w[j+1],out);
    }

    n=read12(in);
    while(n!=-1 && n!=4095){

        if(n<current){//if n exists in dict
            unsigned char temp[ENTRYSIZE];
            add_str(temp,n);
            int len=temp[0]; 
            int i;
            for(i=1;i<=len;i++){//pushing temp in output file
                putc(temp[i],out);
            }
             
            if(w[0]<ENTRYSIZE-1){// case where w is not overflowing
                w[0]++;
                w[w[0]]=dict[n][1];
                add_dict(w);
            }
        }
        else {//case-n doesn't exist in dict
            if(w[0]<ENTRYSIZE-1){//in case no overflow
                w[0]++;
                w[w[0]]=w[1];
                add_dict(w);
            }
            int len=w[0];
            int j=0;
            for(j=0;j<len;j++){//pushing w in output file
                putc(w[j+1],out);
            }
        }

        add_str(w,n);
        n=read12(in);//reading n from input
    
    }

}


/*****************************************************************************/
/* read12() handles the complexities of reading 12 bit numbers from a file.  */
/* It is the simple counterpart of write12(). Like write12(), read12() uses  */
/* static variables. The function reads two 12 bit numbers at a time, but    */
/* only returns one of them. It stores the second in a static variable to be */
/* returned the next time read12() is called.                                */
int read12(FILE *infil)
{
 static int number1 = -1, number2 = -1;
 unsigned char hi8, lo4hi4, lo8;
 int retval;

 if(number2 != -1)                        /* there is a stored number from   */
    {                                     /* last call to read12() so just   */
     retval = number2;                    /* return the number without doing */
     number2 = -1;                        /* any reading                     */
    }
 else                                     /* if there is no number stored    */
    {
     if(fread(&hi8, 1, 1, infil) != 1)    /* read three bytes (2 12 bit nums)*/
        return(-1);
     if(fread(&lo4hi4, 1, 1, infil) != 1)
        return(-1);
     if(fread(&lo8, 1, 1, infil) != 1)
        return(-1);

     number1 = hi8 * 0x10;                /* move hi8 4 bits left            */
     number1 = number1 + (lo4hi4 / 0x10); /* add hi 4 bits of middle byte    */

     number2 = (lo4hi4 % 0x10) * 0x0100;  /* move lo 4 bits of middle byte   */
                                          /* 8 bits to the left              */
     number2 = number2 + lo8;             /* add lo byte                     */

     retval = number1;
    }

 return(retval);
}

/*****************************************************************************/
/* write12() handles the complexities of writing 12 bit numbers to file so I */
/* don't have to mess up the LZW algorithm. It uses "static" variables. In a */
/* C function, if a variable is declared static, it remembers its value from */
/* one call to the next. You could use global variables to do the same thing */
/* but it wouldn't be quite as clean. Here's how the function works: it has  */
/* two static integers: number1 and number2 which are set to -1 if they do   */
/* not contain a number waiting to be written. When the function is called   */
/* with an integer to write, if there are no numbers already waiting to be   */
/* written, it simply stores the number in number1 and returns. If there is  */
/* a number waiting to be written, the function writes out the number that   */
/* is waiting and the new number as two 12 bit numbers (3 bytes total).      */
int write12(FILE *outfil, int int12)
{
 static int number1 = -1, number2 = -1;
 unsigned char hi8, lo4hi4, lo8;
 unsigned long bignum;

 if(number1 == -1)                         /* no numbers waiting             */
    {
     number1 = int12;                      /* save the number for next time  */
     return(0);                            /* actually wrote 0 bytes         */
    }

 if(int12 == -1)                           /* flush the last number and put  */
    number2 = 0x0FFF;                      /* padding at end                 */
 else
    number2 = int12;

 bignum = number1 * 0x1000;                /* move number1 12 bits left      */
 bignum = bignum + number2;                /* put number2 in lower 12 bits   */

 hi8 = (unsigned char) (bignum / 0x10000);                     /* bits 16-23 */
 lo4hi4 = (unsigned char) ((bignum % 0x10000) / 0x0100);       /* bits  8-15 */
 lo8 = (unsigned char) (bignum % 0x0100);                      /* bits  0-7  */

 fwrite(&hi8, 1, 1, outfil);               /* write the bytes one at a time  */
 fwrite(&lo4hi4, 1, 1, outfil);
 fwrite(&lo8, 1, 1, outfil);

 number1 = -1;                             /* no bytes waiting any more      */
 number2 = -1;

 return(3);                                /* wrote 3 bytes                  */
}


/** Write out the remaining partial codes */
void flush12(FILE *outfil)
{
 write12(outfil, -1);                      /* -1 tells write12() to write    */
}                                          /* the number in waiting          */


/** Remove the ".LZW" extension from a filename */
void strip_lzw_ext(char *fname)
{
    char *end = fname + strlen(fname);

    while (end > fname && *end != '.' && *end != '\\' && *end != '/') {
        --end;
    }
    if ((end > fname && *end == '.') &&
        (*(end - 1) != '\\' && *(end - 1) != '/')) {
        *end = '\0';
    }
}


void main(int argc, char* argv[]){

    char flagmode;
    char *ifilename;
    char *ofilename;
    flagmode=argv[2][0];
    ifilename=argv[1];
    intitial();

    if(argc<3){//if less than 3 arguements then break
        printf("Error: No input file specified!");
        exit(1);
    }
    if((flagmode!='e' && flagmode!='d')){//checking if flagmode has valid values
        printf("Usage, expected: RLE {filename} [e | d] to the console");
        exit(4);
    }


    //calling encode or decode according to user input
    if(flagmode=='e'){
        file_checker(ifilename);//checking whether file exists or not
        FILE *ifp= fopen(ifilename,"rb");//reads file
        ofilename=strcat(ifilename,".LZW");
        FILE *ofp= fopen(ofilename,"wb");//write file
        encode(ifp,ofp);
        fclose(ifp);//ifp closed
        fclose(ofp);
    }

    if(flagmode=='d'){
        file_checker(ifilename);//checking whether file exists or not
        FILE *ifp= fopen(ifilename,"rb");//reads file
        strip_lzw_ext(ifilename);
        FILE *ofp= fopen(ifilename,"wb");//write file
        decode(ifp,ofp);
        fclose(ofp);
    }
}





