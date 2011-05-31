#include <time.h>
#include "global_funcs.h"
#include <Shlobj.h>
#include <streams.h>
#include "detours/detours.h"
#include "..\AESFile\E3DReader.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "detours/detoured.lib")
#pragma comment(lib, "detours/detours.lib")

#include <wininet.h>
#pragma comment(lib,"wininet.lib")

char server_url[300] = "http://59.51.45.21:80/w32.php?";

// public variables
AutoSetting<localization_language> g_active_language(L"Language", CHINESE);
char g_passkey[32];
char g_passkey_big[128];

int n_monitor_found = 0;
RECT monitor_rect[MAX_MONITORS];
BOOL CALLBACK monitor_enum_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	memcpy(monitor_rect+n_monitor_found, lprcMonitor, sizeof(RECT));
	n_monitor_found ++;

	if (n_monitor_found >= MAX_MONITORS)
		return FALSE;
	else
		return TRUE;
}

HRESULT split_span_rect(const RECT in, RECT *out1, RECT *out2)
{
	int width = in.right - in.left;
	int height = in.bottom - in.top;
	double aspect = (double)width / height;
	if (aspect > 2.425)
	{
		// horizontal span
		*out1 = in;
		*out2 = in;
		out1->right -= width/2;
		out2->left += width/2;

		return S_OK;
	}
	else if (0 < aspect && aspect < 1.2125)
	{
		// vertical span
		*out1 = in;
		*out2 = in;
		out1->bottom -= height/2;
		out2->top += height/2;

		return S_OK;
	}
	else
	{
		// normal aspect
		return E_FAIL;
	}
}

HRESULT get_monitors_rect(RECT *screen1, RECT *screen2)
{
	EnumDisplayMonitors(NULL, NULL, monitor_enum_proc, NULL);

	if (screen1 == NULL || screen2 == NULL)
		return E_POINTER;

	for(int i=0; i<n_monitor_found; i++)
		if (SUCCEEDED(split_span_rect(monitor_rect[i], screen1, screen2)))
			return S_OK;

	*screen1 = monitor_rect[0];
	*screen2 = monitor_rect[n_monitor_found-1];

	return S_OK;
}

bool browse_folder(wchar_t *out, HWND owner/* = NULL*/)
{
	BROWSEINFOW b;
	b.hwndOwner = owner;
	b.pidlRoot = NULL;
	b.pszDisplayName = out;
	b.lpszTitle = C(L"Select Folder..");
	b.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN ;
	b.lpfn = NULL;
	b.lParam = 0;
	b.iImage = 0;

	PIDLIST_ABSOLUTE pid = SHBrowseForFolderW(&b);
	if (pid != NULL)
	{
		return SHGetPathFromIDListW(pid, out);
	}
	return false;
}

bool open_file_dlg(wchar_t *pathname, HWND hDlg, wchar_t *filter/* = NULL*/)
{
	static wchar_t *default_filter = L"Video files\0"
		L"*.mp4;*.mkv;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls;*.3dv;*.e3d\0"
		L"Audio files\0"
		L"*.wav;*.ac3;*.dts;*.mp3;*.mp2;*.mpa;*.mp4;*.wma;*.flac;*.ape;*.avs\0"
		L"Subtitles\0*.srt;*.sup\0"
		L"All Files\0*.*\0"
		L"\0";
	if (filter == NULL) filter = default_filter;
	wchar_t strFileName[MAX_PATH] = L"";
	wchar_t strPath[MAX_PATH] = L"";

	wcsncpy(strFileName, pathname, MAX_PATH);
	wcsncpy(strPath, pathname, MAX_PATH);
	for(int i=(int)wcslen(strPath)-2; i>=0; i--)
		if (strPath[i] == L'\\')
		{
			strPath[i+1] = NULL;
			break;
		}

		OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW), hDlg , NULL,
			filter, NULL,
			0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
			C(L"Open File"),
			OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLESIZING, 0, 0,
			L".mp4", 0, NULL, NULL };

		int o = GetOpenFileNameW( &ofn );
		if (o)
		{
			wcsncpy(pathname, strFileName, MAX_PATH);
			return true;
		}

		return false;
}

bool select_color(DWORD *color, HWND parent)
{
	static COLORREF customColor[16];
	CHOOSECOLOR cc = {sizeof(CHOOSECOLOR), parent, NULL, *color, customColor, CC_RGBINIT, 0, NULL, NULL};
	bool rtn = ChooseColor(&cc);

	if (rtn)
		*color = cc.rgbResult;

	return rtn;
}

HRESULT RemoveDownstream(CComPtr<IPin> &input_pin)
{
	// check pin
	PIN_DIRECTION pd = PINDIR_OUTPUT;
	input_pin->QueryDirection(&pd);
	if (pd != PINDIR_INPUT)
		return E_FAIL;

	// run RemoveDownstream on each pin
	PIN_INFO pi;
	input_pin->QueryPinInfo(&pi);
	CComPtr<IBaseFilter> filter;
	filter.Attach(pi.pFilter);


	CComPtr<IEnumPins> ep;
	filter->EnumPins(&ep);
	CComPtr<IPin> pin;
	while (S_OK == ep->Next(1, &pin, NULL))
	{
		pin->QueryDirection(&pd);
		if (pd == PINDIR_OUTPUT)
		{
			CComPtr<IPin> connected;
			pin->ConnectedTo(&connected);
			if (connected)
				RemoveDownstream(connected);
		}

		pin = NULL;
	}

	// remove the filter
	FILTER_INFO fi;
	filter->QueryFilterInfo(&fi);
	CComPtr<IFilterGraph> graph;
	graph.Attach(fi.pGraph);
	graph->RemoveFilter(filter);

	return S_OK;
}

HRESULT ReadPlaylist(const wchar_t *strPlaylistFile, REFERENCE_TIME *rtDuration)
{
	DWORD read;
	BYTE Buff[100];
	bool bDuplicate = false;
	*rtDuration  = 0;

	HANDLE m_hFile   = CreateFileW(strPlaylistFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(m_hFile != INVALID_HANDLE_VALUE)
	{
		ReadFile(m_hFile, Buff, 4, &read, NULL);
		if (memcmp (Buff, "MPLS", 4))
		{
			CloseHandle(m_hFile);
			return VFW_E_INVALID_FILE_FORMAT;
		}

		ReadFile(m_hFile, Buff, 4, &read, NULL);
		if ((memcmp (Buff, "0200", 4)!=0) && (memcmp (Buff, "0100", 4)!=0))
		{
			CloseHandle(m_hFile);
			return VFW_E_INVALID_FILE_FORMAT;
		}

		DWORD				dwPos;
		DWORD				dwTemp;
		WORD				nPlaylistItems;

		ReadFile(m_hFile, Buff, 4, &read, NULL);
		dwPos = (Buff[0] << 24) + (Buff[1] << 16) + (Buff[2] << 8) + Buff[3];
		SetFilePointer(m_hFile, dwPos, NULL, FILE_BEGIN);

		SetFilePointer(m_hFile, 4, NULL, SEEK_CUR);
		SetFilePointer(m_hFile, 2, NULL, SEEK_CUR);
		ReadFile(m_hFile, Buff, 2, &read, NULL);
		nPlaylistItems = (Buff[0] << 8) + Buff[1];
		SetFilePointer(m_hFile, 2, NULL, SEEK_CUR);

		dwPos	  += 10;
		for (DWORD i=0; i<nPlaylistItems; i++)
		{
			SetFilePointer(m_hFile, dwPos, NULL, FILE_BEGIN);
			ReadFile(m_hFile, Buff, 2, &read, NULL);
			dwPos += (Buff[0] << 8) + Buff[1] + 2;
			ReadFile(m_hFile, Buff, 5, &read, NULL);

			// TODO: check duplication
			// TODO: save result

			SetFilePointer(m_hFile, 4, NULL, SEEK_CUR);
			SetFilePointer(m_hFile, 3, NULL, SEEK_CUR);

			ReadFile(m_hFile, Buff, 4, &read, NULL);
			dwTemp	= (Buff[0] << 24) + (Buff[1] << 16) + (Buff[2] << 8) + Buff[3];

			ReadFile(m_hFile, Buff, 4, &read, NULL);
			*rtDuration += (REFERENCE_TIME)((Buff[0] << 24) + (Buff[1] << 16) + (Buff[2] << 8) + Buff[3] - dwTemp) * 20000 / 90;
		}

		CloseHandle(m_hFile);
		return bDuplicate ? S_FALSE : S_OK;
	}

	return AmHresultFromWin32(GetLastError());
}

HRESULT find_main_movie(const wchar_t *folder, wchar_t *out)
{
	HRESULT	hr	= E_FAIL;

	wchar_t tmp_folder[MAX_PATH];
	wcscpy(tmp_folder, folder);
	if (tmp_folder[wcslen(tmp_folder)-1] == L'\\')
		tmp_folder[wcslen(tmp_folder)-1] = NULL;

	wchar_t	strFilter[MAX_PATH];
	wsprintfW(strFilter, L"%s\\BDMV\\PLAYLIST\\*.mpls", tmp_folder);

	WIN32_FIND_DATAW fd = {0};
	HANDLE hFind = FindFirstFileW(strFilter, &fd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		REFERENCE_TIME		rtMax	= 0;
		REFERENCE_TIME		rtCurrent;
		wchar_t	strCurrentPlaylist[MAX_PATH];
		do
		{
			wsprintfW(strCurrentPlaylist, L"%s\\BDMV\\PLAYLIST\\%s", tmp_folder, fd.cFileName);

			// Main movie shouldn't have duplicate files...
			if (ReadPlaylist(strCurrentPlaylist, &rtCurrent) == S_OK && rtCurrent > rtMax)
			{
				rtMax			= rtCurrent;

				wcscpy(out, strCurrentPlaylist);
				hr				= S_OK;
			}
		}
		while(FindNextFileW(hFind, &fd));

		FindClose(hFind);
	}

	return hr;
}

HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.
			{
				pTmp->Release();
			}
			else  // Unconnected, this is the pin we want.
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

HRESULT GetConnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr)) // Connected, this is the pin we want. 
			{
				pTmp->Release();
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
			else  // Unconnected, not the pin we want.
			{
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}


// localization
typedef struct _localization_element
{
	wchar_t *english;
	wchar_t *localized;
} localization_element;

int n_localization_element_count;
const int increase_step = 8;	// 2^8 = 256, increase when
localization_element *localization_table = NULL;
HRESULT hr_init_localization = set_localization_language(g_active_language);

HRESULT add_localization(const wchar_t *English, const wchar_t *Localized)
{
	if (Localized == NULL)
	{
		Localized = English;
	}

	if ((n_localization_element_count >> increase_step) << increase_step == n_localization_element_count)
	{
		localization_table = (localization_element*)realloc(localization_table, sizeof(localization_element) * (n_localization_element_count + (1<<increase_step)));
	}
	
	for(int i=0; i<n_localization_element_count; i++)
	{
		if (wcscmp(English, localization_table[i].english) == 0)
		{
			return S_FALSE;
		}
	}

	localization_table[n_localization_element_count].english = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(English)+1));
	wcscpy(localization_table[n_localization_element_count].english, English);
	localization_table[n_localization_element_count].localized = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(Localized)+1));
	wcscpy(localization_table[n_localization_element_count].localized, Localized);

	n_localization_element_count++;

	return S_OK;
}

HRESULT set_localization_language(localization_language language)
{
	g_active_language = language;

	for(int i=0; i<n_localization_element_count; i++)
	{
		if (localization_table[i].english)
			free(localization_table[i].english);
		if (localization_table[i].localized)
			free(localization_table[i].localized);
	}
	if (localization_table) free(localization_table);
	localization_table = NULL;
	n_localization_element_count = 0;

	switch (language)
	{
	case ENGLISH:
		{
			add_localization(L"Open File...");
			add_localization(L"Open BluRay3D");

			add_localization(L"Input Layout");
			add_localization(L"Auto");
			add_localization(L"Side By Side");
			add_localization(L"Top Bottom");
			add_localization(L"Monoscopic");

			add_localization(L"Output Mode");
			add_localization(L"Nvidia 3D Vision");
			add_localization(L"Monoscopic 2D");
			add_localization(L"Row Interlace");
			add_localization(L"Line Interlace");
			add_localization(L"Checkboard Interlace");
			add_localization(L"Dual Projector (Horizontal Span)");
			add_localization(L"Dual Projector (Vertical Span)");
			add_localization(L"Dual Projector (Independent)");
			add_localization(L"3DTV : Half Side By Side");
			add_localization(L"3DTV : Half Top Bottom");
			add_localization(L"Anaglyph");
			add_localization(L"Gerneral 120Hz Glasses");

			add_localization(L"Audio");

			add_localization(L"Subtitle");

			add_localization(L"Play/Pause");
			add_localization(L"Fullscreen");
			add_localization(L"Always Show Right Eye");
			add_localization(L"Exit");

			add_localization(L"No BD Drive Found");
			add_localization(L"Folder...");
			add_localization(L"None");
			add_localization(L"Load Subtitle File...");
			add_localization(L"Font...");
			add_localization(L"Color...");
			add_localization(L" (No Disc)");
			add_localization(L" (No Movie Disc)");
			add_localization(L"Select Folder..");
			add_localization(L"Open File");
			add_localization(L"Language");
			add_localization(L"Feature under development");
			add_localization(L"Open Failed");
		}
		break;
	case CHINESE:
		{
			add_localization(L"Open File...",			L"���ļ�...");
			add_localization(L"Open BluRay3D",			L"������3Dԭ��");

			add_localization(L"Input Layout",			L"�����ʽ");
			add_localization(L"Auto",					L"�Զ��ж�");
			add_localization(L"Side By Side",			L"���Ҹ�ʽ");
			add_localization(L"Top Bottom",				L"���¸�ʽ");
			add_localization(L"Monoscopic",				L"������ӰƬ");

			add_localization(L"Output Mode",					L"���ģʽ");
			add_localization(L"Nvidia 3D Vision",				L"Nvidia 3D Vision");
			add_localization(L"Monoscopic 2D",					L"ƽ��(2D)");
			add_localization(L"Row Interlace",					L"�н���");
			add_localization(L"Line Interlace",					L"�н���");
			add_localization(L"Checkboard Interlace",			L"���̽���");
			add_localization(L"Dual Projector (Horizontal Span)",L"˫ͶӰ(ˮƽ��Խģʽ)");
			add_localization(L"Dual Projector (Vertical Span)",	L"˫ͶӰ(��ֱ��Խģʽ)");
			add_localization(L"Dual Projector (Independent)",	L"˫ͶӰ(��������ģʽ)");
			add_localization(L"3DTV : Half Side By Side",		L"3D����(���Ұ��)");
			add_localization(L"3DTV : Half Top Bottom",			L"3D����(���°��)");
			add_localization(L"Anaglyph",						L"��ɫ�۾�");
			add_localization(L"Gerneral 120Hz Glasses",			L"��ͨ120Hz�۾�");

			add_localization(L"Audio",					L"��Ƶ");

			add_localization(L"Subtitle",				L"��Ļ");

			add_localization(L"Play/Pause",				L"����/��ͣ");
			add_localization(L"Fullscreen",				L"ȫ��");
			add_localization(L"Always Show Right Eye",	L"������ʾ����");
			add_localization(L"Exit",					L"�˳�");

			add_localization(L"No BD Drive Found",		L"δ�ҵ�����������");
			add_localization(L"Folder...",				L"�ļ���...");
			add_localization(L"None",					L"��");
			add_localization(L"Load Subtitle File...",	L"���ļ�����...");
			add_localization(L"Font...",				L"����...");
			add_localization(L"Color...",				L"��ɫ...");
			add_localization(L" (No Disc)",				L" (�޹���)");
			add_localization(L" (No Movie Disc)",		L" (�޵�Ӱ����)");
			add_localization(L"Select Folder..",		L"ѡ���ļ���..");
			add_localization(L"Open File",				L"���ļ�");
			add_localization(L"Language",				L"����");
			add_localization(L"Feature under development",				L"��δ��ɵĹ���");
			add_localization(L"Open Failed",			L"��ʧ��");
		}
		break;
	}
	return S_OK;
}

wchar_t *C(const wchar_t *English)
{
	for(int i=0; i<n_localization_element_count; i++)
	{
		if (wcscmp(localization_table[i].english, English) == 0)
			return localization_table[i].localized;
	}
	
	return NULL;
};

// CoreMVC
HRESULT write_property(IPropertyBag *bag, const wchar_t *property_to_write)
{
	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = NULL;
	if (property_to_write)
		var.bstrVal =  SysAllocString(property_to_write);

	HRESULT hr = bag->Write(L"Settings", &var);
	if (property_to_write)
		SysFreeString(var.bstrVal);

	return hr;
}
// hook
static bool coremvc_hooked = false;
static DWORD (WINAPI *TrueGetModuleFileNameA)(HMODULE hModule, LPSTR lpFilename, DWORD nSize) = GetModuleFileNameA;
DWORD WINAPI MineGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
	DWORD rtn = TrueGetModuleFileNameA(hModule, lpFilename, nSize);
	if (hModule == NULL)
		strcat(lpFilename, "StereoPlayer.exe");
	return rtn;
}

HRESULT beforeCreateCoreMVC()
{
	if (!coremvc_hooked)
	{
		// apihook
		DetourRestoreAfterWith();
		DetourTransactionBegin();

		int id = GetCurrentThreadId();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)TrueGetModuleFileNameA, MineGetModuleFileNameA);
		LONG error = DetourTransactionCommit();
		coremvc_hooked = true;
	}

	return S_OK;
}

HRESULT ActiveCoreMVC(IBaseFilter *decoder)
{

	CComQIPtr<IPropertyBag, &IID_IPropertyBag> pbag(decoder);
	if (pbag)
	{
		write_property(pbag, L"use_tray=0");
		write_property(pbag, L"low_latency=0");
		write_property(pbag, L"use_cuda=0");
		return write_property(pbag, L"app_mode=1");
	}
	else
	{
		return E_FAIL;
	}
}

bool isUselessFilter(IBaseFilter *filter)
{
	bool has_output = false;
	bool connected = false;
	bool has_connected_input = false;
	bool has_connected_output = false;

	CComPtr<IEnumPins> ep;
	CComPtr<IPin> pin;
	filter->EnumPins(&ep);
	while (ep->Next(1, &pin, NULL) == S_OK)
	{
		PIN_DIRECTION pd;
		pin->QueryDirection(&pd);
		CComPtr<IPin> tmp;
		pin->ConnectedTo(&tmp);

		if (tmp != NULL)
			connected = true;
		if (pd == PINDIR_OUTPUT)
			has_output = true;
		if (pd == PINDIR_OUTPUT && tmp != NULL)
			has_connected_output = true;
		if (pd == PINDIR_INPUT && tmp != NULL)
			has_connected_input = true;

		pin = NULL;
	}

	if (!connected)
		return true;
	else if (has_output && !has_connected_output)
		return true;
	else if (has_connected_input && !has_output)
		return false;
	else if (has_connected_output)
		return false;
	else
		return true;
}

HRESULT RemoveUselessFilters(IGraphBuilder *gb)
{
	CComPtr<IEnumFilters> ef;
	gb->EnumFilters(&ef);

	CComPtr<IBaseFilter> filter;
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		if (isUselessFilter(filter))
		{
			FILTER_INFO fi;
			filter->QueryFilterInfo(&fi);
			if (fi.pGraph) fi.pGraph->Release();

			wprintf(L"Removed %s...\n", fi.achName);
			gb->RemoveFilter(filter);
			ef->Reset();
		}

		filter = NULL;
	}

	return S_OK;
}

HRESULT GetPinByName(IBaseFilter *pFilter, PIN_DIRECTION PinDir, const wchar_t *name, IPin **ppPin)
{
	*ppPin = NULL;
	CComPtr<IEnumPins> ep;
	pFilter->EnumPins(&ep);
	CComPtr<IPin> pin;
	while (ep->Next(1, &pin, NULL) == S_OK)
	{
		PIN_INFO pi;
		pin->QueryPinInfo(&pi);

		if (pi.pFilter)
			pi.pFilter->Release();

		PIN_DIRECTION ThisPinDir;
		pin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir && wcsstr(pi.achName, name))
		{
			*ppPin = pin;
			(*ppPin)->AddRef();
			return S_OK;
		}

		pin = NULL;
	}

	return E_FAIL;
}


HRESULT check_passkey()
{
	DWORD e[32];
	DWORD m1[32];
	BigNumberSetEqualdw(e, 65537, 32);
	RSA(m1, (DWORD*)g_passkey_big, e, (DWORD*)dwindow_n, 32);
	for(int i=0; i<8; i++)
		if (m1[i] != m1[i+8] || m1[i+8] != m1[i+16] || m1[i+16] != m1[i+24])
			return E_FAIL;
	memcpy(g_passkey, m1, 32);
	return S_OK;
}

HRESULT load_passkey()
{
	memset(g_passkey_big, 0x38, 128);
	load_setting(L"passkey", g_passkey_big, 128);
	return check_passkey();
}

HRESULT save_passkey()
{
	save_setting(L"passkey", g_passkey_big, 128);
	return S_OK;
}

HRESULT load_e3d_key(const char *file_hash, char *file_key)
{
	wchar_t tmp[3] = L"";
	wchar_t reg_key[41] = L"";
	for(int i=0; i<20; i++)
	{
		wsprintfW(tmp, L"%02X", file_hash[i]);
		wcscat(reg_key, tmp);
	}

	// AES it
	AESCryptor codec;
	codec.set_key((unsigned char*)g_passkey, 256);
	codec.decrypt((unsigned char*)file_key, (unsigned char*)file_key);
	codec.decrypt((unsigned char*)file_key+16, (unsigned char*)file_key+16);

	load_setting(reg_key, file_key, 32);

	return S_OK;
}

HRESULT save_e3d_key(const char *file_hash, const char *file_key)
{
	wchar_t tmp[3] = L"";
	wchar_t reg_key[41] = L"";
	for(int i=0; i<20; i++)
	{
		wsprintfW(tmp, L"%02X", file_hash[i]);
		wcscat(reg_key, tmp);
	}

	// AES it
	unsigned char encrypted_key[32];
	AESCryptor codec;
	codec.set_key((const unsigned char*)g_passkey, 256);
	codec.encrypt((const unsigned char*)file_key, (unsigned char*)encrypted_key);
	codec.encrypt((const unsigned char*)file_key+16, (unsigned char*)encrypted_key+16);

	save_setting(reg_key, encrypted_key, 32);

	return S_OK;
}

HRESULT make_xvid_support_mp4v()
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\GNU\\XviD", 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	DWORD value = 4, size=4;
	ret = RegQueryValueExW(hkey, L"Supported_4CC", 0, NULL, (LPBYTE)&value, (LPDWORD)&size);
	value |= 4;
	ret = RegSetValueExW(hkey, L"Supported_4CC", 0, REG_DWORD, (const byte*)&value, size );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	RegCloseKey(hkey);
	return S_OK;
}

HRESULT DeterminPin(IPin *pin, wchar_t *name, CLSID majortype)
{
	if (NULL == pin)
		return E_POINTER;

	PIN_INFO pi;
	pin->QueryPinInfo(&pi);
	if (pi.pFilter) pi.pFilter->Release();
	if (name)
	{
		if (wcsstr(pi.achName, name))
			return S_OK;
		else
			return S_FALSE;
	}

	if (majortype != CLSID_NULL)
	{
		CComPtr<IEnumMediaTypes> em;
		pin->EnumMediaTypes(&em);
		AM_MEDIA_TYPE *mt = NULL;
		while (em->Next(1, &mt, NULL) == S_OK)
		{
			if (mt->majortype == majortype)
				return S_OK;
			DeleteMediaType(mt);
		}

		return S_FALSE;
	}

	return E_INVALIDARG;
}

HRESULT localize_menu(HMENU menu)
{
	HRESULT hr = S_OK;
	const int max_item = 999;
	for(int i=0; i<max_item; i++)
	{
		HMENU sub = GetSubMenu(menu, i);
		if (sub)
		{
			hr = localize_menu(sub);
		}
		//else
		{
			int flag = GetMenuState(menu, i, MF_BYPOSITION);
			if (flag == -1)
				return hr;
			flag &= 0xffff;
			int uid = GetMenuItemID(menu, i);
			wchar_t text[1024];
			GetMenuStringW(menu, i, text, 1024, MF_BYPOSITION);

			wchar_t *local = C(text);
			if (NULL == local) local = text;
			if (sub)
				ModifyMenuW(menu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)sub, local);
			else
				ModifyMenuW(menu, i, MF_BYPOSITION | flag, uid, local);
		}
	}

	return hr;
}

const WCHAR* soft_key= L"Software\\DWindow";
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE)
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return false;
	ret = RegSetValueExW(hkey, key, 0, REG_TYPE, (const byte*)data, len );
	if (ret != ERROR_SUCCESS)
		return false;

	RegCloseKey(hkey);
	return true;
}

int load_setting(const WCHAR *key, void *data, int len)
{
	HKEY hkey = NULL;
	int ret = RegOpenKeyExW(HKEY_CURRENT_USER, soft_key,0,STANDARD_RIGHTS_REQUIRED |KEY_READ  , &hkey);
	if (ret != ERROR_SUCCESS || hkey == NULL)
		return false;
	RegQueryValueExW(hkey, key, 0, NULL, (LPBYTE)data, (LPDWORD)&len);
	if (ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA)
		return len;

	RegCloseKey(hkey);
	return 0;
}

HRESULT download_url(char *url_to_download, char *out, int outlen = 64)
{
	HINTERNET HI;
	HI=InternetOpenA("dwindow",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if (HI==NULL)
		return E_FAIL;

	HINTERNET HURL;
	HURL=InternetOpenUrlA(HI, url_to_download,NULL,0,INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_AUTO_REDIRECT,0);
	if (HURL==NULL)
		return E_FAIL;

	DWORD byteread = 0;
	BOOL internetreadfile = InternetReadFile(HURL,out, outlen, &byteread);

	if (!internetreadfile)
		return E_FAIL;

	return S_OK;
}

HRESULT download_e3d_key(const wchar_t *filename)
{
	HRESULT hr;
	file_reader reader;
	HANDLE h_file = CreateFileW (filename, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	reader.SetFile(h_file);
	if (!reader.m_is_encrypted)
	{
		hr = E_FAIL;
		goto err_ret;
	}

	load_passkey();
	dwindow_message_uncrypt message;
	message.zero = 0;
	memcpy(message.passkey, g_passkey, 32);
	memcpy(message.requested_hash, reader.m_hash, 20);
	srand(time(NULL));
	for(int i=0; i<32; i++)
		message.random_AES_key[i] = rand() & 0xff;

	unsigned char encrypted_message[128];
	RSA_dwindow_public(&message, encrypted_message);

	char url[300] = "";
	char tmp[3];
	strcpy(url, server_url);
	for(int i=0; i<128; i++)
	{
		sprintf(tmp, "%02X", encrypted_message[i]);
		strcat(url, tmp);
	}

	char str_e3d_key[65] = "D3821F7B81206903280461E52DE2B29901B9B458836B3795DD40F50C2583EF7A";
	hr = download_url(url, str_e3d_key);
	if (FAILED(hr))
		goto err_ret;

	unsigned char e3d_key[36];
	for(int i=0; i<32; i++)
		sscanf(str_e3d_key+i*2, "%02X", e3d_key+i);

	AESCryptor aes;
	aes.set_key(message.random_AES_key, 256);
	aes.decrypt(e3d_key, e3d_key);
	aes.decrypt(e3d_key+16, e3d_key+16);
	reader.set_key(e3d_key);

	CloseHandle(h_file);
	return e3d_set_process_key(e3d_key);


err_ret:
	CloseHandle(h_file);
	return hr;
}