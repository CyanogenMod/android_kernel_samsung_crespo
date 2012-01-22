#include "s3c-otg-hcdi-hcd.h" 
#include "debug-mem.h"

#define MAX_ALLOCS 1024

typedef void *ElemType;

static ElemType alloced[MAX_ALLOCS];
static int numalloced = 0;

#define fail(args...) ( printk(args), dump_stack() )

void debug_alloc(void *addr) {
  ElemType *freeloc = NULL;
  ElemType *c = alloced;
  int i;

    if(!addr)
      fail("MEMD alloc of NULL");

  for(i = 0; i < numalloced; i++, c++) {
    if(*c == NULL && freeloc == NULL)
      freeloc = c;
    else if(*c == addr)
      fail("MEMD multiple allocs of %p", addr);
  }

  if(freeloc)
    *freeloc = addr;
  else {
    if(numalloced >= MAX_ALLOCS)
      fail("MEMD too many allocs");
    else {
      alloced[numalloced++] = addr;
    }
  }
}

void debug_free(void *addr) {
  ElemType *c = alloced;
  int i;

    if(!addr)
      fail("free of NULL");

  for(i = 0; i < numalloced; i++, c++) {
    if(*c == addr) {
      *c = NULL;
      return;
    }
  }

  fail("MEMD freed addr %p was never alloced\n", addr);
}

