#include "json.h"

static inline int luaL_json_init(lua_State *L) {
  /* 元表 */
  luaL_newmetatable(L, "json_table_mt");
  luaL_newmetatable(L, "json_array_mt");

  luaL_Reg json_libs[] = {
    {"encode", ljson_encode},
    {"decode", ljson_decode},
    {NULL, NULL}
  };
  luaL_newlib(L, json_libs);

  /* 关联`empty_array`表 */
  lua_newtable(L);
  luaL_setmetatable(L, "json_array_mt");
  lua_setfield (L, -2, "empty_array");

  /* 设置版本号 */
  lua_pushnumber(L, 0.1);
  lua_setfield (L, -2, "version");
  return 1;
}

LUAMOD_API int luaopen_ljson(lua_State *L) {
  luaL_checkversion(L);
  return luaL_json_init(L);
}