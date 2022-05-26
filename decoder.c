#include "json.h"

#define case_comma   case ','
#define case_colon   case ':'

#define case_array_s case '['
#define case_array_e case ']'

#define case_object_s case '{'
#define case_object_e case '}'


#define case_null     case 'n'
#define case_true     case 't'
#define case_false    case 'f'

#define case_string   case '"'
#define case_number   case '+': case '-': case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':case '9'

/* 跳转到下一个有效字节开始位置 */
static inline size_t json_next_char(const char* ptr, size_t len) {
  size_t pos = 0;
  for (;pos < len; pos++)
    if (!isspace(ptr[pos]))
      break;
  return pos;
}

/* 查找key->value分割的`:`位置 */
static inline size_t json_next_colon(const char* ptr, size_t len) {
  size_t pos = 0;
  for (;pos < len; pos++)
    if (ptr[pos] == ':')
      break;
  return pos + 1;
}

static inline const char* json_get_error(lua_State *L, const char *buffer, size_t bsize) {
  size_t esize = (bsize < 20 ? bsize : 20) + 1;
  char* errinfo = lua_newuserdata(L, esize);
  memset(errinfo, 0x0, esize);
  memcpy(errinfo, buffer, esize - 1);
  return errinfo;
}

static inline int json_decode_cstring(lua_State *L, const char* buffer, size_t bsize) {
  luaL_Buffer B;
  luaL_buffinitsize(L, &B, json_buffer_size);
  size_t pos = 1; size_t s = 1;
  char u8buffer[4];
  while (pos < bsize) {
    /* 处理特殊符号 */
    if (buffer[pos] == '\\') {
      /* 处理双引号转义符 */
      if (buffer[pos+1] == '"' || buffer[pos+1] == '\\') {
        luaL_addlstring(&B, buffer + s, pos - s);
        luaL_addchar(&B, buffer[pos+1]);
        s = pos += 2;
        continue;
      }
      /* 处理Unicode转义符 */
      if (buffer[pos+1] == 'u' && bsize - pos > 5) {
        int code = json_cstring_to_utf8_hex(buffer + pos + 2);
        if (code == -1) {
          pos++; continue;
        }
        int len = json_cstring_to_utf8(u8buffer, code);
        if (code == -1) {
          pos++; continue;
        }
        luaL_addlstring(&B, buffer + s, pos - s);
        luaL_addlstring(&B, u8buffer, len);
        s = pos += 6;
        continue;
      }
    }
    if (buffer[pos] == '"') {
      if (buffer[pos-1] != '\\' || buffer[pos-2] == '\\') {
        luaL_addlstring(&B, buffer + s, pos - s);
        break;
      }
    }
    pos++;
  }

  if (pos == bsize)
    return luaL_error(L, "[json decode]: cstring was invalid in `%s`.", json_get_error(L, buffer, bsize));

  luaL_pushresult(&B);
  return pos + 1;
}

static inline int json_decode_number(lua_State *L, const char* buffer, size_t bsize) {
  bool sig = 0;
  size_t pos = 0;
  for (;pos < bsize; pos++)
  {
    uint8_t code = buffer[pos];
    if (code == '-' || code == '+') {
      if (sig)
        return luaL_error(L, "[json decode]: error signed number `%s`.", json_get_error(L, buffer + pos, bsize - pos));
      sig = 1;
      continue;
    }
    if (code == ',' || code == ']' || code == '}')
      break;
    /* 检查合法性 */
    if (pos > 40)
      return luaL_error(L, "[json decode]: invalid number length(%d) in `%s`.", pos, json_get_error(L, buffer, bsize));
  }

  /* 构造 */
  char num [41] = { [0 ... 40] = 0 };
  memcpy(num, buffer, pos);
  /* 转换失败 */
  if (!lua_stringtonumber(L, num))
    return luaL_error(L, "[json decode]: invalid number `%s`.", json_get_error(L, buffer, bsize));
  return pos;
}

static inline int json_decode_boolean(lua_State *L, const char* buffer, size_t bsize, const char *cmp, size_t csize) {
  if (bsize < csize)
    luaL_error(L, "[json decode]: error boolean value in `%s`.", json_get_error(L, buffer, bsize));
  
  if (strncmp(buffer, cmp, csize))
    luaL_error(L, "[json decode]: invalid char in `%s`.", json_get_error(L, buffer, bsize));

  return csize;
}

static inline int json_decode_array(lua_State *L, const char* buffer, size_t bsize);
static inline int json_decode_object(lua_State *L, const char* buffer, size_t bsize);

static inline int json_decode_array(lua_State *L, const char* buffer, size_t bsize) {
  if (buffer[0] == '{')
    return json_decode_object(L, buffer, bsize);
  
  bool json_start = 0;
  bool json_comma = 0;
  size_t idx = 1;
  size_t pos = 0;
  size_t ret = 0;
  lua_createtable(L, 8, 0);
  while (bsize)
  {
    switch (buffer[0]) {
      case_object_s:
        ret = json_decode_object(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        lua_rawseti(L, -2, idx++);
        json_comma = 0;
        break;
      case_array_s:
        if (!json_start) {
          json_start = 1;
          buffer++; bsize--; pos++;
        } else {
          ret += json_decode_array(L, buffer, bsize);
          buffer += ret; bsize -= ret; pos += ret;
          lua_rawseti(L, -2, idx++);
        }
        json_comma = 0;
        break;
      case_comma:
        if (json_comma)
          return luaL_error(L, "[json decode]: invalide split in `%s`.", json_get_error(L, buffer, bsize));
        json_comma = 1;
        buffer++; bsize--; pos++;
        break;
      case_string:
        ret = json_decode_cstring(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        lua_rawseti(L, -2, idx++);
        json_comma = 0;
        break;  
      case_number:
        ret = json_decode_number(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        lua_rawseti(L, -2, idx++);
        json_comma = 0;
        break;
      case_null:
        ret = json_decode_boolean(L, buffer, bsize, "null", 4);
        buffer += ret; bsize -= ret; pos += ret;
        lua_pushlightuserdata(L, NULL);
        lua_rawseti(L, -2, idx++);
        json_comma = 0;
        break;
      case_true:
        ret = json_decode_boolean(L, buffer, bsize, "true", 4);
        buffer += ret; bsize -= ret; pos += ret;
        lua_pushboolean(L, 1);
        lua_rawseti(L, -2, idx++);
        json_comma = 0;
        break;
      case_false:
        ret = json_decode_boolean(L, buffer, bsize, "false", 5);
        buffer += ret; bsize -= ret; pos += ret;
        lua_pushboolean(L, 0);
        lua_rawseti(L, -2, idx++);
        json_comma = 0;
        break;
      case_array_e :
        if (json_comma)
          return luaL_error(L, "[json decode]: invalide array buffer split in `%s`.", json_get_error(L, buffer, bsize));
        buffer++; bsize--; pos++;
        ret = json_next_char(buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        luaL_setmetatable(L, "json_array");
        return pos;
      default:
        return luaL_error(L, "[json decode]: Invalid array buffer in `%s`.", json_get_error(L, buffer, bsize));
    }

    ret = json_next_char(buffer, bsize);
    buffer += ret; bsize -= ret; pos += ret;
  }

  if (json_start)
    luaL_error(L, "[json decode]: the `[` character appears repeatedly in `%s` at array.", json_get_error(L, buffer, bsize));

  return pos;
}

static inline int json_decode_object(lua_State *L, const char* buffer, size_t bsize) {
  if (buffer[0] == '[')
    return json_decode_array(L, buffer, bsize);

  if (buffer[0] != '{')
    return luaL_error(L, "[json decode]: object decode failed in `%s`.", json_get_error(L, buffer, bsize));

  bool json_comma = 0;
  size_t pos = 0;
  buffer++; bsize--; pos++;
  size_t ret = json_next_char(buffer, bsize);
  buffer += ret; bsize -= ret; pos += ret;

  lua_createtable(L, 0, 8);
  while (bsize)
  {
    /* key */
    switch (buffer[0])
    {
      case_string:
        ret = json_decode_cstring(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        json_comma = 0;
        break;
      case_object_e:
        if (json_comma)
          return luaL_error(L, "[json decode]: Invalid object comma in `%s`", json_get_error(L, buffer, bsize));
        buffer++; bsize--; pos++;
        ret = json_next_char(buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        luaL_setmetatable(L, "json_table");
        return pos;
      default:
        return luaL_error(L, "[json decode]: Invalid object key buffer in `%s`.", json_get_error(L, buffer, bsize));
    }

    ret = json_next_colon(buffer, bsize);
    if (ret == bsize)
      return luaL_error(L, "[json decode]: json split ':' failed in `%s`. 1", json_get_error(L, buffer, bsize));
    buffer += ret; bsize -= ret; pos += ret;

    ret = json_next_char(buffer, bsize);
    if (ret == bsize)
      return luaL_error(L, "[json decode]: json split ':' failed in `%s`. 2", json_get_error(L, buffer, bsize));
    buffer += ret; bsize -= ret; pos += ret;

    /* value */
    switch (buffer[0])
    {
      case_null:
        ret = json_decode_boolean(L, buffer, bsize, "null", 4);
        buffer += ret; bsize -= ret; pos += ret;
        lua_pushlightuserdata(L, NULL);
        lua_rawset(L, -3);
        break;
      case_true:
        ret = json_decode_boolean(L, buffer, bsize, "true", 4);
        buffer += ret; bsize -= ret; pos += ret;
        lua_pushboolean(L, 1);
        lua_rawset(L, -3);
        break;
      case_false:
        ret = json_decode_boolean(L, buffer, bsize, "false", 5);
        buffer += ret; bsize -= ret; pos += ret;
        lua_pushboolean(L, 0);
        lua_rawset(L, -3);
        break;
      case_string:
        ret = json_decode_cstring(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        lua_rawset(L, -3);
        break;
      case_number:
        ret = json_decode_number(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        lua_rawset(L, -3);
        break;
      case_object_s:
        ret = json_decode_object(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        lua_rawset(L, -3);
        break;
      case_array_s:
        ret = json_decode_array(L, buffer, bsize);
        buffer += ret; bsize -= ret; pos += ret;
        lua_rawset(L, -3);
        break;
      default:
        return luaL_error(L, "[json decode]: Invalid object value buffer in `%s`.", json_get_error(L, buffer, bsize));
    }
    /* 检查下一次 Key-> value 位置 */
    ret = json_next_char(buffer, bsize);
    buffer += ret; bsize -= ret; pos += ret;

    if (buffer[0] == ',') {
      json_comma = 1;
      buffer++; bsize--; pos++;
      ret = json_next_char(buffer, bsize);
      buffer += ret; bsize -= ret; pos += ret;
    }
  }

  return luaL_error(L, "[json decode]: the `{` character appears repeatedly in `%s` at object.", json_get_error(L, buffer, bsize));  
}

/* 反序列化 */
static inline int json_decode_table(lua_State *L, const char* buffer, size_t bsize) {
  size_t pos = json_next_char(buffer, bsize);
  if (pos == bsize)
    return luaL_error(L, "[json decode]: empty json buffer.");

  lua_settop(L, 1);
  buffer += pos; bsize -= pos;

  /* 解析完毕需要检查结果字符串结尾. */
  pos = json_decode_object(L, buffer, bsize);
  if (pos != bsize)
    return luaL_error(L, "[json decode]: decoder failed in `%s`", json_get_error(L, buffer + pos, bsize - pos));
  /* 解析完毕需要检查堆栈是否正确 */
  if (lua_gettop(L) != 2)
    return luaL_error(L, "[json decode]: decoder failed in top %d.", lua_gettop(L));
  return 1;
}


int ljson_decode(lua_State *L) {
  size_t bsize;
  const char* buffer = luaL_checklstring(L, 1, &bsize);
  if (bsize < 2)
    return luaL_error(L, "[json decode]: Invalid json buffer.");

  return json_decode_table (L, buffer, bsize);
}