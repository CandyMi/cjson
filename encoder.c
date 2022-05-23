#include "json.h"

enum json_type_t {
  XRTYPE_INTEGER  = 10,
  XRTYPE_NUMBER,   
  XRTYPE_STRING,
  XRTYPE_BOOLEAN,
  XRTYPE_NULL,
  XRTYPE_TABLE,
};

struct json_item_t {
  uint32_t type;
  uint32_t slen;
  union {
    lua_Integer i;
    lua_Number  n;
    const char *str;
    struct json_table_t *t;
  };
  struct json_item_t *next;
};

struct json_table_t {
  size_t mode;
  struct json_item_t *item;
};

#define json_block_start(B, mode)   if (mode) luaL_addchar(B, '['); else luaL_addchar(B, '{')
#define json_block_over(B, mode)    if (mode) luaL_addchar(B, ']'); else luaL_addchar(B, '}')

static inline int json_encode_table(lua_State *L, struct json_table_t *json, int idx);

static inline void json_to_string(lua_State *L, luaL_Buffer *B, struct json_table_t* json);

static inline struct json_table_t *json_new() {
  struct json_table_t* json = xrio_malloc(sizeof(struct json_table_t));
  json->item = NULL; json->mode = 0;
  return json;
}

static inline void json_free(struct json_table_t * tab) {
  if (tab)
    xrio_free(tab);
}


static inline struct json_item_t * json_item_new() {
  struct json_item_t* value = xrio_malloc(sizeof(struct json_item_t));
  value->t = NULL; value->type = 0; value ->next = NULL;
  return value;
}


static inline void json_item_free(struct json_item_t * item) {
  if (item)
    xrio_free(item);
}

/* 检查内部元表是否存在 */
static inline int json_has_array_metatable(lua_State *L, int idx, const char* name) {
  if (!lua_getmetatable(L, idx))
    return 0;
  luaL_getmetatable(L, name);
  int r = lua_rawequal(L, -1, -2);
  lua_pop(L, 2);
  return r;
}

static inline struct json_item_t* json_fetch_key(lua_State *L, int kidx, size_t *idx) {
  int ktype = lua_type(L, kidx);

  struct json_item_t* key = json_item_new();

  switch (ktype)
  {
    case LUA_TNUMBER:
      if (lua_isinteger(L, kidx)){
        key->type = XRTYPE_INTEGER;
        key->i = lua_tointeger(L, kidx);
        *idx = key->i;
      } else {
        key->type = XRTYPE_NUMBER;
        key->n = lua_tonumber(L, kidx);
      }
      break;
    case LUA_TSTRING:
      key->type = XRTYPE_STRING;
      key->str = lua_tolstring(L, kidx, (size_t*)&key->slen);
      break;
    default:
      luaL_error(L, "[json encode]: Invalid key type `%s`.", lua_typename(L, ktype));
  }

  return key;
}

static inline struct json_item_t* json_fetch_val(lua_State *L, int vidx) {
  int vtype = lua_type(L, vidx);

  struct json_item_t* val = json_item_new();

  switch (vtype)
  {
    case LUA_TLIGHTUSERDATA:
      if (lua_touserdata(L, vidx) != NULL)
        luaL_error(L, "[json encode]: userdata must be null.");
      val->type = XRTYPE_NULL;
      break;
    case LUA_TNUMBER:
      if (lua_isinteger(L, vidx)){
        val->type = XRTYPE_INTEGER;
        val->i = lua_tointeger(L, vidx);
      }else{
        val->type = XRTYPE_NUMBER;
        val->n = lua_tonumber(L, vidx);
        if (val->n == NAN || val->n == INFINITY)
          luaL_error(L, "Get `nan` or `inf` in lua_Number value.");
      }
      break;
    case LUA_TBOOLEAN:
      val->type = XRTYPE_BOOLEAN;
      val->i = lua_toboolean(L, vidx);
      break;
    case LUA_TSTRING:
      val->type = XRTYPE_STRING;
      val->str = lua_tolstring(L, vidx, (size_t*)&val->slen);
      break;
    case LUA_TTABLE:
      val->type = XRTYPE_TABLE;
      val->t = json_new();
      json_encode_table(L, val->t, vidx);
      break;
    default:
      luaL_error(L, "[json encode]: Invalid value type `%s`.", lua_typename(L, vtype));
  }

  return val;
}

/* Table */
static inline int json_encode_table(lua_State *L, struct json_table_t *json, int idx) {

  json->mode = 0;
  if (json_has_array_metatable(L, idx, "json_array"))
    json->mode = 1;
  
  struct json_item_t* item = NULL;
  size_t kindex = -1;
  size_t index = 0;

  lua_pushnil(L);
  while (lua_next(L, idx))
  {
    index++;
    struct json_item_t* key = json_fetch_key(L, idx + 1, &kindex);
    struct json_item_t* val = json_fetch_val(L, idx + 2);
    // printf("kindex = %zu, index = %zu\n", kindex, index);

    if (!item) {
      item = key->next = val;
      json->item = key;
    } else {
      item->next = key;
      item = key->next = val;
    }
    
    lua_pop(L, 1);
  }
  if (kindex == index)
    json->mode = 1;
  return 0;
}

static inline void json_string_wrap(lua_State *L, const char* str, size_t len, int mode) {
  luaL_Buffer TB;
  luaL_buffinit(L, &TB);

  luaL_addchar(&TB, '\"');
  for (size_t i = 0; i < len; i++) {
    if (str[i] == '\"')
      luaL_addlstring(&TB, "\\\"", 2);
    else
      luaL_addchar(&TB, str[i]);
  }

  if (mode)
    luaL_addlstring(&TB, "\":", 2);
  else
    luaL_addchar(&TB, '\"');
  luaL_pushresult(&TB);
}

static inline void json_key_value_to_string(lua_State *L, luaL_Buffer *B, struct json_item_t* k, struct json_item_t* v, int mode) {

  luaL_Buffer TB;
  if (v->type == XRTYPE_TABLE)
    luaL_buffinit(L, &TB);

  switch (v->type)
  {
    case XRTYPE_NULL:
      lua_pushliteral(L, "null");
      break;
    case XRTYPE_BOOLEAN:
      lua_pushstring(L, (v->i ? "true" : "false"));
      break;
    case XRTYPE_INTEGER:
      lua_pushfstring(L, "%I", v->i);
      break;
    case XRTYPE_NUMBER:
      lua_pushfstring(L, "%f", v->n);
      break;
    case XRTYPE_STRING:
      json_string_wrap(L, v->str, (size_t)v->slen, 0);
      break;
    case XRTYPE_TABLE:
      json_to_string(L, &TB, v->t);
      break;
    default:
      luaL_error(L, "[json encode]: Invalid xrio value type `%s`.", v->type);
  }

  if (mode == 0) {
    switch (k->type)
    {
      case XRTYPE_INTEGER:
        lua_pushfstring(L, "\"%d\":", k->i);
        break;
      case XRTYPE_NUMBER:
        lua_pushfstring(L, "\"%f\":", k->n);
        break;
      case XRTYPE_STRING:
        json_string_wrap(L, k->str, (size_t)k->slen, 1);
        break;
      default:
        luaL_error(L, "[json encode]: Invalid xrio key type `%s`.", k->type);
    }
    luaL_addvalue(B);
  }
  luaL_addvalue(B);
}

static inline void json_to_string(lua_State *L, luaL_Buffer *B, struct json_table_t* json) {
  json_block_start(B, json->mode);

  struct json_item_t *tmp1;
  struct json_item_t *tmp2;
  struct json_item_t *key;
  struct json_item_t *val;

  key = json->item;
  if (key)
    val  = key->next;

  while (key && val)
  {
    if (key != json->item)
      luaL_addchar(B, ',');
    json_key_value_to_string(L, B, key, val, json->mode);
    tmp1 = key; tmp2 = val;
    if ((key = val->next))
      val = key->next;
    json_item_free(tmp1);
    json_item_free(tmp2);
  }

  json_block_over(B, json->mode);
  json_free(json);
}

int ljson_encode(lua_State *L){
  if (lua_type(L, 1) != LUA_TTABLE)
    return luaL_error(L, "json encoder need lua table.");

  /* table to json */
  lua_settop(L, 1);
  struct json_table_t* json = json_new();
  json_encode_table(L, json, 1);

  lua_settop(L, 1);
  /* dump string */
  luaL_Buffer B;
  luaL_buffinit(L, &B);
  json_to_string(L, &B, json);
  luaL_pushresult(&B);
  return 1;
}