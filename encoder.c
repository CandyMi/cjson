#include "json.h"

/* json mode */
#define j_table (0)
#define j_array (1)

static char json_start[2] = {'{', '['};
static char json_over[2]  = {'}', ']'};

#define json_block_start(B, mode) xrio_addchar(B, json_start[mode])
#define json_block_over(B, mode)  xrio_addchar(B, json_over[mode])

static inline int json_encode_table(lua_State *L, int mode, xrio_Buffer *B);

/* 参考查表 */
static const char *char2escape[256] = {
  "\\u0000", "\\u0001", "\\u0002", "\\u0003",
  "\\u0004", "\\u0005", "\\u0006", "\\u0007",
  "\\b", "\\t", "\\n", "\\u000b",
  "\\f", "\\r", "\\u000e", "\\u000f",
  "\\u0010", "\\u0011", "\\u0012", "\\u0013",
  "\\u0014", "\\u0015", "\\u0016", "\\u0017",
  "\\u0018", "\\u0019", "\\u001a", "\\u001b",
  "\\u001c", "\\u001d", "\\u001e", "\\u001f",
  NULL, NULL, "\\\"", NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\/",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, "\\\\", NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\u007f",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static inline void json_pushstring(lua_State *L, xrio_Buffer *B, int idx, int mode) {
  size_t bsize;
  const char* buffer = lua_tolstring(L, idx, &bsize);
  if (!bsize) {
    if (mode)
      xrio_pushliteral(B, "\"\":");
    else
      xrio_pushliteral(B, "\"\"");
    return ;
  }

  const char *escstr = NULL;
  xrio_addchar(B, '"');
  for (size_t i = 0; i < bsize; i++)
  {
    int code = buffer[i];
    escstr = char2escape[(unsigned char)code];
    if (!escstr)
      xrio_addchar(B, code);
    else
      xrio_addstring(B, escstr);
  }
  if (mode)
    xrio_pushliteral(B, "\":");
  else
    xrio_addchar(B, '"');
}

static inline int json_encode_table(lua_State *L, int mode, xrio_Buffer *B) {
  int idx  = lua_gettop(L);
  int kidx = idx + 1;
  int vidx = idx + 2;
  uint32_t times = 0;

  /* 数组类型 */
  if (lua_rawlen(L, idx) > 0)
    mode = j_array;

  /* 如果被`json_array_mt`标记 */
  if (mode == j_table && lua_getmetatable(L, idx)) {
    luaL_getmetatable(L, "json_array_mt");
    if (lua_rawequal(L, -1 , -2))
      mode = j_array;
    lua_pop(L, 2);
  }

  size_t pos = xrio_buffgetidx(B);

  lua_pushnil(L);
  while(lua_next(L, idx))
  {
    int ktype = lua_type(L, kidx); int vtype = lua_type(L, vidx);

    if (times++) {
      xrio_addchar(B, ',');
    } else {
      if (mode == j_array && (ktype != LUA_TNUMBER || lua_tointeger(L, kidx) != times))
        mode = j_table;
      json_block_start(B, mode);
    }

    if (mode == j_table) {
      switch (ktype)
      {
        case LUA_TNUMBER:
          if (lua_isinteger(L, kidx))
            xrio_pushfstring(B, "\"%I\":", lua_tointeger(L, kidx));
          else {
            lua_Number n = lua_tonumber(L, kidx);
            if (isnan(n) || isinf(n)) {
              xrio_reset(B);
              return luaL_error(L, "Cannot serialise number: must not be NaN or Infinity");
            }
            xrio_pushfstring(B, "\"%f\":", n);
          }
          break;
        case LUA_TSTRING:
          json_pushstring(L, B, kidx, 1);
          break;
        default:
          xrio_reset(B);
          luaL_error(L, "[json encode]: Invalid key type `%s`.", lua_typename(L, ktype));
      }
    } else {
      /* 如果检查到稀疏数组则转为哈希表达, 指针回溯并丢弃所有已有数据 */
      if (ktype == LUA_TNUMBER && lua_isinteger(L, kidx) && lua_tointeger(L, kidx) != times) {
        mode = j_table; times = 0; xrio_buffreset(B, pos);
        lua_pop(L, 2); lua_pushnil(L);
        continue;
      }
    }

    switch (vtype)
    {
      case LUA_TLIGHTUSERDATA:
        xrio_pushliteral(B, "null");
        break;
      case LUA_TNUMBER:
        if (lua_isinteger(L, vidx))
          xrio_pushfstring(B, "%I", (lua_Integer)lua_tointeger(L, vidx));
        else {
          lua_Number n = lua_tonumber(L, vidx);
          if (isnan(n) || isinf(n)) {
            xrio_reset(B);
            return luaL_error(L, "Cannot serialise number: must not be NaN or Infinity");
          }
          xrio_pushfstring(B, "%f", n);
        }
        break;
      case LUA_TBOOLEAN:
        if (lua_toboolean(L, vidx))
          xrio_pushliteral(B, "true");
        else
          xrio_pushliteral(B, "false");
        break;
      case LUA_TSTRING:
        json_pushstring(L, B, vidx, 0);
        break;
      case LUA_TTABLE:
        json_encode_table(L, j_table, B);
        break;
      default:
        xrio_reset(B);
        luaL_error(L, "[json encode]: Invalid value type `%s`.", lua_typename(L, vtype));
    }
    lua_pop(L, 1);
  }

  if (!times)
    json_block_start(B, mode);
  json_block_over(B, mode);

  return 0;
}

static inline int json_init_encode(lua_State *L) {
  xrio_Buffer B;
  xrio_buffinit(L, &B);

  bool jsonp = lua_isboolean(L, 2) && lua_toboolean(L, 2) ? 1 : 0;
  if (jsonp) {
    xrio_addstring(&B, lua_tostring(L, 3));
    xrio_addchar(&B, '(');
  }
  lua_settop(L, 1);
  json_encode_table(L, j_table, &B);
  if (jsonp)
    xrio_addchar(&B, ')');
  xrio_pushresult(&B);
  return 1;
}

int ljson_encode(lua_State *L) {
  if (lua_type(L, 1) != LUA_TTABLE)
    return luaL_error(L, "[json encode]: need lua table.");

  return json_init_encode(L);
}