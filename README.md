# cjson

  JSON library written using Lua 5.3+ C API.

## Install

  1. clone to `3rd`.

  2. `make build`

  3. Done.

## Defined

```lua
local ljson = require "ljson"
local ljson_encode = ljson.encode
local ljson_decode = ljson.decode

local type = type

local json = { }

---comment 空数组
json.empty_array = ljson.empty_array

---comment @将`table`序列化为`json`格式的`string`.
---@param tab    table     @可序列化为`json`类型的`table`.
---@param prefix table?    @`jsonp`前缀.
---@return string          @成功返回`json`格式的`string`, 失败会抛出异常.
function json.encode(tab, prefix)
  return ljson_encode(tab, type(prefix) == 'string', prefix)
end

---comment @将`json`格式的`string`解析为`table`
---@param buffer string    @`json`格式的`string`.
---@param jsonp  boolean?  @会尝试检查`jsonp`结构.
---@return table | false   @成功返回`table`, 失败返回`nil`与`errinfo`.
---@return string?         @成功返回`table`, 失败返回`nil`与`errinfo`.
function json.decode(buffer, jsonp)
  return ljson_decode(buffer, jsonp and true or false)
end

return json
```

## Usage

```lua
  local json = require "ljson"
  -- json.encode ( table )
  
  -- json.decode (json string)
```
