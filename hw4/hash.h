
#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include <pthread.h>
#include "list.h"

#define NUM_HASH_LISTS		16384

#define HASH_INDEX(_addr,_size_mask) (((_addr) >> 2) & (_size_mask))

pthread_mutex_t list_lock[NUM_HASH_LISTS];

template<class Ele, class Keytype> class hash;

template<class Ele, class Keytype> class hash {
 private:
  unsigned my_size_log;
  unsigned my_size;
  unsigned my_size_mask;
  list<Ele,Keytype> *entries;
  list<Ele,Keytype> *get_list(unsigned the_idx);

 public:
  void setup(unsigned the_size_log=5);
  void insert(Ele *e);
  Ele *lookup(Keytype the_key);
  void print(FILE *f=stdout);
  void reset();
  void cleanup();
  void locklist(Keytype k);
  void unlocklist(Keytype k);
  void join(hash* h);
};

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::setup(unsigned the_size_log){
  my_size_log = the_size_log;
  my_size = 1 << my_size_log;
  my_size_mask = (1 << my_size_log) - 1;
  entries = new list<Ele,Keytype>[my_size];
}

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::locklist(Keytype k){
	pthread_mutex_lock(&list_lock[HASH_INDEX(k, my_size_mask)]);
}

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::unlocklist(Keytype k){
	pthread_mutex_unlock(&list_lock[HASH_INDEX(k, my_size_mask)]);
}

template<class Ele, class Keytype> 
list<Ele,Keytype> *
hash<Ele,Keytype>::get_list(unsigned the_idx){
  if (the_idx >= my_size){
    fprintf(stderr,"hash<Ele,Keytype>::list() public idx out of range!\n");
    exit (1);
  }
  return &entries[the_idx];
}

template<class Ele, class Keytype> 
Ele *
hash<Ele,Keytype>::lookup(Keytype the_key){
  list<Ele,Keytype> *l;

  l = &entries[HASH_INDEX(the_key,my_size_mask)];
  return l->lookup(the_key);
}  

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::print(FILE *f){
  unsigned i;

  for (i=0;i<my_size;i++){
    entries[i].print(f);
  }
}

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::reset(){
  unsigned i;
  for (i=0;i<my_size;i++){
    entries[i].cleanup();
  }
}

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::cleanup(){
  unsigned i;
  reset();
  delete [] entries;
}

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::insert(Ele *e){
  entries[HASH_INDEX(e->key(),my_size_mask)].push(e);
}

template<class Ele, class Keytype> 
void 
hash<Ele,Keytype>::join(hash *h){
  unsigned i;
  Ele *e;
  for (i=0; i < my_size;i++){
    while ( (e = h->get_list(i)->pop()) != NULL)
      entries[HASH_INDEX(e->key(),my_size_mask)].push(e);
  }
}

#endif
