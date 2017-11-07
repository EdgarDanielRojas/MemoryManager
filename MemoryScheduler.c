#include <ctype.h>                 /* Used for the isdigit() function */
#include <stdlib.h>                     /* Used for malloc definition */
#include <stdio.h>                                /* Used for fprintf */
#include <math.h>
/***********************************************************************
 *                       Global constant values                        *
 **********************************************************************/
#define LOGADD 16 //!<Constant used to define the logical address space
#define PHYADD 13 //!<Constant used to define the physical address space
#define OFFSET 9 //!<Constant used to defin the offset Used
#define TLBSIZE 8 //!<Constant for the TLB size
#define NUMPARAMS 2 //!< Constant used to define the number of parameters we must receive.

// Used to get a number from a file passed as a parameter
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

// Gets the smallest LRU time from an array passed as parameter and the size of said parameter;
int getTLB(int tlb[][3],int size){
  int i,tlbn;
  int smallest=tlb[0][2]; //Get the first LRU from the first entry in the TLB
  for(i=0;i<size;i++){ // The array is searched for a lower LRU or a LRU of zero
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
  return tlbn; //Return the index of the lowest LRU in the TLB
}

/***********************************************************************
 *                          Main entry point                           *
 **********************************************************************/
int main (int argc, const char * argv[]) {
    int tmem,ttlb,tfault;                       /* Memory Access Time, TLB Access Time, Page Fault Time*/
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
            if (tmem == EXIT_FAILURE || ttlb == EXIT_FAILURE || tfault == EXIT_FAILURE) { //We check to see if we read succesfully the three times
                printf("%s\n","Time not found" );//In case of error we notify the user and terminate the program
                return EXIT_FAILURE;
            } else {
              int pageSize,frameSize; //Variables declared that will hold the page and frame size or spaces
              pageSize = pow(2,LOGADD-OFFSET); //Page size is calculated
              int pageTable[pageSize][2]; // Array is declared with the page size and 2 int spaces, one for the frame number and the second for validity
              frameSize = pow(2,PHYADD-OFFSET); //frame size in calculated
              int frame[frameSize][3]; // Array is declared with the frame size and 3 int spaces
                                      //One is for the page entry, the second as a dirty bit, the third for the LRU time
              #ifdef DEBUG
                //In debug mode the number of entries is printed
                printf("%d Page Entries\n",temp);
                printf("%d Frame Entries\n",temp);
                printf("%d TLB Entries\n",TLBSIZE);
              #endif
              int tlb[TLBSIZE][3]; //TLB is declared with three int spaces, one for page entry, second for the frame and third for the LRU time
              int rem,n,i; //Rem is the remainder used to convert to binary, n stores numeric value of the address and i is used for 'fors'
              int frameN,tlbn,pageEntry,smallest;//Frame number, TLB number, Page Entry and Smallest LRU time
              double accessTime,sum; //Access time of the current memory request and the sum of all the access time of memories request
              int counter=1;  //Counter serves as a time counter
              int hits=0; //Hits stores the number of TLB hits we have
              int pageOut=0,pageIn=0; //Page outs into disk and page ins from disk
              int found; //Bool variable that is set to 1 if found
              for(i=0;i<pageSize;i++){ //We set the page table to neutral values
                pageTable[i][0]=0;
                pageTable[i][1]=0;
              }
              for(i=0;i<frameSize;i++){ //We set the frame table to neutral values
                frame[i][0]=0;
                frame[i][1]=0;
                frame[i][2]=0;
              }
              for(i=0;i<TLBSIZE;i++){ //We set the TLB to neutral values
                tlb[i][0]=-1;
                tlb[i][1]=0;
                tlb[i][2]=0;
              }
              while(!feof(fp)){ //While the eof isn't reached
                fscanf(fp,"%x %c",&address,&operation); //Address and operation are read
                if(feof(fp)) //If end of file we break
                  break;
                pageEntry = 0; //Page entry is reset
                accessTime=0;//Access time is reset
                frameN=0; //Frame number is reset
                n=address; //Numeric value is passed to n
                for(i = LOGADD;i>0;i--){ //We convert the address read to a string representation in binary
                  rem = n%2;
                  n = n/2;
                  if(rem == 1){
                    caddress[i-1]='1';
                  }
                  else{
                    caddress[i-1]='0';
                  }
                }
                for(i = LOGADD-OFFSET;i>0;i--){ //The page entry is calculated based on the conversion made before
                  int num = LOGADD-OFFSET-i;
                  if(caddress[i-1] == '1'){
                    pageEntry=pageEntry+ pow(2,num);
                  }
                }
                found=0; // Found is set to false
                for(i=0;i<TLBSIZE;i++){ //TLB is scanned for the page entry
                  if(tlb[i][0]==pageEntry){ //If we have a hit we increase the counter,set new LRU and get the Frame registered with the page
                    hits++;
                    frameN = tlb[i][1];
                    tlb[i][2]=counter;
                    found=1;
                    break;
                  }
                }
                accessTime+=ttlb; //TLB accessed so we add the TLB access time
                if(found==0){ //In case it wasn't found in the TLB
                  accessTime+=tmem; //Page table is accessed so we add the Page table access time
                  if(pageTable[pageEntry][1]==1){ //If the Page table has a valid entry we get the frame registered with the entry and set a new TLB entry
                    frameN=pageTable[pageEntry][0];
                    tlbn=getTLB(tlb,TLBSIZE);
                    tlb[tlbn][0]=pageEntry;
                    tlb[tlbn][1]=frameN;
                    tlb[tlbn][2]=counter;
                  }
                  else{
                    accessTime+=tfault; //No valid frame found so we add the Page Fault time
                    smallest=counter; // We look for the frame with the smallest LRU time or with a time of 0
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
                    if(frame[frameN][1]==1){ //In case the frame is dirty we do a page out
                      accessTime+=tfault;
                      pageOut++;
                    }
                    pageIn++; //Page ins is incremented
                    pageTable[frame[frameN][0]][1]=0; // We set the old page entry to invalid
                    frame[frameN][0]=pageEntry; //The new page is set to the frame
                    frame[frameN][1]=0; //The dirty bit is set to 0
                    pageTable[pageEntry][0]=frameN; //The page table is updated with the new frame
                    pageTable[pageEntry][1]=1; //The page table is updated with a valid bit
                    tlbn=getTLB(tlb,TLBSIZE); //We update the TLB table with the new page/frame entry
                    tlb[tlbn][0]=pageEntry;
                    tlb[tlbn][1]=frameN;
                    tlb[tlbn][2]=counter;
                  }
                }
              if(operation=='W'){ //In case the operation is a write we set the bit to dirty
                  frame[frameN][1]=1;
              }
              frame[frameN][2]=counter; // The LRU time for the frame is set
              //accessTime+=tmem; // The final memory access time is added
              counter++; //Counter is increased
              sum+=accessTime; //The total access time of all the request is updated
              #ifdef DEBUG //Information about the Memory request is printed
                printf("Number: %d\n",address);
                printf("Operation: %c\n",operation);
                printf("Binary: %s\n",caddress);
                printf("Page Entry: %d\n",pageEntry);
              #endif
            }
            #ifdef DEBUG //Final frame and TLB state is printed
              for(i=0;i<frameSize;i++){
                printf("Page: %d LRU: %d Dirty: %d\n",frame[i][0],frame[i][2],frame[i][1]);
              }
              for(i=0;i<TLBSIZE;i++){
                printf("Page: %d Frame: %d LRU %d\n",tlb[i][0],tlb[i][1],tlb[i][2]);
              }
            #endif
              counter--; //Counter has one more than the actual events so we subtract one
              // The information required is displayed
              printf("Number of events: %d\n",counter);
              printf("Page in: %d\n",pageIn);
              printf("Page out: %d\n",pageOut);
              printf("Avg: %f\n",sum/(double)counter);
              printf("Hit Ratio: %f\n",(float)hits/(counter));
            }
        }
        //Program terminated succesfully
        printf("Program terminated correctly\n");
        return (EXIT_SUCCESS);
    }
}
