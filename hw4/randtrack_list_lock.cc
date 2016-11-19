
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "defs.h"
#include "hash.h"

#define SAMPLES_TO_COLLECT   10000000
#define RAND_NUM_UPPER_BOUND   100000
#define NUM_SEED_STREAMS            4	

/* 
 * ECE454 Students: 
 * Please fill in the following team struct 
 */
team_t team = {
    "Team Awesome",                  /* Team name */

    "Rahul Chandan",                    /* First member full name */
    "999781801",                 /* First member student number */
    "rahul.chandan@mail.utoronto.ca",                 /* First member email address */

    "Lennox Stevenson",                           /* Second member full name */
    "999585667",                           /* Second member student number */
    "lennox.stevenson@mail.utoronto.ca"                            /* Second member email address */
};

unsigned num_threads;
unsigned samples_to_skip;

class sample;

class sample {
  unsigned my_key;
 public:
  sample *next;
  unsigned count;

  sample(unsigned the_key){my_key = the_key; count = 0;};
  unsigned key(){return my_key;}
  void print(FILE *f){printf("%d %d\n",my_key,count);}
};

typedef struct {
        int start;
        int end;
} thread_args;

// This instantiates an empty hash table
// it is a C++ template, which means we define the types for
// the element and key value here: element is "class sample" and
// key value is "unsigned".  
hash<sample,unsigned> h;

pthread_t tid[4];

void* sample_shit(void *args) {
  int i,j,k;
  int rnum;
  unsigned key;
  sample *s;

  // process streams starting with different initial numbers
  thread_args bounds = *((thread_args *) args);
  free(args);
  for (i = bounds.start; i <= bounds.end; i++){
    rnum = i;

    // collect a number of samples
    for (j=0; j<SAMPLES_TO_COLLECT; j++){

      // skip a number of samples
      for (k=0; k<samples_to_skip; k++){
	rnum = rand_r((unsigned int*)&rnum);
      }

      // force the sample to be within the range of 0..RAND_NUM_UPPER_BOUND-1
      key = rnum % RAND_NUM_UPPER_BOUND;
      
      h.locklist(key);
      // if this sample has not been counted before
      if (!(s = h.lookup(key))){
	
	// insert a new element for it into the hash table
	s = new sample(key);
	h.insert(s);
      }

      // increment the count for the sample
      s->count++;
      h.unlocklist(key);
    }
  }
  return NULL;
}

int main (int argc, char* argv[]){
  int i;
  int err;

  // Print out team information
  printf( "Team Name: %s\n", team.team );
  printf( "\n" );
  printf( "Student 1 Name: %s\n", team.name1 );
  printf( "Student 1 Student Number: %s\n", team.number1 );
  printf( "Student 1 Email: %s\n", team.email1 );
  printf( "\n" );
  printf( "Student 2 Name: %s\n", team.name2 );
  printf( "Student 2 Student Number: %s\n", team.number2 );
  printf( "Student 2 Email: %s\n", team.email2 );
  printf( "\n" );

  // Parse program arguments
  if (argc != 3){
    printf("Usage: %s <num_threads> <samples_to_skip>\n", argv[0]);
    exit(1);  
  }
  sscanf(argv[1], " %d", &num_threads); // not used in this single-threaded version
  sscanf(argv[2], " %d", &samples_to_skip);

  // initialize a 16K-entry (2**14) hash of empty lists
  h.setup(14);
  
  if (num_threads == 1) {
        thread_args* bounds = (thread_args *) malloc(sizeof(thread_args));
        bounds->start = 0 ;
        bounds->end = 3;
        err = pthread_create(&tid[0], NULL, &sample_shit, bounds);
  }
  else if (num_threads == 2) {
        thread_args *bounds1 = (thread_args *) malloc(sizeof(thread_args));
        bounds1->start = 0 ;
        bounds1->end = 1;
        
        thread_args *bounds2 = (thread_args *) malloc(sizeof(thread_args));
        bounds2->start = 2 ;
        bounds2->end = 3;
        err = pthread_create(&tid[0], NULL, &sample_shit, (void *) bounds1);
        err = pthread_create(&tid[1], NULL, &sample_shit, (void *) bounds2);
        
  }
  else if (num_threads == 4) {
        thread_args *bounds1 = (thread_args *) malloc(sizeof(thread_args));
        bounds1->start = 0 ;
        bounds1->end = 0;
        
        thread_args *bounds2 = (thread_args *) malloc(sizeof(thread_args));
        bounds2->start = 1 ;
        bounds2->end = 1;
        
        thread_args *bounds3 = (thread_args *) malloc(sizeof(thread_args));
        bounds3->start = 2 ;
        bounds3->end = 2;
        
        thread_args *bounds4 = (thread_args *) malloc(sizeof(thread_args));
        bounds4->start = 3 ;
        bounds4->end = 3;
        
        err = pthread_create(&tid[0], NULL, &sample_shit, (void *) bounds1);
        err = pthread_create(&tid[1], NULL, &sample_shit, (void *) bounds2);
        err = pthread_create(&tid[2], NULL, &sample_shit, (void *) bounds3);
        err = pthread_create(&tid[3], NULL, &sample_shit, (void *) bounds4);
  }
  
        // wait for all threads to be done
        for(i = 0; i < num_threads; i++) {
                pthread_join(tid[i], NULL);
        }

  // print a list of the frequency of all samples
  h.print();
}
