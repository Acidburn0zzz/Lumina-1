#include "LuaObject.h"

#include "lgc.h"
#include "lstate.h"

#include "LuaGlobals.h"

#define BLACKBIT	2  /* object is black */
#define FINALIZEDBIT	3  /* object has been separated for finalization */
#define SEPARATED	4  /* object is in 'finobj' list or in 'tobefnz' */
#define OLDBIT		6  /* object is old (only in generational mode) */
#define TESTGRAYBIT		7 // bit 7 is currently used by tests (luaL_checkmemory)
#define FIXEDBIT	5  /* object is fixed (should not be collected) */


/* Layout for bit use in `marked' field: */
#define WHITE0BIT	0  /* object is white (type 0) */
#define WHITE1BIT	1  /* object is white (type 1) */
#define WHITEBITS	((1 << WHITE0BIT) | (1 << WHITE1BIT))

const int LuaObject::colorA = (1 << WHITE0BIT);
const int LuaObject::colorB = (1 << WHITE1BIT);

void *luaM_alloc_ (size_t size, int type, int pool);

int LuaObject::instanceCounts[256];

LuaObject::LuaObject(LuaType type) {

  next = NULL;
  flags_ = 0;
  color_ = 0;
  if(thread_G) flags_ = thread_G->livecolor;
  type_ = type;

  LuaObject::instanceCounts[type_]++;
}

LuaObject::~LuaObject() {
  LuaObject::instanceCounts[type_]--;
}

void LuaObject::linkGC(LuaObject*& gcHead) {
  assert(next == NULL);
  next = gcHead;
  gcHead = this;
}

// Sanity check object state
void LuaObject::sanityCheck() {
  assert((flags_ & WHITEBITS) != WHITEBITS);
}

uint8_t LuaObject::getFlags() {
  return flags_;
}

bool LuaObject::isDead() {  
  if(isFixed()) return false;
  if((flags_ & WHITEBITS) == 0) return false;
  return !(flags_ & thread_G->livecolor);
}

bool LuaObject::isWhite() {
  return flags_ & ((1 << WHITE0BIT) | (1 << WHITE1BIT)) ? true : false;
}

bool LuaObject::isGray() {
  return !isBlack() && !isWhite();
}

// Clear existing color + old bits, set color to current white.
void LuaObject::setWhite() {
  uint8_t mask = (1 << OLDBIT) | (1 << BLACKBIT) | (1 << WHITE0BIT) | (1 << WHITE1BIT);
  flags_ &= ~mask;
  flags_ |= (thread_G->livecolor);
}

void LuaObject::changeWhite() {
  flags_ ^= WHITEBITS;
}

void LuaObject::whiteToGray() {
  flags_ &= ~WHITEBITS;
}

void LuaObject::blackToGray() {
  clearBlack();
}

void LuaObject::stringmark() {
  flags_ &= ~WHITEBITS;
}

void LuaObject::grayToBlack() {
  flags_ |= (1 << BLACKBIT);
}

bool LuaObject::isBlack()        { return flags_ & (1 << BLACKBIT) ? true : false; }
void LuaObject::setBlack()       { flags_ |= (1 << BLACKBIT); }
void LuaObject::clearBlack()     { flags_ &= ~(1 << BLACKBIT); }

bool LuaObject::isFinalized()    { return flags_ & (1 << FINALIZEDBIT) ? true : false; }
void LuaObject::setFinalized()   { flags_ |= (1 << FINALIZEDBIT); }
void LuaObject::clearFinalized() { flags_ &= ~(1 << FINALIZEDBIT); }

// TODO(aappleby): change to SEPARATEDBIT
bool LuaObject::isSeparated()    { return flags_ & (1 << SEPARATED) ? true : false; }
void LuaObject::setSeparated()   { flags_ |= (1 << SEPARATED); }
void LuaObject::clearSeparated() { flags_ &= ~(1 << SEPARATED); }

bool LuaObject::isFixed()        { return flags_ & (1 << FIXEDBIT) ? true : false; }
void LuaObject::setFixed()       { flags_ |= (1 << FIXEDBIT); }
void LuaObject::clearFixed()     { flags_ &= ~(1 << FIXEDBIT); }

/* MOVE OLD rule: whenever an object is moved to the beginning of
   a GC list, its old bit must be cleared */
bool LuaObject::isOld()          { return flags_ & (1 << OLDBIT) ? true : false; }
void LuaObject::setOld()         { flags_ |= (1 << OLDBIT); }
void LuaObject::clearOld()       { flags_ &= ~(1 << OLDBIT); }

bool LuaObject::isTestGray()     { return flags_ & (1 << TESTGRAYBIT) ? true : false; }
void LuaObject::setTestGray()    { flags_ |= (1 << TESTGRAYBIT); }
void LuaObject::clearTestGray()  { flags_ &= ~(1 << TESTGRAYBIT); }

extern char** luaT_typenames;
const char * LuaObject::typeName() const {
  return luaT_typenames[type_+1];
}