#include "json.h"

static inline void xrio_resize(xrio_Buffer *B, size_t rsize) {
  /* use stack to fast handle all string. */
  if (rsize <= xrio_buffer_size) {
    B->blen = xrio_buffer_size;
    B->b = B->ptr;
    return ;
  }
  if (B->b == B->ptr)  /* Using heap for more string buffer. */
    B->b = memcpy(xrio_realloc(NULL, rsize), B->ptr, B->bidx);
  else                 /* Using `realloc` to got more memory. */
    B->b = xrio_realloc(B->b, rsize);
  B->blen = rsize;
}

void xrio_buffinit(lua_State *L, xrio_Buffer *B) {
  xrio_buffinitsize(L, B, xrio_buffer_size);
}

void* xrio_buffinitsize(lua_State *L, xrio_Buffer *B, size_t rsize) {
  B->b = NULL; B->bidx = 0; B->L = L; B->blen = 0;
  xrio_resize(B, rsize);
  return B->b;
}

void xrio_pushresult(xrio_Buffer *B) {
  if (B->L)
    lua_pushlstring(B->L, B->b, B->bidx);
  if (B->b && B->b != B->ptr)
    xrio_realloc(B->b, 0);
  B->b = NULL; B->L = NULL;
}

void xrio_addchar(xrio_Buffer *B, char c) {
  if (B->bidx + 1 >= B->blen)
    xrio_resize(B, B->blen << 1);
  B->b[B->bidx++] = c;
}

void xrio_addlstring(xrio_Buffer *B, const char *b, size_t len) {
  if (len > 0)
  {
    if (len == 1)
      return xrio_addchar(B, b[0]);
    if (B->bidx + len >= B->blen)
    {
      size_t nsize = B->bidx + len;
      size_t blen = B->blen << 1;
      while (nsize > blen)
        blen <<= 1;
      xrio_resize(B, blen);
    }
    memcpy(B->b + B->bidx, b, len);
    B->bidx += len;
  }
}

void xrio_addstring(xrio_Buffer *B, const char *b) {
  xrio_addlstring(B, b, strlen(b));
}


