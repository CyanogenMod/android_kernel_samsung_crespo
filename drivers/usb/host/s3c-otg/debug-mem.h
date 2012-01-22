#ifndef __DEBUGMEM_H
#define __DEBUGMEM_H

// kevinh quick hack to see if the s3c stuff is doing memory properly

void debug_alloc(void *addr);
void debug_free(void *addr);

#endif
