#include "LuaGlobals.h"

#include "LuaState.h"
#include "LuaString.h"
#include "LuaTable.h"

#define LUAI_GCPAUSE	200  /* 200% */
#define LUAI_GCMAJOR	200  /* 200% */
#define LUAI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */
#define MINSTRTABSIZE	32

#define MEMERRMSG       "not enough memory"

extern const char* luaX_tokens[];
extern int luaX_tokens_count;

const lua_Number *lua_version (lua_State *L);

/* ORDER TM */
const char *const gk_tagmethod_names[] = {
  "__index",
  "__newindex",
  "__gc",
  "__mode",
  "__len",
  "__eq",
  "__add",
  "__sub",
  "__mul",
  "__div",
  "__mod",
  "__pow",
  "__unm",
  "__lt",
  "__le",
  "__concat",
  "__call"
};

//------------------------------------------------------------------------------

LuaObject** getGlobalGCHead() {
  return &thread_G->allgc;
}

global_State::global_State()
: uvhead(NULL)
{
  GCdebt_ = 0;
  isShuttingDown = false;
  totalbytes_ = sizeof(global_State);
  call_depth_ = 0;

  livecolor = LuaObject::colorA;
  deadcolor = LuaObject::colorB;

  uvhead.uprev = &uvhead;
  uvhead.unext = &uvhead;

  gcstate = GCSpause;
  gckind = KGC_NORMAL;
  gcrunning = 0;  /* no GC while building state */
  gcpause = LUAI_GCPAUSE;
  gcmajorinc = LUAI_GCMAJOR;
  gcstepmul = LUAI_GCMUL;
  lastmajormem = 0;

  allgc = NULL;
  sweepgc = NULL;
  finobj = NULL;

  panic = NULL;
  version = lua_version(NULL);

  anchor_head_ = NULL;
  anchor_tail_ = NULL;

  strings_ = NULL;
  mainthread = NULL;
  memerrmsg = NULL;

  memset(tagmethod_names_,0,sizeof(tagmethod_names_));
  memset(base_metatables_,0,sizeof(base_metatables_));
}

global_State::~global_State() {
  assert(anchor_head_ == NULL);
  assert(anchor_tail_ == NULL);
}

void global_State::init(lua_State* mainthread2) {

  // Create global registry.
  Table* registry = new Table(LUA_RIDX_LAST, 0);
  l_registry = TValue(registry);

  // Create global variable table.
  Table* globals = new Table();
  registry->set(TValue(LUA_RIDX_GLOBALS), TValue(globals));

  // Store main thread in the registry.
  mainthread = mainthread2;
  registry->set(TValue(LUA_RIDX_MAINTHREAD), TValue(mainthread));

  // Create global string table.
  strings_ = new stringtable();
  strings_->Resize(MINSTRTABSIZE);  /* initial size of string table */

  // Create memory error message string.
  memerrmsg = strings_->Create(MEMERRMSG);
  memerrmsg->setFixed();

  // Create tagmethod name strings.
  int tm_count = sizeof(gk_tagmethod_names) / sizeof(gk_tagmethod_names[0]);
  for (int i=0; i < tm_count; i++) {
    tagmethod_names_[i] = strings_->Create(gk_tagmethod_names[i]);
    tagmethod_names_[i]->setFixed();
  }

  // Create lexer reserved word strings.
  for (int i = 0; i < luaX_tokens_count; i++) {
    TString *ts = strings_->Create(luaX_tokens[i]);
    ts->setFixed();
    ts->setReserved(i+1);
  }
}

void global_State::setGCDebt(size_t debt) {
  GCdebt_ = debt;
}

void global_State::incTotalBytes(int bytes) {
  totalbytes_ += bytes;
}

void global_State::incGCDebt(int debt) { 
  GCdebt_ += debt;
}

void global_State::PushGray(LuaObject* o) {
  grayhead_.Push(o);
}

void global_State::PushGrayAgain(LuaObject* o) {
  grayagain_.Push(o);
}

void global_State::PushWeak(LuaObject* o) {
  weak_.Push(o);
}

void global_State::PushAllWeak(LuaObject* o) {
  allweak_.Push(o);
}

void global_State::PushEphemeron(LuaObject* o) {
  ephemeron_.Push(o);
}


void global_State::markObject(LuaObject *o) {
  GCVisitor visitor;
  visitor.MarkObject(o);
}

void global_State::markValue(TValue* v) {
  if(v->isCollectable()) {
    markObject(v->getObject());
  }
}

