#include "global_funcs.h"
#include "..\lua\my12doom_lua.h"

UTF82W C(const wchar_t *English)
{
	luaState L;
	lua_getglobal(L, "L");
	lua_pushstring(L, W2UTF8(English));
	lua_mypcall(L, 1, 1, 0);

	UTF82W w(lua_tostring(L, -1));

	return w;
};