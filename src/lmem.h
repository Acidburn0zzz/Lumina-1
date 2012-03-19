/*
** $Id: lmem.h,v 1.38 2011/12/02 13:26:54 roberto Exp $
** Interface to Memory Manager
** See Copyright Notice in lua.h
*/

#ifndef lmem_h
#define lmem_h


#include <stddef.h>

#include "llimits.h"
#include "lua.h"

/* memory allocator control variables */
struct Memcontrol {
  Memcontrol();

  bool alloc(size_t size, int type);
  bool free(size_t size, int type);
  bool canAlloc(size_t size);

  void enableLimit();
  void disableLimit();

  bool limitEnabled;
  size_t numblocks;
  size_t total;
  size_t maxmem;
  size_t memlimit;
  size_t objcount[256];
  size_t objcount2[256];
};

extern Memcontrol l_memcontrol;

enum LuaAllocPool {
  LAP_STARTUP,
  LAP_RUNTIME,
  LAP_OBJECT,
  LAP_VECTOR,
};

void* luaM_alloc(size_t size, int pool);
void  luaM_free(void * blob, size_t size, int pool);

void* luaM_newobject(int tag, size_t size);
void  luaM_delobject(void * blob, size_t size, int type);

void* default_alloc   (size_t size, int type, int pool);
void  default_free    (void * blob, size_t size, int type, int pool);

#endif

