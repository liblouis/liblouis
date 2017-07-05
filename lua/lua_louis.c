/* lua-louis: liblouis bindings for Lua

Copyright (C) 2017 Swiss Library for the Blind, Visually Impaired and Print Disabled

This file is part of lua-louis.

lua-louis is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

lua-louis is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with liblouis. If not, see <http://www.gnu.org/licenses/>.

*/

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include "liblouis.h"
#include "internal.h"
#include "unistr.h"

#ifdef WIDECHARS_ARE_UCS4
#define to_utf8(input, inputlen, outlen) u32_to_u8(input, inputlen, NULL, outlen)
#define from_utf8(input, inputlen, outlen) u8_to_u32(input, inputlen, NULL, outlen)
#else
#define to_utf8(input, inputlen, outlen) u16_to_u8(input, inputlen, NULL, outlen)
#define from_utf8(input, inputlen, outlen) u8_to_u16(input, inputlen, NULL, outlen)
#endif

const char version_etc_copyright[] = "Copyright %s %d Swiss Library for the Blind, Visually Impaired and Print Disabled";

static int lua_lou_version (lua_State *L) {
  lua_pushstring(L, lou_version());
  return 1;
}

static int lua_lou_translateString (lua_State *L) {
  const char *tableList = lua_tostring(L, 1);
  const char *input = lua_tostring(L, 2);

  size_t input_len = u8_strlen(input);
  size_t inbuflen;
  widechar *inbuf = from_utf8(input, input_len, &inbuflen);

  int inlen = (int) inbuflen;
  int outlen;
  widechar* outbuf = malloc(sizeof(widechar) * input_len * 10);

  if (!lou_translateString(tableList, inbuf, &inlen, outbuf, &outlen,
			   NULL, NULL, 0))
    luaL_error(L, "Braille translation failed.");

  size_t outputlen;
  char *output = to_utf8(outbuf, outlen, &outputlen);
  lua_pushlstring(L, output, outputlen);
  free(inbuf);
  free(outbuf);
  free(output);
  return 1; 
}

static const struct luaL_reg lua_louis [] = {
  {"version", lua_lou_version},
  {"translate", lua_lou_translateString},
  {NULL, NULL}  /* sentinel */
};

int luaopen_louis (lua_State *L) {
  luaL_openlib(L, "louis", lua_louis, 0);
  return 1;
}
