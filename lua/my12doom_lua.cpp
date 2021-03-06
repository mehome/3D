#include <Windows.h>
#include "my12doom_lua.h"
#include <list>
#include <math.h>
#include "..\dwindow\global_funcs.h"
#include "..\dwindow\dwindow_log.h"
#include "..\hookdshow\hookdshow.h"

lua_State *g_L = NULL;
CCritSec g_csL;
lua_manager *g_lua_core_manager = NULL;
lua_manager *g_lua_setting_manager = NULL;
std::list<lua_State*> free_threads;
std::list<lua_State*> running_threads;
lua_State * dwindow_lua_get_thread();
void dwindow_lua_release_thread(lua_State * p);

static int lua_GetTickCount (lua_State *L) 
{
	lua_pushinteger(L, timeGetTime());
	return 1;
}
bool setup_gc(lua_State *L, lua_CFunction f)
{
	int n = lua_gettop(L);

	lua_newtable(L);
	lua_pushcfunction(L, f);
	lua_setfield(L, -2, "__gc");
	lua_setmetatable(L, -2);

	return n == lua_gettop(L);
}
static int lua_ApplySetting(lua_State *L)
{
	const char *name = lua_tostring(L, -1);
	if (!name)
		return 0;

	g_lua_setting_manager->get_const(name).read_from_lua();

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_Sleep(lua_State *L)
{
	DWORD time = lua_tointeger(L, -1);
	Sleep(time);

	return 0;
}

wchar_t * wcsstr_nocase(const wchar_t *search_in, const wchar_t *search_for);
const wchar_t *URL2Token(const wchar_t *URL);


static int execute_luafile(lua_State *L)
{
// 	int parameter_count = lua_gettop(L);
	const char *filename = NULL;
	filename = luaL_checkstring(L, 1);
	USES_CONVERSION;
	UTF82W w(filename);
	const wchar_t *filenamew = w;
	g_lua_core_manager->get_variable("loading_file") = filenamew;

	if (wcsstr_nocase(filenamew, L"http://") == filenamew)
		filenamew = URL2Token(filenamew);

	if(luaL_loadfile(L, W2A(filenamew)) || lua_pcall(L, 0, 0, 0))
	{
		const char * result;
		result = lua_tostring(L, -1);
		lua_pushboolean(L, 0);
		lua_pushstring(L, W2UTF8(A2W(result)));

		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_prefetch_http_file(lua_State *L)
{
	int start = 0;
	int end = -1;
	const char *filename = NULL;
	filename = lua_tostring(L, 1);
	if (lua_gettop(L)>1)
		start = lua_tointeger(L, -lua_gettop(L)+1);
	if (lua_gettop(L)>2)
		end = lua_tointeger(L, -lua_gettop(L)+2);

	USES_CONVERSION;
	UTF82W w(filename);
	const wchar_t *filenamew = w;
	if (wcsstr_nocase(filenamew, L"http://") == filenamew)
		filenamew = URL2Token(filenamew);
	else
		return 0;

	char buf[1024];
	FILE * f = _wfopen(filenamew, L"rb");
	if (!f)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	fseek(f, 0, SEEK_END);
	if (end <= 0)
		end = ftell(f);
	fseek(f, start, SEEK_SET);

	int noerror = 1;

	int size = end - start;
	while (size>0)
	{
		int got = fread(buf,1 , min(sizeof(buf), size), f);
		if (got != min(sizeof(buf), size))
		{
			noerror = 0;
			break;
		}
		size -= got;
	}

	fclose(f);

	lua_pushboolean(L, noerror);
	return 1;
}

#pragma pack(push, 1)
typedef struct
{
	char leading[17];// = "local signature=\"";
	char signature[256];
	char ending[3];// = "\"";
} lua_signature;
#pragma pack(pop)

static int load_signed_string(lua_State *L)
{
	if (lua_type(L, -1) != LUA_TSTRING)
		return 0;

	const char *string = lua_tostring(L, 1);
	int len = lua_objlen(L, -1);


	// check signatures

	lua_signature *sig_txt = (lua_signature *)string;
	if (len < sizeof(lua_signature) 
		|| memcmp(sig_txt->leading, "\xef\xbb\xbf-- signature=\"", sizeof(sig_txt->leading))
		|| memcmp(sig_txt->ending, "\"\r\n", sizeof(sig_txt->ending)))
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "no signature found");
		return 2;
	}

	unsigned char sha1[20] = {0};
	SHA1Hash(sha1, (const unsigned char*)string+sizeof(lua_signature), len-sizeof(lua_signature));
	char signature[128+8];
	
	for(int i=0; i<128; i++)
		sscanf(sig_txt->signature+i*2, "%02X", signature+i);

	RSA_dwindow_network_public(signature, signature);

	if (memcmp(sha1, signature, sizeof(sha1)) != 0)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "invalid signature found");

		return 2;
	}
	if(luaL_loadstring(L, string+3))
	{
		const char * result;
		result = lua_tostring(L, -1);
		lua_pushboolean(L, 0);
		lua_pushstring(L, result);

		return 2;
	}

	return 1;
}


static int execute_signed_luafile(lua_State *L)
{
	const char *filename = lua_tostring(L, 1);
	UTF82W w(filename);
	const wchar_t *filenamew = w;
	g_lua_core_manager->get_variable("loading_file") = filenamew;

	if (wcsstr_nocase(filenamew, L"http://") == filenamew)
		filenamew = URL2Token(filenamew);


	// check signatures
	FILE * f = _wfopen(filenamew, L"rb");
	if (!f)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "open file failed");
		return 2;
	}


	lua_signature sig_txt;
	if (fread(&sig_txt, 1, sizeof(sig_txt), f) != sizeof(sig_txt) 
		|| memcmp(sig_txt.leading, "\xef\xbb\xbf-- signature=\"", sizeof(sig_txt.leading))
		|| memcmp(sig_txt.ending, "\"\r\n", sizeof(sig_txt.ending)))
	{
		fclose(f);
		lua_pushboolean(L, 0);
		lua_pushstring(L, "no signature found");
		return 2;
	}

	fseek(f, 0, SEEK_END);
	int file_size = ftell(f);
	fseek(f, sizeof(sig_txt), SEEK_SET);
	unsigned char *data = new unsigned char[file_size];
	fread(data, 1, file_size-sizeof(sig_txt), f);
	fclose(f);

	unsigned char sha1[20] = {0};
	SHA1Hash(sha1, data, file_size-sizeof(sig_txt));
	char signature[128+8];
	
	for(int i=0; i<128; i++)
		sscanf(sig_txt.signature+i*2, "%02X", signature+i);

	RSA_dwindow_network_public(signature, signature);

	if (memcmp(sha1, signature, sizeof(sha1)) != 0)
	{
		delete data;
		lua_pushboolean(L, 0);
		lua_pushstring(L, "invalid signature found");

		return 2;
	}

	USES_CONVERSION;
	g_lua_core_manager->get_variable("loading_file") = UTF82W(filename);
	if(luaL_loadfile(L, W2A(filenamew)) || lua_pcall(L, 0, 0, 0))
	{
		delete data;
		const char * result;
		result = lua_tostring(L, -1);
		lua_pushboolean(L, 0);
		lua_pushstring(L, result);

		return 2;
	}

	delete data;
	lua_pushboolean(L, 1);
	return 1;
}

static int loaddll(lua_State *L)
{
// 	int parameter_count = lua_gettop(L);
	const char *filename = NULL;
	filename = luaL_checkstring(L, 1);
	UTF82W filenamew(filename);

	HMODULE hdll = LoadLibraryW(filenamew);
	if (!hdll)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "dll not loaded");
		return 2;
	}

	FARPROC proc = GetProcAddress(hdll, "dwindow_dll_go");
	if (!proc)
	{
		FreeLibrary(hdll);
		lua_pushboolean(L, 0);
		lua_pushstring(L, "entry dwindow_dll_go not found");
		return 2;
	}


	int (*dwindow_dll_go)(lua_State* L) = (int (cdecl*)(lua_State*))proc;
	lua_State *L2 = dwindow_lua_get_thread();
 	dwindow_dll_go(L2);

	lua_pushboolean(L, 1);
	return 1;
}
static int http_request(lua_State *L)
{
	const char *url = NULL;
	url = luaL_checkstring(L, 1);

	char *buffer = new char[1024*1024];
	int size = 1024*1024;
	download_url((char*)url, buffer, &size);

	lua_pushlstring(L, buffer, size);
	delete buffer;
	lua_pushnumber(L, 200);
	return 2;
}

static int track_back(lua_State *L)
{
	const char* err = lua_tostring(L, -1);
	char tmp[10240];
	lua_Debug debug;
	for(int level = 1; lua_getstack(L, level, &debug); level++)
	{
		int suc = lua_getinfo(L, "Sl", &debug);
		sprintf(tmp, "%s(%d,1)\n", debug.source+1, debug.currentline);
		OutputDebugStringA(tmp);
	}
	return 0;
}


// threads

int luaResumeThread(lua_State *L)
{
	HANDLE h = (HANDLE)lua_touserdata(L, -1);
	if (h == NULL)
		return 0;

	ResumeThread(h);
	return 0;
}
int luaSuspendThread(lua_State *L)
{
	HANDLE h = (HANDLE)lua_touserdata(L, -1);
	if (h == NULL)
		return 0;
	SuspendThread(h);
	return 0;
}
int luaTerminateThread(lua_State *L)
{
	int n = -lua_gettop(L);
	HANDLE h = (HANDLE)lua_touserdata(L, n+0);
	DWORD exitcode = lua_tointeger(L, n+1);
	if (h == NULL)
		return 0;
	TerminateThread(h, exitcode);
	return 0;
}int luaWaitForSingleObject(lua_State *L)
{
	int n = -lua_gettop(L);
	HANDLE h = (HANDLE)lua_touserdata(L, n+0);
	if (h == NULL)
		return 0;
	DWORD timeout = INFINITE;
	if (n== -2)
		timeout = lua_tointeger(L, n+1);
	DWORD o = WaitForSingleObject(h, timeout);

	lua_pushinteger(L, o);
	return 1;
}

DWORD WINAPI luaCreateThreadEntry(LPVOID parameter)
{
	lua_State *L2 = (lua_State *)parameter;

	if (lua_isfunction(L2, 1))
		lua_mypcall(L2, lua_gettop(L2)-1, 0, 0);

	dwindow_lua_release_thread(L2);

	return 0;
}

int luaCreateThread(lua_State *L)
{
	int n = lua_gettop(L);

	lua_State *L2 =dwindow_lua_get_thread();
	for(int i=0; i<n; i++)
	{
		int ref = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_rawgeti(L2, LUA_REGISTRYINDEX, ref);
		lua_insert(L2, 1);
		luaL_unref(L, LUA_REGISTRYINDEX, ref);
	}

	lua_pushlightuserdata(L, CreateThread(NULL, NULL, luaCreateThreadEntry, L2, NULL, NULL));
	return 1;
}

CRITICAL_SECTION zero_cs = {0};
int luaDestroyCritSec(lua_State *L)
{
	CRITICAL_SECTION *cs = (CRITICAL_SECTION*)lua_touserdata(L, -1);
	if (!cs || memcmp(cs, &zero_cs, sizeof(zero_cs)) == 0)
		return 0;

	DeleteCriticalSection(cs);
	memset(cs, 0, sizeof(CRITICAL_SECTION));
	return 1;
}

int luaCreateCritSec(lua_State *L)
{
	CRITICAL_SECTION *cs = (CRITICAL_SECTION *)lua_newuserdata(L, sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(cs);

	bool b = setup_gc(L, &luaDestroyCritSec);

	return 1;
}

int luaLockCritSec(lua_State *L)
{
	CRITICAL_SECTION *cs = (CRITICAL_SECTION*)lua_touserdata(L, -1);
	if (!cs)
		return 0;

	EnterCriticalSection(cs);
	lua_pushboolean(L, 1);
	return 1;
}
int luaUnlockCritSec(lua_State *L)
{
	CRITICAL_SECTION *cs = (CRITICAL_SECTION*)lua_touserdata(L, -1);
	if (!cs)
		return 0;

	LeaveCriticalSection(cs);
	lua_pushboolean(L, 1);
	return 1;
}

int luaFAILED(lua_State *L)
{
	HRESULT hr = lua_tointeger(L, -1);
	lua_pushboolean(L, FAILED(hr));
	return 1;
}


int luaGetConfigFile(lua_State *L)
{
	wchar_t config_file[MAX_PATH];
	wcscpy(config_file, dwindow_log_get_filename());
	wcscpy(wcsrchr(config_file, '\\'), L"\\config.lua");

	lua_pushstring(L, W2UTF8(config_file));
	return 1;
}

int luaWriteConfigFile(lua_State *L)
{
	if (lua_gettop(L) < 1)
		return 0;
	if (!lua_isstring(L, -1))
		return 0;

	wchar_t config_file[MAX_PATH];
	wcscpy(config_file, dwindow_log_get_filename());
	wcscpy(wcsrchr(config_file, '\\'), L"\\config.lua");

	FILE * f = _wfopen(config_file, L"wb");
	if (!f)
		return 0;

	const char * p = lua_tostring(L, -1);
	fwrite(p, 1, strlen(p), f);
	fclose(f);

	lua_pushboolean(L, 1);
	return 1;
}

int luaGetSystemDefaultLCID(lua_State *L)
{
	lua_pushinteger(L, GetSystemDefaultLCID());
	return 1;
}

static int lua_restart_this_program(lua_State *L)
{
	restart_this_program(lua_toboolean(L, 1));
	return 1;
}

DWORD WINAPI luadebug_thread(LPVOID p)
{
	while(true)
	{
		Sleep(1000);
		int i1 = GetKeyState(VK_CONTROL);
		Sleep(1000);
		int i2 = GetKeyState(VK_CONTROL);

		if (i1 == i2 && i1 < 0)
		{
			CAutoLock lck(&g_csL);

			for(std::list<lua_State*>::iterator i = running_threads.begin(); i!= running_threads.end(); ++i)
			{
				char tmp[1024];
				sprintf(tmp, "thread %08x:\r\n", *i);
				OutputDebugStringA(tmp);
				track_back(*i);
			}
		}
	}
	return 0;
}

int lua_stop_all_handles(lua_State*L)
{
	stop_all_handles();
	lua_pushboolean(L, 1);
	return 1;
}

int lua_clear_all_handles(lua_State*L)
{
	clear_all_handles();
	lua_pushboolean(L, 1);
	return 1;
}


int lua_shellexecute(lua_State *L)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	int n = lua_gettop(L);

	lua_pushboolean(L, (int)ShellExecuteW(NULL, UTF82W(lua_tostring(L,1)), UTF82W(lua_tostring(L,2)), UTF82W(lua_tostring(L,3)), UTF82W(lua_tostring(L,4)), lua_tointeger(L,5)) > 32);

	return 1;
}


extern "C" int luaopen_cjson_safe(lua_State *l);
bool lua_inited = false;
int dwindow_lua_init () 
{
	if (lua_inited)
		return 0;

	dwindow_lua_exit();
	CAutoLock lck(&g_csL);
	int result;
	g_L = luaL_newstate();  /* create state */
	if (g_L == NULL) {
	  return EXIT_FAILURE;
	}

	/* open standard libraries */
	luaL_checkversion(g_L);
 	lua_gc(g_L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(g_L);  /* open libraries */
 	lua_gc(g_L, LUA_GCRESTART, 0);


	result = lua_toboolean(g_L, -1);  /* get result */

	// environment variables
	lua_manager *app_manager = new lua_manager("app");
	wchar_t config_path[MAX_PATH];
	wcscpy(config_path, dwindow_log_get_filename());
	((wchar_t*)wcsrchr(config_path, L'\\')) [1] = NULL;
	app_manager->get_variable("path") = g_apppath;
	app_manager->get_variable("config_path") = config_path;

	luaState L;
	luaL_dostring(L, "app.config = app.config_path .. \"config.lua\"");
	luaL_dostring(L, "app.cache_path = app.config_path .. \"cache\\\\\"");
	luaL_dostring(L, "app.plugin_path = app.path .. \"plugins\\\\\"");

	// setting
	g_lua_setting_manager = new lua_manager("setting");

	// utils
	g_lua_core_manager = new lua_manager("core");
	g_lua_core_manager->get_variable("ApplySetting") = &lua_ApplySetting;
	g_lua_core_manager->get_variable("FAILED") = &luaFAILED;
	g_lua_core_manager->get_variable("GetTickCount") = &lua_GetTickCount;
	g_lua_core_manager->get_variable("execute_luafile") = &execute_luafile;
	g_lua_core_manager->get_variable("execute_signed_luafile") = &execute_signed_luafile;
	g_lua_core_manager->get_variable("shellexecute") = &lua_shellexecute;
	g_lua_core_manager->get_variable("load_signed_string") = &load_signed_string;
	g_lua_core_manager->get_variable("loaddll") = &loaddll;
	g_lua_core_manager->get_variable("track_back") = &track_back;
	g_lua_core_manager->get_variable("http_request") = &http_request;
	g_lua_core_manager->get_variable("prefetch_http_file") = &lua_prefetch_http_file;
	g_lua_core_manager->get_variable("Sleep") = &lua_Sleep;

	// threads
	g_lua_core_manager->get_variable("ResumeThread") = &luaResumeThread;
	g_lua_core_manager->get_variable("SuspendThread") = &luaSuspendThread;
	g_lua_core_manager->get_variable("WaitForSingleObject") = &luaWaitForSingleObject;
	g_lua_core_manager->get_variable("CreateThread") = &luaCreateThread;
	g_lua_core_manager->get_variable("TerminateThread") = &luaTerminateThread;
	g_lua_core_manager->get_variable("CreateCritSec") = &luaCreateCritSec;
	g_lua_core_manager->get_variable("LockCritSec") = &luaLockCritSec;
	g_lua_core_manager->get_variable("UnlockCritSec") = &luaUnlockCritSec;
	g_lua_core_manager->get_variable("DestroyCritSec") = &luaDestroyCritSec;

	// file hooker
	g_lua_core_manager->get_variable("clear_all_handles") = &lua_clear_all_handles;
	g_lua_core_manager->get_variable("stop_all_handles") = &lua_stop_all_handles;


	g_lua_core_manager->get_variable("cjson") = &luaopen_cjson_safe;
	
	g_lua_core_manager->get_variable("GetConfigFile") = &luaGetConfigFile;
	g_lua_core_manager->get_variable("WriteConfigFile") = &luaWriteConfigFile;
	g_lua_core_manager->get_variable("GetSystemDefaultLCID") = &luaGetSystemDefaultLCID;
	g_lua_core_manager->get_variable("restart_this_program") = &lua_restart_this_program;

#ifdef VSTAR
	g_lua_core_manager->get_variable("v") = true;
#endif

#ifdef DEBUG
	CreateThread(NULL, NULL, luadebug_thread, NULL, NULL, NULL);
#endif

	lua_inited = true;
	return 0;
}

lua_State * dwindow_lua_get_thread()
{
	CAutoLock lck(&g_csL);
	if (lua_checkstack (g_L, 2) < 0)
		dwindow_log_line("lua stack overflowing\n");
	lua_State *rtn = lua_newthread(g_L);
	running_threads.push_back(rtn);
	return rtn;
}

void dwindow_lua_release_thread(lua_State * p)
{
	lua_settop(p, 0);
	CAutoLock lck(&g_csL);
	int count = lua_gettop(g_L);
	for(int i=1; i<=count; i++)
	{
		lua_State *pp = lua_tothread(g_L, i);
		if (pp == p)
		{
			lua_remove(g_L, i);
			break;
		}
	}

	for(std::list<lua_State*>::iterator i = running_threads.begin(); i!= running_threads.end(); ++i)
	{
		if (*i == p)
		{
			running_threads.erase(i);
			break;
		}
	}
}

luaState::luaState()
{
	L =dwindow_lua_get_thread();
}

luaState::~luaState()
{
	dwindow_lua_release_thread(L);
}
luaState::operator lua_State*()
{
	return L;
}

int dwindow_lua_exit()
{
	CAutoLock lck(&g_csL);
	if (g_L == NULL)
		return 0;

	lua_close(g_L);
	g_L = NULL;
	return 0;
}

lua_manager::lua_manager(const char* table_name)
{
	m_table_name = new char[strlen(table_name)+1];
	strcpy(m_table_name, table_name);

	luaState L;

	lua_getglobal(L, table_name);
	if (!lua_istable(L, -1))
	{
		lua_newtable(L);
		lua_setglobal(L, table_name);
	}
	lua_settop(L,0);
}

lua_manager::~lua_manager()
{
	std::list<lua_variable*>::iterator it;
	for(it = m_variables.begin(); it != m_variables.end(); it++)
		delete *it;
	delete [] m_table_name;
}

int lua_manager::refresh()
{
	// TODO
	return 0;
}

int lua_manager::delete_variable(const char*name)
{
	CAutoLock lck(&m_cs);
	std::list<lua_variable*>::iterator it;
	for(it = m_variables.begin(); it != m_variables.end(); it++)
	{
		if (strcmp((*it)->m_name, name) == 0)
		{
			lua_variable *tmp = *it;
			m_variables.remove(*it);
			delete tmp;
			return 1;
		}
	}
	
	return 0;
}

lua_variable & lua_manager::get_variable(const char*name)
{
	CAutoLock lck(&m_cs);
	std::list<lua_variable*>::iterator it;
	for(it = m_variables.begin(); it != m_variables.end(); it++)
	{
		if (strcmp((*it)->m_name, name) == 0)
		{
			return **it;
		}
	}

	lua_variable * p = new lua_variable(name, this);
	m_variables.push_back(p);
	return *p;
}

lua_const & lua_manager::get_const(const char*name)
{
	CAutoLock lck(&m_cs);
	std::list<lua_const*>::iterator it;
	for(it = m_consts.begin(); it != m_consts.end(); it++)
	{
		if (strcmp((*it)->m_name, name) == 0)
		{
			return **it;
		}
	}

	lua_const * p = new lua_const(name, this);
	m_consts.push_back(p);
	return *p;
}

lua_variable::lua_variable(const char*name, lua_manager *manager)
{
	m_manager = manager;
	m_name = new char [strlen(name)+1];
	strcpy(m_name, name);
}

lua_variable::~lua_variable()
{
	delete m_name;
};

lua_variable::operator int()
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);

	int o = lua_tointeger(L, -1);

	lua_pop(L, 2);
	
	return o;
}

void lua_variable::operator=(const int in)
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushinteger(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}


lua_variable::operator bool()
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);

	int o = lua_toboolean(L, -1);

	lua_pop(L, 2);

	return o == 0;
}

void lua_variable::operator=(const bool in)
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushboolean(L, in ? 1 : 0);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

lua_variable::operator double()
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);

	double o = lua_tonumber(L, -1);

	lua_pop(L, 2);

	return o;
}

void lua_variable::operator=(const double in)
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushnumber(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}


// lua_global_variable::operator const wchar_t*()
// {
// 	luaState L;
// 
// 	lua_getglobal(L, m_manager->m_table_name);
// 	lua_getfield(L, -1, m_name);
// 
// 	const char* o = lua_tostring(L, -1);
// 
// 	lua_pop(L, 2);
// 
// 	return o;
// }

void lua_variable::operator=(const wchar_t *in)
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushstring(L, W2UTF8(in));
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

void lua_variable::operator=(lua_CFunction func)
{
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}



// const
lua_const::lua_const(const char*name, lua_manager *manager)
{
	m_manager = manager;
	m_name = new char [strlen(name)+1];
	strcpy(m_name, name);
	m_type = _int;
	m_value.i = 0;

	read_from_lua();
}

int lua_const::read_from_lua()
{
	luaState L;
	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);
	
	if (lua_type(L, -1) == LUA_TNUMBER)
	{
		m_type = _double;
		m_value.d = lua_tonumber(L, -1);
	}
	else if (lua_type(L, -1) == LUA_TTABLE)
	{
		m_type = _rect;
		lua_getfield(L, -1, "left");
		m_value.r.left = lua_tointeger(L, -1);
		lua_pop(L,1);
		lua_getfield(L, -1, "right");
		m_value.r.right = lua_tointeger(L, -1);
		lua_pop(L,1);
		lua_getfield(L, -1, "top");
		m_value.r.top = lua_tointeger(L, -1);
		lua_pop(L,1);
		lua_getfield(L, -1, "bottom");
		m_value.r.bottom = lua_tointeger(L, -1);
		lua_pop(L,1);
	}
	else if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char * s = lua_tostring(L, -1);
		UTF82W w(s);
		m_type = _string;
		m_value.s = new wchar_t[wcslen(w)+1];
		wcscpy(m_value.s, w);
	}
	else if (lua_type(L, -1) == LUA_TBOOLEAN)
	{
		m_type = _bool;
		m_value.b = lua_toboolean(L, -1);
	}
	else
	{
		// lua variable not found, undefined behavior
#ifdef DEBUG
		printf("%s not found in setting\n", m_name);
		DebugBreak();	
#endif
		return -1;
	}

	lua_pop(L,2);

	return 0;
}

lua_const::~lua_const()
{
	delete m_name;
	if (m_type == _string && m_value.s)
		delete m_value.s;

};

lua_const::operator int()
{
	if (m_type == _int)
		return m_value.i;
	if (m_type == _double)
		return floor(m_value.d+0.5);
	if (m_type == _bool)
		return m_value.b ? 1 : 0;

	return 0;
}

int& lua_const::operator=(const int in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushinteger(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);

	if (m_type == _string && m_value.s) delete m_value.s;
	m_type = _int;
	m_value.i = in;

	return m_value.i;
}

lua_const::operator DWORD()
{
	if (m_type != _int && m_type != _double)
		return 0;

	return m_type == _int ? m_value.i : (m_value.d+0.5);
}

DWORD& lua_const::operator=(const DWORD in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushinteger(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);

	if (m_type == _string && m_value.s) delete m_value.s;
	m_type = _int;
	m_value.i = in;

	return *(DWORD*)&m_value.i;
}

lua_const::operator bool()
{
	if (m_type != _bool && m_type != _int)
		return 0;

	return m_type == _bool ? m_value.b : (m_value.i != 0);
}

bool& lua_const::operator=(const bool in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushboolean(L, in ? 1 : 0);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);

	if (m_type == _string && m_value.s) delete m_value.s;
	m_type = _bool;
	m_value.b = in;

	return m_value.b;
}

lua_const::operator double()
{
	if (m_type == _double)
		return m_value.d;
	if (m_type == _int)
		return m_value.i;
	return 0;
}

double& lua_const::operator=(const double in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushnumber(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);

	if (m_type == _string && m_value.s) delete m_value.s;
	m_type = _double;
	m_value.d = in;

	return m_value.d;
}


lua_const::operator const wchar_t*()
{
	if (m_type != _string)
		return 0;

	return m_value.s;
}

wchar_t *& lua_const::operator=(const wchar_t *in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushstring(L, W2UTF8(in));
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);

	if (m_type == _string && m_value.s) delete m_value.s;
	m_type = _string;
	m_value.s = new wchar_t[wcslen(in)+1];
	wcscpy(m_value.s, in);

	return m_value.s;
}

lua_const::operator RECT()
{
	RECT zero = {0};
	if (m_type != _rect)
		return zero;

	return m_value.r;
}

RECT & lua_const::operator=(const RECT in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_newtable(L);
	lua_pushinteger(L, in.left);
	lua_setfield(L, -2, "left");
	lua_pushinteger(L, in.right);
	lua_setfield(L, -2, "right");
	lua_pushinteger(L, in.top);
	lua_setfield(L, -2, "top");
	lua_pushinteger(L, in.bottom);
	lua_setfield(L, -2, "bottom");
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);

	if (m_type == _string && m_value.s) delete m_value.s;
	m_type = _rect;
	m_value.r = in;

	return m_value.r;
}


int lua_track_back(lua_State *L)
{
	const char* err = lua_tostring(L, -1);

#ifdef DEBUG
	OutputDebugStringA("----trackback----\r\n");
	char tmp[10240];
	lua_Debug debug;
	for(int level = 1; lua_getstack(L, level, &debug); level++)
	{
		int suc = lua_getinfo(L, "Sl", &debug);
		sprintf(tmp, "%s(%d,1) : %s \n", debug.source+1, debug.currentline, level == 1 ? (strrchr(err, ':') ? (strrchr(err, ':')+1) : err)  : "");
		OutputDebugStringA(tmp);
	}
	if (MessageBoxA(NULL, "Debug ? ", "Debug ? " , MB_YESNO) == IDYES)
	{
		lua_getglobal(L, "debug");
		lua_getfield(L, -1, "debug");
		lua_mypcall(L, 0, 0, NULL);
		lua_pop(L, 1);
	}
#else
	dwindow_log_line("lua error:%s", err);
#endif

	lua_pushstring(L, err);
	return 1;
}

int lua_mypcall(lua_State *L, int n, int r, int flag)
{
	lua_pushcfunction(L, &lua_track_back);
	lua_insert(L, lua_gettop(L) - n - 1);
	int o = lua_pcall(L, n, r, lua_gettop(L) - n - 1);
	lua_remove(L, -r-1);
	return o;
}

int lua_load_settings()
{
	luaState L;
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "load_settings");
	lua_mypcall(L, 0, 0, 0);
	lua_settop(L, 0);

	return 0;
}

int lua_save_settings(bool restore_play_after_reset /*=true */)
{
	luaState L;
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "save_settings");
	lua_pushboolean(L, restore_play_after_reset);
	lua_mypcall(L, 1, 0, 0);
	lua_settop(L, 0);

	return 0;
}