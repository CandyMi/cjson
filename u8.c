#include <stdint.h>

/* 查表比计算快 */
static int8_t chartab[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,

  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

/* ASCII 字符转换为整数 */
int json_cstring_to_utf8_hex(const uint8_t hex[4])
{
  int code;
  int digit[4];
  for (int i = 0; i < 4; i++) {
    code = chartab[hex[i]];
    if (code < 0)
      return -1;
    digit[i] = code;
  }
  return (digit[0] << 12) + (digit[1] << 8) + (digit[2] << 4) + digit[3];
}


/* unicode 转换为 utf-8 */
int json_cstring_to_utf8(char *utf8, int codepoint)
{
  /* 0xxxxxxx */
  if (codepoint <= 0x7F) {
    utf8[0] = codepoint;
    return 1;
  }

  /* 110xxxxx 10xxxxxx */
  if (codepoint <= 0x7FF) {
    utf8[0] = (codepoint >> 6) | 0xC0;
    utf8[1] = (codepoint & 0x3F) | 0x80;
    return 2;
  }

  /* 1110xxxx 10xxxxxx 10xxxxxx */
  if (codepoint <= 0xFFFF) {
    utf8[0] = (codepoint >> 12) | 0xE0;
    utf8[1] = ((codepoint >> 6) & 0x3F) | 0x80;
    utf8[2] = (codepoint & 0x3F) | 0x80;
    return 3;
  }

  /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
  if (codepoint <= 0x1FFFFF) {
    utf8[0] = (codepoint  >> 18) | 0xF0;
    utf8[1] = ((codepoint >> 12) & 0x3F) | 0x80;
    utf8[2] = ((codepoint >>  6) & 0x3F) | 0x80;
    utf8[3] = (codepoint & 0x3F) | 0x80;
    return 4;
  }

  return -1;
}