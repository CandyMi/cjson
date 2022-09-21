/*
**  LICENSE: BSD
**  Author: CandyMi[https://github.com/candymi]
*/

#define LUA_LIB

#include <core.h>
#include <stdbool.h>
#include <ctype.h>

#ifndef xrio_realloc
  #define xrio_realloc xrealloc
#endif

#ifndef xrio_malloc
  #define xrio_malloc xmalloc
#endif

#ifndef xrio_free
  #define xrio_free xfree
#endif

/* Buffer 实现 */
#define xrio_buffer_size (4096)

typedef struct xrio_Buffer {
  char* b; lua_State *L;
  size_t bidx; size_t blen;
  char ptr[xrio_buffer_size];
} xrio_Buffer;

#define xrio_buffgetidx(B)                (B->bidx)
#define xrio_buffreset(B, idx)            ({B->bidx = idx;})

#define xrio_pushliteral(B, s)            xrio_addlstring((B), (s), strlen(s))
#define xrio_pushstring(B, s)             xrio_addlstring((B), (s), strlen(s))
#define xrio_pushlstring(B, s, l)         xrio_addlstring((B), (s), l)
#define xrio_pushvfstring(B, fmt, argp)   ({xrio_addstring(B, lua_pushvfstring(B->L, fmt, argp)); lua_pop(L, 1);})
#define xrio_pushfstring(B, fmt, ...)     ({xrio_addstring(B, lua_pushfstring(B->L, fmt, ##__VA_ARGS__)); lua_pop(L, 1);})

void xrio_buffinit(lua_State *L, xrio_Buffer *B);
void*xrio_buffinitsize(lua_State *L, xrio_Buffer *B, size_t rsize);

void xrio_pushresult(xrio_Buffer *B);
void xrio_pushresultsize(xrio_Buffer *B, size_t size);

void xrio_addchar(xrio_Buffer *B, char c);
void xrio_addstring(xrio_Buffer *B, const char *b);
void xrio_addlstring(xrio_Buffer *B, const char *b, size_t l);

void xrio_reset(xrio_Buffer *B);

int json_cstring_to_utf8_hex(const char hex[4]);

int json_cstring_to_utf8(char utf8[4], int codepoint);

int ljson_encode(lua_State *L);

int ljson_decode(lua_State *L);
