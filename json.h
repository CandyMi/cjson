#define LUA_LIB

#include <xrio.h>
#include <ctype.h>


int ljson_encode(lua_State *L);

int ljson_decode(lua_State *L);