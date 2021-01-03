#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream.h>

#define MAX_NEW 50000

static void *cur_mem[MAX_NEW];
static size_t cur_size[MAX_NEW];
static int cur_type[MAX_NEW];

static int next_ent=0;

int new_type = 0;

typedef void (*PFVV)();

extern PFVV _new_handler= 0;

extern PFVV set_new_handler(PFVV handler)
{
  PFVV rr = _new_handler;
  _new_handler = handler;
  return rr;
}

extern void* _last_allocation= 0;

static void regroup()
{
  next_ent = 0;
  for (int i=0; i < MAX_NEW; i++)
    if (cur_mem[i]) {
      if (next_ent != i) {
	cur_mem[next_ent] = cur_mem[i];
	cur_size[next_ent] = cur_size[i];
	cur_type[next_ent] = cur_type[i];

	cur_mem[i] = 0;
	cur_size[i] = 0;
	cur_type[i] = 0;
      }

      next_ent++;
    }

  if (next_ent == MAX_NEW) {
    cerr << "Error: need more mem/size entries" << endl;
    abort();
  }

}


void print_leaks ()
{

  regroup();

  int j = 0;
  for (int i=0; i < next_ent; i++) 
    if ((cur_mem[i]) && (cur_type[i] != 777)) {
      printf ("%d -- size = %d -- address %x \n", 
	      cur_type[i],  cur_size[i], cur_mem[i]);
      j++;
    }
}

extern void* operator new(size_t size)
{
  void *p;

  int nwords= (size % sizeof(int)) ? (size/sizeof(int))+1 : size/sizeof(int) ;
  int new_size= (nwords+2)*sizeof(int);

  while ( (p=malloc(new_size))==0 ) {
    if (_new_handler)
      (*_new_handler)();
    else {
      fprintf(stderr,"Can't get %d bytes memory!\n",new_size);
      return 0;
    }
  }

  *(int*)p= size;
  *(((int*)p) + nwords + 1)= size;

  if (next_ent == MAX_NEW) regroup();
  cur_mem[next_ent] = p;
  cur_size[next_ent] = size;
  cur_type[next_ent++] = new_type;
  
  return _last_allocation= (void *)((int*)p+1);
}

extern void operator delete( void *target )
{
  if (!target) return; // delete of NULL is always harmless
  void *real_start= (void *)(((int*)target) - 1);
  int front_size= *((int *)real_start);
  int nwords= (front_size % sizeof(int)) ?
    (front_size/sizeof(int))+1 : front_size/sizeof(int) ;
  int *last_word= (((int*)real_start) + nwords + 1);
  int back_size= *last_word;

  if (front_size != back_size) {
    if (back_size == -front_size) {
      fprintf(stderr,"Apparently deleted memory at %x (size %d)twice!\n",
              (int)target,front_size);
      abort();
    }
    else {
      fprintf(stderr,
       "Memory block overwritten or otherwise corrupted at %x (size %d)!\n",
              (int)target,front_size);
      abort();
    }
  }
  *last_word= -front_size;
  for (int i=1; i<nwords-1; i++) *((int*)real_start + i)= 0;

  for (int j=0; j < next_ent; j++) 
    if (cur_mem[j] == real_start) {
      cur_mem[j] = 0;
      cur_size[j] = 0;
      cur_type[j] = 0;
      break;
    }
  if (j == next_ent) cerr << "Warning: didn't find memref when freeing" <<endl;

  free( real_start );
}


char *strdup (const char *s)
{

  char *t = new char[strlen(s) + 1];
  strcpy (t, s);

  return t;
}


