#pragma once
#include "LuaCallinfo.h"    // for base_ci
#include "LuaObject.h"
#include "LuaVector.h"

/* chain list of long jump buffers */
struct lua_longjmp {
  lua_longjmp *previous;
  int b;
  volatile int status;  /* error code */
};

/*
** `per thread' state
*/
class lua_State : public LuaObject {
public:

  lua_State();
  ~lua_State();

  uint8_t status;
  StkId top;  /* first free slot in the stack */
  global_State *l_G;

  CallInfo callinfo_head_;  /* CallInfo for first level (C calling Lua) */
  CallInfo *ci_;  /* call info for current function */

  const Instruction *oldpc;  /* last pc traced */
  TValue* stack_last;  /* last free slot in the stack */
  LuaVector<TValue> stack;
  unsigned short nonyieldable_count_;  /* number of non-yieldable calls in stack */
  unsigned short nCcalls;  /* number of nested C calls */
  uint8_t hookmask;
  uint8_t allowhook;
  int basehookcount;
  int hookcount;
  lua_Hook hook;
  LuaObject *openupval;  /* list of open upvalues in this stack */
  lua_longjmp *errorJmp;  /* current error recover point */
  ptrdiff_t errfunc;  /* current error handling function (stack index) */

  void freeCI();

  void initstack();
  void freestack();
  int stackinuse();
  void growstack(int size);
  void shrinkstack();
  void reallocstack(int size);
  void checkstack(int size);
  
  void closeUpvals(StkId level);

  TValue at(int idx);
  void   push(TValue v);
  void   push(const TValue* v);
  TValue pop();
};

