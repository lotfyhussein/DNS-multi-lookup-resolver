
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "util.h"
#include "queue.h"
#include "multi-lookup.h"




#define MIN_Arguments 5
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

#define MAX_INPUT_FILES 10
#define MAX_Res_THREADS 10
#define MIN_Res_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define QUEUE_SIZE 1024



FILE* outputfp;
FILE* Serviced;
queue q;
int files_are_read;
pthread_mutexattr_t attr;
pthread_mutex_t Q_mutex;

pthread_mutexattr_t attr2;
pthread_mutex_t output_mutex;

pthread_mutexattr_t attr3;
pthread_mutex_t mutex_for_files_are_read;


void* Requester(char* file_name)
{
   
    FILE* input_file = NULL;
	char* hostname;


	Serviced = fopen("Serviced.txt", "w");    
	fprintf(Serviced, "%s\n", file_name);
	   printf("Thread number %ld,%s\n", pthread_self(),file_name );
	 

	input_file = fopen(file_name, "r");
	if(!input_file)
	{
		printf("Error Opening Input File: %s", file_name);
		
	}
	
	
	hostname = (char*)malloc((MAX_NAME_LENGTH+1)*sizeof(char));
	
	
	while(fscanf(input_file, INPUTFS, hostname) > 0){
		
	
		pthread_mutex_lock(&Q_mutex); 
		
		
		if(queue_is_full(&q))
		{
			pthread_mutex_unlock(&Q_mutex);

			usleep(rand()%100);
			pthread_mutex_lock(&Q_mutex); 
		}
		
		
		if(queue_push(&q, hostname) == QUEUE_FAILURE) 
		{
			printf("ERROR: adding %s\n to queue failed ", hostname);
		}
		pthread_mutex_unlock(&Q_mutex); 
		
		
		hostname = (char*)malloc((MAX_NAME_LENGTH+1)*sizeof(char));
	 }
	 
	 
	 fclose(input_file);


    free(hostname); 
    hostname=NULL;
    

    pthread_exit(NULL);
    return NULL;
}

void* resolve_domain(void* p)
{	
	char *hostname;
    char IP[INET6_ADDRSTRLEN];
	int QNotEmpty = 0; 
	int resume = 0; 
	
	pthread_mutex_lock(&Q_mutex);
	QNotEmpty = !queue_is_empty(&q); 
	pthread_mutex_unlock(&Q_mutex);
	
	pthread_mutex_lock(&mutex_for_files_are_read);
	resume = (!files_are_read)||(QNotEmpty); 
	pthread_mutex_unlock(&mutex_for_files_are_read);
	
	while(resume)
	{
		pthread_mutex_lock(&Q_mutex);
		if((hostname = (char*)queue_pop(&q)) == NULL) 
		{ 
			//printf("ERROR: queue_pop failed with : %s\n", hostname);
			pthread_mutex_unlock(&Q_mutex); //unlock the queue for others to use
		}
		else
		{	
			pthread_mutex_unlock(&Q_mutex);
			
			
			if(dnslookup(hostname, IP, sizeof(IP)) == UTIL_FAILURE)
			{
				fprintf(stderr, "dnslookup error: %s\n", hostname);
				strncpy(IP, "", sizeof(IP)); //copy a NULL value to the IP string
			}
			
			
			pthread_mutex_lock(&output_mutex); //lock the output file
				fprintf(outputfp, "%s,%s\n", hostname, IP);
			pthread_mutex_unlock(&output_mutex); //unlock the output file
			
			free(hostname); 
			hostname = NULL;
		}
			
		
		pthread_mutex_lock(&Q_mutex);
			QNotEmpty = !queue_is_empty(&q); 
		pthread_mutex_unlock(&Q_mutex);
		pthread_mutex_lock(&mutex_for_files_are_read);
			resume = (!files_are_read)||(QNotEmpty); 
		pthread_mutex_unlock(&mutex_for_files_are_read);
		
	}
	
	pthread_mutex_lock(&Q_mutex); 
	if(queue_is_empty(&q)) 
	{
		
		pthread_mutex_unlock(&Q_mutex); 
		pthread_exit(NULL);
		return NULL;
	}
	else  
	{
		fprintf(stderr, "Error: Q isn't empty\n");
		pthread_mutex_unlock(&Q_mutex); 
		pthread_exit((void*)EXIT_FAILURE); 
		return (void*)EXIT_FAILURE;
	}
	
}

int main(int argc, char* argv[])
{
    
    outputfp = NULL;
    int num_input_files = argc-4; 
	
char **filenames;

filenames = malloc(num_input_files * sizeof(char*));
for (int i = 0; i < num_input_files; i++)
    filenames[i] = malloc((11) * sizeof(char));

for (int i = 0; i < num_input_files; i++)
	strcpy(filenames[i], argv[4+i]);


/*
printf("========================\n");
for (int i = 0; i < num_input_files; i++)
printf("%s\n", filenames[i]);
printf("========================\n");

*/


const int qSize = QUEUE_SIZE;
    int rc;
    long t;
    files_are_read=0;
    
int NumberOfReqTHreads = atoi(argv[1]);
   pthread_t *file_threads;
  file_threads = (pthread_t *) malloc( sizeof(pthread_t)*NumberOfReqTHreads);


  
int NumberOfResTHreads = atoi(argv[2]);
   pthread_t *resolver_threads;
  resolver_threads = (pthread_t *) malloc( sizeof(pthread_t)*NumberOfResTHreads);


	pthread_mutexattr_settype(&attr, (long)NULL);
	pthread_mutex_init(&Q_mutex, &attr);

	pthread_mutexattr_settype(&attr2, (long)NULL);
	pthread_mutex_init(&output_mutex, &attr2);
	
	pthread_mutexattr_settype(&attr3,(long) NULL);
	pthread_mutex_init(&mutex_for_files_are_read, &attr3);

	
	
	if(argc < MIN_Arguments){
		fprintf(stderr, "Too few arguments: %d\n", (argc - 1));
		
		return EXIT_FAILURE;
	}
	
    if(argc > MAX_INPUT_FILES+9){
		fprintf(stderr, "Too many arguments: %d\n", (argc - 1));
		
		return EXIT_FAILURE;
    }


    outputfp = fopen(argv[3], "w");
    if(!outputfp){
		perror("Error Opening Output File\n");
		return EXIT_FAILURE;
    }


    if(queue_init(&q, qSize) == QUEUE_FAILURE)
    {
		fprintf(stderr, "ERROR: queue_init failed!\n");
		return EXIT_FAILURE; 
    }

    for(t=3; t<3 + argc -4; t++)
    { 
		
		rc = pthread_create(&(file_threads[t-3]), NULL, (void *)Requester, argv[t+1]); //pass it the input filename and the output file
		if (rc)
		{ 
			fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
	}
	

	for(t=0; t<NumberOfResTHreads; t++)
	{ 
		
		rc = pthread_create(&(resolver_threads[t]), NULL, resolve_domain, NULL); //pass it the input filename and the output file
		if (rc)
		{ 
			fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
	}
	


    for(t=0;t<NumberOfReqTHreads;t++)
    {
		pthread_join(file_threads[t], NULL);
    }

    pthread_mutex_lock(&mutex_for_files_are_read);
		files_are_read=1; 
	pthread_mutex_unlock(&mutex_for_files_are_read);
    
    for(t=0;t<NumberOfResTHreads;t++)
    {
		pthread_join(resolver_threads[t], NULL);
    }
    


	

printf("Done!!!\n");

printf("Time:");



	fclose(outputfp); 

    queue_cleanup(&q); 
    pthread_mutex_destroy(&Q_mutex);
     pthread_mutex_destroy(&output_mutex);




    return EXIT_SUCCESS;
}
