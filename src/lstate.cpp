/*
** $Id: lstate.c,v 2.92 2011/10/03 17:54:25 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/


#include <stddef.h>

#define lstate_c
#define LUA_CORE

#include "lua.h"

#include "lapi.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "llex.h"
#include "lmem.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"


#if !defined(LUAI_GCPAUSE)
#define LUAI_GCPAUSE	200  /* 200% */
#endif

#if !defined(LUAI_GCMAJOR)
#define LUAI_GCMAJOR	200  /* 200% */
#endif

#if !defined(LUAI_GCMUL)
#define LUAI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */
#endif


#define MEMERRMSG       "not enough memory"

/*
** set GCdebt to a new value keeping the value (totalbytes + GCdebt)
** invariant
*/
void luaE_setdebt (global_State *g, l_mem debt) {
  g->totalbytes -= (debt - g->GCdebt);
  g->GCdebt = debt;
}


CallInfo *luaE_extendCI (lua_State *L) {
  THREAD_CHECK(L);
  CallInfo *ci = (CallInfo*)luaM_alloc(sizeof(CallInfo));
  assert(L->ci->next == NULL);
  L->ci->next = ci;
  ci->previous = L->ci;
  ci->next = NULL;
  return ci;
}


void luaE_freeCI (lua_State *L) {
  THREAD_CHECK(L);
  CallInfo *ci = L->ci;
  CallInfo *next = ci->next;
  ci->next = NULL;
  while ((ci = next) != NULL) {
    next = ci->next;
    luaM_free(ci, sizeof(CallInfo));
  }
}


static void stack_init (lua_State *L1, lua_State *L) {
  THREAD_CHECK(L);
  int i; CallInfo *ci;
  /* initialize stack array */
  L1->stack = (TValue*)luaM_allocv(BASIC_STACK_SIZE, sizeof(TValue));
  L1->stacksize = BASIC_STACK_SIZE;
  for (i = 0; i < BASIC_STACK_SIZE; i++)
    setnilvalue(L1->stack + i);  /* erase new stack */
  L1->top = L1->stack;
  L1->stack_last = L1->stack + L1->stacksize - EXTRA_STACK;
  /* initialize first ci */
  ci = &L1->base_ci;
  ci->next = ci->previous = NULL;
  ci->callstatus = 0;
  ci->func = L1->top;
  setnilvalue(L1->top++);  /* 'function' entry for this 'ci' */
  ci->top = L1->top + LUA_MINSTACK;
  L1->ci = ci;
}


static void freestack (lua_State *L) {
  THREAD_CHECK(L);
  if (L->stack == NULL)
    return;  /* stack not completely built yet */
  L->ci = &L->base_ci;  /* free the entire 'ci' list */
  luaE_freeCI(L);
  luaM_free(L->stack, L->stacksize * sizeof(TValue));
}


/*
** Create registry table and its predefined values
*/
static void init_registry (lua_State *L, global_State *g) {
  THREAD_CHECK(L);
  TValue mt;
  /* create registry */
  Table *registry = luaH_new();
  sethvalue(L, &g->l_registry, registry);
  luaH_resize(registry, LUA_RIDX_LAST, 0);
  /* registry[LUA_RIDX_MAINTHREAD] = L */
  setthvalue(L, &mt, L);
  luaH_setint(registry, LUA_RIDX_MAINTHREAD, &mt);
  /* registry[LUA_RIDX_GLOBALS] = table of globals */
  sethvalue(L, &mt, luaH_new());
  luaH_setint(registry, LUA_RIDX_GLOBALS, &mt);
}


/*
** open parts of the state that may cause memory-allocation errors
*/
static void f_luaopen (lua_State *L, void *ud) {
  THREAD_CHECK(L);
  global_State *g = G(L);
  UNUSED(ud);
  stack_init(L, L);  /* init stack */
  init_registry(L, g);
  luaS_resize(L, MINSTRTABSIZE);  /* initial size of string table */
  luaT_init(L);
  luaX_init(L);
  /* pre-create memory-error message */
  g->memerrmsg = luaS_newliteral(L, MEMERRMSG);
  luaS_fix(g->memerrmsg);  /* it should never be collected */
  g->gcrunning = 1;  /* allow gc */
}


/*
** preinitialize a state with consistent values without allocating
** any memory (to avoid errors)
*/
static void preinit_state (lua_State *L, global_State *g) {
  //THREAD_CHECK(L);
  G(L) = g;
  L->stack = NULL;
  L->ci = NULL;
  L->stacksize = 0;
  L->errorJmp = NULL;
  L->nCcalls = 0;
  L->hook = NULL;
  L->hookmask = 0;
  L->basehookcount = 0;
  L->allowhook = 1;
  L->hookcount = L->basehookcount;
  L->openupval = NULL;
  L->nny = 1;
  L->status = LUA_OK;
  L->errfunc = 0;
}


static void close_state (lua_State *L) {
  THREAD_CHECK(L);
  global_State *g = G(L);
  luaF_close(L, L->stack);  /* close all upvalues for this thread */
  luaC_freeallobjects(L);  /* collect all objects */
  luaS_freestrt(L, G(L)->strt);
  delete G(L)->strt;
  G(L)->strt = NULL;

	g->buff.buffer = (char*)luaM_reallocv(g->buff.buffer, g->buff.buffsize, 0, sizeof(char));
	g->buff.buffsize = 0;

  freestack(L);
  assert(gettotalbytes(g) == (sizeof(lua_State) + sizeof(global_State)));
  default_realloc(g, sizeof(global_State), 0, 0);
  L->l_G = NULL;
  default_realloc(L, sizeof(lua_State), 0, 0);  /* free main block */
}


lua_State *lua_newthread (lua_State *L) {
  THREAD_CHECK(L);
  lua_State *L1;
  lua_lock(L);
  luaC_checkGC(L);
  LuaBase* o = luaC_newobj(LUA_TTHREAD, sizeof(lua_State), NULL);
  L1 = gco2th(o);
  setthvalue(L, L->top, L1);
  api_incr_top(L);
  preinit_state(L1, G(L));
  L1->hookmask = L->hookmask;
  L1->basehookcount = L->basehookcount;
  L1->hook = L->hook;
  L1->hookcount = L1->basehookcount;
  stack_init(L1, L);  /* init stack */
  lua_unlock(L);
  return L1;
}


void luaE_freethread (lua_State *L, lua_State *L1) {
  THREAD_CHECK(L);
  {
    THREAD_CHANGE(L1);
    luaF_close(L1, L1->stack);  /* close all upvalues for this thread */
    assert(L1->openupval == NULL);
    freestack(L1);
  }
  luaM_free(L1, sizeof(lua_State));
}


lua_State *lua_newstate () {
  int i;
  lua_State *L;
  global_State *g;
  L = (lua_State*)default_realloc(NULL, LUA_TTHREAD, sizeof(lua_State), 0);
  if(L == NULL) { return NULL; }
  g = (global_State*)default_realloc(NULL, 0, sizeof(global_State), 0);
  if(g == NULL) {
    default_realloc(L, sizeof(lua_State), 0, 0);
    return NULL;
  }
  L->next = NULL;
  L->tt = LUA_TTHREAD;
  g->currentwhite = bit2mask(WHITE0BIT, FIXEDBIT);
  L->marked = luaC_white(g);
  g->gckind = KGC_NORMAL;
  preinit_state(L, g);
  g->mainthread = L;
  g->uvhead.uprev = &g->uvhead;
  g->uvhead.unext = &g->uvhead;
  g->gcrunning = 0;  /* no GC while building state */
  g->lastmajormem = 0;
  g->strt = new stringtable();
  luaS_initstrt(g->strt);
  setnilvalue(&g->l_registry);
  luaZ_initbuffer(L, &g->buff);
  g->panic = NULL;
  g->version = lua_version(NULL);
  g->gcstate = GCSpause;
  g->allgc = NULL;
  g->finobj = NULL;
  g->tobefnz = NULL;
  g->gray = g->grayagain = NULL;
  g->weak = g->ephemeron = g->allweak = NULL;
  g->totalbytes = sizeof(lua_State) + sizeof(global_State);
  g->GCdebt = 0;
  g->gcpause = LUAI_GCPAUSE;
  g->gcmajorinc = LUAI_GCMAJOR;
  g->gcstepmul = LUAI_GCMUL;
  {
    GLOBAL_CHANGE(L);
    for (i=0; i < LUA_NUMTAGS; i++) g->mt[i] = NULL;
    {
      if (luaD_rawrunprotected(L, f_luaopen, NULL) != LUA_OK) {
        /* memory allocation error: free partial state */
        close_state(L);
        L = NULL;
      }
    }
  }
  return L;
}


void lua_close (lua_State *L) {
  THREAD_CHECK(L);
  L = G(L)->mainthread;  /* only the main thread can be closed */
  lua_lock(L);
  close_state(L);
}


