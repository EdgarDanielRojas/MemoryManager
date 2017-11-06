#include <ctype.h>                 /* Used for the isdigit() function */
#include <stdlib.h>                     /* Used for malloc definition */
#include <stdio.h>                                /* Used for fprintf */
#include <string.h>                                /* Used for strcmp */
#include <assert.h>                      /* Used for the assert macro */
#include <math.h>
/***********************************************************************
 *                       Global constant values                        *
 **********************************************************************/
#define LOGADD 16 //!<Constant used to define the logical address space
#define PHYADD 13 //!<Constant used to define the physical address space
#define OFFSET 9 //!<Constant used to defin the offset Used
#define TLBSIZE 8 //!<Constant for the TLB size
#define NUMPARAMS 2 //!< Constant used to define the number of parameters we must receive.

int GetInt (FILE *fp) {
    int	c,i;	   /* Character read and integer representation of it */
    int sign = 1;

    do {
        c = getc (fp);                          /* Get next character */
        if ( c == '#' )	                          /* Skip the comment */
            do {
                c = getc (fp);
            } while ( c != '\n');
        if ( c == '-')
            sign = -1;
    } while (!isdigit(c) && !feof(fp));

    if (feof(fp)){
        return (EXIT_FAILURE);
    } else {
    /* Found 1st digit, begin conversion until a non-digit is found */
        i = 0;
        while (isdigit (c) && !feof(fp)){
            i = (i*10) + (c - '0');
            c = getc (fp);
        }

        return (i*sign);
    }
}

int getTLB(int tlb[][3],int size){
  int i,tlbn;
  int smallest=tlb[0][2];
  for(i=0;i<size;i++){
    if(tlb[i][2]==0)
    {
      tlbn=i;
      break;
    }
    if(tlb[i][2]<smallest){
      smallest=tlb[i][2];
      tlbn=i;
    }

  }
  return tlbn;
}
/***********************************************************************
 *                          Main entry point                           *
 **********************************************************************/
int main (int argc, const char * argv[]) {
    int tmem,ttlb,tfault;
    FILE   *fp;                                /* Pointer to the file */
    int address;                               /* Numeric value of the logical address*/
    char operation;                            /* Operation to be done (W or R)*/
    char caddress[LOGADD];                    /* Binary representation of the logical address*/
    /* Check if the number of parameters is correct */
    if (argc < NUMPARAMS){
        printf("Need a file with the process information\n");
        printf("Abnormal termination\n");
        return (EXIT_FAILURE);

    } else {
        /* Open the file and check that it exists */
        fp = fopen (argv[1],"r");	  /* Open file for read operation */
        if (!fp) {                               /* There is an error */
            printf("%s\n","File not found" );
            return EXIT_FAILURE;
        } else {
            /* Read the times from the file*/
            tmem = GetInt(fp);
            ttlb = GetInt(fp);
            tfault = GetInt(fp);
            if (tmem == EXIT_FAILURE || ttlb == EXIT_FAILURE || tfault == EXIT_FAILURE) {
                //ErrorMsg("main","Quantum not found");
                printf("%s\n","Time not found" );
                return EXIT_FAILURE;
            } else {
              int pageSize,frameSize;
              pageSize = pow(2,LOGADD-OFFSET);
              int pageTable[pageSize][2];
              frameSize = pow(2,PHYADD-OFFSET);
              int frame[frameSize][3];
              //printf("%d Page Entries\n",temp);
              //printf("%d Frame Entries\n",temp);
              //printf("%d TLB Entries\n",TLBSIZE);
              int tlb[TLBSIZE][3];
              int rem,n,i,frameN,tlbn,pageEntry,smallest;
              double accessTime,sum;
              int counter=1;
              int hits=0;
              int pageOut=0,pageIn=0;
              for(i=0;i<pageSize;i++){
                pageTable[i][0]=0;
                pageTable[i][1]=0;
              }
              for(i=0;i<frameSize;i++){
                frame[i][0]=0;
                frame[i][1]=0;
                frame[i][2]=0;
              }
              for(i=0;i<TLBSIZE;i++){
                tlb[i][0]=-1;
                tlb[i][1]=0;
                tlb[i][2]=0;
              }
              while(!feof(fp)){
                fscanf(fp,"%x %c",&address,&operation);
                if(feof(fp))
                  break;
                accessTime=0;
                n=address;
                frameN=0;
                for(i = LOGADD;i>0;i--){
                  rem = n%2;
                  n = n/2;
                  if(rem == 1){
                    caddress[i-1]='1';
                  }
                  else{
                    caddress[i-1]='0';
                  }
                }
                pageEntry = 0;
                for(i = LOGADD-OFFSET;i>0;i--){
                  int num = LOGADD-OFFSET-i;
                  if(caddress[i-1] == '1'){
                    pageEntry=pageEntry+ pow(2,num);
                  }
                }
                int found=0;
                //int frameN;
                for(i=0;i<TLBSIZE;i++){
                  if(tlb[i][0]==pageEntry){
                    hits++;
                    frameN = tlb[i][1];
                    tlb[i][2]=counter;
                    found=1;
                    break;
                  }
                }
                accessTime+=ttlb;
                if(found==0){
                  accessTime+=tmem;
                  if(pageTable[pageEntry][1]==1){
                    frameN=pageTable[pageEntry][0];
                    tlbn=getTLB(tlb,TLBSIZE);
                    tlb[tlbn][0]=pageEntry;
                    tlb[tlbn][1]=frameN;
                    tlb[tlbn][2]=counter;
                  }
                  else{
                    accessTime+=tfault;
                    smallest=counter;
                    for(i=0;i<frameSize;i++){
                      if(frame[i][2]==0){
                        frameN=i;
                        break;
                      }
                      if(frame[i][2]<smallest){
                        smallest=frame[i][2];
                        frameN=i;
                      }
                    }
                    if(frame[frameN][1]==1){
                      accessTime+=tfault;
                      pageOut++;
                    }
                    /*for(i=0;i<TLBSIZE;i++){
                      if(tlb[i][0]==frame[frameN][0]){
                        tlb[tlbn][0]=-1;
                        tlb[tlbn][1]=0;
                        tlb[tlbn][2]=0;
                        break;
                      }
                    }*/
                    pageIn++;
                    pageTable[frame[frameN][0]][1]=0;
                    frame[frameN][0]=pageEntry;
                    frame[frameN][1]=0;
                    pageTable[pageEntry][0]=frameN;
                    pageTable[pageEntry][1]=1;
                    //int tlbn;
                    tlbn=getTLB(tlb,TLBSIZE);
                    tlb[tlbn][0]=pageEntry;
                    tlb[tlbn][1]=frameN;
                    tlb[tlbn][2]=counter;
                  }
                }
              if(operation=='W'){
                  frame[frameN][1]=1;
                }
              frame[frameN][2]=counter;
              //accessTime+=tmem;
              counter++;
              sum+=accessTime;
              /*printf("Number: %d\n",address);
              printf("Operation: %c\n",operation);
              printf("Binary: %s\n",caddress);
              printf("Page Entry: %d\n",pageEntry);*/
            }
              /*for(i=0;i<frameSize;i++){
                printf("Page: %d LRU: %d Dirty: %d\n",frame[i][0],frame[i][2],frame[i][1]);
              }
              for(i=0;i<TLBSIZE;i++){
                printf("Page: %d Frame: %d LRU %d\n",tlb[i][0],tlb[i][1],tlb[i][2]);
              }*/
              counter--;
              printf("Number: %d\n",counter);
              printf("Page in: %d\n",pageIn);
              printf("Page out: %d\n",pageOut);
              printf("Avg: %f\n",sum/(double)counter);
              printf("Hit Ratio: %f\n",(float)hits/(counter));
            }
        }
        printf("Program terminated correctly\n");
        return (EXIT_SUCCESS);
    }
}
