#include <math.h>
#include "resource.h"
#include "dx_player.h"
#include "global_funcs.h"
#include "private_filter.h"
#include "..\libchecksum\libchecksum.h"
#include "..\AESFile\E3DReader.h"
#include "latency_dialog.h"
#include "..\mySplitter\audio_downmix.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#define JIF(x) if (FAILED(hr=(x))){goto CLEANUP;}
#define DS_EVENT (WM_USER + 4)
AutoSetting<BOOL> g_ExclusiveMode(L"ExclusiveMode", false, REG_DWORD);

#include "bomb_network.h"

// helper functions

DWORD color_GDI2ARGB(DWORD in)
{
	BYTE tmp[5];
	*((DWORD*)tmp) = in;
	tmp[3] = 0xff;

	tmp[4] = tmp[0];
	tmp[0] = tmp[2];
	tmp[2] = tmp[4];

	return *((DWORD*)tmp);
}

HRESULT dx_player::CrackPD10(IBaseFilter *filter)
{
	if (!filter)
		return E_POINTER;

	// check if PD10 decoder
	CLSID filter_id;
	filter->GetClassID(&filter_id);
	if (filter_id != CLSID_PD10_DECODER)
		return E_FAIL;

	// query graph builder
	FILTER_INFO fi;
	filter->QueryFilterInfo(&fi);
	if (!fi.pGraph)
		return E_FAIL; // not in a graph
	CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> gb(fi.pGraph);
	fi.pGraph->Release();

	// create source and demuxer and add to graph
	CComPtr<IBaseFilter> h264;
	CComPtr<IBaseFilter> demuxer;
	h264.CoCreateInstance(CLSID_AsyncReader);
	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> h264_control(h264);
	myCreateInstance(CLSID_PD10_DEMUXER, IID_IBaseFilter, (void**)&demuxer);

	if (demuxer == NULL)
		return E_FAIL;	// demuxer not registered

	gb->AddFilter(h264, L"MVC");
	gb->AddFilter(demuxer, L"Demuxer");

	// write active file and load
	unsigned int mvc_data[149] = {0x01000000, 0x29006467, 0x7800d1ac, 0x84e52702, 0xa40f0000, 0x00ee0200, 0x00000010, 0x00806f01, 0x00d1ac29, 0xe5270278, 0x0f000084, 0xee0200a4, 0xaa4a1500, 0xe0f898b2, 0x207d0000, 0x00701700, 0x00000080, 0x63eb6801, 0x0000008b, 0xdd5a6801, 0x0000c0e2, 0x7a680100, 0x00c0e2de, 0x6e010000, 0x00070000, 0x65010000, 0x9f0240b8, 0x1f88f7fe, 0x9c6fcb32, 0x16734a68, 0xc9a57ff0, 0x86ed5c4b, 0xac027e73, 0x0000fca8, 0x03000003, 0x00030000, 0x00000300, 0xb4d40303, 0x696e5f00, 0x70ac954a, 0x00030000, 0x03000300, 0x030000ec, 0x0080ca00, 0x00804600, 0x00e02d00, 0x00401f00, 0x00201900, 0x00401c00, 0x00c01f00, 0x00402600, 0x00404300, 0x00808000, 0x0000c500, 0x00d80103, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00080800, 0x54010000, 0xe0450041, 0xfe9f820c, 0x00802ab5, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0xab010003};
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscat(tmp, L"ac.mvc");
	FILE *f = _wfopen(tmp, L"wb");
	if(!f)
		return E_FAIL;	// failed writing file
	fwrite(mvc_data,1,596,f);
	fflush(f);
	fclose(f);

	h264_control->Load(tmp, NULL);
	
	// connect source & demuxer
	CComPtr<IPin> h264_o;
	GetUnconnectedPin(h264, PINDIR_OUTPUT, &h264_o);
	CComPtr<IPin> demuxer_i;
	GetUnconnectedPin(demuxer, PINDIR_INPUT, &demuxer_i);
	gb->ConnectDirect(h264_o, demuxer_i, NULL);

	// connect demuxer & decoder
	CComPtr<IPin> demuxer_o;
	GetUnconnectedPin(demuxer, PINDIR_OUTPUT, &demuxer_o);
	CComPtr<IPin> decoder_i;
	GetConnectedPin(filter, PINDIR_INPUT, &decoder_i);
	if (NULL == decoder_i)
		GetUnconnectedPin(filter, PINDIR_INPUT, &decoder_i);
	CComPtr<IPin> decoder_up;
	decoder_i->ConnectedTo(&decoder_up);
	if (decoder_up)
	{
		gb->Disconnect(decoder_i);
		gb->Disconnect(decoder_up);
	}
	gb->ConnectDirect(demuxer_o, decoder_i, NULL);

	// remove source & demuxer, and reconnect decoder(if it is connected before)
	
	gb->RemoveFilter(h264);
	gb->RemoveFilter(demuxer);
	if (decoder_up)gb->ConnectDirect(decoder_up, decoder_i, NULL);

	// delete file
	_wremove(tmp);

	return S_OK;
}

RECT rect_zero = {0,0,0,0};
inline bool compare_rect(const RECT in1, const RECT in2)
{
	return memcmp(&in1, &in2, sizeof(RECT)) == 0;
}

// constructor & destructor
LOGFONTW empty_logfontw = {0};
dx_player::dx_player(HINSTANCE hExe):
m_log(NULL),
m_renderer1(NULL),
dwindow(m_screen1, m_screen2),
m_lFontPointSize(L"FontSize", 40),
m_FontName(L"Font", g_active_language == CHINESE ? L"����" : L"Arial"),
m_FontStyle(L"FontStyle", L"Regular"),
m_font_color(L"FontColor", 0x00ffffff),
m_input_layout(L"InputLayout", input_layout_auto),
m_output_mode(L"OutputMode", anaglyph),
m_mask_mode(L"MaskMode", row_interlace),
m_display_subtitle(L"DisplaySubtitle", true),
m_anaglygh_left_color(L"AnaglyghLeftColor", RGB(255,0,0)),
m_anaglygh_right_color(L"AnaglyghRightColor", RGB(0,255,255)),
m_volume(L"Volume", 1.0),
m_aspect(L"Aspect", -1),
m_subtitle_latency(L"SubtitleLatency", 0),
m_subtitle_ratio(L"SubtitleRatio", 1.0),
m_bitstreaming(L"BitStreaming", false),
m_saved_screen1(L"Screen1", rect_zero),
m_saved_screen2(L"Screen2", rect_zero),
m_saved_rect1(L"Window1", rect_zero),
m_saved_rect2(L"Window2", rect_zero),
m_useLAV(L"LAV", true),
m_downmix(L"Downmix", false),
m_forced_deinterlace(L"ForcedDeinterlace", false),
m_saturation(L"Saturation", 0.5),
m_luminance(L"Luminance", 0.5),
m_hue(L"Hue", 0.5),
m_contrast(L"Contrast", 0.5),
m_saturation2(L"Saturation2", 0.5),
m_luminance2(L"Luminance2", 0.5),
m_hue2(L"Hue2", 0.5),
m_contrast2(L"Contrast2", 0.5),
m_theater_owner(NULL),
m_trial_shown(L"Trial", false),
m_LogFont(L"LogFont", empty_logfontw),
m_aspect_mode(L"AspectRatioMode", aspect_letterbox)
{
	detect_monitors();

	// Enable away mode and prevent the sleep idle time-out.
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_AWAYMODE_REQUIRED);

	// font init
	select_font(false);
	m_grenderer.set_font(m_font);
	m_grenderer.set_font_color(m_font_color);

	// playlist
	m_playlist_playing = m_playlist_count = 0;
	m_playlist[0] = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t) * max_playlist);
	for(int i=0; i<max_playlist; i++)
		m_playlist[i] = m_playlist[0] + i * MAX_PATH;

	// subtitle
	m_lastCBtime = -1;
	m_srenderer = NULL;

	// vars
	m_dragging = -1;
	m_reset_and_load = false;
	m_file_loaded = false;
	m_select_font_active = false;
	m_log = (wchar_t*)malloc(100000);
	m_log[0] = NULL;
	m_mouse.x = m_mouse.y = -10;
	m_bar_drawer.load_resource(hExe);
	m_revert = false;
	m_mirror1 = 0;
	m_mirror2 = 0;
	m_letterbox_delta = 0.0;
	m_parallax = 0;
	m_subtitle_center_x = 0.5;
	m_subtitle_bottom_y = 0.95;
	m_hexe = hExe;
	m_user_offset = 0;
	m_internel_offset = 10; // offset set to 10*0.1% of width

	// size and position
	init_window_size_positions();
	
	// show it!
	show_window(1, true);
	show_window(2, m_output_mode == dual_window || m_output_mode == iz3d);

	// to init video zone
	SendMessage(m_hwnd1, WM_INITDIALOG, 0, 0);
	SendMessage(m_hwnd2, WM_INITDIALOG, 0, 0);

	// set timer for ui drawing
	reset_timer(2, 125);

	// init dshow
	init_direct_show();

	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(m_gb);
	event_ex->SetNotifyWindow((OAHWND)id_to_hwnd(1), DS_EVENT, 0);

	// network bomb thread
	CreateThread(0,0,bomb_network_thread, id_to_hwnd(1), NULL, NULL);
	CreateThread(0,0,ad_thread, id_to_hwnd(1), NULL, NULL);
}

HRESULT dx_player::detect_monitors()
{
	::detect_monitors();
	// detect monitors
	m_screen1 = m_saved_screen1;
	m_screen2 = m_saved_screen2;
	bool saved_screen1_exist = false;
	bool saved_screen2_exist = false;
	for(int i=0; i<get_mixed_monitor_count(true,true); i++)
	{
		RECT rect;
		get_mixed_monitor_by_id(i, &rect, NULL, true, true);
		if (compare_rect(rect, m_saved_screen1))
			saved_screen1_exist = true;
		if (compare_rect(rect, m_saved_screen2))
			saved_screen2_exist = true;
	}

	if (compare_rect(m_saved_screen1, rect_zero) || compare_rect(m_saved_screen2, rect_zero) ||
		!saved_screen1_exist || !saved_screen2_exist)
	{
		// reset position if monitor changed.
		log_line(L"monitor changed, resetting.\n");
		if (g_logic_monitor_count == 1)
			m_screen1 = m_screen2 = g_logic_monitor_rects[0];
		else if (g_logic_monitor_count == 2)
		{
			m_screen1 = g_logic_monitor_rects[0];
			m_screen2 = g_logic_monitor_rects[1];
		}

		m_saved_rect1 = rect_zero;
		m_saved_rect2 = rect_zero;
	}

	m_saved_screen1 = m_screen1;
	m_saved_screen2 = m_screen2;

	return S_OK;
}

HRESULT dx_player::init_window_size_positions()
{
	// window size & pos
	int width1 = m_screen1.right - m_screen1.left;
	int height1 = m_screen1.bottom - m_screen1.top;
	int width2 = m_screen2.right - m_screen2.left;
	int height2 = m_screen2.bottom - m_screen2.top;

	SetWindowPos(id_to_hwnd(1), NULL, m_screen1.left, m_screen1.top, width1, height1, SWP_NOZORDER);

	RECT result;
	GetClientRect(id_to_hwnd(1), &result);

	int dcx = m_screen1.right - m_screen1.left - (result.right - result.left);
	int dcy = m_screen1.bottom - m_screen1.top - (result.bottom - result.top);

	if (compare_rect(m_saved_screen1, rect_zero) || compare_rect(m_saved_screen2, rect_zero) || compare_rect(m_saved_rect1, rect_zero))
	{
		const double ratio = 0.1;
		SetWindowPos(id_to_hwnd(1), NULL, m_screen1.left + width1*ratio, m_screen1.top + height1*ratio,
			width1*(1-ratio*2) + dcx, height1*(1-ratio*2) + dcy, SWP_NOZORDER);
		SetWindowPos(id_to_hwnd(2), NULL, m_screen2.left + width2*ratio, m_screen2.top + height2*ratio,
			width2*(1-ratio*2) + dcx, height2*(1-ratio*2) + dcy, SWP_NOZORDER);

		log_line(L"reset position:%dx%d\n", width1, height1);
	}
	else
	{
		RECT r1 = m_saved_rect1;
		RECT r2 = m_saved_rect2;

		SetWindowPos(id_to_hwnd(1), NULL, r1.left, r1.top,
			r1.right - r1.left, r1.bottom - r1.top, SWP_NOZORDER);
		SetWindowPos(id_to_hwnd(2), NULL, r2.left, r2.top,
			r2.right - r2.left, r2.bottom - r2.top, SWP_NOZORDER);

		log_line(L"use position:%dx%d\n", width1, height1);

	}

	return S_OK;
}

HRESULT dx_player::set_output_monitor(int out_id, int monitor_id)
{

	::detect_monitors();
	if (monitor_id < 0 || monitor_id >= get_mixed_monitor_count(true, true))
		return E_FAIL;
	if (out_id<0 || out_id>1)
		return E_FAIL;

	bool toggle = false;
	if (m_full1 || m_full2)
	{
		toggle = true;
		toggle_fullscreen();
	}

	if (out_id)
		get_mixed_monitor_by_id(monitor_id, &m_screen2, NULL, true, true);
		//m_screen2 = g_logic_monitor_rects[monitor_id];
	else
		//m_screen1 = g_logic_monitor_rects[monitor_id];
		get_mixed_monitor_by_id(monitor_id, &m_screen1, NULL, true, true);


	m_saved_screen1 = m_screen1;
	m_saved_screen2 = m_screen2;
	m_saved_rect1 = rect_zero;
	init_done_flag = 0;

	init_window_size_positions();

	init_done_flag = 0x12345678;
	if (toggle)
		toggle_fullscreen();

	return S_OK;
}

dx_player::~dx_player()
{
	free(m_playlist[0]);

	if (m_log)
	{
		free(m_log);
		m_log = NULL;
	}
	close_and_kill_thread();
	CAutoLock lock(&m_draw_sec);
	exit_direct_show();
	delete m_renderer1;

	// Clear EXECUTION_STATE flags to disable away mode and allow the system to idle to sleep normally.
	SetThreadExecutionState(ES_CONTINUOUS);
}

HRESULT dx_player::reset()
{
	BRC();
	log_line(L"reset!");
	CAutoLock lock(&m_draw_sec);


	// reinit
	exit_direct_show();
	init_direct_show();
	CAutoLock lck(&m_subtitle_sec);
	m_srenderer = NULL;
	m_external_subtitles.RemoveAll();

	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(m_gb);
	event_ex->SetNotifyWindow((OAHWND)id_to_hwnd(1), DS_EVENT, 0);

	// caption
	set_window_text(1, L"");
	set_window_text(2, L"");
	
	// repaint
	InvalidateRect(m_hwnd1, NULL, FALSE);

	return S_OK;
}

DWORD WINAPI dx_player::select_font_thread(LPVOID lpParame)
{
	dx_player *_this = (dx_player*) lpParame;

	_this->m_select_font_active = true;

	HRESULT hr = E_POINTER;
	{
		CAutoLock lck(&_this->m_subtitle_sec);
		if (_this->m_srenderer)
			hr = _this->m_srenderer->set_font_color(_this->m_font_color);
	}
	if (SUCCEEDED(hr))
	{
		_this->select_font(true);
		CAutoLock lck(&_this->m_subtitle_sec);
		if (_this->m_srenderer)
		{
			_this->m_srenderer->set_font(_this->m_font);
			_this->m_srenderer->set_font_color(_this->m_font_color);
			_this->m_lastCBtime = -1;
		}
	}
	_this->m_select_font_active = false;

	return 0;
}

// play control
HRESULT dx_player::play()
{
	BRC();
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Run();
}
HRESULT dx_player::pause()
{
	BRC();
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	OAFilterState fs;
	m_mc->GetState(INFINITE, &fs);
	if(fs == State_Running)
		return m_mc->Pause();
	else
		return m_mc->Run();
}
HRESULT dx_player::stop()
{
	BRC();
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Stop();
}
HRESULT dx_player::seek(int time)
{
	BRC();
	CAutoLock lock(&m_seek_sec);
	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

	REFERENCE_TIME target = (REFERENCE_TIME)time *10000;

	if(m_renderer1) m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0);				// refresh subtitle on next frame
	printf("seeking to %I64d\n", target);
	HRESULT hr = m_ms->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
	m_ms->GetPositions(&target, NULL);

	printf("seeked to %I64d\n", target);
	m_lastCBtime = (REFERENCE_TIME)time * 10000;

	OAFilterState state = State_Running;
	m_mc->GetState(500, &state);
	if (state != State_Running)
		m_mc->StopWhenReady();

	return hr;
}
HRESULT dx_player::tell(int *time)
{
	CAutoLock lock(&m_seek_sec);

	if (time == NULL)
		return E_POINTER;

	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

    REFERENCE_TIME current;
	HRESULT hr = m_ms->GetCurrentPosition(&current);
	if(SUCCEEDED(hr))
		*time = (int)(current / 10000);
	return hr;
}
HRESULT dx_player::total(int *time)
{
	CAutoLock lock(&m_seek_sec);
	if (time == NULL)
		return E_POINTER;

	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

	REFERENCE_TIME total;
	HRESULT hr = m_ms->GetDuration(&total);
	if(SUCCEEDED(hr))
		*time = (int)(total / 10000);
	return hr;
}
HRESULT dx_player::set_volume(double volume)
{
	BRC();

	if (volume > 1)
		volume = 1;

	if (volume < 0)
		volume = 0;

	m_volume = volume;

	if (m_ba)
	{
		LONG ddb;
		if (volume>0)
			ddb = (int)(2000 * log10(volume));
		else
			ddb = -10000;
		m_ba->put_Volume(ddb);
	}

	if (m_renderer1)
		m_renderer1->m_volume = m_volume;
	return S_OK;
}
HRESULT dx_player::get_volume(double *volume)
{
	if (volume == NULL)
		return E_POINTER;

	*volume = m_volume;
	return S_OK;
}

bool dx_player::is_playing()
{
	bool is = false;

	if (m_mc)
	{
		OAFilterState state = State_Stopped;
		m_mc->GetState(500, &state);
		if (state == State_Running)
			is = true;
	}

	return is;
}

bool dx_player::is_closed()
{
	return !is_visible(1) && !is_visible(2);
}

// window handle part

LRESULT dx_player::on_unhandled_msg(int id, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == DS_EVENT)
	{
		on_dshow_event();
	}

	else if (message == WM_COPYDATA)
	{
		if (lParam)
		{
			COPYDATASTRUCT *copy = (COPYDATASTRUCT*) lParam;
			wchar_t next_to_load[MAX_PATH];
			memcpy(next_to_load, copy->lpData, min(MAX_PATH*2, copy->cbData));
			next_to_load[MAX_PATH-1] = NULL;

			if (copy->dwData == WM_LOADFILE)
				reset_and_loadfile_internal(next_to_load);
		}
		else
		{
			SendMessageW(id_to_hwnd(1), WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);
		}
	}


	else if (message == WM_NV_NOTIFY)
	{
		if (m_renderer1)
			m_renderer1->NV3D_notify(wParam);
	}
	return dwindow::on_unhandled_msg(id, message, wParam, lParam);
}


LRESULT dx_player::on_display_change()
{
	OutputDebugStringA("DISPLAY CHANGE!\n");
	init_done_flag = 0;
	detect_monitors();
	init_window_size_positions();
	init_done_flag = 0x12345678;

	return S_OK;
}

LRESULT dx_player::on_key_down(int id, int key)
{
	switch (key)
	{
	case '1':
		m_mirror1 ++;
		break;

	case '2':
		m_mirror2 ++;
		break;

	case VK_SPACE:
		pause();
		break;

	case VK_LEFT:
		{
			int t;
			tell(&t);
			seek(max(0, t-5000));
		}
		break;

	case VK_RIGHT:
		{
			int t, mt;
			total(&mt);
			tell(&t);
			seek(min(t+5000, mt));
		}
		break;

	case VK_ESCAPE:
		if (m_full1 || (m_renderer1 && m_renderer1->get_fullscreen()))
			toggle_fullscreen();
		break;

	case VK_TAB:
		set_revert(!m_revert);
		break;

	case VK_NUMPAD7:									// image up
		set_letterbox(m_letterbox_delta - 0.005);
		break;

	case VK_NUMPAD1:
		set_letterbox(m_letterbox_delta + 0.005);		// down
		break;

	case VK_NUMPAD5:									// reset position
		set_letterbox(0);
		set_subtitle_pos(0.5, 0.95);
		set_subtitle_offset(0);
		m_parallax = 0;
		if (m_renderer1) m_renderer1->set_parallax(m_parallax);
		break;

	case VK_NUMPAD4:		//subtitle left
		set_subtitle_pos(m_subtitle_center_x - 0.0025, m_subtitle_bottom_y);
		break;

	case VK_NUMPAD6:		//right
		set_subtitle_pos(m_subtitle_center_x + 0.0025, m_subtitle_bottom_y);
		break;

	case VK_NUMPAD8:		//up
		set_subtitle_pos(m_subtitle_center_x, m_subtitle_bottom_y - 0.0025);
		break;

	case VK_NUMPAD2:		//down
		set_subtitle_pos(m_subtitle_center_x, m_subtitle_bottom_y + 0.0025);
		break;

	case VK_NUMPAD9:
		set_subtitle_offset(m_user_offset + 1);
		break;
		
	case VK_NUMPAD3:
		set_subtitle_offset(m_user_offset - 1);
		break;

	case VK_MULTIPLY:
		m_parallax += 0.001;
		if (m_renderer1)m_renderer1->set_parallax(m_parallax);
		break;

	case VK_DIVIDE:
		m_parallax -= 0.001;
		if (m_renderer1)m_renderer1->set_parallax(m_parallax);
		break;
	}
	return S_OK;
}

LRESULT dx_player::on_nc_mouse_move(int id, int x, int y)
{
	return on_mouse_move(id, -10, -10);	// just to show ui, should not trigger any other events
}

LRESULT dx_player::on_mouse_move(int id, int x, int y)
{
	POINT mouse;
	GetCursorPos(&mouse);
	if ( (mouse.x - m_mouse.x) * (mouse.x - m_mouse.x) + (mouse.y - m_mouse.y) * (mouse.y - m_mouse.y) >= 100)
	{
		show_mouse(true);
		show_ui(true);
		reset_timer(1, 2000);
	}

	RECT r;
	double v;
	GetClientRect(id_to_hwnd(id), &r);
	m_bar_drawer.total_width = r.right - r.left;
	int height = r.bottom - r.top;
	int hit_x = x;
	int hit_y = y;

	if (m_output_mode == out_tb)
	{
		height /= 2;
		hit_y %= height;
	}

	if (m_output_mode == out_htb)
	{
		hit_y = (hit_y%(height/2)) * 2;
	}

	if (m_output_mode == out_sbs)
	{
		m_bar_drawer.total_width /= 2;
		hit_x %= m_bar_drawer.total_width;
	}

	if (m_output_mode == out_hsbs)
	{
		hit_x *= 2;
		hit_x %= m_bar_drawer.total_width;
	}

	int type = -1;
	if (m_renderer1) type = m_renderer1->hittest(hit_x, hit_y, &v);
	if (type == hit_volume && GetAsyncKeyState(VK_LBUTTON) < 0 && m_dragging == hit_volume)
	{
		set_volume(v);
	}
	else if (type == hit_progress && GetAsyncKeyState(VK_LBUTTON) < 0 && m_dragging == hit_progress)
	{
		int total_time = 0;
		total(&total_time);
		seek((int)(total_time * v));
	}

	return S_OK;
}


LRESULT dx_player::on_mouse_up(int id, int button, int x, int y)
{
	m_dragging = -1;
	return S_OK;
}

HRESULT dx_player::popup_menu(HWND owner)
{
	if (m_renderer1 && m_renderer1->get_fullscreen())
		return S_FALSE;

	HMENU menu = LoadMenu(m_hexe, MAKEINTRESOURCE(IDR_MENU1));
	menu = GetSubMenu(menu, 0);
	localize_menu(menu);
	HMENU video = GetSubMenu(menu, 4);

	// output selection menu
	::detect_monitors();
	HMENU output1 = GetSubMenu(video, 4);
	HMENU output2 = GetSubMenu(video, 5);
	DeleteMenu(output1, 0, MF_BYPOSITION);
	DeleteMenu(output2, 0, MF_BYPOSITION);

	// disable selection while full screen
	if (m_full1 && m_full2 && false)
	{
		ModifyMenuW(video, 4, MF_BYPOSITION | MF_GRAYED, ID_OUTPUT1, C(L"Output 1"));
		ModifyMenuW(video, 5, MF_BYPOSITION | MF_GRAYED, ID_OUTPUT2, C(L"Output 2"));
	}
#ifdef no_dual_projector
	ModifyMenuW(video, 4, MF_BYPOSITION, ID_OUTPUT1, C(L"Fullscreen Output"));
	DeleteMenu(video, 5, MF_BYPOSITION);
	//DeleteMenu(video, ID_OUTPUTMODE_IZ3D, MF_BYCOMMAND);
#endif

#ifdef nologin
	DeleteMenu(menu, ID_LOGOUT, MF_BYCOMMAND);
#endif
	// list monitors
	for(int i=0; i<get_mixed_monitor_count(true, true); i++)
	{
		RECT rect;// = g_logic_monitor_rects[i];
		wchar_t tmp[256];

		get_mixed_monitor_by_id(i, &rect, tmp, true, true);

		DWORD flag1 = compare_rect(rect, m_screen1) ? MF_CHECKED : MF_UNCHECKED;
		DWORD flag2 = compare_rect(rect, m_screen2) ? MF_CHECKED : MF_UNCHECKED;

		//wsprintfW(tmp, L"%s %d (%dx%d)", C(L"Monitor"), i+1, rect.right - rect.left, rect.bottom - rect.top);
		InsertMenuW(output1, 0, flag1, 'M0' + i, tmp);
		InsertMenuW(output2, 0, flag2, 'N0' + i, tmp);
	}

	// disable output mode when fullscreen
	if (m_full1 || (m_renderer1 ? m_renderer1->get_fullscreen() : false))
	{
		//ModifyMenuW(video, 1, MF_BYPOSITION | MF_GRAYED, ID_PLAY, C(L"Output Mode"));
	}


	// play / pause
	bool paused = true;
	if (m_mc)
	{
		static OAFilterState fs = State_Running;
		HRESULT hr = m_mc->GetState(100, &fs);
		if (fs == State_Running)
			paused = false;
	}
	int flag = GetMenuState(menu, ID_PLAY, MF_BYCOMMAND);
	ModifyMenuW(menu, ID_PLAY, MF_BYCOMMAND| flag, ID_PLAY, paused ? C(L"Play\t(Space)") : C(L"Pause\t(Space)"));

	// find BD drives
	HMENU sub_open_BD = GetSubMenu(menu, 1);
	bool drive_found = false;
	for(int i=L'Z'; i>L'B'; i--)
	{
		wchar_t tmp[MAX_PATH] = L"C:\\";
		wchar_t tmp2[MAX_PATH];
		tmp[0] = i;
		tmp[4] = NULL;
		if (GetDriveTypeW(tmp) == DRIVE_CDROM)
		{
			drive_found = true;
			UINT flag = MF_BYPOSITION | MF_STRING;
			if (!GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
			{
				wcscat(tmp, C(L" (No Disc)"));
				flag |= MF_GRAYED;
			}
			else if (FAILED(find_main_movie(tmp, tmp2)))
			{
				wcscat(tmp, C(L" (No Movie Disc)"));
				flag |= MF_GRAYED;
			}
			else
			{
				GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
				wsprintfW(tmp, L"%s (%s)", tmp, tmp2);
			}
			InsertMenuW(sub_open_BD, 0, flag, i, tmp);
		}
	}
	if (drive_found)
		DeleteMenu(sub_open_BD, ID_OPENBLURAY3D_NONE, MF_BYCOMMAND);

	// input mode
	CheckMenuItem(menu, ID_INPUTLAYOUT_AUTO,		m_input_layout == input_layout_auto ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_INPUTLAYOUT_SIDEBYSIDE,	m_input_layout == side_by_side ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_INPUTLAYOUT_TOPBOTTOM,	m_input_layout == top_bottom ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_INPUTLAYOUT_MONOSCOPIC,	m_input_layout == mono2d ? MF_CHECKED:MF_UNCHECKED);

	// output mode
	CheckMenuItem(menu, ID_OUTPUTMODE_AMDHD3D,				m_output_mode == hd3d ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_NVIDIA3DVISION,		m_output_mode == NV3D ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_INTEL,				m_output_mode == intel3d ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_MONOSCOPIC2D,			m_output_mode == mono ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_ROWINTERLACE,			m_output_mode == masking && m_mask_mode == row_interlace ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_LINEINTERLACE,		m_output_mode == masking && m_mask_mode == line_interlace? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_CHECKBOARDINTERLACE,	m_output_mode == masking && m_mask_mode == checkboard_interlace ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_DUALPROJECTOR,		m_output_mode == dual_window ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_DUALPROJECTOR_SBS,	m_output_mode == out_sbs ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_DUALPROJECTOR_TB,		m_output_mode == out_tb ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_IZ3D,					m_output_mode == iz3d ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_GERNERAL120HZGLASSES,	m_output_mode == pageflipping ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_3DTV_SBS,				m_output_mode == out_hsbs ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_3DTV_TB,				m_output_mode == out_htb ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_OUTPUTMODE_ANAGLYPH,				m_output_mode == anaglyph ? MF_CHECKED:MF_UNCHECKED);

	// Aspect Ratio
	if (m_aspect == -1) CheckMenuItem(menu, ID_ASPECTRATIO_DEFAULT, MF_CHECKED);
	if (m_aspect == 2.35) CheckMenuItem(menu, ID_ASPECTRATIO_235, MF_CHECKED);
	if (m_aspect == (double)16/9) CheckMenuItem(menu, ID_ASPECTRATIO_169, MF_CHECKED);
	if (m_aspect == (double)4/3) CheckMenuItem(menu, ID_ASPECTRATIO_43, MF_CHECKED);

	// Aspect Ratio Mode
	if (m_aspect_mode == aspect_letterbox) CheckMenuItem(menu, ID_ASPECTRATIO_LETTERBOX, MF_CHECKED);
	if (m_aspect_mode == aspect_vertical_fill) CheckMenuItem(menu, ID_ASPECTRATIO_VERTICAL, MF_CHECKED);
	if (m_aspect_mode == aspect_horizontal_fill) CheckMenuItem(menu, ID_ASPECTRATIO_HORIZONTAL, MF_CHECKED);
	if (m_aspect_mode == aspect_stretch) CheckMenuItem(menu, ID_ASPECTRATIO_STRETCH, MF_CHECKED);

	// subtitle menu
	CheckMenuItem(menu, ID_SUBTITLE_DISPLAYSUBTITLE, MF_BYCOMMAND | (m_display_subtitle ? MF_CHECKED : MF_UNCHECKED));
	HMENU sub_subtitle = GetSubMenu(menu, 6);
	{
		CAutoLock lck(&m_subtitle_sec);
		if (!m_srenderer || FAILED(m_srenderer->set_font_color(m_font_color)))
		{
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_FONT, MF_GRAYED);
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_COLOR, MF_GRAYED);
		}
		else
		{
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_FONT, MF_ENABLED);
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_COLOR, MF_ENABLED);
		}
	}

	// LAV Audio Decoder and downmixing
	CheckMenuItem(menu, ID_AUDIO_USELAV, MF_BYCOMMAND | (m_useLAV ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(menu, ID_AUDIO_BITSTREAM, MF_BYCOMMAND | (m_useLAV && m_bitstreaming ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(menu, ID_AUDIO_DOWNMIX, MF_BYCOMMAND | (m_downmix ? MF_CHECKED : MF_UNCHECKED));

	// audio tracks
	HMENU sub_audio = GetSubMenu(menu, 5);
	list_audio_track(sub_audio);

	// subtitle tracks
	list_subtitle_track(sub_subtitle);

	// language
	if (g_active_language == ENGLISH)
		CheckMenuItem(menu, ID_LANGUAGE_ENGLISH, MF_CHECKED | MF_BYCOMMAND);
	else if (g_active_language == CHINESE)
		CheckMenuItem(menu, ID_LANGUAGE_CHINESE, MF_CHECKED | MF_BYCOMMAND);

	// swap
	CheckMenuItem(menu, ID_SWAPEYES, m_revert ? MF_CHECKED:MF_UNCHECKED);

	// CUDA
	CheckMenuItem(menu, ID_CUDA, g_CUDA ? MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(menu, ID_VIDEO_DEINTERLACE, m_forced_deinterlace ? MF_CHECKED:MF_UNCHECKED);


	// show it
	POINT mouse_pos;
	GetCursorPos(&mouse_pos);
	TrackPopupMenu(menu, TPM_TOPALIGN | TPM_LEFTALIGN, mouse_pos.x-5, mouse_pos.y-5, 0, owner, NULL);

	return S_OK;
}

LRESULT dx_player::on_mouse_wheel(int id, WORD wheel_delta, WORD button_down, int x, int y)
{
	short delta = wheel_delta;

	double current_volume = 1.0;
	if (FAILED(get_volume(&current_volume)))
		return S_OK;

	current_volume += (double)delta * 0.05 / WHEEL_DELTA;
	set_volume(current_volume);

	return S_OK;
}


LRESULT dx_player::on_mouse_down(int id, int button, int x, int y)
{
	if (!m_gb)
		return __super::on_mouse_down(id, button, x, y);
	show_ui(true);
	show_mouse(true);
	reset_timer(1, 99999999);

	RECT r;
	GetClientRect(id_to_hwnd(id), &r);
	m_bar_drawer.total_width = r.right - r.left;
	int height = r.bottom - r.top;
	if (m_output_mode == out_tb)
	{
		//if (hit_y < height/2)
		//	hit_y += height/2;
		height /= 2;
		y %= height;
	}

	if (m_output_mode == out_htb)
	{
		y = (y%(height/2)) * 2;
	}

	if (m_output_mode == out_sbs)
	{
		m_bar_drawer.total_width /= 2;
		x %= m_bar_drawer.total_width;
	}

	if (m_output_mode == out_hsbs)
	{
		x *= 2;
		x %= m_bar_drawer.total_width;
	}

	if ( (button == VK_RBUTTON || (!m_file_loaded && m_renderer1 && m_renderer1->hittest(x, y, NULL) == hit_logo) && 
		(!m_renderer1 || !m_renderer1->get_fullscreen())))
	{
		popup_menu(id_to_hwnd(1));
	}
	else if (button == VK_LBUTTON)
	{
		double v;
		int type;
		
		int hit_x = x;
		int hit_y = y;



		type = -1;
		if (m_renderer1) type = m_renderer1->hittest(hit_x, hit_y, &v);

		if (type == hit_play)
		{
			pause();
		}
		else if (type == hit_full)
		{
			toggle_fullscreen();
		}
		else if (type == hit_volume)
		{
			set_volume(v);
			m_dragging = hit_volume;
		}
		else if (type == hit_progress)
		{
			int total_time = 0;
			total(&total_time);
			seek((int)(total_time * v));
			m_dragging = hit_progress;
		}
		else if (type < 0 && !m_full1 && (!m_renderer1 || !m_renderer1->get_fullscreen()))
		{
			// move this window
			ReleaseCapture();
			SendMessage(id_to_hwnd(id), WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
	}

	reset_timer(1, 2000);

	return S_OK;
}

LRESULT dx_player::on_timer(int id)
{
	if (m_renderer1)
	{
#ifndef no_dual_projector
		if (timeGetTime() - m_renderer1->m_last_reset_time > TRAIL_TIME_1 &&!m_trial_shown && is_trial_version())
		{
			// TRIAL
			m_trial_shown = true;
			show_mouse(true);
			reset_timer(1, 999999);
			if (is_trial_version())
				MessageBoxW(id_to_hwnd(1), C(L"You are using a trial copy of DWindow, each clip will play normally for 10 minutes, after that the picture will become grayscale.\nYou can reopen it to play normally for another 10 minutes.\nRegister to remove this limitation."), L"....", MB_ICONINFORMATION);
			reset_timer(1, 2000);
		}
#endif
	}

	if (id == 1)
	{
		RECT client1, client2;
		POINT mouse1, mouse2;
		int test1 = -1, test2 = -1;
		GetClientRect(id_to_hwnd(1), &client1);
		GetClientRect(id_to_hwnd(2), &client2);
		GetCursorPos(&mouse1);
		GetCursorPos(&mouse2);
		ScreenToClient(id_to_hwnd(1), &mouse1);
		ScreenToClient(id_to_hwnd(2), &mouse2);

		m_bar_drawer.total_width = client1.right - client1.left;
		if (m_bar_drawer.total_width == 0)
			test1 = -1;
		else
		{
			int height = client1.bottom - client1.top;
			int hit_x = mouse1.x;
			int hit_y = mouse1.y;

			if (m_output_mode == out_tb)
			{
				height /= 2;
				hit_y %= height;
			}

			if (m_output_mode == out_htb)
			{
				hit_y = (hit_y%(height/2)) * 2;
			}

			if (m_output_mode == out_sbs)
			{
				m_bar_drawer.total_width /= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if (m_output_mode == out_hsbs)
			{
				hit_x *= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if (m_renderer1) test1 = m_renderer1->hittest(hit_x, hit_y, NULL);
		}

		m_bar_drawer.total_width = client2.right - client2.left;
		if (m_bar_drawer.total_width == 0)
			test2 = -1;
		else
		{
			int height = client2.bottom - client2.top;
			int hit_x = mouse2.x;
			int hit_y = mouse2.y;

			if (m_output_mode == out_tb)
			{
				height /= 2;
				hit_y %= height;
			}

			if (m_output_mode == out_htb)
			{
				hit_y = (hit_y%(height/2)) * 2;
			}

			if (m_output_mode == out_sbs)
			{
				m_bar_drawer.total_width /= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if (m_output_mode == out_hsbs)
			{
				hit_x *= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if(m_renderer1) test2 = m_renderer1->hittest(hit_x, hit_y, NULL);
		}
		if (test1 < 0 && test2 < 0)
		{
			show_mouse(false);
			show_ui(false);
		}
	}

	if (id == 2)
		draw_ui();
	return S_OK;
}

LRESULT dx_player::on_move(int id, int x, int y)
{
	if (id == 1 && init_done_flag == 0x12345678 && !m_full1)
	{
		RECT rect1;
		GetWindowRect(id_to_hwnd(1), &rect1);
		m_saved_rect1 = rect1;
	}
	if (id == 2 && init_done_flag == 0x12345678 && !m_full1)
	{
		RECT rect2;
		GetWindowRect(id_to_hwnd(2), &rect2);
		m_saved_rect2 = rect2;
	}

	if (id == 1 && is_visible(2) && init_done_flag == 0x12345678 && !m_full1)
	{
		RECT rect1;
		GetWindowRect(id_to_hwnd(1), &rect1);
		x = rect1.left - m_screen1.left;
		y = rect1.top - m_screen1.top;
		SetWindowPos(id_to_hwnd(2), NULL, m_screen2.left + x, m_screen2.top + y, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE);
	}

	else if (id == 2 && is_visible(1) && init_done_flag == 0x12345678 && !m_full2)
	{
		RECT rect2;
		GetWindowRect(id_to_hwnd(2), &rect2);
		x = rect2.left - m_screen2.left;
		y = rect2.top - m_screen2.top;
		m_saved_rect2 = rect2;
		SetWindowPos(id_to_hwnd(1), NULL, m_screen1.left + x, m_screen1.top + y, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE);
	}

	return S_OK;
}

LRESULT dx_player::on_size(int id, int type, int x, int y)
{
	if (id == 1 && init_done_flag == 0x12345678 && !m_full1)
	{
		RECT rect1;
		GetWindowRect(id_to_hwnd(1), &rect1);
		m_saved_rect1 = rect1;
	}
	if (id == 2 && init_done_flag == 0x12345678 && !m_full1)
	{
		RECT rect2;
		GetWindowRect(id_to_hwnd(2), &rect2);
		m_saved_rect2 = rect2;
	}

	if (id == 1 && is_visible(2) && init_done_flag == 0x12345678 && !m_full1)
	{
		RECT rect1;
		GetWindowRect(id_to_hwnd(1), &rect1);
		m_saved_rect1 = rect1;
		SetWindowPos(id_to_hwnd(2), NULL, 0, 0, rect1.right - rect1.left, rect1.bottom - rect1.top, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	}

	else if (id == 2 && is_visible(1) && init_done_flag == 0x12345678 && !m_full2)
	{
		RECT rect2;
		GetWindowRect(id_to_hwnd(2), &rect2);
		m_saved_rect2 = rect2;
		SetWindowPos(id_to_hwnd(1), NULL, 0, 0, rect2.right - rect2.left, rect2.bottom - rect2.top, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	}

	if (m_renderer1)
		m_renderer1->pump();
	return S_OK;
}

LRESULT dx_player::on_paint(int id, HDC hdc)
{
	// paint Icon
	/*
	int cxIcon = GetSystemMetrics(SM_CXICON);
	int cyIcon = GetSystemMetrics(SM_CYICON);
	RECT rect;
	GetClientRect(id_to_hwnd(id), &rect);
	int x = (rect.right - rect.left - cxIcon + 1) / 2;
	int y = (rect.bottom - rect.top - cyIcon + 1) / 2;
	HICON hIcon = LoadIcon(m_hexe, MAKEINTRESOURCE(IDI_ICON1));
	DrawIcon(hdc, x, y, hIcon);
	*/

	if (m_renderer1)
		m_renderer1->repaint_video();
	else
	{
		RECT client;
		GetClientRect(id_to_hwnd(id), &client);
		FillRect(hdc, &client, (HBRUSH)BLACK_BRUSH+1);
	}

	draw_ui();
	return S_OK;
}

LRESULT dx_player::on_double_click(int id, int button, int x, int y)
{
	if (button == VK_LBUTTON)
		toggle_fullscreen();

	return S_OK;
}

LRESULT dx_player::on_getminmaxinfo(int id, MINMAXINFO *info)
{
	info->ptMinTrackSize.x = 300;
	info->ptMinTrackSize.y = 200;

	return S_OK;
}
LRESULT dx_player::on_command(int id, WPARAM wParam, LPARAM lParam)
{
	int uid = LOWORD(wParam);
	show_ui(true);
	show_mouse(true);
	reset_timer(1, 99999999);
	if (uid == ID_OPENFILE)
	{
		wchar_t file[MAX_PATH] = L"";
		if (open_file_dlg(file, m_theater_owner ? m_theater_owner : id_to_hwnd(1), 
			L"Video files\0"
			L"*.mp4;*.mkv;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls;*.3dv;*.e3d\0"
			L"All Files\0"
			L"*.*\0"
			L"\0\0"))
		{
			reset_and_loadfile_internal(file);
		}
	}

	else if (uid == ID_LOGOUT)
	{
		if (MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1), C(L"Are you sure want to logout?"), L"Are you sure?", MB_YESNO) == IDYES)
		{
			memset(g_passkey_big, 0, 128);
			save_passkey();

			MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1), C(L"Logged out, the program will exit now, restart the program to login."), L"...", MB_OK);
			TerminateProcess(GetCurrentProcess(), 1);
		}
	}

	// LAV Audio Decoder
	else if (uid == ID_AUDIO_USELAV)
	{
		m_useLAV = !m_useLAV;
		if (m_file_loaded) MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1), C(L"Audio Decoder setting may not apply until next file play or audio swtiching."), L"...", MB_OK);
	}

	else if (uid == ID_AUDIO_DOWNMIX)
	{
		m_downmix = !m_downmix;
		handle_downmixer();
	}

	// Bitstreaming
	else if (uid == ID_AUDIO_BITSTREAM)
	{
		m_bitstreaming = !m_bitstreaming;
		CComPtr<IEnumFilters> ep;
		CComPtr<IBaseFilter> filter;
		m_gb->EnumFilters(&ep);

		while (ep->Next(1, &filter, NULL) == S_OK)
		{
			set_lav_audio_bitstreaming(filter, m_bitstreaming);
			filter = NULL;
		}
		if (m_file_loaded) MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1), C(L"Bitstreaming setting may not apply until next file play or audio swtiching."), L"...", MB_OK);
	}

	// CUDA
	else if (uid == ID_CUDA)
	{
		g_CUDA = !g_CUDA;
		if (m_file_loaded) MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1), C(L"CUDA setting will apply on next file play."), L"...", MB_OK);
	}

	// deinterlacing
	else if (uid == ID_VIDEO_DEINTERLACE)
	{
		m_forced_deinterlace = !m_forced_deinterlace;
		m_renderer1->m_forced_deinterlace = m_forced_deinterlace;

		if (m_forced_deinterlace)
			MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1), C(L"Deinterlacing is not recommended unless you see combing artifacts on moving objects."), L"...", MB_ICONINFORMATION);
	}

	// color adjusting
	else if (uid == ID_VIDEO_ADJUSTCOLOR)
	{
		show_color_adjust(m_hexe, m_theater_owner ? m_theater_owner : id_to_hwnd(id), this);
	}


	// input layouts
	else if (uid == ID_INPUTLAYOUT_AUTO)
	{
		m_input_layout = input_layout_auto;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	else if (uid == ID_INPUTLAYOUT_SIDEBYSIDE)
	{
		m_input_layout = side_by_side;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	else if (uid == ID_INPUTLAYOUT_TOPBOTTOM)
	{
		m_input_layout = top_bottom;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	else if (uid == ID_INPUTLAYOUT_MONOSCOPIC)
	{
		m_input_layout = mono2d;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	// output mode
	else if (uid == ID_OUTPUTMODE_NVIDIA3DVISION)
	{
		set_output_mode(NV3D);			
	}
	else if (uid == ID_OUTPUTMODE_INTEL)
	{
		HRESULT hr = set_output_mode(intel3d);
		if (hr == E_RESOLUTION_MISSMATCH)
		{
			IGFX_S3DCAPS caps;
			m_renderer1->intel_get_caps(&caps);

			wchar_t msg[1024];
			wchar_t tmp[1024];
			wcscpy(msg, C(L"Plase switch to one of device supported 3D reslutions first:\n\n"));
			for(int i=0; i<caps.ulNumEntries; i++)
			{
				wsprintfW(tmp, L"%dx%d @ %dHz\n", caps.S3DSupportedModes[i].ulResWidth,
							caps.S3DSupportedModes[i].ulResHeight,
							caps.S3DSupportedModes[i].ulRefreshRate);
				wcscat(msg, tmp);
			}

			MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1),
						msg, C(L"Error"), MB_ICONINFORMATION);
		}
		else if (hr == E_NOINTERFACE)
		{
			MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1),
				C(L"No supported device found."), C(L"Error"), MB_ICONERROR);
		}
	}
	else if (uid == ID_OUTPUTMODE_AMDHD3D)
	{
		HRESULT hr = set_output_mode(hd3d);
		if (hr == E_RESOLUTION_MISSMATCH)
		{
			D3DDISPLAYMODE modes[100];
			int count = 100;
			m_renderer1->HD3DGetAvailable3DModes(modes, &count);

			wchar_t msg[1024];
			wchar_t tmp[1024];
			wcscpy(msg, C(L"Plase switch to one of device supported 3D reslutions first:\n\n"));
			for(int i=0; i<count; i++)
			{
				wsprintfW(tmp, L"%dx%d @ %dHz\n", modes[i].Width,
					modes[i].Height,
					modes[i].RefreshRate);
				wcscat(msg, tmp);
			}

			MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1),
				msg, C(L"Error"), MB_ICONINFORMATION);
		}
		else if (hr == E_NOINTERFACE)
		{
			MessageBoxW(m_theater_owner ? m_theater_owner : id_to_hwnd(1),
				C(L"No supported device found."), C(L"Error"), MB_ICONERROR);
		}	}
	else if (uid == ID_OUTPUTMODE_MONOSCOPIC2D)
	{
		set_output_mode(mono);			
	}
	else if (uid == ID_OUTPUTMODE_ROWINTERLACE)
	{
		m_mask_mode = row_interlace;
		if (m_renderer1)
		{
			m_renderer1->set_mask_mode(m_mask_mode);
		}
		set_output_mode(masking);
	}
	else if (uid == ID_OUTPUTMODE_LINEINTERLACE)
	{
		m_mask_mode = line_interlace;
		if (m_renderer1)
		{
			m_renderer1->set_mask_mode(m_mask_mode);
		}
		set_output_mode(masking);
	}
	else if (uid == ID_OUTPUTMODE_CHECKBOARDINTERLACE)
	{
		m_mask_mode = checkboard_interlace;
		if (m_renderer1)
		{
			m_renderer1->set_mask_mode(m_mask_mode);
		}
		set_output_mode(masking);
	}
#ifdef no_dual_projector
	else if (uid == ID_OUTPUTMODE_DUALPROJECTOR ||
			 uid == ID_OUTPUTMODE_DUALPROJECTOR_SBS ||
			 uid == ID_OUTPUTMODE_DUALPROJECTOR_TB ||
			 uid == ID_OUTPUTMODE_IZ3D)
	{
		MessageBoxW(id_to_hwnd(id), C(L"Dual projector and IZ3D mode is only available in registered version."), L"", MB_OK | MB_ICONINFORMATION);
		WinExec("explorer.exe http://www.bo3d.net/buy.php",SW_SHOW);
	}
#else
	else if (uid == ID_OUTPUTMODE_DUALPROJECTOR)
	{
		set_output_mode(dual_window);			
	}
	else if (uid == ID_OUTPUTMODE_DUALPROJECTOR_SBS)
	{
		set_output_mode(out_sbs);			
	}
	else if (uid == ID_OUTPUTMODE_DUALPROJECTOR_TB)
	{
		set_output_mode(out_tb);			
	}
	else if (uid == ID_OUTPUTMODE_IZ3D)
	{
		set_output_mode(iz3d);
	}
#endif
	else if (uid == ID_OUTPUTMODE_3DTV_SBS)
	{
		set_output_mode(out_hsbs);			
	}
	else if (uid == ID_OUTPUTMODE_3DTV_TB)
	{
		set_output_mode(out_htb);			
	}
	else if (uid == ID_OUTPUTMODE_GERNERAL120HZGLASSES)
	{
		set_output_mode(pageflipping);			
	}
	else if (uid == ID_OUTPUTMODE_ANAGLYPH)
	{
		m_anaglygh_left_color = RGB(255,0,0);
		m_anaglygh_right_color = RGB(0,255,255);
		if (m_renderer1)
		{
			m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
			m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
		}
		set_output_mode(anaglyph);

	}

	// aspect ratio
	else if (uid == ID_ASPECTRATIO_DEFAULT)
	{
		m_aspect = -1;
		if (m_renderer1)
			m_renderer1->set_aspect(m_aspect);
	}

	else if (uid == ID_ASPECTRATIO_169)
	{
		m_aspect = (double)16/9;
		if (m_renderer1)
			m_renderer1->set_aspect(m_aspect);
	}

	else if (uid == ID_ASPECTRATIO_43)
	{
		m_aspect = (double)4/3;
		if (m_renderer1)
			m_renderer1->set_aspect(m_aspect);
	}

	else if (uid == ID_ASPECTRATIO_235)
	{
		m_aspect = 2.35;
		if (m_renderer1)
			m_renderer1->set_aspect(m_aspect);
	}

	// aspect ratio mode
	else if (uid == ID_ASPECTRATIO_LETTERBOX)
	{
		m_aspect_mode = aspect_letterbox;
		if (m_renderer1)
			m_renderer1->set_aspect_mode(m_aspect_mode);
	}

	else if (uid == ID_ASPECTRATIO_VERTICAL)
	{
		m_aspect_mode = aspect_vertical_fill;
		if (m_renderer1)
			m_renderer1->set_aspect_mode(m_aspect_mode);
	}

	else if (uid == ID_ASPECTRATIO_HORIZONTAL)
	{
		m_aspect_mode = aspect_horizontal_fill;
		if (m_renderer1)
			m_renderer1->set_aspect_mode(m_aspect_mode);
	}

	else if (uid == ID_ASPECTRATIO_STRETCH)
	{
		m_aspect_mode = aspect_stretch;
		if (m_renderer1)
			m_renderer1->set_aspect_mode(m_aspect_mode);
	}

	// swap eyes
	else if (uid == ID_SWAPEYES)
	{
		set_revert(!m_revert);
	}

	/*
	else if (uid == ID_ANAGLYPH_CUSTOMCOLOR)
	{
		m_output_mode = anaglyph;
		if (m_renderer1)
		{
			m_renderer1->set_output_mode(m_output_mode);
			m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
			m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
		}
	}


	else if (uid == ID_OUTPUTMODE_ANAGLYPHCOLOR)
	{
		DWORD tmp = m_anaglygh_left_color;
		if (select_color(&tmp, id_to_hwnd(id)))
		{
			m_anaglygh_left_color = tmp;

			m_output_mode = anaglyph;

			if (m_renderer1)
			{
				m_renderer1->set_output_mode(m_output_mode);
				m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
			}
		}
	}
	else if (uid == ID_OUTPUTMODE_ANAGLYPHCOLORRIGHTEYE)
	{
		DWORD tmp = m_anaglygh_right_color;
		if (select_color(&tmp, id_to_hwnd(id)))
		{
			m_anaglygh_right_color = tmp;

			m_output_mode = anaglyph;

			if (m_renderer1)
			{
				m_renderer1->set_output_mode(m_output_mode);
				m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
			}
		}
	}
	*/

	else if (uid == ID_LOADAUDIOTRACK)
	{
#ifndef no_dual_projector
		wchar_t file[MAX_PATH] = L"";
		if (open_file_dlg(file, m_theater_owner ? m_theater_owner : id_to_hwnd(1), L"Audio Tracks\0*.mp3;*.dts;*.ac3;*.aac;*.m4a;*.mka\0All Files\0*.*\0\0"))
		{
			load_audiotrack(file);
		}
#else
		MessageBoxW(id_to_hwnd(id), C(L"External audio track support is only available in registered version."), L"", MB_OK | MB_ICONINFORMATION);
		WinExec("explorer.exe http://www.bo3d.net/buy.php",SW_SHOW);
#endif
	}

	else if (uid == ID_SUBTITLE_LOADFILE)
	{
		wchar_t file[MAX_PATH] = L"";
		if (open_file_dlg(file, m_theater_owner ? m_theater_owner : id_to_hwnd(1), L"Subtitles\0*.srt;*.sup;*.ssa;*.ass\0All Files\0*.*\0\0"))
		{
			load_subtitle(file, false);
		}
	}

	else if (uid == ID_CLOSE)
	{
		reset();
	}

	else if (uid == ID_SUBTITLE_LATENCY)
	{
		int t_latency = m_subtitle_latency;
		double t_ratio = m_subtitle_ratio;
		HRESULT hr = latency_modify_dialog(m_hexe, m_theater_owner ? m_theater_owner : id_to_hwnd(id), &t_latency, &t_ratio);
		m_subtitle_latency = t_latency;
		m_subtitle_ratio = t_ratio;

		draw_subtitle();
	}

	else if (uid == ID_SUBTITLE_FONT)
	{
		if (!m_select_font_active)
			HANDLE show_thread = CreateThread(0,0,select_font_thread, this, NULL, NULL);
	}

	else if (uid == ID_SUBTITLE_COLOR)
	{
		DWORD tmp = m_font_color;
		bool ok;
		if (ok = select_color(&tmp, m_theater_owner ? m_theater_owner : id_to_hwnd(id)))
			m_font_color = tmp;
		CAutoLock lck(&m_subtitle_sec);
		if (m_srenderer && ok)
			m_srenderer->set_font_color(m_font_color);
		m_lastCBtime = -1;
	}

	else if (uid == ID_SUBTITLE_DISPLAYSUBTITLE)
	{
		m_display_subtitle = !m_display_subtitle;
		if (m_display_subtitle)
			set_subtitle_pos(m_subtitle_center_x, m_subtitle_bottom_y);
		else
			if (m_renderer1) m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0);
	}

	else if (uid == ID_OPENBDFOLDER)
	{
		wchar_t file[MAX_PATH] = L"";
		if (browse_folder(file, m_theater_owner ? m_theater_owner : id_to_hwnd(id)))
		{
			reset_and_loadfile_internal(file);
		}
	}

	else if (uid == ID_PLAY) 
	{
		pause();
	}

	else if (uid == ID_FULLSCREEN)
	{
		toggle_fullscreen();
	}
	else if (uid == ID_EXIT)
	{
		show_window(1, false);
		show_window(2, false);
	}
	else if (uid == ID_LANGUAGE_CHINESE)
	{
		set_localization_language(CHINESE);
	}
	else if (uid == ID_LANGUAGE_ENGLISH)
	{
		set_localization_language(ENGLISH);
	}

	// open drive
	else if (uid > 'B' && uid <= 'Z')
	{
		wchar_t tmp[MAX_PATH] = L"C:\\";
		tmp[0] = uid;
		reset_and_loadfile_internal(tmp);
	}

	// audio track
	else if (uid >= 'A0' && uid < 'B0')
	{
		int trackid = uid - 'A0';
		enable_audio_track(trackid);
	}

	// subtitle track
	else if (uid >= 'S0' && uid < 'T0')
	{
		int trackid = uid - 'S0';
		enable_subtitle_track(trackid);
	}

	// output monitor 1
	else if (uid >= 'M0' && uid <'N0')
	{
		int monitorid = uid - 'M0';
		set_output_monitor(0, monitorid);
	}

	// output monitor 2
	else if (uid >= 'N0' && uid <'O0')
	{
		int monitorid = uid - 'N0';
		set_output_monitor(1, monitorid);
	}

	if (m_output_mode == dual_window || m_output_mode == iz3d)
	{
		show_window(2, true);
		on_move(1, 0, 0);		// to correct second window position
		on_move(2, 0, 0);		// to correct second window position
		set_fullscreen(2, m_full1);
	}
	else
	{
		show_window(2, false);
	}
	reset_timer(1, 2000);
	return S_OK;
}
LRESULT dx_player::on_sys_command(int id, WPARAM wParam, LPARAM lParam)
{
	const int SC_MAXIMIZE2 = 0xF032;
	if (wParam == SC_MAXIMIZE || wParam == SC_MAXIMIZE2)
	{
		toggle_fullscreen();
		return S_OK;
	}
	else if (LOWORD(wParam) == SC_SCREENSAVE)
		return 0;

	return S_FALSE;
}

LRESULT dx_player::on_init_dialog(int id, WPARAM wParam, LPARAM lParam)
{
	HICON h_icon = LoadIcon(m_hexe, MAKEINTRESOURCE(IDI_ICON1));
	SendMessage(id_to_hwnd(id), WM_SETICON, TRUE, (LPARAM)h_icon);
	SendMessage(id_to_hwnd(id), WM_SETICON, FALSE, (LPARAM)h_icon);
	
	m_accel = LoadAccelerators(m_hexe, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	/*
	LONG style1 = GetWindowLongPtr(m_hwnd1, GWL_STYLE);
	LONG exstyle1 = GetWindowLongPtr(m_hwnd1, GWL_EXSTYLE);

	LONG f = style1 & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
	LONG exf =  exstyle1 & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;

	SetWindowLongPtr(m_hwnd1, GWL_STYLE, f);
	SetWindowLongPtr(m_hwnd1, GWL_EXSTYLE, exf);
	*/

// 	MARGINS margins = {-1};
// 	HRESULT hr = DwmExtendFrameIntoClientArea(id_to_hwnd(1),&margins);
// 	LONG_PTR exstyle1 = GetWindowLongPtr(m_hwnd1, GWL_EXSTYLE);
// 	exstyle1 |= WS_EX_COMPOSITED;
// 	SetWindowLongPtr(m_hwnd1, GWL_EXSTYLE, exstyle1);




	SetFocus(id_to_hwnd(id));
	if (id == 1)
	{
		m_renderer1 = new my12doomRenderer(id_to_hwnd(1), id_to_hwnd(2));
		unsigned char passkey_big_decrypted[128];
		RSA_dwindow_public(&g_passkey_big, passkey_big_decrypted);
		m_renderer1->m_AES.set_key((unsigned char*)passkey_big_decrypted+64, 256);
		memcpy(m_renderer1->m_key, (unsigned char*)passkey_big_decrypted+64, 32);
		memset(passkey_big_decrypted, 0, 128);
		init_done_flag = 0x12345678;
	}

	return S_OK;
}

// directshow part

HRESULT dx_player::init_direct_show()
{
	HRESULT hr = S_OK;
	hr = exit_direct_show();

	CoInitialize(NULL);
	JIF(CoCreateInstance(CLSID_FilterGraph , NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&m_gb));
	JIF(m_gb->QueryInterface(IID_IMediaControl,  (void **)&m_mc));
	JIF(m_gb->QueryInterface(IID_IMediaSeeking,  (void **)&m_ms));
	JIF(m_gb->QueryInterface(IID_IBasicAudio,  (void **)&m_ba));

	myCreateInstance(CLSID_LAVAudio, IID_IBaseFilter, (void**)&m_lav);
	CDWindowAudioDownmix *mixer = new CDWindowAudioDownmix(L"Downmixer", NULL, &hr);
	mixer->QueryInterface(IID_IBaseFilter, (void**)&m_downmixer);

	return S_OK;

CLEANUP:
	exit_direct_show();
	return hr;
}

HRESULT dx_player::exit_direct_show()
{
	if (m_mc)
		m_mc->Stop();

	if(m_gb)
	{
		m_gb->RemoveFilter(m_renderer1->m_dshow_renderer1);
		m_gb->RemoveFilter(m_renderer1->m_dshow_renderer2);
	}

	// reconfig renderer
	m_renderer1->reset();
	m_renderer1->set_output_mode(m_output_mode);
	m_renderer1->set_input_layout(m_input_layout);
	m_renderer1->set_mask_mode(m_mask_mode);
	m_renderer1->set_callback(this);
	m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
	m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
	m_renderer1->set_bmp_offset((double)m_internel_offset/1000 + (double)m_user_offset/1920);
	m_renderer1->set_aspect(m_aspect);
	m_renderer1->set_aspect_mode(m_aspect_mode);
	m_renderer1->m_forced_deinterlace = m_forced_deinterlace;
	m_renderer1->m_saturation1 = m_saturation;;
	m_renderer1->m_luminance1 = m_luminance;
	m_renderer1->m_hue1 = m_hue;
	m_renderer1->m_contrast1 = m_contrast;
	m_renderer1->m_saturation2 = m_saturation2;
	m_renderer1->m_luminance2 = m_luminance2;
	m_renderer1->m_hue2 = m_hue2;
	m_renderer1->m_contrast2 = m_contrast2;

	m_file_loaded = false;
	
	m_offset_metadata = NULL;
	m_downmixer = NULL;
	m_lav = NULL;
	m_ba = NULL;
	m_ms = NULL;
	m_mc = NULL;
	m_gb = NULL;


	return S_OK;
}

HRESULT dx_player::SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn)
{
	if (!m_display_subtitle || !m_renderer1)
		return S_OK;

	// latency and ratio
	REFERENCE_TIME time_for_offset_metadata = TimeStart;
	TimeStart -= m_subtitle_latency * 10000;
	TimeStart /= m_subtitle_ratio;

	m_internel_offset = 5;
	bool movie_has_offset_metadata = false;
	if (m_offset_metadata)
	{
		REFERENCE_TIME frame_time = m_renderer1->m_frame_length;
		HRESULT hr = m_offset_metadata->GetOffset(time_for_offset_metadata, frame_time, &m_internel_offset);
		//log_line(L"offset = %d(%s)", m_internel_offset, hr == S_OK ? L"S_OK" : L"S_FALSE");

		if (SUCCEEDED(hr))
			movie_has_offset_metadata = true;
	}
	if (!m_subtitle_has_offset) m_renderer1->set_bmp_offset((double)m_internel_offset/1000 + (double)m_user_offset/1920);

	int ms_start = (int)(TimeStart / 10000.0 + 0.5);
	int ms_end = (int)(TimeEnd / 10000.0 + 0.5);

	// CSubtitleRenderer test
	rendered_subtitle sub;
	HRESULT hr = E_FAIL;
	{
		CAutoLock lck(&m_subtitle_sec);
		if (m_srenderer)
		{
			if (S_OK == m_srenderer->set_output_aspect(m_renderer1->get_aspect()))
				m_lastCBtime = -1;		// aspect changed
			hr = m_srenderer->get_subtitle(ms_start, &sub, m_lastCBtime);
		}
	}

	// clear
	if (m_lastCBtime == -1)
	{
		//m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0); // FIXME: maybe we don't need to clear here?
	}

	{
		if (S_FALSE == hr)		// same subtitle, ignore
		{
			return S_OK;
		}
		else if (S_OK == hr)	// need to update
		{
			// empty result, clear it
			if( sub.width == 0 || sub.height ==0 || sub.width_pixel==0 || sub.height_pixel == 0 || sub.data == NULL)
			{
				m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0);
			}

			// draw it
			else
			{
				m_subtitle_has_offset = sub.delta_valid;
				if (sub.delta_valid)
					hr = m_renderer1->set_bmp_offset(sub.delta + (double)m_user_offset/1920);

				hr = m_renderer1->set_bmp(sub.data, sub.width_pixel, sub.height_pixel, sub.width,
					sub.height,
					sub.left + (m_subtitle_center_x-0.5),
					sub.top + (m_subtitle_bottom_y-0.95));

				free(sub.data);
				if (FAILED(hr))
				{
					m_lastCBtime = -1;				// failed, refresh on next frame
					return hr;
				}
			}
		}
		m_lastCBtime = ms_start;		// only do this if SetAlphaBitmap didn't fail, yes, it CAN FAIL!
	}

	return S_OK;
}

HRESULT dx_player::calculate_movie_rect(RECT *source, RECT *client, RECT *letterboxd, bool ui)
{
	if (source == NULL || client == NULL)
		return E_FAIL;

	RECT letterbox_c;

	memset(&letterbox_c, 0, sizeof(RECT));

	int source_width = source->right - source->left;
	int source_height = source->bottom - source->top;

	if (source_width == 0 || source_height == 0)
	{
		source_width = 1920;
		source_height = 1920*(m_screen1.bottom - m_screen1.top) / (m_screen1.right - m_screen1.left);
	}

    double source_aspect = (double) source_width / source_height;

	int client_width = client->right - client->left;
	int client_height = client->bottom - client->top;
	double client_aspect = (double) client_width / client_height;

	if (client_aspect > source_aspect)
	{
		letterbox_c.left = (int) (client_height * (client_aspect - source_aspect)+0.5);
		letterbox_c.right = letterbox_c.left / 2;
		letterbox_c.left -= letterbox_c.right;

		client->left += letterbox_c.left;
		client->right-= letterbox_c.right;
	}
	else
	{
		letterbox_c.top =  (int) (client_width * (1/client_aspect - 1/source_aspect)+0.5);
		letterbox_c.bottom = letterbox_c.top / 2;
		letterbox_c.top -= letterbox_c.bottom;

		client->top    += letterbox_c.top;
		client->bottom -= letterbox_c.bottom;
	}

	if (ui)
	{
		int delta = max(0, 30-letterbox_c.bottom);
		client->bottom -= delta;
		source->bottom = (int)(source->bottom - (double)delta * source_height / client_height + 0.5);
	}

	if (letterboxd)
		*letterboxd = letterbox_c;
	return S_OK;
}

HRESULT dx_player::paint_letterbox(int id, RECT letterbox)
{
	HDC hdc = GetDC(id_to_hwnd(id));
	HBRUSH brush = (HBRUSH)BLACK_BRUSH+1;
	
	/*
	//bottom
	GetClientRect(id_to_hwnd(id), &client);
	client.top = client.bottom - letterbox.bottom;
	if (m_show_ui) client.bottom -= 30;	// don't draw here to avoid flash
	if (client.bottom > client.top)
		FillRect(hdc, &client, brush);

	//top
	GetClientRect(id_to_hwnd(id), &client);
	client.bottom = client.top + letterbox.top;
	FillRect(hdc, &client, brush);

	//right
	GetClientRect(id_to_hwnd(id), &client);
	client.left = client.right - letterbox.right;
	if (m_show_ui) client.bottom -= 30;	// don't draw here to avoid flash
	FillRect(hdc, &client, brush);

	//left
	GetClientRect(id_to_hwnd(id), &client);
	client.right = client.left + letterbox.left;
	if (m_show_ui) client.bottom -= 30;	// don't draw here to avoid flash
	FillRect(hdc, &client, brush);
	*/

	ReleaseDC(id_to_hwnd(id), hdc);
	return S_OK;
}

HRESULT dx_player::on_dshow_event()
{
	CComQIPtr<IMediaEvent, &IID_IMediaEvent> event(m_gb);

	long event_code;
	LONG_PTR param1;
	LONG_PTR param2;
	if (FAILED(event->GetEvent(&event_code, &param1, &param2, 0)))
		return S_OK;

	if (event_code == EC_COMPLETE)
	{
		stop();
		seek(0);

		play_next_file();
	}

	else if (event_code == EC_VIDEO_SIZE_CHANGED)
	{
		printf("EC_VIDEO_SIZE_CHANGED\n");
	}


	return S_OK;
}

HRESULT dx_player::set_revert(bool revert)
{
	m_revert = revert;

	if (m_renderer1)
	{
		m_renderer1->set_swap_eyes(m_revert);
	}

	return S_OK;
}

HRESULT dx_player::set_letterbox(double delta)
{
	m_letterbox_delta = delta;

	if (m_renderer1)
		m_renderer1->set_movie_pos(2, m_letterbox_delta);

	return S_OK;
}

HRESULT dx_player::show_ui(bool show)
{
	m_show_ui = show;
	return S_OK;
}

HRESULT dx_player::draw_ui()
{
	if (m_renderer1)
		m_renderer1->set_ui_visible(m_show_ui && m_theater_owner == NULL);
	return S_OK;
}

HRESULT dx_player::start_loading()
{
	m_gb->AddFilter(m_renderer1->m_dshow_renderer1, L"Renderer #1");
	m_gb->AddFilter(m_renderer1->m_dshow_renderer2, L"Renderer #2");

	return S_OK;
}

HRESULT dx_player::reset_and_loadfile(const wchar_t *pathname, bool stop)
{
	if (GetCurrentThreadId() == m_thread_id1)
	{
		HRESULT hr = reset_and_loadfile_internal(pathname);

		if (stop)
			pause();

		return hr;
	}

	wcscpy(m_file_to_load, pathname);
	m_reset_and_load = true;
	m_stop_after_load = stop;
	m_reset_load_done = false;

	while (!m_reset_load_done)
		Sleep(1);

	return S_OK;
}

HRESULT dx_player::reset_and_loadfile_internal(const wchar_t *pathname)
{
	reset();
	start_loading();
	HRESULT hr;
	//hr = load_file(L"Z:\\00001.m2ts");
	hr = load_file(pathname);
	//hr = load_file(L"http://127.0.0.1/C%3A/TDDOWNLOAD/ding540%20(1)%20(1).mkv");
	//hr = load_file(L"D:\\Users\\my12doom\\Desktop\\haa\\00002.haa");
	//hr = load_file(L"D:\\Users\\my12doom\\Desktop\\test\\00005n.m2ts");
	//hr = load_file(L"D:\\Users\\my12doom\\Desktop\\test\\00006.m2ts");
	//hr = load_file(L"K:\\BDMV\\STREAM\\00001.m2ts");
	//hr = load_file(L"K:\\BDMV\\STREAM\\00002.m2ts");
	//hr = load_file(L"D:\\Users\\my12doom\\Documents\\00002.m2ts");
	if (FAILED(hr))
		goto fail;
	hr = end_loading();
	if (FAILED(hr))
		goto fail;
	play();

	// search and load subtitles
	wchar_t file_to_search[MAX_PATH];
	wchar_t file_folder[MAX_PATH];
	GetWindowTextW(m_hwnd1, file_to_search, MAX_PATH);
	GetWindowTextW(m_hwnd1, file_folder, MAX_PATH);
	for(int i=wcslen(file_to_search)-1; i>0; i--)
		if (file_to_search[i] == L'.')
		{
			file_to_search[i] = NULL;
			break;
		}
	for(int i=wcslen(file_folder)-1; i>0; i--)
		if (file_folder[i] == L'\\')
		{
			file_folder[i+1] = NULL;
			break;
		}

	wchar_t search_pattern[MAX_PATH];
	wchar_t exts[5][512] = {L"*.srt", L"*.sup", L"*.ssa", L"*.ass", L"*.idx"};
	for(int i=0; i<5; i++)
	{
		wcscpy(search_pattern, file_to_search);
		wcscat(search_pattern, exts[i]);

		WIN32_FIND_DATAW find_data;
		HANDLE find_handle = FindFirstFileW(search_pattern, &find_data);

		if (find_handle != INVALID_HANDLE_VALUE)
		{
			wchar_t subtitle_to_load[MAX_PATH];
			wcscpy(subtitle_to_load, file_folder);
			wcscat(subtitle_to_load, find_data.cFileName);
			load_subtitle(subtitle_to_load, false);

			while( FindNextFile(find_handle, &find_data ) )
			{
				wcscpy(subtitle_to_load, file_folder);
				wcscat(subtitle_to_load, find_data.cFileName);
				load_subtitle(subtitle_to_load, false);
			}
		}
	}
	return hr;
fail:
	reset();
	set_window_text(1, C(L"Open Failed"));
	return hr;
}

HRESULT dx_player::play_next_file()
{
restart:
	if (m_playlist_playing >= m_playlist_count - 1)
		return E_FAIL;

	m_playlist_playing ++ ;
	if (FAILED(reset_and_loadfile(m_playlist[m_playlist_playing], false)))
		goto restart;

	return S_OK;
}

HRESULT dx_player::on_dropfile(int id, int count, wchar_t **filenames)
{

	if (count == 1)
	{
		HRESULT hr = load_subtitle(filenames[0], false);
		if (SUCCEEDED(hr) && m_file_loaded)
			return S_OK;
	}

	m_playlist_count = count;
	m_playlist_playing = -1;
	for(int i=0; i<count; i++)
		wcscpy(m_playlist[i], filenames[i]);

	play_next_file();

	return S_OK;
}

LRESULT dx_player::on_idle_time()
{
	if (init_done_flag != 0x12345678)
		return S_FALSE;

	if (m_renderer1)
		m_renderer1->pump();

	if (m_reset_and_load)
	{
		m_reset_load_hr = reset_and_loadfile_internal(m_file_to_load);
		m_reset_and_load = false;

		if (m_stop_after_load)
			pause();

		m_reset_load_done = true;
	}
	return S_FALSE;
}

HRESULT dx_player::load_file(const wchar_t *pathname, bool non_mainfile /* = false */, int audio_track /* = MKV_FIRST_TRACK */, int video_track /* = MKV_ALL_TRACK */)
{
	wchar_t file_to_play[MAX_PATH];
	wcscpy(file_to_play, pathname);

	// Bluray Directory
	log_line(L"start loading %s", file_to_play);
	if(PathIsDirectoryW(file_to_play))
	{
		wchar_t playlist[MAX_PATH];
		HRESULT hr;
		if (FAILED(hr = find_main_movie(file_to_play, playlist)))
		{
			log_line(L"main movie not found for %s, error=0x%08x", file_to_play, hr);
			return hr;
		}
		else
		{
			log_line(L"main movie of %s is %s", file_to_play, playlist);
			wcscpy(file_to_play, playlist);
		}
	}

	// subtitle file
	HRESULT hr = load_subtitle(pathname, false);
	if (SUCCEEDED(hr))
	{
		log_line(L"loaded as subtitle");
		return hr;
	}

	// subtitle renderer
	m_gb->AddFilter(m_grenderer.m_filter, L"Subtitle Renderer");

	// Legacy Remux file
	m_renderer1->m_remux_mode = m_is_remux_file = verify_file(file_to_play) == 2;

	// check private source and whether is MVC content
	CLSID source_clsid;
	hr = GetFileSource(file_to_play, &source_clsid);

	// E3D keys
	if (source_clsid == CLSID_E3DSource)
	{
		log_line(L"loading local E3D key for %s", file_to_play);
		HANDLE h_file = CreateFileW (file_to_play, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		file_reader reader;
		reader.SetFile(h_file);
		CloseHandle(h_file);

		if (!reader.m_is_encrypted)
		{
			// normal file, do nothing
		}
		else
		{
			unsigned char key[32];
			load_e3d_key(reader.m_hash, key);
			reader.set_key((unsigned char*)key);

			if (!reader.m_key_ok)
			{
				log_line(L"local E3D key failed.");
				log_line(L"downloading E3D key for %s", file_to_play);
				if (SUCCEEDED(download_e3d_key(file_to_play)))
				{
					log_line(L"download E3D key OK, saving to local store.");
					e3d_get_process_key(key);
					save_e3d_key(reader.m_hash, key);
				}
			}
			else
			{
				log_line(L"local E3D key OK.");
			}
			e3d_set_process_key(key);
		}
	}

	if (SUCCEEDED(hr))
	{
		log_line(L"loading with private filter");
		// private file types
		// create source, load file and join it into graph
		CComPtr<IBaseFilter> source_base;
		hr = myCreateInstance(source_clsid, IID_IBaseFilter, (void**)&source_base);
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source_source(source_base);
		if (source_source)
		{
			source_source->Load(file_to_play, NULL);
			m_gb->AddFilter(source_base, L"Source");

			CComQIPtr<IOffsetMetadata, &IID_IOffsetMetadata> offset(source_base);
			if (offset)
				m_offset_metadata = offset;
		}
		else
		{
			CComPtr<IBaseFilter> async_reader;
			async_reader.CoCreateInstance(CLSID_AsyncReader);
			CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> reader_source(async_reader);
			reader_source->Load(file_to_play, NULL);
			m_gb->AddFilter(source_base, L"Splitter");
			m_gb->AddFilter(async_reader, L"Reader");

			CComPtr<IPin> reader_o;
			CComPtr<IPin> source_i;
			GetUnconnectedPin(async_reader, PINDIR_OUTPUT, &reader_o);
			GetUnconnectedPin(source_base, PINDIR_INPUT, &source_i);

			m_gb->ConnectDirect(reader_o, source_i, NULL);
		}

		log_line(L"renderer ing pins");
		// then render pins
		CComPtr<IPin> pin;
		CComPtr<IEnumPins> pEnum;
		int audio_num = 0, video_num = 0;
		hr = source_base->EnumPins(&pEnum);
		while (pEnum->Next(1, &pin, NULL) == S_OK)
		{
			PIN_DIRECTION ThisPinDir;
			pin->QueryDirection(&ThisPinDir);
			if (ThisPinDir == PINDIR_OUTPUT)
			{
				CComPtr<IPin> pin_tmp;
				pin->ConnectedTo(&pin_tmp);
				if (pin_tmp)  // Already connected, not the pin we want.
					pin_tmp = NULL;
				else  // Unconnected, this is the pin we may want.
				{
					PIN_INFO info;
					pin->QueryPinInfo(&info);
					if (info.pFilter) info.pFilter->Release();
					log_line(L"testing pin %s", info.achName);

					if (S_OK == DeterminPin(pin, NULL, MEDIATYPE_Video))
					{

						if ( (video_track>=0 && (LOADFILE_TRACK_NUMBER(video_num) & video_track ))
							|| video_track == LOADFILE_ALL_TRACK)
						{
							CLSID CLSID_mp4v = FOURCCMap('v4pm');
							if (m_is_remux_file)
							{
								log_line(L"adding pd10 decoder");
								CComPtr<IBaseFilter> pd10;
								hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10);
								hr = m_gb->AddFilter(pd10, L"PD10 Decoder");

								if (FAILED(hr = CrackPD10(pd10)))
									return hr;
							}

							else if(S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('v4pm')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('V4PM')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('XVID')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('xvid')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('divx')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('DIVX')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('05XD')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('05xd')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('3VID')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('4VID')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('5VID')) )
							{
								log_line(L"adding xvid decoder");
								make_xvid_support_mp4v();
								CComPtr<IBaseFilter> xvid;
								hr = myCreateInstance(CLSID_XvidDecoder, IID_IBaseFilter, (void**)&xvid);
								hr = m_gb->AddFilter(xvid, L"Xvid Deocder");
							}

							else if(S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('1cva')) || 
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('1CVA')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('CVMA')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462h')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462H')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462x')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462X')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('1VCC')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('1vcc')))
							{
								log_line(L"adding coremvc decoder");
								coremvc_hooker mvc_hooker;
								CComPtr<IBaseFilter> coremvc;
								hr = myCreateInstance(CLSID_CoreAVC, IID_IBaseFilter, (void**)&coremvc);
								hr = ActiveCoreMVC(coremvc);
								hr = m_gb->AddFilter(coremvc, L"CoreMVC");

								FILTER_INFO fi;
								fi.pGraph = NULL;
								if (coremvc) coremvc->QueryFilterInfo(&fi);
								if (fi.pGraph)
									fi.pGraph->Release();
								else
									log_line(L"couldn't add CoreMVC to graph(need rename to StereoPlayer.exe.");

								log_line(L"CoreMVC hr = 0x%08x", hr);
							}

							else if (S_OK == DeterminPin(pin, NULL, CLSID_NULL, MEDIASUBTYPE_MPEG2_VIDEO) ||
								S_OK == DeterminPin(pin, NULL, CLSID_NULL, MEDIASUBTYPE_MPEG1Payload)  )
							{
								log_line(L"adding pd10 decoder");
								CComPtr<IBaseFilter> pd10;
								hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10);
								hr = m_gb->AddFilter(pd10, L"PD10 Decoder");
							}

							log_line(L"renderering video pin #%d", video_num);
							//debug_list_filters();
							hr = m_gb->Render(pin);
							log_line(L"done renderering video pin #%d", video_num);
						}
						video_num ++;
					}

					else if (S_OK == DeterminPin(pin, NULL, MEDIATYPE_Audio))
					{
						if ( (audio_track>=0 && (LOADFILE_TRACK_NUMBER(audio_num) & audio_track ))
							|| audio_track == LOADFILE_ALL_TRACK)
						{
							set_lav_audio_bitstreaming(m_lav, m_bitstreaming);
							if(m_useLAV)hr = m_gb->AddFilter(m_lav, L"LAV Audio Decoder");
							m_gb->Render(pin);
							handle_downmixer();
							log_line(L"done renderering audio pin #%d", audio_num);
						}
						audio_num ++;
					}

					else if (S_OK == DeterminPin(pin, NULL, MEDIATYPE_Subtitle))
					{
						log_line(L"renderering subtitle pin %s", info.achName);
						CComPtr<IPin> srenderer_pin;
						GetUnconnectedPin(m_grenderer.m_filter, PINDIR_INPUT, &srenderer_pin);
						if (srenderer_pin) m_gb->ConnectDirect(pin, srenderer_pin, NULL);
						log_line(L"done renderering subtitle %s", info.achName);
					}

					else;	// other tracks, ignore them
				}
			}
			pin = NULL;
		}

		// if it doesn't work....
		if (video_num == 0 && audio_num == 0)
		{
			log_line(L"failed rendering \"%s\" (error = 0x%08x).", file_to_play, hr);
			if (m_is_remux_file)
				return E_FAIL;


			log_line(L"private filters failed, trying system filters. (%s)", file_to_play);

			CComPtr<IBaseFilter> pd10;
			hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10);
			hr = m_gb->AddFilter(pd10, L"PD10 Deocder");

			CComPtr<IBaseFilter> xvid;
			hr = myCreateInstance(CLSID_XvidDecoder, IID_IBaseFilter, (void**)&xvid);
			hr = m_gb->AddFilter(xvid, L"Xvid Deocder");

			{
				CComPtr<IBaseFilter> coremvc;
				coremvc_hooker mvc_hooker;

				hr = myCreateInstance(CLSID_CoreAVC, IID_IBaseFilter, (void**)&coremvc);
				hr = ActiveCoreMVC(coremvc);
				hr = m_gb->AddFilter(coremvc, L"CoreMVC");
				FILTER_INFO fi;
				fi.pGraph = NULL;
				if (coremvc) coremvc->QueryFilterInfo(&fi);
				if (fi.pGraph)
					fi.pGraph->Release();
				else
					log_line(L"couldn't add CoreMVC to graph(need rename to StereoPlayer.exe.");
			}

			set_lav_audio_bitstreaming(m_lav, m_bitstreaming);
			if(m_useLAV)hr = m_gb->AddFilter(m_lav, L"LAV Audio Decoder");

			hr = m_gb->RenderFile(file_to_play, NULL);
			handle_downmixer();
		}
		else
		{
			log_line(L"private filter OK");
			hr = S_OK;
		}
	}


	// normal file, just render it.
	else
	{
		if (m_is_remux_file)
		{
			log_line(L"Remux file should always use private filters (%s)", file_to_play);
			return hr;
		}

		log_line(L"private filters failed, trying system filters. (%s)", file_to_play);

		CComPtr<IBaseFilter> pd10;
		hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10);
		hr = m_gb->AddFilter(pd10, L"PD10 Deocder");

		CComPtr<IBaseFilter> xvid;
		hr = myCreateInstance(CLSID_XvidDecoder, IID_IBaseFilter, (void**)&xvid);
		hr = m_gb->AddFilter(xvid, L"Xvid Deocder");

		{
			CComPtr<IBaseFilter> coremvc;
			coremvc_hooker mvc_hooker;

			hr = myCreateInstance(CLSID_CoreAVC, IID_IBaseFilter, (void**)&coremvc);
			hr = ActiveCoreMVC(coremvc);
			hr = m_gb->AddFilter(coremvc, L"CoreMVC");
			FILTER_INFO fi;
			fi.pGraph = NULL;
			if (coremvc) coremvc->QueryFilterInfo(&fi);
			if (fi.pGraph)
				fi.pGraph->Release();
			else
				log_line(L"couldn't add CoreMVC to graph(need rename to StereoPlayer.exe.");
		}

		set_lav_audio_bitstreaming(m_lav, m_bitstreaming);
		if(m_useLAV)hr = m_gb->AddFilter(m_lav, L"LAV Audio Decoder");

		hr = m_gb->RenderFile(file_to_play, NULL);
		handle_downmixer();
	}

	// check result
	if (hr == VFW_S_AUDIO_NOT_RENDERED)
		log_line(L"warning: audio not rendered. \"%s\"", file_to_play);

	if (hr == VFW_S_PARTIAL_RENDER)
		log_line(L"warning: Some of the streams in this movie are in an unsupported format. \"%s\"", file_to_play);

	if (hr == VFW_S_VIDEO_NOT_RENDERED)
		log_line(L"warning: video not rendered. \"%s\"", file_to_play);

	if (FAILED(hr))
	{
		log_line(L"failed rendering \"%s\" (error = 0x%08x).", file_to_play, hr);
	}
	else
	{
		log_line(L"load OK.");
		

		if (!non_mainfile)
		{
			m_srenderer = m_grenderer.GetSubtitleRenderer();
			m_file_loaded = true;
			set_window_text(1, file_to_play);
			set_window_text(2, file_to_play);
		}
	}

	return hr;
}

HRESULT dx_player::end_loading()
{
	set_volume(m_volume);

	int num_renderer_found = 0;

	debug_list_filters();
	RemoveUselessFilters(m_gb);

	CComPtr<IPin> renderer1_input;
	GetUnconnectedPin(m_renderer1->m_dshow_renderer1, PINDIR_INPUT, &renderer1_input);
	if (renderer1_input)
	{
		log_line(L"no video stream found.");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT dx_player::debug_list_filters()
{
	// debug: list filters
	log_line(L"Listing filters.");
	CComPtr<IEnumFilters> pEnum;
	CComPtr<IBaseFilter> filter;
	m_gb->EnumFilters(&pEnum);
	while(pEnum->Next(1, &filter, NULL) == S_OK)
	{

		FILTER_INFO filter_info;
		filter->QueryFilterInfo(&filter_info);
		if (filter_info.pGraph) filter_info.pGraph->Release();
		wchar_t tmp[10240];
		wchar_t tmp2[1024];
		wcscpy(tmp, filter_info.achName);
		
		CComPtr<IEnumPins> ep;
		CComPtr<IPin> pin;
		filter->EnumPins(&ep);
		while (ep->Next(1, &pin, NULL) == S_OK)
		{
			PIN_DIRECTION dir;
			PIN_INFO pi;
			pin->QueryDirection(&dir);
			pin->QueryPinInfo(&pi);
			if (pi.pFilter) pi.pFilter->Release();

			CComPtr<IPin> connected;
			PIN_INFO pi2;
			FILTER_INFO fi;
			pin->ConnectedTo(&connected);
			pi2.pFilter = NULL;
			if (connected) connected->QueryPinInfo(&pi2);
			if (pi2.pFilter)
			{
				pi2.pFilter->QueryFilterInfo(&fi);
				if (fi.pGraph) fi.pGraph->Release();
				pi2.pFilter->Release();
			}
			
			wsprintfW(tmp2, L", %s %s", pi.achName, connected?L"Connected to ":L"Unconnected");
			if (connected) wcscat(tmp2, fi.achName);

			wcscat(tmp, tmp2);
			pin = NULL;
		}


		log_line(L"%s", tmp);

		filter = NULL;
	}
	log_line(L"");

	return S_OK;
}

// font/d3d/subtitle part

HRESULT dx_player::load_audiotrack(const wchar_t *pathname)
{
#ifndef no_dual_projector
	//disable all audio first
	enable_audio_track(-1);

	// Save Filter State and Stop
	int time = 0;
	tell(&time);
	time += 10;
	OAFilterState state_before, state;
	m_mc->GetState(INFINITE, &state_before);
	if (state_before != State_Stopped)
	{
		m_mc->Stop();
		m_mc->GetState(INFINITE, &state);
	}

	// load and restore
	HRESULT hr = load_file(pathname, true, LOADFILE_FIRST_TRACK, LOADFILE_NO_TRACK);
	if (state_before == State_Running)
		m_mc->Run();
	m_mc->GetState(INFINITE, &state);
	hr = seek(time);
	return hr;
#else
	return S_OK;
#endif
}
HRESULT dx_player::load_subtitle(const wchar_t *pathname, bool reset)			//FIXME : always reset 
{
	if (pathname == NULL)
		return E_POINTER;

	// find duplication
	for(POSITION i = m_external_subtitles.GetHeadPosition(); i; m_external_subtitles.GetNext(i))
	{
		CAutoPtr<subtitle_file_handler> &tmp = m_external_subtitles.GetAt(i);
		if (wcscmp(pathname, tmp->m_pathname) == 0)
			return S_FALSE;
	}

	CAutoPtr<subtitle_file_handler> tmp;
	tmp.Attach(new subtitle_file_handler(pathname));
	if(tmp->m_renderer == NULL)
		return E_FAIL;

	m_external_subtitles.AddTail(tmp);
	return enable_subtitle_track((int)m_external_subtitles.GetCount()-1);
}

HRESULT dx_player::draw_subtitle()
{	
	REFERENCE_TIME t = (REFERENCE_TIME)m_lastCBtime * 10000;
	m_lastCBtime = -1;

	return SampleCB(t, t, NULL);
}

HRESULT dx_player::set_subtitle_offset(int offset)
{
	m_user_offset = offset;
	m_renderer1->set_bmp_offset((double)m_internel_offset/1000 + (double)m_user_offset/1920);
	draw_subtitle();
	return S_OK;
}

HRESULT dx_player::set_subtitle_pos(double center_x, double bottom_y)
{
	m_subtitle_center_x = center_x;
	m_subtitle_bottom_y = bottom_y;

	return draw_subtitle();
}

HRESULT dx_player::log_line(wchar_t *format, ...)
{
#ifdef DEBUG
	if (!m_log)
		return E_POINTER;
	wcscat(m_log, L"\r\n");

	wchar_t tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfW(tmp, format, valist);
	va_end(valist);

	OutputDebugStringW(L"log:");
	OutputDebugStringW(tmp);
	OutputDebugStringW(L"\n");

	wcscat(m_log, tmp);
#endif
	return S_OK;
}

HRESULT dx_player::select_font(bool show_dlg)
{
	CHOOSEFONTW cf={0};
	memset(&cf, 0, sizeof(cf));
	LOGFONTW lf=m_LogFont; 
	HDC hdc;
	LONG lHeight;

	// Convert requested font point size to logical units
	hdc = GetDC( NULL );
	lHeight = -MulDiv( m_lFontPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
	ReleaseDC( NULL, hdc );

	// Initialize members of the LOGFONT structure.
	if (memcmp(&lf, &empty_logfontw, sizeof(lf)) == 0)
	{
		lstrcpynW(lf.lfFaceName, m_FontName, 32);
		lf.lfHeight = lHeight;      // Logical units
		lf.lfCharSet = GB2312_CHARSET;
		lf.lfOutPrecision =  OUT_STROKE_PRECIS;
		lf.lfClipPrecision = CLIP_STROKE_PRECIS;
		lf.lfQuality = DEFAULT_QUALITY;
		lf.lfPitchAndFamily = VARIABLE_PITCH;
	}

	// Initialize members of the CHOOSEFONT structure. 
	cf.lStructSize = sizeof(CHOOSEFONT); 
	cf.hwndOwner   = NULL; 
	cf.hDC         = (HDC)NULL; 
	cf.lpLogFont   = &lf; 
	cf.iPointSize  = m_lFontPointSize * 10; 
	cf.rgbColors   = m_font_color; 
	cf.lCustData   = 0L; 
	cf.lpfnHook    = (LPCFHOOKPROC)NULL; 
	cf.hInstance   = (HINSTANCE) NULL; 
	cf.lpszStyle   = m_FontStyle; 
	cf.nFontType   = SCREEN_FONTTYPE; 
	cf.nSizeMin    = 0; 
	cf.lpTemplateName = NULL; 
	cf.nSizeMax = 720; 
	cf.Flags = CF_SCREENFONTS | CF_SCALABLEONLY | CF_INITTOLOGFONTSTRUCT | 
		CF_EFFECTS     | CF_USESTYLE     | CF_LIMITSIZE; 

	if (show_dlg)
		ChooseFontW(&cf);

	lstrcpynW(m_FontName, lf.lfFaceName, sizeof(m_FontName)/sizeof(TCHAR));
	m_FontStyle.save();
	m_FontName.save();
	m_LogFont = lf;
	m_lFontPointSize = cf.iPointSize / 10;  // Specified in 1/10 point units
	//m_font_color = cf.rgbColors;
	m_font = CreateFontIndirectW(cf.lpLogFont); 

	return S_OK;
}

HRESULT dx_player::set_output_mode(int mode)
{
	if (!m_renderer1)
		return VFW_E_NOT_CONNECTED;

	bool toggle = false;
	if (m_full1 || m_full2)
	{
		toggle = true;
		toggle_fullscreen();
	}

	HRESULT hr = m_renderer1->set_output_mode(mode);
	if (FAILED(hr))
	{
		OutputDebugStringA("set_output_mode() fail\n");

	}	
	else
	{
		m_output_mode = mode;
	}

	if (toggle)
		toggle_fullscreen();

	return hr;
}

HRESULT dx_player::toggle_fullscreen()
{

	if (m_output_mode == dual_window || m_output_mode == iz3d)
	{
		//show_window(2, !m_full2);		// show/hide it before set fullscreen, or you may got a strange window
		show_window(2, true);
		set_fullscreen(2, !m_full2);	// yeah, this is not a bug
		set_fullscreen(1, m_full2);
		//show_window(2, m_full2);		// show/hide it again
	}

	else if (m_output_mode != intel3d &&
		 (m_output_mode == pageflipping || m_output_mode == NV3D || m_output_mode == hd3d || g_ExclusiveMode))
	{
		if (m_renderer1)
			m_renderer1->set_fullscreen(!m_renderer1->get_fullscreen());
	}
	else
	{
		show_window(2, false);
		set_fullscreen(1, !m_full1);
	}
	return S_OK;
}

HRESULT dx_player::enable_audio_track(int track)
{
	CComPtr<IEnumFilters> ef;

	// Save Filter State
	int time;
	tell(&time);
	OAFilterState state_before, state;
	m_mc->GetState(INFINITE, &state_before);
	if (state_before != State_Stopped)
	{
		m_mc->Stop();
		m_mc->GetState(INFINITE, &state);
	}
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int audio_track_found = 0;

	CComPtr<IPin> pin_to_render;

	// delete any downstream of audio pins
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				pin = NULL;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}

			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				filter->EnumPins(&ep);
				bool efreset = false;
				pin = NULL;
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK)
					{
						if (connected)
						{
							RemoveDownstream(connected);
							ef->Reset();
							efreset = true;
						}
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin

				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(DWORD i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);
					if (DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK || wcsstr(name , L"Audio") || wcsstr(name, L"A:"))
					{
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"Audio", &pin);
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"A:", &pin);

						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);

						if (connected)
						{
							RemoveDownstream(connected);
							ef->Reset();
						}
					}
					CoTaskMemFree (name);
				}

			}

		}
		filter = NULL;
	}

	ef = NULL;
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				pin = NULL;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}
			//break;

			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				pin = NULL;
				filter->EnumPins(&ep);
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK)
					{
						if (audio_track_found == track)
						{
							pin_to_render = pin;
						}

						audio_track_found++;
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin

				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(DWORD i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);
					if (DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK || wcsstr(name , L"Audio") || wcsstr(name, L"A:"))
					{
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"Audio", &pin);
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"A:", &pin);

						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);

						if (audio_track_found == track)
						{
							stream_select->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
							pin_to_render = pin;
						}

						audio_track_found++;
					}
					CoTaskMemFree (name);
				}

			}

		}
		filter = NULL;
	}




	HRESULT hr = S_OK;
	if (pin_to_render)
	{
		set_lav_audio_bitstreaming(m_lav, m_bitstreaming);
		if(m_useLAV)m_gb->AddFilter(m_lav, L"LAV Audio Deocder");
		m_gb->Render(pin_to_render);
		handle_downmixer();
	}
	else
		hr = VFW_E_NOT_FOUND;


	// restore filter state
	seek(time);
	if (state_before == State_Running) 
		m_mc->Run();
	else if (state_before == State_Paused)
		m_mc->Pause();

	return hr;
}
HRESULT dx_player::enable_subtitle_track(int track)
{
	CComPtr<IEnumFilters> ef;
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int subtitle_track_found = 0;

	// Save Filter State
	bool filter_stopped = false;
	OAFilterState state_before = 3, state;
	HRESULT hr = m_mc->GetState(1000, &state_before);
	if (state_before == 3)
		return E_FAIL;
	CComPtr<IPin> grenderer_input_pin;
	GetConnectedPin(m_grenderer.m_filter, PINDIR_INPUT, &grenderer_input_pin);

	POSITION t = m_external_subtitles.GetHeadPosition();
	for(DWORD i=0; i<m_external_subtitles.GetCount(); i++, m_external_subtitles.GetNext(t))
	{
		CAutoPtr<subtitle_file_handler> &tmp = m_external_subtitles.GetAt(t);
		if (i == track)
		{
			tmp->actived = true;
			{
				CAutoLock lck(&m_subtitle_sec);
				m_srenderer = tmp->m_renderer;
			}

			// disable dshow subtitle if needed
			if (state_before != State_Stopped && !filter_stopped && grenderer_input_pin)
			{
				m_mc->Stop();
				m_mc->GetState(INFINITE, &state);

				// remove (and re-add later if needed) the subtitle renderer
				m_gb->RemoveFilter(m_grenderer.m_filter);
			}
		}
		else
		{
			tmp->actived = false;
		}
		subtitle_track_found++;
	}


	if (track >= m_external_subtitles.GetCount())
	{

		// find splitter, which supports IFileSourceFilter, if it is AsyncReader, use its downstream
		m_gb->EnumFilters(&ef);
		while (ef->Next(1, &filter, NULL) == S_OK)
		{
			CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
			if (fs != NULL)
			{
				CLSID clsid;
				filter->GetClassID(&clsid);
				if (clsid == CLSID_AsyncReader)
				{
					// assume connected
					GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					PIN_INFO pi;
					connected->QueryPinInfo(&pi);
					pin = NULL;
					filter = NULL;
					filter.Attach(pi.pFilter);
				}
				CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
				if (stream_select == NULL)
				{
					// splitter that doesn't support IAMStreamSelect should have multiple Pins
					ep = NULL;
					filter->EnumPins(&ep);
					while (ep->Next(1, &pin, NULL) == S_OK)
					{
						PIN_INFO pi;
						pin->QueryPinInfo(&pi);
						if (pi.pFilter) pi.pFilter->Release();

						CComPtr<IPin> connected;
						pin->ConnectedTo(&connected);

						if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK)
						{
							if (subtitle_track_found == track)
							{
								// stop filters if needed
								if (state_before != State_Stopped && !filter_stopped)
								{
									m_mc->Stop();
									m_mc->GetState(INFINITE, &state);

								}
								// remove (and re-add later if needed) the subtitle renderer
								m_gb->RemoveFilter(m_grenderer.m_filter);
								m_gb->AddFilter(m_grenderer.m_filter, L"Subtitle Renderer");

								// render new subtitle pin
								CComPtr<IPin> srenderer_pin;
								GetUnconnectedPin(m_grenderer.m_filter, PINDIR_INPUT, &srenderer_pin);
								if (srenderer_pin) m_gb->ConnectDirect(pin, srenderer_pin, NULL);
							}

							subtitle_track_found++;
						}

						pin = NULL;
					}
				}
				else
				{
					// splitter that supports IAMStreamSelect should have only one subtitle Pin
					DWORD stream_count = 0;
					stream_select->Count(&stream_count);
					for(int i=0; i<stream_count; i++)
					{
						WCHAR *name = NULL;
						DWORD enabled = 0;
						stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

						pin = NULL;
						GetPinByName(filter, PINDIR_OUTPUT, name, &pin);

						if (DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK || wcsstr(name , L"Subtitle") || wcsstr(name, L"S:"))
						{
							if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"Subtitle", &pin);
							if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"S:", &pin);
							if (subtitle_track_found == track)
							{
								if (state_before != State_Stopped && !filter_stopped)
								{
									m_mc->Stop();
									m_mc->GetState(INFINITE, &state);

								}
								// remove (and re-add later if needed) the subtitle renderer
								m_gb->RemoveFilter(m_grenderer.m_filter);
								m_gb->AddFilter(m_grenderer.m_filter, L"Subtitle Renderer");

								// render new subtitle pin
								stream_select->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
								CComPtr<IPin> srenderer_pin;
								GetUnconnectedPin(m_grenderer.m_filter, PINDIR_INPUT, &srenderer_pin);
								if (srenderer_pin) m_gb->ConnectDirect(pin, srenderer_pin, NULL);
							}

							subtitle_track_found++;
						}
						CoTaskMemFree (name);
					}
				}
			}
			filter = NULL;
		}

	}

	// restore filter state
	if (state_before == State_Running) 
		m_mc->Run();
	else if (state_before == State_Paused)
		m_mc->Pause();

	CAutoLock lck(&m_subtitle_sec);
	if (track >= m_external_subtitles.GetCount()) m_srenderer = m_grenderer.GetSubtitleRenderer();
	if (m_srenderer)
	{
		m_srenderer->set_font(m_font);
		m_srenderer->set_font_color(m_font_color);
	}
	m_display_subtitle = true;
	draw_subtitle();
	return S_OK;
}

HRESULT dx_player::handle_downmixer()
{
	// search audio renderer
retry:
	CComPtr<IEnumFilters> ef;
	CComPtr<IBaseFilter> renderer;
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &renderer, NULL) == S_OK)
	{
		CComQIPtr<IAMAudioRendererStats, &IID_IAMAudioRendererStats> helper(renderer);
		if (helper)
			break;

		renderer = NULL;
	}

	if (!renderer)
		return E_UNEXPECTED;

	CComPtr<IPin> input;
	GetConnectedPin(renderer, PINDIR_INPUT, &input);
	if (!input)
	{
		// assume stopped
		m_gb->RemoveFilter(renderer);
		goto retry;
	}

	CComPtr<IPin> connectedto;
	input->ConnectedTo(&connectedto);
	PIN_INFO pinfo;
	connectedto->QueryPinInfo(&pinfo);
	CComPtr<IBaseFilter> up_filter;
	up_filter.Attach(pinfo.pFilter);
	CLSID clsid;
	up_filter->GetClassID(&clsid);

	if (m_downmix && clsid==CLSID_DWindowAudioDownmix)
		return S_OK;
	if (!m_downmix && clsid!=CLSID_DWindowAudioDownmix)
		return S_OK;

	// Save Filter State
	bool filter_stopped = false;
	OAFilterState state_before = 3, state = State_Stopped;
	HRESULT hr = m_mc->GetState(1000, &state_before);
	if (state_before == 3)
		return E_FAIL;
	if (state_before != State_Stopped)
	{
		m_mc->Stop();
		m_mc->GetState(INFINITE, &state);
	}

	m_gb->Disconnect(input);
	m_gb->Disconnect(connectedto);

	if (!m_downmix)
	{
		CComPtr<IPin> input2;
		GetConnectedPin(up_filter, PINDIR_INPUT, &input2);
		if (!input2)
			return E_UNEXPECTED;
		connectedto = NULL;
		input2->ConnectedTo(&connectedto);
		if (!connectedto)
			return E_UNEXPECTED;

		m_gb->RemoveFilter(up_filter);
		m_gb->ConnectDirect(connectedto, input, NULL);
	}
	else
	{
		m_gb->AddFilter(m_downmixer, L"Downmixer");
		CComPtr<IPin> mixer_i;
		CComPtr<IPin> mixer_o;
		GetUnconnectedPin(m_downmixer, PINDIR_INPUT, &mixer_i);
		GetUnconnectedPin(m_downmixer, PINDIR_OUTPUT, &mixer_o);

		if (!mixer_i || !mixer_o)
			return E_UNEXPECTED;

		m_gb->ConnectDirect(connectedto, mixer_i, NULL);
		m_gb->ConnectDirect(mixer_o, input, NULL);
	}

	m_gb->Render(connectedto);
	debug_list_filters();

	// restore filter state
	if (state_before == State_Running) 
		m_mc->Run();
	else if (state_before == State_Paused)
		m_mc->Pause();

	return S_OK;
}

HRESULT dx_player::list_audio_track(HMENU submenu)
{
	CComPtr<IEnumFilters> ef;
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int audio_track_found = 0;

	// find splitter, which supports IFileSourceFilter, if it is AsyncReader, use its downstream
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}
			//break;
			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				pin = NULL;
				filter->EnumPins(&ep);
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK)
					{
						int flag = MF_STRING | MF_BYPOSITION;
						if (connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, audio_track_found, flag, 'A0'+audio_track_found, pi.achName);
						audio_track_found++;
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin
				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(int i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);

					if (DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK || wcsstr(name , L"Audio") || wcsstr(name, L"A:"))
					{
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"Audio", &pin);
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"A:", &pin);

						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);
						int flag = MF_STRING | MF_BYPOSITION;
						if (enabled && connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, audio_track_found, flag, 'A0'+audio_track_found, name);
						audio_track_found++;
					}
					CoTaskMemFree (name);
				}

			}
		}
		filter = NULL;
	}

	if (audio_track_found != 0)
	{
		DeleteMenu(submenu, ID_AUDIO_NONE, MF_BYCOMMAND);
	}

	return S_OK;
}

HRESULT dx_player::list_subtitle_track(HMENU submenu)
{
	CComPtr<IEnumFilters> ef;
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int subtitle_track_found = 0;

	// external subtitle files
	for(POSITION i = m_external_subtitles.GetHeadPosition(); i; m_external_subtitles.GetNext(i))
	{
		int flag = MF_STRING | MF_BYPOSITION;
		CAutoPtr<subtitle_file_handler> &tmp = m_external_subtitles.GetAt(i);
		if (tmp->actived) flag |= MF_CHECKED;
		InsertMenuW(submenu, subtitle_track_found, flag, 'S0'+subtitle_track_found, tmp->m_pathname);
		subtitle_track_found++;
	}

	// find splitter, which supports IFileSourceFilter, if it is AsyncReader, use its downstream
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				pin = NULL;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}

			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				filter->EnumPins(&ep);
				pin = NULL;
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK)
					{
						int flag = MF_STRING | MF_BYPOSITION;
						if (connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, subtitle_track_found, flag, 'S0'+subtitle_track_found, pi.achName);
						subtitle_track_found++;
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin
				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(int i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);

					if (DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK || wcsstr(name , L"Subtitle") || wcsstr(name, L"S:"))
					{
						if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"Subtitle", &pin);
						if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"S:", &pin);
						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);

						int flag = MF_STRING | MF_BYPOSITION;
						if (enabled && connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, subtitle_track_found, flag, 'S0'+subtitle_track_found, name);
						subtitle_track_found++;
					}
					CoTaskMemFree (name);
				}
			}
		}
		filter = NULL;
	}

	if (subtitle_track_found != 0)
	{
		InsertMenuW(submenu, subtitle_track_found, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	}

	return S_OK;
}

wchar_t * wcsstr_nocase(const wchar_t *search_in, const wchar_t *search_for);

subtitle_file_handler::subtitle_file_handler(const wchar_t *pathname)
{
	wcscpy(m_pathname, pathname);
	actived = false;
	m_renderer = NULL;

	FILE * f = _wfopen(pathname, L"rb");
	if (!f)
		return;
	fclose(f);

	const wchar_t *p_3 = pathname + wcslen(pathname) -3;
	if ( wcsstr_nocase(pathname, L".srt") || wcsstr_nocase(pathname, L".ssa") || wcsstr_nocase(pathname, L".ass"))
	{
		m_renderer = new CsrtRenderer(NULL, 0xffffff);
	}
	else if (wcsstr_nocase(pathname, L".sup"))
	{
		m_renderer = new PGSRenderer();
	}
	else if (wcsstr_nocase(pathname, L".sub") || wcsstr_nocase(pathname, L".idx"))
	{
		m_renderer = new VobSubRenderer();
	}
	else
	{
		return;
	}

	m_renderer->load_file(m_pathname);
}

subtitle_file_handler::~subtitle_file_handler()
{
	if (m_renderer)
		delete m_renderer;
	m_renderer = NULL;
}

HRESULT dx_player::get_parameter(int parameter, double *value)
{
	switch(parameter)
	{
	case saturation:
		*value = m_saturation;
		break;
	case luminance:
		*value = m_luminance;
		break;
	case hue:
		*value = m_hue;
		break;
	case contrast:
		*value = m_contrast;
		break;

	case saturation2:
		*value = m_saturation2;
		break;
	case luminance2:
		*value = m_luminance2;
		break;
	case hue2:
		*value = m_hue2;
		break;
	case contrast2:
		*value = m_contrast2;
		break;
	default:
		return E_FAIL;
	}
	return S_OK;
}

HRESULT dx_player::set_parameter(int parameter, double value)
{
	switch(parameter)
	{
	case saturation:
		m_renderer1->m_saturation1 = m_saturation = value;
		break;
	case luminance:
		m_renderer1->m_luminance1 = m_luminance = value;
		break;
	case hue:
		m_renderer1->m_hue1 = m_hue = value;
		break;
	case contrast:
		m_renderer1->m_contrast1 = m_contrast = value;
		break;

	case saturation2:
		m_renderer1->m_saturation2 = m_saturation2 = value;
		break;
	case luminance2:
		m_renderer1->m_luminance2 = m_luminance2 = value;
		break;
	case hue2:
		m_renderer1->m_hue2 = m_hue2 = value;
		break;
	case contrast2:
		m_renderer1->m_contrast2 = m_contrast2 = value;
		break;
	default:
		return E_FAIL;
	}
	return S_OK;
}
