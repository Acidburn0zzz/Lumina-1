#pragma once
#include "LuaObject.h"
#include "LuaVector.h"

/*
** Header for string value; string bytes follow the end of this structure
*/
__declspec(align(8)) class TString : public LuaObject {
public:

  //TString(int type, LuaObject** list) : LuaObject(type,list) {}
  TString(LuaObject** list) : LuaObject(LUA_TSTRING, list) {
    buf_ = NULL;
    reserved_ = 0;
    hash_ = 0;
    len_ = 0;
  }

  ~TString() {
    luaM_free(buf_);
    buf_ = NULL;
    len_ = NULL;
  }

  size_t getLen() const { return len_; }
  void setLen(size_t len) { len_ = l; }

  void setBuf(char* buf) {
    buf_ = buf;
  }

  const char * c_str() const {
    return buf_;
  }

  void setText(const char * str, size_t len) {
    len_ = len;
    memcpy(buf_, str, len*sizeof(char));
    buf_[len_] = '\0'; // terminating null
  }

  uint32_t getHash() const { return hash; }
  void setHash(uint32_t h) { hash = h; }

  uint8_t getReserved() const { return reserved; }
  void setReserved(uint8_t r) { reserved = r; }

protected:

  char* buf_;
  uint8_t reserved;
  uint32_t hash;
  size_t len;  /* number of characters in string */

};

class stringtable {
public:
  //LuaObject **hash;
  LuaVector<LuaObject*> hash;
  uint32_t nuse;  /* number of elements */
  int size;
};
