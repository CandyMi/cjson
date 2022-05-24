#define LUA_LIB

#include <xrio.h>
#include <ctype.h>

#ifndef xrio_malloc
  #define xrio_malloc malloc
#endif

#ifndef xrio_free
  #define xrio_free free
#endif

int ljson_encode(lua_State *L);

int ljson_decode(lua_State *L);
