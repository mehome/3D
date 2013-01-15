#include "..\lua\my12doom_lua.h"
#include "my12doomRenderer_lua.h"
#include "my12doomRenderer.h"
#include <windows.h>
#include <atlbase.h>
#include "..\dwindow\dx_player.h"

extern my12doomRenderer *g_renderer;

int my12doomRenderer_lua_loadscript();

static int paint_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int left = lua_tointeger(L, parameter_count+0);
	int top = lua_tointeger(L, parameter_count+1);
	int right = lua_tointeger(L, parameter_count+2);
	int bottom = lua_tointeger(L, parameter_count+3);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+4);

	int s_left = lua_tointeger(L, parameter_count+5);
	int s_top = lua_tointeger(L, parameter_count+6);
	int s_right = lua_tointeger(L, parameter_count+7);
	int s_bottom = lua_tointeger(L, parameter_count+8);
	double alpha = lua_tonumber(L, parameter_count+9);

	RECTF dst_rect = {left, top, right, bottom};
	RECTF src_rect = {s_left, s_top, s_right, s_bottom};
	bool hasROI = s_left > 0 || s_top > 0 || s_right > 0 || s_bottom > 0;

	g_renderer->paint(&dst_rect, resource, hasROI ? &src_rect : NULL, alpha);

	lua_pushboolean(L, 1);
	return 1;
}

static int set_clip_rect_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int left = lua_tointeger(L, parameter_count+0);
	int top = lua_tointeger(L, parameter_count+1);
	int right = lua_tointeger(L, parameter_count+2);
	int bottom = lua_tointeger(L, parameter_count+3);

	g_renderer->set_clip_rect(left, top, right, bottom);

	lua_pushboolean(L, 1);
	return 1;
}
static int get_resource(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int arg = lua_tointeger(L, parameter_count+0);

	resource_userdata *resource = new resource_userdata;
	g_renderer->get_resource(0, resource);
	lua_pushlightuserdata(L, resource);
	return 1;
}

static int load_bitmap_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	const char *filename = lua_tostring(L, parameter_count+0);

	USES_CONVERSION;
	gpu_sample *sample = NULL;
	if (FAILED(g_renderer->loadBitmap(&sample, A2W(filename))) || sample == NULL)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "failed loading bitmap file");
		return 2;
	}

	resource_userdata *resource = new resource_userdata;
	resource->resource_type = resource_userdata::RESOURCE_TYPE_GPU_SAMPLE;
	resource->pointer = sample;
	resource->managed = false;
	lua_pushlightuserdata(L, resource);
	lua_pushinteger(L, sample->m_width);
	lua_pushinteger(L, sample->m_height);
	return 3;
}

static int release_resource_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+0);
	bool is_decommit = lua_toboolean(L, parameter_count+1);

	if (resource == NULL) 
		return 0;

	if (resource->pointer == NULL || resource->managed)
		return 0;

	if (resource->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		delete ((gpu_sample*)resource->pointer);

	delete resource;

	return 0;
}

extern Iplayer *g_player;
extern double UIScale;
static int get_mouse_pos(lua_State *L)
{
	POINT p;
	GetCursorPos(&p);
	HWND wnd = g_player->get_window(1);
	ScreenToClient(wnd, &p);

	lua_pushinteger(L, p.x/UIScale);
	lua_pushinteger(L, p.y/UIScale);

	return 2;
}

static int commit_resource_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+0);

	if (resource == NULL) 
		return 0;

	if (resource->pointer == NULL || resource->managed)
		return 0;

	if (resource->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		((gpu_sample*)resource->pointer)->commit();

	return 0;
}


static int decommit_resource_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+0);

	if (resource == NULL) 
		return 0;

	if (resource->pointer == NULL || resource->managed)
		return 0;

	if (resource->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		((gpu_sample*)resource->pointer)->commit();

	return 0;
}


static int myprint(lua_State *L)
{
	return 0;
}

int my12doomRenderer_lua_init()
{
	g_lua_manager->get_variable("paint_core") = &paint_core;
	g_lua_manager->get_variable("set_clip_rect_core") = &set_clip_rect_core;
	g_lua_manager->get_variable("get_resource") = &get_resource;
	g_lua_manager->get_variable("load_bitmap_core") = &load_bitmap_core;
	g_lua_manager->get_variable("release_resource_core") = &release_resource_core;
	g_lua_manager->get_variable("commit_resource_core") = &commit_resource_core;
	g_lua_manager->get_variable("decommit_resource_core") = &decommit_resource_core;
	g_lua_manager->get_variable("get_mouse_pos") = &get_mouse_pos;

	my12doomRenderer_lua_loadscript();

	return 0;
}

int my12doomRenderer_lua_loadscript()
{
	CAutoLock lck(&g_csL);
	if (luaL_loadfile(g_L, "d:\\private\\base_frame.lua") || lua_pcall(g_L, 0, 0, 0))
	{
		const char * result = lua_tostring(g_L, -1);
		printf("failed loading renderer lua script : %s\n", result);
		lua_settop(g_L, 0);
	}

	if (luaL_loadfile(g_L, "d:\\private\\render.lua") || lua_pcall(g_L, 0, 0, 0))
	{
		const char * result = lua_tostring(g_L, -1);
		printf("failed loading renderer lua script : %s\n", result);
		lua_settop(g_L, 0);
	}

	return 0;
}