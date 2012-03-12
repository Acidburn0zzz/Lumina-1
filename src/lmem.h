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

/* +1 to avoid warnings */
/*
#define luaM_reallocv(L,b,on,n,e) \
	((cast(size_t, (n)+1) > MAX_SIZET/(e)) ?   \
		(luaM_toobig(L), (void *)0) : \
		luaM_realloc_(L, (b), (on)*(e), (n)*(e)))
*/

void* luaM_reallocv(lua_State* L, void* block, size_t osize, size_t nsize, size_t esize);

void luaM_freemem(lua_State* L, void * blob, size_t size);


#define luaM_free(L, b)		luaM_realloc_(L, (b), sizeof(*(b)), 0)
#define luaM_freearray(L, b, n)   luaM_reallocv(L, (b), n, 0, sizeof((b)[0]))

#define luaM_malloc(L,s)	luaM_realloc_(L, NULL, 0, (s))
#define luaM_new(L,t)		cast(t *, luaM_malloc(L, sizeof(t)))
#define luaM_newvector(L,n,t) cast(t *, luaM_reallocv(L, NULL, 0, n, sizeof(t)))

void* luaM_newobject(lua_State* L, int tag, size_t size);
//#define luaM_newobject(L,tag,s)	luaM_realloc_(L, NULL, tag, (s))

#define luaM_growvector(L,v,nelems,size,t,limit,e) \
          if ((nelems)+1 > (size)) \
            ((v)=cast(t *, luaM_growaux_(L,v,&(size),sizeof(t),limit,e)))

#define luaM_reallocvector(L, v,oldn,n,t) ((v)=cast(t *, luaM_reallocv(L, v, oldn, n, sizeof(t))))

template<class T>
void luaM_reallocvector2(lua_State* L, T*& v, size_t oldn, size_t newn) {
  v = (T*)luaM_reallocv(L, v, oldn, newn, sizeof(T));
}

l_noret luaM_toobig (lua_State *L);

/* not to be called directly */
void *luaM_realloc_ (lua_State *L, void *block, size_t oldsize,
                                                          size_t size);
void *luaM_growaux_ (lua_State *L, void *block, int *size,
                               size_t size_elem, int limit,
                               const char *what);

void* default_alloc(void *ptr, size_t osize, size_t nsize);

#endif

