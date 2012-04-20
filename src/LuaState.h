#pragma once
#include "LuaCallinfo.h"    // for base_ci
#include "LuaObject.h"
#include "LuaStack.h"

struct LuaExecutionState {
  CallInfo*  callinfo_;
  uint8_t    allowhook;
  uint16_t   nonyieldable_count_;
  ptrdiff_t  errfunc;
  ptrdiff_t  old_top;
};

/*
** `per thread' state
*/
class lua_State : public LuaObject {
public:

  lua_State();
  ~lua_State();

  virtual void VisitGC(GCVisitor& visitor);
  virtual int PropagateGC(GCVisitor& visitor);

  LuaExecutionState saveState(ptrdiff_t old_top);
  void restoreState(LuaExecutionState s, int status);

  uint8_t status;
  global_State *l_G;

  const Instruction *oldpc;  /* last pc traced */

  LuaStack stack_;

  unsigned short nonyieldable_count_;  /* number of non-yieldable calls in stack */

  uint8_t hookmask;
  uint8_t allowhook;
  int basehookcount;
  int hookcount;
  lua_Hook hook;
  
  ptrdiff_t errfunc;  /* current error handling function (stack index) */

  void closeUpvals(StkId level);
};

struct ScopedCallDepth
{
  ScopedCallDepth(lua_State* state);
  ~ScopedCallDepth();

  lua_State* state_;
};

