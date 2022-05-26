#define LUA_LIB

// #include <xrio.h>

#include <core.h>
#include <stdbool.h>
#include <ctype.h>

#ifndef xrio_malloc
  #define xrio_malloc malloc
#endif

#ifndef xrio_free
  #define xrio_free free
#endif

#define json_buffer_size (1024)

int json_cstring_to_utf8_hex(const char *hex);

int json_cstring_to_utf8(char *utf8, int codepoint);

int ljson_encode(lua_State *L);

int ljson_decode(lua_State *L);