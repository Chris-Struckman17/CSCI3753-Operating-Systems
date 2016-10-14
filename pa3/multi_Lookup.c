#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "queue.h"
#include "util.h"
#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS sysconf(_SC_NPROCESSORS_ONLN);
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
//#define NUM
//char errorstr[SBUFSIZE];

char hostname[SBUFSIZE];

pthread_cond_t writer;
pthread_cond_t reader;
pthread_mutex_t queueLock;
pthread_mutex_t fileLock;

queue* critical;
int *alive;
//char* payload;


void* request(void* file){
//printf("We are in requester\n");
//  char hostname[SBUFSIZE];
 char errorstr[SBUFSIZE];
 char* payload;

 FILE* inputfp = NULL;


 	/* Open Input File */
  inputfp = fopen((char*)file, "r");
 	  if(!inputfp){
 	    sprintf(errorstr, "Error Opening Input File");
 	    perror(errorstr);
      return 0;

 	  }



  while(fscanf(inputfp, INPUTFS, hostname) > 0){

   printf("In While:\n");
    pthread_mutex_lock(&queueLock);
    printf("Locked\n");
    if(queue_is_full(critical)){
      printf("queue is full!\n");
      pthread_cond_wait(&reader,&queueLock);
      printf("requester waited\n");
    }

      //payload =(char *)malloc(SBUFSIZE*sizeof(char));
      payload = strdup(hostname);
      printf("have payload\n");
      queue_push(critical,payload);
      printf("PUSHED!\n");
      pthread_mutex_unlock(&queueLock);
      printf("UNLOCKED MUTEX!\n");
      pthread_cond_signal(&writer);
      printf("READER SIGNALED\n");

  }
fclose(inputfp);
printf("FILE CLOSED!\n");
//free(payload);
//payload = NULL;

return NULL;
}

void* resolve(void* OutFile){

  printf("WE ARE IN RESOLVERS!!\n");

  char firstipstr[INET6_ADDRSTRLEN];


  while(*alive || !(queue_is_empty(critical))){
printf("CREATING LOCK!\n");
  char* host;
  pthread_mutex_lock(&queueLock);
  printf("QUEUE LOCKED!!!\n");
  if(queue_is_empty(critical)){
    printf("WAITING THE WRITER\n");
    pthread_cond_wait(&writer,&queueLock);
    printf("SUCCESS!\n");
  }
  host = queue_pop(critical);
 printf("POPPED!\n");
  pthread_mutex_unlock(&queueLock);
  printf("QUEUE UNLOCKED\n");
  pthread_cond_signal(&reader);
printf("TRYING TO LOCK FILE\n");
pthread_mutex_lock(&fileLock);
printf("SUCCESS\n");
  FILE* outputfp = fopen((char*)OutFile, "a");
  if(!outputfp){
  perror("Error Opening Output File");
  return NULL;
  }
  printf("FILE OPENED!\n");
  printf("SIGNALED WRITER!!\n");
  if(dnslookup(host, firstipstr, sizeof(firstipstr))
     == UTIL_FAILURE){
fprintf(stderr, "dnslookup error: %s\n", hostname);
strncpy(firstipstr, "", sizeof(firstipstr));
  }

  fprintf(outputfp, "%s,%s\n", host, firstipstr);
  printf("WROTE TO OUTPUT FILE!\n");

  printf("ATTEMPTING CLOSED FILE!\n");
  fclose(outputfp);
  printf("CLOSED FILE!\n");
  free(host);
  printf("ATTEMPTING TO UNLOCK FILE!\n");
  pthread_mutex_unlock(&fileLock);
  printf("FILE IS UNLOCKED\n");

}

  return NULL;
}

int main(int argc, char* argv[]){
  printf("IN MAIN!\n");
  printf("WORKING\n");
  critical = malloc(sizeof(queue));
  alive = (int *)malloc(sizeof(int));
  *alive = 1;

  queue_init(critical,QUEUEMAXSIZE);
  pthread_mutex_init(&fileLock,NULL);
  pthread_mutex_init(&queueLock,NULL);


  printf("WORKING\n");

  printf("Working\n");
  int proc = MIN_RESOLVER_THREADS;
printf("Working\n");

  if(argc < MINARGS){
 	  fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
  	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
    return EXIT_FAILURE;
  }

  int ifiles = argc -2;
  printf("working\n");
  pthread_t thread[ifiles]; //(pthread_t *) malloc(sizeof(pthread_t)*ifiles);
  pthread_t outThread[proc]; //(pthread_t *) malloc(sizeof(pthread_t)* proc);
  printf("working\n");
  //char errorstr[SBUFSIZE];
  for(int i = 0; i < ifiles; i++){
     pthread_create(&thread[i], NULL, request, argv[i+1]);
  }
  printf("CREATED REQUESTERS!\n");

  for(int j = 0; j < proc; j++){
    pthread_create(&outThread[j], NULL, resolve, argv[argc-1]);


  }
  printf("CREATED RESOVLERS\n");

  for(int k = 0; k < ifiles; k++){

    pthread_join(thread[k],NULL);


  }
  printf("REQUESTERS JOINED!!\n");
  *alive = 0;

  for(int h = 0; h < proc; h++){

    pthread_join(outThread[h],NULL);

  }
  printf("RESOLVERS JOINED!!!\n");

queue_cleanup(critical);
pthread_cond_destroy(&writer);
pthread_cond_destroy(&reader);
pthread_mutex_destroy(&queueLock);
pthread_mutex_destroy(&fileLock);
free(alive);

}
