//We scan through chunks of size 1MB and print the lines of the file which give results

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

//chunk size = 1MB
#ifndef CHUNKSIZE
#define CHUNKSIZE  1024 * 1024 * 1
#endif

#define DEBUG 0;

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

int wordcount;
typedef struct thrdInpt
{
  char *chunk;//[CHUNKSIZE];
  char *word;//[50];
  unsigned long chunklength;
}thrdInpt;

typedef struct srchRslt
{
  int chunkcount;
  int subchunkcount;
  char line[150];
}srchRslt;

//Declare thread function to access the string array and search for given word in it
void* HandleChunk(void*);

//Declare function to divide a file into chunks of specific size
unsigned long CreateChunk(char* chunk,int chunkcount, FILE *fp)
{
   //Read 100,000 chars from file 1MB into buffer
  unsigned long i = 0;
  DEBUG_PRINT(("CHK Reading Chunk : file.txt position : %lu to %lu \n",(unsigned long)chunkcount * CHUNKSIZE,(unsigned long)chunkcount * CHUNKSIZE + CHUNKSIZE -1));
  fseek(fp, chunkcount * CHUNKSIZE , SEEK_SET);
  for(;i<CHUNKSIZE;i++)
  {
    //ToDo: Need to find a better way to read chunks faster into buffer
    chunk[i] = fgetc(fp);
    if(chunk[i] == EOF)
    {
           DEBUG_PRINT(("CHK CreateChunk EOF found at position: %lu \n",i));
      //if EOF return 0, else return failure in reading file
      if (ferror(fp) != 0)
        return -1;
      else
	return i;
    }
  }
  return -1;
}


void* HandleChunk(void* InData)
{
    //DEBUG_PRINT(("CHK Handler thread \n"));
  thrdInpt * ip = (thrdInpt *)InData;
  char * chunk = ip->chunk;
  char * word = ip->word;
  unsigned long bytesread = ip->chunklength;
  
  //Go through the chunk and print each instance of occurence.
  char * chunk_pointer = chunk;
  char * word_poiter =  word;
  int chunkindex =0;
  int wordindex =0;
  int wordlength=0;
  
  //Buffer for storing and printing lines 150 chars long
  char line_buffer[150];
  int lnbf_count = 0;
  int line_length = 150;
  
  //Flag which indicates if word has been found in chunk
  int flag_wordfount = 0;
  //Get the wordlenght
  int i=0;
    
  wordlength = strlen(word) -1;

  DEBUG_PRINT(("CHK Handler thread :word to be searched: %s \n",word));
  chunkindex =0;
  wordindex=0;
  	    DEBUG_PRINT(("CHK Handler bytes read : %lu \n",bytesread));
  while(chunkindex < bytesread )
  {
    if(chunk[chunkindex] != EOF)
    {
      //DEBUG_PRINT(("CHK2 Handler thread \n"));
     
    // DEBUG_PRINT(("CHK Handler thread :chunkindex : %d :lnbf_count : %d \n",chunkindex,lnbf_count));
      line_buffer[lnbf_count] = chunk[chunkindex];

      if(chunk[chunkindex]==word[wordindex])
      {
	//DEBUG_PRINT(("CHK2 Handler thread : word char found : %c\n",chunk[chunkindex]));	
	if(wordindex == wordlength)
	{
	  //word found
	  DEBUG_PRINT(("CHK Handler thread :word found: %s \n",word));
	  //print the sentence or line
	  //Print 60 letters before and 60 after the word
	  chunkindex++;
	  while(lnbf_count<line_length-1  && 
	    chunk[chunkindex]!='\0' && 
	    chunk[chunkindex]!='\n' && 
	    chunk[chunkindex]!='.' && 
	    chunk[chunkindex] != EOF)
	  {
	    lnbf_count++;
	    line_buffer[lnbf_count] = chunk[chunkindex];
	    chunkindex++;
	  }
	  line_buffer[lnbf_count] = '\0';
	  printf("%s\n\n",line_buffer);
	  lnbf_count=0;
	  wordindex=0;
	 
	}
	else
	  wordindex++;
      }
      else
      {
	wordindex = 0;
      }

      //Start new line if carriage return or . found

      if(chunk[chunkindex]=='\0' || chunk[chunkindex]=='\n' || chunk[chunkindex]=='.' || chunk[chunkindex] == EOF || lnbf_count>=line_length-1)
      {
	line_buffer[0]='\0';
	lnbf_count = -1;
      }

      //printf("%c",chunk[chunkindex]);

      chunkindex++;
      lnbf_count++;
      
    }
  }

}

int main()
{
  //Read the file
  FILE *fp;
  //No of main threads = 100. So max file size = 1 * 100 MB = 100 MB
  pthread_t workerThreadsList[100];
  int workerThreadCount = -1;
  int chunkCount = 0;
  int chunkSize = CHUNKSIZE;
  int filesize = 0;
  thrdInpt *ip = (thrdInpt*)malloc(sizeof(thrdInpt));
  char  chunk[CHUNKSIZE];// = (char*)malloc(CHUNKSIZE * sizeof(char));
  char  word[50] = "word";
  
  //Open file
  if((fp = fopen("file.txt", "r")) == NULL)
  {
     fclose(fp);
     return 1;
  }
  DEBUG_PRINT(("CHK File opened : file.txt\n"));
  //Get the file size
  fseek(fp, 0L, SEEK_END);
  filesize = ftell(fp);
  DEBUG_PRINT(("CHK filesize : %d\n",filesize));
  fseek(fp, 0L, SEEK_SET);
  
  //Initialize the number of threads required ie the number of chunks
   //chunkcount = Filesize / ChunkSize + 1
  chunkCount = filesize / chunkSize + 1;
  
  DEBUG_PRINT(("CHK Number of Chunks : %d\n",chunkCount));
  int i= 0;
  //while file is not completely read do the following again and again
  while(i<chunkCount)
  {
    //create a new chunk 
    unsigned long bytesread = CreateChunk(chunk,i,fp);
    if(bytesread<0)
    {
      printf("Error while creating file chunk : %d", i);
    }
    
    DEBUG_PRINT(("CHK Chunk created \n"));
    /*int j=0;
    while(chunk[j] != EOF && j<CHUNKSIZE)
    {
      printf("%c",chunk[j]);
      j++;
    }*/
    
    
    //Start new thread to check chunk
    DEBUG_PRINT(("CHK Creating Thread : %d Chunkcount : %d\n",workerThreadCount,i));
    ip->chunk = chunk;
    ip->word = word;
    ip->chunklength = bytesread;
    
    workerThreadCount++;
    if(pthread_create(workerThreadsList + workerThreadCount, NULL, HandleChunk, (void *)ip) != 0)
    {
      printf("--ERROR - Could not start a worker Thread\n");
    }
    
    
    if(workerThreadCount==100)
    {
      printf("--ERROR - Ran out of threads\n");
      break;
    }
    i++;
  } 
  //Close file
    DEBUG_PRINT(("Going to sleep: Worker Thread count : %d\n", workerThreadCount));
  sleep(10);
  DEBUG_PRINT(("CHK File closed\n"));
  fclose(fp);
  
  //Wait for all threads to finish
   
  //Print results from data structure or print while in the HandleChunk thread
   
  //Display the count
  
  //Free the chunk
  free(ip);
  
}