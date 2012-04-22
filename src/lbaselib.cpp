/*
** $Id: lbaselib.c,v 1.273 2011/11/30 13:03:24 roberto Exp $
** Basic library
** See Copyright Notice in lua.h
*/

#include "LuaGlobals.h"
#include "LuaState.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lbaselib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "lstate.h" // for THREAD_CHECK



static int luaB_print (lua_State *L) {
  THREAD_CHECK(L);
  int n = L->stack_.getTopIndex();  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    size_t l;
    L->stack_.copy(-1);  /* function to be called */
    L->stack_.copy(i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tolstring(L, -1, &l);  /* get result */
    if (s == NULL)
      return luaL_error(L,
         LUA_QL("tostring") " must return a string to " LUA_QL("print"));
    if (i>1) luai_writestring("\t", 1);
    luai_writestring(s, l);
    L->stack_.pop();  /* pop result */
  }
  luai_writeline();
  return 0;
}


#define SPACECHARS	" \f\n\r\t\v"

static int luaB_tonumber (lua_State *L) {
  THREAD_CHECK(L);
  if (lua_isnoneornil(L, 2)) {  /* standard conversion */
    int isnum;
    lua_Number n = lua_tonumberx(L, 1, &isnum);
    if (isnum) {
      lua_pushnumber(L, n);
      return 1;
    }  /* else not a number; must be something */
    luaL_checkany(L, 1);
  }
  else {
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);
    const char *e = s + l;  /* end point for 's' */
    int base = luaL_checkint(L, 2);
    int neg = 0;
    luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    s += strspn(s, SPACECHARS);  /* skip initial spaces */
    if (*s == '-') { s++; neg = 1; }  /* handle signal */
    else if (*s == '+') s++;
    if (isalnum((unsigned char)*s)) {
      lua_Number n = 0;
      do {
        int digit = (isdigit((unsigned char)*s)) ? *s - '0'
                       : toupper((unsigned char)*s) - 'A' + 10;
        if (digit >= base) break;  /* invalid numeral; force a fail */
        n = n * (lua_Number)base + (lua_Number)digit;
        s++;
      } while (isalnum((unsigned char)*s));
      s += strspn(s, SPACECHARS);  /* skip trailing spaces */
      if (s == e) {  /* no invalid trailing characters? */
        lua_pushnumber(L, (neg) ? -n : n);
        return 1;
      }  /* else not a number */
    }  /* else not a number */
  }
  L->stack_.push(TValue::Nil()); /* not a number */
  return 1;
}


static int luaB_error (lua_State *L) {
  THREAD_CHECK(L);
  int level = luaL_optint(L, 2, 1);
  L->stack_.setTopIndex(1);
  if (lua_isStringable(L, 1) && level > 0) {  /* add extra information? */
    luaL_where(L, level);
    L->stack_.copy(1);
    lua_concat(L, 2);
  }
  return lua_error(L);
}


static int luaB_getmetatable (lua_State *L) {
  THREAD_CHECK(L);
  luaL_checkany(L, 1);
  if (!lua_getmetatable(L, 1)) {
    L->stack_.push(TValue::Nil());
    return 1;  /* no metatable */
  }
  luaL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int luaB_setmetatable (lua_State *L) {
  THREAD_CHECK(L);
  TValue* pv2 = index2addr(L, 2);
  TValue v2 = pv2 ? *pv2 : TValue::Nil();
  luaL_checkIsTable(L, 1);
  luaL_argcheck(L,
                v2.isNil() ||
                v2.isTable(),
                2,
                "nil or table expected");
  if (luaL_getmetafield(L, 1, "__metatable"))
    return luaL_error(L, "cannot change a protected metatable");
  L->stack_.setTopIndex(2);
  lua_setmetatable(L, 1);
  return 1;
}


static int luaB_rawequal (lua_State *L) {
  THREAD_CHECK(L);
  luaL_checkany(L, 1);
  luaL_checkany(L, 2);
  lua_pushboolean(L, lua_rawequal(L, 1, 2));
  return 1;
}


static int luaB_rawlen (lua_State *L) {
  THREAD_CHECK(L);
  int t = lua_type(L, 1);
  luaL_argcheck(L, t == LUA_TTABLE || t == LUA_TSTRING, 1,
                   "table or string expected");
  lua_pushinteger(L, lua_rawlen(L, 1));
  return 1;
}


static int luaB_rawget (lua_State *L) {
  THREAD_CHECK(L);
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  L->stack_.setTopIndex(2);
  lua_rawget(L, 1);
  return 1;
}

static int luaB_rawset (lua_State *L) {
  THREAD_CHECK(L);
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  luaL_checkany(L, 3);
  L->stack_.setTopIndex(3);
  lua_rawset(L, 1);
  return 1;
}


static int luaB_collectgarbage (lua_State *L) {
  THREAD_CHECK(L);
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul",
    "setmajorinc", "isrunning", "generational", "incremental", NULL};
  static const int optsnum[] = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
    LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPAUSE, LUA_GCSETSTEPMUL,
    LUA_GCSETMAJORINC, LUA_GCISRUNNING, LUA_GCGEN, LUA_GCINC};
  int o = optsnum[luaL_checkoption(L, 1, "collect", opts)];
  int ex = luaL_optint(L, 2, 0);
  int res = lua_gc(L, o, ex);
  switch (o) {
    case LUA_GCCOUNT: {
      int b = lua_gc(L, LUA_GCCOUNTB, 0);
      lua_pushnumber(L, res + ((lua_Number)b/1024));
      lua_pushinteger(L, b);
      return 2;
    }
    case LUA_GCSTEP: case LUA_GCISRUNNING: {
      lua_pushboolean(L, res);
      return 1;
    }
    default: {
      lua_pushinteger(L, res);
      return 1;
    }
  }
}


static int luaB_type (lua_State *L) {
  THREAD_CHECK(L);
  luaL_checkany(L, 1);
  lua_pushstring(L, luaL_typename(L, 1));
  return 1;
}


static int pairsmeta (lua_State *L, const char *method, int iszero,
                      lua_CFunction iter) {
  THREAD_CHECK(L);
  if (!luaL_getmetafield(L, 1, method)) {  /* no metamethod? */
    luaL_checktype(L, 1, LUA_TTABLE);  /* argument must be a table */
    lua_pushcclosure(L, iter, 0);  /* will return generator, */
    L->stack_.copy(1);  /* state, */
    if (iszero) lua_pushinteger(L, 0);  /* and initial value */
    else L->stack_.push(TValue::Nil());
  }
  else {
    L->stack_.copy(1);  /* argument 'self' to metamethod */
    lua_call(L, 1, 3);  /* get 3 values from metamethod */
  }
  return 3;
}


static int luaB_next (lua_State *L) {
  THREAD_CHECK(L);
  luaL_checktype(L, 1, LUA_TTABLE);
  L->stack_.setTopIndex(2);  /* create a 2nd argument if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    L->stack_.push(TValue::Nil());
    return 1;
  }
}


static int luaB_pairs (lua_State *L) {
  THREAD_CHECK(L);
  return pairsmeta(L, "__pairs", 0, luaB_next);
}


static int ipairsaux (lua_State *L) {
  THREAD_CHECK(L);
  int i = luaL_checkint(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  i++;  /* next value */
  lua_pushinteger(L, i);
  lua_rawgeti(L, 1, i);
  return (lua_isnil(L, -1)) ? 1 : 2;
}


static int luaB_ipairs (lua_State *L) {
  THREAD_CHECK(L);
  return pairsmeta(L, "__ipairs", 1, ipairsaux);
}


static int load_aux (lua_State *L, int status) {
  THREAD_CHECK(L);
  if (status == LUA_OK)
    return 1;
  else {
    L->stack_.push(TValue::Nil());
    lua_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


static int luaB_loadfile (lua_State *L) {
  THREAD_CHECK(L);
  const char *fname = luaL_optstring(L, 1, NULL);
  const char *mode = luaL_optstring(L, 2, NULL);
  int env = !lua_isnone(L, 3);  /* 'env' parameter? */
  int status = luaL_loadfilex(L, fname, mode);
  if (status == LUA_OK && env) {  /* 'env' parameter? */
    L->stack_.copy(3);
    lua_setupvalue(L, -2, 1);  /* set it as 1st upvalue of loaded chunk */
  }
  return load_aux(L, status);
}


/*
** {======================================================
** Generic Read function
** =======================================================
*/


/*
** reserved slot, above all arguments, to hold a copy of the returned
** string to avoid it being collected while parsed. 'load' has four
** optional arguments (chunk, source name, mode, and environment).
*/
#define RESERVEDSLOT	5


/*
** Reader for generic `load' function: `lua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (lua_State *L, void *ud, size_t *size) {
  THREAD_CHECK(L);
  (void)(ud);  /* not used */
  luaL_checkstack(L, 2, "too many nested functions");
  L->stack_.copy(1);  /* get function */
  lua_call(L, 0, 1);  /* call it */
  if (lua_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (!lua_isStringable(L, -1))
    luaL_error(L, "reader function must return a string");
  lua_replace(L, RESERVEDSLOT);  /* save string in reserved slot */
  return lua_tolstring(L, RESERVEDSLOT, size);
}


static int luaB_load (lua_State *L) {
  THREAD_CHECK(L);
  int status;
  size_t l;
  int top = L->stack_.getTopIndex();
  const char *s = lua_tolstring(L, 1, &l);
  const char *mode = luaL_optstring(L, 3, "bt");
  if (s != NULL) {  /* loading a string? */
    const char *chunkname = luaL_optstring(L, 2, s);
    status = luaL_loadbufferx(L, s, l, chunkname, mode);
  }
  else {  /* loading from a reader function */
    const char *chunkname = luaL_optstring(L, 2, "=(load)");
    luaL_checkIsFunction(L, 1);
    L->stack_.setTopIndex(RESERVEDSLOT);  /* create reserved slot */
    status = lua_load(L, generic_reader, NULL, chunkname, mode);
  }
  if (status == LUA_OK && top >= 4) {  /* is there an 'env' argument */
    L->stack_.copy(4);  /* environment for loaded function */
    lua_setupvalue(L, -2, 1);  /* set it as 1st upvalue */
  }
  return load_aux(L, status);
}

/* }====================================================== */


static int dofilecont (lua_State *L) {
  THREAD_CHECK(L);
  return L->stack_.getTopIndex() - 1;
}


static int luaB_dofile (lua_State *L) {
  THREAD_CHECK(L);
  const char *fname = luaL_optstring(L, 1, NULL);
  L->stack_.setTopIndex(1);
  if (luaL_loadfile(L, fname) != LUA_OK) lua_error(L);
  lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
  return dofilecont(L);
}


static int luaB_assert (lua_State *L) {
  THREAD_CHECK(L);
  if (!lua_toboolean(L, 1))
    return luaL_error(L, "%s", luaL_optstring(L, 2, "assertion failed!"));
  return L->stack_.getTopIndex();
}


static int luaB_select (lua_State *L) {
  THREAD_CHECK(L);
  int n = L->stack_.getTopIndex();
  if (lua_type(L, 1) == LUA_TSTRING && *lua_tostring(L, 1) == '#') {
    lua_pushinteger(L, n-1);
    return 1;
  }
  else {
    int i = luaL_checkint(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    luaL_argcheck(L, 1 <= i, 1, "index out of range");
    return n - i;
  }
}


static int finishpcall (lua_State *L, int status) {
  THREAD_CHECK(L);
  if (!lua_checkstack(L, 1)) {  /* no space for extra boolean? */
    L->stack_.setTopIndex(0);  /* create space for return values */
    lua_pushboolean(L, 0);
    lua_pushstring(L, "stack overflow");
    return 2;  /* return false, msg */
  }
  lua_pushboolean(L, status);  /* first result (status) */
  lua_replace(L, 1);  /* put first result in first slot */
  return L->stack_.getTopIndex();
}


static int pcallcont (lua_State *L) {
  THREAD_CHECK(L);
  int status = lua_getctx(L, NULL);
  return finishpcall(L, (status == LUA_YIELD));
}


static int luaB_pcall (lua_State *L) {
  THREAD_CHECK(L);
  int status;
  luaL_checkany(L, 1);
  L->stack_.push(TValue::Nil());
  lua_insert(L, 1);  /* create space for status result */
  status = lua_pcallk(L, L->stack_.getTopIndex() - 2, LUA_MULTRET, 0, 0, pcallcont);
  return finishpcall(L, (status == LUA_OK));
}


static int luaB_xpcall (lua_State *L) {
  THREAD_CHECK(L);
  int status;
  int n = L->stack_.getTopIndex();
  luaL_argcheck(L, n >= 2, 2, "value expected");
  L->stack_.copy(1);  /* exchange function... */
  lua_copy(L, 2, 1);  /* ...and error handler */
  lua_replace(L, 2);
  status = lua_pcallk(L, n - 2, LUA_MULTRET, 1, 0, pcallcont);
  return finishpcall(L, (status == LUA_OK));
}


static int luaB_tostring (lua_State *L) {
  THREAD_CHECK(L);
  luaL_checkany(L, 1);
  luaL_tolstring(L, 1, NULL);
  return 1;
}


static const luaL_Reg base_funcs[] = {
  {"assert", luaB_assert},
  {"collectgarbage", luaB_collectgarbage},
  {"dofile", luaB_dofile},
  {"error", luaB_error},
  {"getmetatable", luaB_getmetatable},
  {"ipairs", luaB_ipairs},
  {"loadfile", luaB_loadfile},
  {"load", luaB_load},
  {"next", luaB_next},
  {"pairs", luaB_pairs},
  {"pcall", luaB_pcall},
  {"print", luaB_print},
  {"rawequal", luaB_rawequal},
  {"rawlen", luaB_rawlen},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"select", luaB_select},
  {"setmetatable", luaB_setmetatable},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"type", luaB_type},
  {"xpcall", luaB_xpcall},
  {NULL, NULL}
};


int luaopen_base (lua_State *L) {
  THREAD_CHECK(L);
  /* set global _G */
  lua_pushglobaltable(L);
  lua_pushglobaltable(L);
  lua_setfield(L, -2, "_G");
  /* open lib into global table */
  luaL_setfuncs(L, base_funcs, 0);
  lua_pushliteral(L, LUA_VERSION);
  lua_setfield(L, -2, "_VERSION");  /* set global _VERSION */
  return 1;
}

