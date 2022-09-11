#include "json.h"

LUAMOD_API int luaopen_ljson(lua_State *L) {
  luaL_checkversion(L);
  /* 元表 */
  luaL_newmetatable(L, "lua_Table");
  luaL_newmetatable(L, "lua_List");

  luaL_Reg json_libs[] = {
    {"encode", ljson_encode},
    {"decode", ljson_decode},
    {NULL, NULL}
  };
  luaL_newlib(L, json_libs);

  /* 关联`empty_array`表 */
  lua_newtable(L);
  luaL_setmetatable(L, "lua_List");
  lua_setfield (L, -2, "empty_array");

  /* 设置版本号 */
  lua_pushnumber(L, 0.1);
  lua_setfield (L, -2, "version");
  return 1;
}