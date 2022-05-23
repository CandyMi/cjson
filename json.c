#include "json.h"

static inline int luaL_json_init(lua_State *L) {
  luaL_Reg json_libs[] = {
    {"encode", ljson_encode},
    {"decode", ljson_decode},
    {NULL, NULL}
  };
  luaL_newlib(L, json_libs);
  /* 数组元表 */
  luaL_newmetatable(L, "json_array");
  lua_setfield (L, -2, "json_array_mt");
  /* 哈希元表 */
  luaL_newmetatable(L, "json_table");
  lua_setfield (L, -2, "json_table_mt");
  /* 空数组 */
  lua_newtable(L);
  luaL_setmetatable(L, "json_array_mt");
  lua_setfield (L, -2, "empty_array");
  return 1;
}

LUAMOD_API int luaopen_ljson(lua_State *L) {
  luaL_checkversion(L);
  return luaL_json_init(L);
}