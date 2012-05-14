/*
** $Id: llex.h,v 1.72 2011/11/30 12:43:51 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#ifndef llex_h
#define llex_h

#include<vector>

#include "lobject.h"
#include "lzio.h"


#define FIRST_RESERVED	257



/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_AND = FIRST_RESERVED, TK_BREAK,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
  /* other terminal symbols */
  TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE, TK_DBCOLON, TK_EOS,
  TK_NUMBER, TK_NAME, TK_STRING
};

/* number of reserved words */
#define NUM_RESERVED	(cast(int, TK_WHILE-FIRST_RESERVED+1))

class LexState;

struct Token {
 
  Token()
  : reserved_(0) {
  }

  int token;
  double r;

  void setString(const char* s, size_t len);

  int getReserved();

  const char* c_str() {
    return text_.c_str();
  }

  size_t getLen() {
    return text_.size();
  }

protected:

  std::string text_;
  int reserved_;
};

class FuncState;

/* state of the lexer plus state of the parser when shared by all
   functions */
class LexState {
public:

  LexState() {
    buff_.reserve(128);
  }

  int current_;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token `consumed' */

  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  FuncState *fs;  /* current function (parser) */
  LuaThread *L;
  Zio *z;  /* input stream */
  // buffer for tokens
  std::vector<char> buff_;

  struct Dyndata *dyd;  /* dynamic structures used by the parser */
  LuaString *source;  /* current source name */
  LuaString *envn;  /* environment variable name */
  char decpoint;  /* locale decimal point */
};


void luaX_setinput (LuaThread *L, LexState *ls, Zio *z,
                              LuaString *source, int firstchar);
LuaString *luaX_newstring (LexState *ls, const char *str, size_t l);
LuaResult luaX_next (LexState *ls);
LuaResult luaX_lookahead (LexState *ls, int& out);
LuaResult luaX_syntaxerror (LexState *ls, const char *s);
const char *luaX_token2str (LexState *ls, int token);


#endif
