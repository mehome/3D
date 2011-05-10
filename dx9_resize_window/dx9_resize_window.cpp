//-----------------------------------------------------------------------------
//           Name: dx9_resize_window.cpp
//         Author: Kevin Harris
//  Last Modified: 06/16/05
//    Description: This sample demonstrates how to respond to the app's window  
//                 getting resized by resizing the front and back buffers of  
//                 the Direct3D device to match it. If you don't do this,   
//                 Direct3D will be forced to perform a stretch blit when the 
//                 window is enlarged and everything rendered will appear
//                 course and blocky. For example, if the initial window size 
//                 and Direct3D device are set to 640x480 and you enlarge the 
//                 window to be 1024x768, the Direct3D device will continue to 
//                 render at 640x480 unless its front and back buffers are  
//                 resized accordingly.
//
//                 To see what happens when you don't handle resizing properly, 
//                 run the sample and maximize the window. Once maximized, 
//                 note how smooth the teapot is and how clean the texturing 
//                 on the quad is. Next, grab the window's corner and 
//                 considerably reduce the window's size and hit F1 to toggle 
//                 off the handling code and maximize it again. When the 
//                 window is maximized the teapot and textured quad should 
//                 appear highly pixilated.
//
//   Control Keys: F1 - Toggle handling of WM_SIZE window message
//                 Left Mouse Button - Spin the teapot and textured quad
//-----------------------------------------------------------------------------

#define STRICT

#include <windows.h>
#include <atlbase.h>
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "resource.h"
#include "my12doomRenderer.h"
#include "3dvideo.h"

//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define DS_EVENT (WM_USER + 4)

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
HWND                    g_hWnd          = NULL;
HWND                    g_hWnd2         = NULL;
CComPtr<IDirect3D9>		g_pD3D;
CComPtr<IDirect3DDevice9> g_pd3dDevice;
CComPtr<IDirect3DVertexBuffer9> g_pVertexBuffer;
CComPtr<IDirect3DTexture9>      g_tex_Y;
CComPtr<IDirect3DTexture9> g_tex_U;
CComPtr<IDirect3DTexture9> g_tex_V;
CComPtr<IDirect3DTexture9> g_tex_rgb_left;
CComPtr<IDirect3DTexture9> g_tex_rgb_right;
CComPtr<IDirect3DTexture9> g_tex_row_mask;
CComPtr<IDirect3DSurface9> g_sbs_surface;
CComPtr <IDirect3DPixelShader9> ps_yv12_to_rgb;

CComPtr<IDirect3DSwapChain9> g_swap1;
CComPtr<IDirect3DSwapChain9> g_swap2;

D3DMATERIAL9            g_quadMtrl;
D3DPRESENT_PARAMETERS   g_active_pp;
D3DPRESENT_PARAMETERS   g_new_pp;
bool g_bDeviceLost = false;
bool g_bDeviceReset= false;
bool g_objects_reset = false;
bool g_render_thread_exit = false;
HANDLE h_render_thread = INVALID_HANDLE_VALUE;
HINSTANCE g_instance;
int g_render_threadid = 0;
int g_device_threadid = GetCurrentThreadId();
HRESULT hr;
CTextureRenderer *renderer = new CTextureRenderer(NULL, &hr);
CComPtr<IGraphBuilder> gb;


// FVF Vertex Order
// This is the grand master order that all D3D Vertices must adhere to or else %)

/*
    float x, y, z;   // Position (untransformed or transformed)
    float w; // The reciprocal homogeneous component

    float blend1, blend2, blend3; // Blending weight data

    float vnX, vnY, vnZ; // The vertex normal for untransformed vertices only

    float ptSize; // The vertex point size

    DWORD diffuseColor; // The diffuse color in ARGB
    DWORD specularColor; // The specular color in ARGB

    float u1, v1; // Texture coordinates (Set 1)
    float u2, v2; // Texture coordinates (Set 2)
    float u3, v3; // Texture coordinates (Set 3)
    float u4, v4; // Texture coordinates (Set 4)
    float u5, v5; // Texture coordinates (Set 5)
    float u6, v6; // Texture coordinates (Set 6)
    float u7, v7; // Texture coordinates (Set 7)
    float u8, v8; // Texture coordinates (Set 8)
                         
*/
struct QuadVertex
{
    float x , y, z;
	float w;
	DWORD diffuse;
	DWORD specular;
    float tu, tv;
};

const DWORD FVF_Flags = D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;

QuadVertex g_quadVertices[] =
{
//     x      y     z     w   	diffuse					specular	 			  tu   tv
// pass1 whole
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.0f},
    { 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.0f},
    {-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,1.0f},
    { 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,1.0f},
// pass1 left
// pass1 right
// pass1 top
// pass1 bottom

};
enum output_mode_types
{
	NV3D, masking, anaglyph, mono, pageflipping, dual_window, output_mode_types_max, safe_interlace_row, safe_interlace_line
} output_mode = mono;

enum input_layout_types
{
	side_by_side, top_bottom, mono2d, input_layout_types_max
} input_layout = mono2d;
bool g_swapeyes = false;

enum mask_mode_types
{
	row_interlace, line_interlace, checkboard_interlace, mask_mode_types_max
} mask_mode;

CCritSec g_frame_lock;
CCritSec g_device_lock;
//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void oneTimeSystemInit(void);
HRESULT restoreDeviceObjects(void);
HRESULT invalidateDeviceObjects(void);
HRESULT load_image(bool forced = false);
HRESULT handle_reset();
HRESULT generate_mask();
HRESULT render(int time = 1, bool forced = false);
HRESULT render_unlocked(int time = 1, bool forced = false);
void shutDown(void);
void set_full_screen(bool full);
void terminate_render_thread();
void create_render_thread();


DWORD WINAPI render_thread(LPVOID lpParame)
{
	g_render_threadid = GetCurrentThreadId();
	// I don't know why, but WM_SIZE seems to be a little different
	int l = timeGetTime();
	while (timeGetTime() - l < 0 && !g_render_thread_exit)
		Sleep(1);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while(!g_render_thread_exit)
	{
		if (output_mode != pageflipping)
		{
			if (renderer->m_state == State_Running)
			{
				Sleep(1);
				continue;
			}
			else
			{
				render_unlocked(1, true);
				l = timeGetTime();
				while (timeGetTime() - l < 333 && !g_render_thread_exit)
					Sleep(1);
			}
		}
		else if (render_unlocked(1, true) == S_FALSE)
			Sleep(1);
	}
	return 0;
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
			(L"Open File"),
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

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpCmdLine,
                    int       nCmdShow )
{
	g_instance = hInstance;

	HANDLE h = GetCurrentProcess();
	//SetProcessAffinityMask(h, 1);
	CloseHandle(h);

    WNDCLASSEX winClass; 
    MSG        uMsg;

    memset(&uMsg,0,sizeof(uMsg));
    
    winClass.lpszClassName = _T("MY_WINDOWS_CLASS");
    winClass.cbSize        = sizeof(WNDCLASSEX);
    winClass.style         = CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc   = WindowProc;
    winClass.hInstance     = hInstance;
    winClass.hIcon         = LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
    winClass.hIconSm       = LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
    winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    winClass.hbrBackground = NULL;//(HBRUSH)GetStockObject(BLACK_BRUSH);
    winClass.lpszMenuName  = NULL;
    winClass.cbClsExtra    = 0;
    winClass.cbWndExtra    = 0;

    if( !RegisterClassEx(&winClass) )
        return E_FAIL;

    g_hWnd = CreateWindowEx( NULL, _T("MY_WINDOWS_CLASS"),
                             _T("Direct3D (DX9) - Resize Window"),
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							 //WS_EX_TOPMOST | WS_POPUP,    // fullscreen values
                             1000, 0, 800, 480, NULL, NULL, hInstance, NULL );

	g_hWnd2 = CreateWindowEx( NULL, _T("MY_WINDOWS_CLASS"),
		_T("Direct3D (DX9) - Resize Window2"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		//WS_EX_TOPMOST | WS_POPUP,    // fullscreen values
		1500, 50, 800, 480, NULL, NULL, hInstance, NULL );

    if( g_hWnd == NULL  || g_hWnd2 == NULL)
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );
    UpdateWindow( g_hWnd );
	ShowWindow( g_hWnd2, output_mode == dual_window ? nCmdShow : SW_HIDE );
	UpdateWindow( g_hWnd2 );

    oneTimeSystemInit();

	while( uMsg.message != WM_QUIT )
    {
        if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
        { 
            TranslateMessage( &uMsg );
            DispatchMessage( &uMsg );
        }
        else
		{
			if (S_OK == handle_reset())
				Sleep(1);
		}
    }

	//terminate_render_thread();
	gb = NULL;

    shutDown();

    UnregisterClass( _T("MY_WINDOWS_CLASS"), winClass.hInstance );

    return uMsg.wParam;
}

//-----------------------------------------------------------------------------
// Name: WindowProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT CALLBACK WindowProc( HWND   hWnd,
                             UINT   msg,
                             WPARAM wParam,
                             LPARAM lParam )
{
    static POINT ptLastMousePosit;
    static POINT ptCurrentMousePosit;
    static bool bMousing;
    
    switch( msg )
    {
		case DS_EVENT:
			{
				CComQIPtr<IMediaEvent, &IID_IMediaEvent> event(gb);

				long event_code;
				LONG_PTR param1;
				LONG_PTR param2;
				if (FAILED(event->GetEvent(&event_code, &param1, &param2, 0)))
					return S_OK;

				if (event_code == EC_COMPLETE)
				{
					CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> ms(gb);
					REFERENCE_TIME target = 0;
					ms->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
				}
			}
			break;

        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;

				case '1':
					input_layout = (input_layout_types) ((input_layout+1)%input_layout_types_max);
					g_objects_reset = true;
					handle_reset();
					render(1, true);
					break;

				case '2':
					g_swapeyes = !g_swapeyes;
					load_image(true);
					render(1, true);
					break;

				case '3':
					output_mode = (output_mode_types)((output_mode+1) % output_mode_types_max);
					if (output_mode == dual_window && !g_active_pp.Windowed)
						output_mode = (output_mode_types)((output_mode+1) % output_mode_types_max);

					ShowWindow(g_hWnd2, output_mode == dual_window ? SW_SHOW : SW_HIDE);
					generate_mask();
					render(1, true);
					break;

				case '4':
					mask_mode = (mask_mode_types) ((mask_mode+1)% mask_mode_types_max);
					generate_mask();
					render(1, true);
					break;

				case VK_F11:
					if (output_mode != dual_window)
					{
						set_full_screen(g_active_pp.Windowed);
						g_bDeviceReset = true;
						handle_reset();
						render(1, true);
					}
					break;

				case VK_F1:
					{
						CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
						mc->Pause();
					}
					break;

				case VK_F2:
					{
						CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
						mc->Run();
					}
					break;

				case VK_F3:
					render(1, true);
					break;
           }
        }
        break;

        case WM_SIZE:
        {
			if(!g_bDeviceLost)
			{
				// If the device is not NULL and the WM_SIZE message is not a
				// SIZE_MINIMIZED event, resize the device's swap buffers to match
				// the new window size.
				if( g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED )
				{
					g_new_pp.BackBufferWidth  = LOWORD(lParam);
					g_new_pp.BackBufferHeight = HIWORD(lParam);
					g_bDeviceReset = true;

					handle_reset();
					render(1, true);
				}
			}
        }
        break;

        case WM_CLOSE:
        {
            PostQuitMessage(0); 
        }
        
		
		case WM_MOVE:
			{
				generate_mask();
				render(1, true);
			}
			break;

        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;

        default:
        {
            return DefWindowProc( hWnd, msg, wParam, lParam );
        }
        break;
    }

    return 0;
}

void set_full_screen(bool full)
{
	D3DDISPLAYMODE d3ddm;
	g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	g_new_pp.BackBufferFormat       = d3ddm.Format;
	if(full)
	{
		g_new_pp.Windowed = FALSE;
		g_new_pp.BackBufferWidth = d3ddm.Width;
		g_new_pp.BackBufferHeight = d3ddm.Height;
		g_new_pp.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
	}
	else
	{
		g_new_pp.Windowed = TRUE;
		g_new_pp.BackBufferWidth = 0;
		g_new_pp.BackBufferHeight = 0;
		g_new_pp.hDeviceWindow = g_hWnd;
		g_new_pp.FullScreen_RefreshRateInHz = 0;
	}

}
void oneTimeSystemInit( void )
{
	timeBeginPeriod(1);
    g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );

    ZeroMemory( &g_active_pp, sizeof(g_active_pp) );
    g_active_pp.Windowed               = TRUE;
    g_active_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    g_active_pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;
	g_active_pp.BackBufferCount = 1;
	g_active_pp.Flags = D3DPRESENTFLAG_VIDEO;

	set_full_screen(false);

	UINT AdapterToUse=D3DADAPTER_DEFAULT;
	D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL;
	for (UINT Adapter=0;Adapter<g_pD3D->GetAdapterCount();Adapter++)
	{ 
		D3DADAPTER_IDENTIFIER9  Identifier; 
		HRESULT       Res; 

		Res = g_pD3D->GetAdapterIdentifier(Adapter,0,&Identifier); 
		if (strstr(Identifier.Description,"PerfHUD") != 0) 
		{ 
			AdapterToUse=Adapter; 
			DeviceType=D3DDEVTYPE_REF; 
			break; 
		}
	}
    HRESULT hr = g_pD3D->CreateDevice( AdapterToUse, DeviceType,
                          g_hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                          &g_active_pp, &g_pd3dDevice );

	g_new_pp = g_active_pp;

	// dshow
	wchar_t file[MAX_PATH] = L"test.avi";
	open_file_dlg(file, g_hWnd, NULL);
	gb.CoCreateInstance(CLSID_FilterGraph);
	CComQIPtr<IBaseFilter, &IID_IBaseFilter> renderer_base(renderer);
	gb->AddFilter(renderer_base, L"Texture Renderer");
	//gb->RenderFile(L"F:\\TDDOWNLOAD\\00019hsbs.mkv", NULL);
	//gb->RenderFile(L"Z:\\avts.ts", NULL);
	if (FAILED(gb->RenderFile(file, NULL)))
		exit(-1);
	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
	mc->Run();
	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(gb);
	event_ex->SetNotifyWindow((OAHWND)g_hWnd, DS_EVENT, 0);

    restoreDeviceObjects();
}


HRESULT load_image(bool forced /* = false */)
{
	CAutoLock lck(&g_frame_lock);
	// loading YV12 image as three L8 texture
	{
		CAutoLock lck(&renderer->m_data_lock);
		if (!forced && !renderer->m_data_changed)
			return S_FALSE;

		renderer->m_data_changed = false;

		D3DLOCKED_RECT d3dlr;
		BYTE * src = renderer->m_data;
		BYTE * dst;

		if (!g_tex_Y || !g_tex_U ||!g_tex_V)
			return E_FAIL;

		// load Y
		if( FAILED(hr = g_tex_Y->LockRect(0, &d3dlr, 0, D3DLOCK_DISCARD)))
			return hr;
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<renderer->m_lVidHeight; i++)
		{
			memcpy(dst, src, renderer->m_lVidWidth);
			src += renderer->m_lVidWidth;
			dst += d3dlr.Pitch;
		}
		// black level test
		// memset(d3dlr.pBits, 0, d3dlr.Pitch * 100);
		g_tex_Y->UnlockRect(0);

		// load V
		if( FAILED(hr = g_tex_V->LockRect(0, &d3dlr, 0, D3DLOCK_DISCARD)))
			return hr;
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<renderer->m_lVidHeight/2; i++)
		{
			memcpy(dst, src, renderer->m_lVidWidth/2);
			src += renderer->m_lVidWidth/2;
			dst += d3dlr.Pitch;
		}
		// black level test
		// memset(d3dlr.pBits, 128, d3dlr.Pitch * 50);
		g_tex_V->UnlockRect(0);

		// load U
		if( FAILED(hr = g_tex_U->LockRect(0, &d3dlr, 0, D3DLOCK_DISCARD)))
			return hr;
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<renderer->m_lVidHeight/2; i++)
		{
			memcpy(dst, src, renderer->m_lVidWidth/2);
			src += renderer->m_lVidWidth/2;
			dst += d3dlr.Pitch;
		}
		// black level test
		//memset(d3dlr.pBits, 128, d3dlr.Pitch * 50);
		g_tex_U->UnlockRect(0);
	}

	if (!g_pd3dDevice || !g_tex_rgb_left ||!g_tex_rgb_right)
		return E_FAIL;

	// pass 1: render full resolution to two seperate texture
	hr = g_pd3dDevice->BeginScene();
	hr = g_pd3dDevice->SetPixelShader(ps_yv12_to_rgb);
	hr = g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = g_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = g_tex_rgb_right->GetSurfaceLevel(0, &right_surface);
	if (!g_swapeyes)
		hr = g_pd3dDevice->SetRenderTarget(0, left_surface);
	else
		hr = g_pd3dDevice->SetRenderTarget(0, right_surface);


	// drawing
	hr = g_pd3dDevice->SetTexture( 0, g_tex_Y );
	hr = g_pd3dDevice->SetTexture( 1, g_tex_U );
	hr = g_pd3dDevice->SetTexture( 2, g_tex_V );
	hr = g_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	hr = g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetStreamSource( 0, g_pVertexBuffer, 0, sizeof(QuadVertex) );
	hr = g_pd3dDevice->SetFVF( FVF_Flags );

	// modify vertex, this also avoid buffer miss
	// -0.5 is the difference between texture and triangle coordinate
	int surface_width = renderer->m_lVidWidth;
	int surface_height = renderer->m_lVidHeight;
	if (input_layout == side_by_side)
		surface_width /= 2;
	else if (input_layout == top_bottom)
		surface_height /= 2;

	g_quadVertices[0].x = -0.5f; g_quadVertices[0].y = -0.5f;
	g_quadVertices[1].x = surface_width-0.5f; g_quadVertices[1].y = -0.5f;
	g_quadVertices[2].x = -0.5f; g_quadVertices[2].y = surface_height-0.5f;
	g_quadVertices[3].x =  surface_width-0.5f; g_quadVertices[3].y =surface_height-0.5f;
	if (input_layout == side_by_side)
	{
		g_quadVertices[0].tu = 0.0f; g_quadVertices[0].tv = 0.0f;
		g_quadVertices[1].tu = 0.5f; g_quadVertices[1].tv = 0.0f;
		g_quadVertices[2].tu = 0.0f; g_quadVertices[2].tv = 1.0f;
		g_quadVertices[3].tu = 0.5f; g_quadVertices[3].tv = 1.0f;
	}
	else if (input_layout == top_bottom)
	{
		g_quadVertices[0].tu = 0.0f; g_quadVertices[0].tv = 0.0f;
		g_quadVertices[1].tu = 1.0f; g_quadVertices[1].tv = 0.0f;
		g_quadVertices[2].tu = 0.0f; g_quadVertices[2].tv = 0.5f;
		g_quadVertices[3].tu = 1.0f; g_quadVertices[3].tv = 0.5f;
	}
	else if (input_layout == mono2d)
	{
		g_quadVertices[0].tu = 0.0f; g_quadVertices[0].tv = 0.0f;
		g_quadVertices[1].tu = 1.0f; g_quadVertices[1].tv = 0.0f;
		g_quadVertices[2].tu = 0.0f; g_quadVertices[2].tv = 1.0f;
		g_quadVertices[3].tu = 1.0f; g_quadVertices[3].tv = 1.0f;
	}
	void *pVertices = NULL;
	g_pVertexBuffer->Lock( 0, sizeof(g_quadVertices), (void**)&pVertices, D3DLOCK_DISCARD );
	memcpy( pVertices, g_quadVertices, sizeof(g_quadVertices) );
	g_pVertexBuffer->Unlock();

	hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

	// modify vertex again, change render target
	if (input_layout == side_by_side)
	{
		g_quadVertices[0].tu = 0.5f; g_quadVertices[0].tv = 0.0f;
		g_quadVertices[1].tu = 1.0f; g_quadVertices[1].tv = 0.0f;
		g_quadVertices[2].tu = 0.5f; g_quadVertices[2].tv = 1.0f;
		g_quadVertices[3].tu = 1.0f; g_quadVertices[3].tv = 1.0f;
	}
	else if (input_layout == top_bottom)
	{
		g_quadVertices[0].tu = 0.0f; g_quadVertices[0].tv = 0.5f;
		g_quadVertices[1].tu = 1.0f; g_quadVertices[1].tv = 0.5f;
		g_quadVertices[2].tu = 0.0f; g_quadVertices[2].tv = 1.0f;
		g_quadVertices[3].tu = 1.0f; g_quadVertices[3].tv = 1.0f;
	}
	else if (input_layout == mono2d)
	{
		g_quadVertices[0].tu = 0.0f; g_quadVertices[0].tv = 0.0f;
		g_quadVertices[1].tu = 1.0f; g_quadVertices[1].tv = 0.0f;
		g_quadVertices[2].tu = 0.0f; g_quadVertices[2].tv = 1.0f;
		g_quadVertices[3].tu = 1.0f; g_quadVertices[3].tv = 1.0f;
	}
	g_pVertexBuffer->Lock( 0, sizeof(g_quadVertices), (void**)&pVertices, D3DLOCK_DISCARD );
	memcpy( pVertices, g_quadVertices, sizeof(g_quadVertices) );
	g_pVertexBuffer->Unlock();

	if (!g_swapeyes)
		hr = g_pd3dDevice->SetRenderTarget(0, right_surface);
	else
		hr = g_pd3dDevice->SetRenderTarget(0, left_surface);

	hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );


	hr = g_pd3dDevice->EndScene();

	// copy to 2x width +1 height surface, which already have NV3D tag.
	// note: don't overwrite that tag
	// this can also be used in process that need a double back_buffer width sbs rendered surface
	RECT tar = {0,0, g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight};
	double aspect = (double)renderer->m_lVidWidth / renderer->m_lVidHeight;
	if (aspect> 2.425)
		aspect /= 2;
	else if (aspect< 1.2125)
		aspect *= 2;
	int delta_w = (int)(g_active_pp.BackBufferWidth - g_active_pp.BackBufferHeight * aspect + 0.5);
	int delta_h = (int)(g_active_pp.BackBufferHeight- g_active_pp.BackBufferWidth  / aspect + 0.5);
	if (delta_w > 0)
	{
		tar.left += delta_w/2;
		tar.right -= delta_w/2;
	}
	else if (delta_h > 0)
	{
		tar.top += delta_h/2;
		tar.bottom -= delta_h/2;
	}
	hr = g_pd3dDevice->StretchRect(left_surface, NULL, g_sbs_surface, &tar, D3DTEXF_LINEAR);
	tar.left += g_active_pp.BackBufferWidth;
	tar.right += g_active_pp.BackBufferWidth;
	hr = g_pd3dDevice->StretchRect(right_surface, NULL, g_sbs_surface, &tar, D3DTEXF_LINEAR);
	return S_OK;
}


HRESULT generate_mask()
{
	if (output_mode != masking)
		return S_FALSE;

	if (!g_tex_row_mask)
		return VFW_E_NOT_CONNECTED;

	CAutoLock lck(&g_frame_lock);
	D3DLOCKED_RECT locked;
	hr = g_tex_row_mask->LockRect(0, &locked, NULL, D3DLOCK_DISCARD);
	if (FAILED(hr)) return hr;

	RECT rect;
	GetWindowRect(g_hWnd, &rect);
	BYTE *dst = (BYTE*) locked.pBits;

	if (mask_mode == row_interlace)
	{
		// init row mask texture
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<g_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, g_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (mask_mode == line_interlace)
	{
		for(DWORD i=0; i<g_active_pp.BackBufferHeight; i++)
		{
			memset(dst, (i+rect.top)%2 == 0 ? 0 : 255 ,g_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (mask_mode == checkboard_interlace)
	{
		// init row mask texture
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<g_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line + (i%2), g_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	hr = g_tex_row_mask->UnlockRect(0);

	return hr;
}

//-----------------------------------------------------------------------------
// Name: restoreDeviceObjects()
// Desc: You are encouraged to develop applications with a single code path to 
//       respond to device loss. This code path is likely to be similar, if not 
//       identical, to the code path taken to initialize the device at startup.
//-----------------------------------------------------------------------------
HRESULT restoreDeviceObjects( void )
{
	HRESULT hr = g_pd3dDevice->CreateTexture(renderer->m_lVidWidth, renderer->m_lVidHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&g_tex_Y, NULL);
	hr = g_pd3dDevice->CreateTexture(renderer->m_lVidWidth/2, renderer->m_lVidHeight/2, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&g_tex_U, NULL);	    
	hr = g_pd3dDevice->CreateTexture(renderer->m_lVidWidth/2, renderer->m_lVidHeight/2, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&g_tex_V, NULL);

	int surface_width = renderer->m_lVidWidth;
	int surface_height = renderer->m_lVidHeight;
	if (input_layout == side_by_side)
		surface_width /= 2;
	else if (input_layout == top_bottom)
		surface_height /= 2;
	hr = g_pd3dDevice->CreateTexture(surface_width, surface_height, 1, D3DUSAGE_RENDERTARGET, g_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_tex_rgb_left, NULL);
	hr = g_pd3dDevice->CreateTexture(surface_width, surface_height, 1, D3DUSAGE_RENDERTARGET, g_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_tex_rgb_right, NULL);
	hr = g_pd3dDevice->CreateTexture(g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &g_tex_row_mask, NULL);
	hr = g_pd3dDevice->CreateRenderTarget(g_active_pp.BackBufferWidth*2, g_active_pp.BackBufferHeight+1, g_active_pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &g_sbs_surface, NULL);


	generate_mask();


	// add 3d vision tag at last line
	D3DLOCKED_RECT lr;
	RECT lock_tar={0, g_active_pp.BackBufferHeight, g_active_pp.BackBufferWidth*2, g_active_pp.BackBufferHeight+1};
	g_sbs_surface->LockRect(&lr,&lock_tar,0);
	LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
	pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
	pSIH->dwBPP = 32;
	pSIH->dwFlags = SIH_SIDE_BY_SIDE;
	pSIH->dwWidth = g_active_pp.BackBufferWidth;
	pSIH->dwHeight = g_active_pp.BackBufferHeight;
	g_sbs_surface->UnlockRect();



    //
    // Create a vertex buffer...
    //
    // NOTE: When a device is lost, vertex buffers created using  
    // D3DPOOL_DEFAULT must be released properly before calling 
    // IDirect3DDevice9::Reset.
    //

    g_pd3dDevice->CreateVertexBuffer( 4*sizeof(QuadVertex),
                                      D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                      FVF_Flags,
                                      D3DPOOL_DEFAULT,
                                      &g_pVertexBuffer, NULL );
    void *pVertices = NULL;

    g_pVertexBuffer->Lock( 0, sizeof(g_quadVertices), (void**)&pVertices, D3DLOCK_DISCARD );
    memcpy( pVertices, g_quadVertices, sizeof(g_quadVertices) );
    g_pVertexBuffer->Unlock();

	HGLOBAL hDllData = LoadResource(g_instance, FindResource(g_instance, MAKEINTRESOURCE(IDR_PLANAR2RGB), RT_RCDATA));
	DWORD * shader_data = (DWORD*)LockResource(hDllData);

	if (shader_data)
	{
		hr = g_pd3dDevice->CreatePixelShader(shader_data, &ps_yv12_to_rgb);
	}
	else
		hr = E_FAIL;

	// Create the pixel shader
	if (FAILED(hr)) return E_FAIL;

	load_image(true);

	hr = g_pd3dDevice->GetSwapChain(0, &g_swap1);
	hr = g_pd3dDevice->CreateAdditionalSwapChain(&g_active_pp, &g_swap2);

	// create render thread
	create_render_thread();

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: invalidateDeviceObjects()
// Desc: If the lost device can be restored, the application prepares the 
//       device by destroying all video-memory resources and any 
//       swap chains. This is typically accomplished by using the SAFE_RELEASE 
//       macro.
//-----------------------------------------------------------------------------
HRESULT invalidateDeviceObjects( void )
{
	terminate_render_thread();
	ps_yv12_to_rgb = NULL;
	g_tex_row_mask = NULL;
	g_tex_rgb_left = NULL;
	g_tex_rgb_right = NULL;
	g_sbs_surface = NULL;
    g_tex_Y = NULL;
	g_tex_V = NULL;
	g_tex_U = NULL;
	g_pVertexBuffer = NULL;

	g_swap1 = NULL;
	g_swap2 = NULL;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: shutDown()
// Desc: 
//-----------------------------------------------------------------------------
void shutDown( void )
{
	CAutoLock lck(&g_device_lock);
    invalidateDeviceObjects();
	g_pd3dDevice = NULL;
    g_pD3D = NULL;
}

void create_render_thread()
{
	if (INVALID_HANDLE_VALUE != h_render_thread)
		terminate_render_thread();
	g_render_thread_exit = false;
	h_render_thread = CreateThread(0,0,render_thread, NULL, NULL, NULL);
}
void terminate_render_thread()
{
	if (INVALID_HANDLE_VALUE == h_render_thread)
		return;
	g_render_thread_exit = true;
	WaitForSingleObject(h_render_thread, INFINITE);
	h_render_thread = INVALID_HANDLE_VALUE;
}

HRESULT handle_reset()
{
	if(g_bDeviceLost)
	{
		if (g_device_threadid != GetCurrentThreadId())
			return E_FAIL;

		Sleep( 100 );

		HRESULT hr = S_OK;
		if( FAILED( hr = g_pd3dDevice->TestCooperativeLevel() ) )
		{
			if( hr == D3DERR_DEVICENOTRESET )
			{
				CAutoLock lck(&g_device_lock);
				invalidateDeviceObjects();
				hr = g_pd3dDevice->Reset( &g_new_pp );

				if( FAILED(hr ) )
					return hr;
				g_active_pp = g_new_pp;
				restoreDeviceObjects();
				g_bDeviceLost = false;
				g_bDeviceReset = false;
				g_objects_reset = false;

				OutputDebugStringA("device restored.\n");
			}

			else
				return hr;
		}
	}

	else if (g_bDeviceReset)
	{
		if (g_device_threadid != GetCurrentThreadId())
			return E_FAIL;

		CAutoLock lck(&g_device_lock);

		invalidateDeviceObjects();
		hr = g_pd3dDevice->Reset( &g_new_pp );

		if( FAILED(hr ) )
			return hr;
		g_active_pp = g_new_pp;
		restoreDeviceObjects();
		g_bDeviceLost = false;
		g_bDeviceReset = false;
		g_objects_reset = false;
		OutputDebugStringA("device reseted.\n");
	}

	else if (g_objects_reset)
	{
		CAutoLock lck(&g_device_lock);
		invalidateDeviceObjects();
		restoreDeviceObjects();
		g_objects_reset = false;
		OutputDebugStringA("objects reseted.\n");
	}

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: render()
// Desc: 
//-----------------------------------------------------------------------------

HRESULT render( int time, bool forced)		// with device lock
{
	CAutoLock lck(&g_device_lock);
	if (g_pd3dDevice)
		return render_unlocked(time, forced);
	else
		return E_FAIL;
}
int last_render_time = timeGetTime();
HRESULT render_unlocked( int time, bool forced)
{

	// device state check and restore
	if (FAILED(handle_reset()))
		return E_FAIL;


	// image loading and idle check
	if (load_image() != S_OK && !forced)	// no more rendering except pageflipping mode
		return S_FALSE;

	if (!g_pd3dDevice)
		return E_FAIL;

	last_render_time = timeGetTime();

	OutputDebugStringA("render!\n");
	HRESULT hr;
	for(int i=0; i<time; i++)
    {
		CAutoLock lck(&g_frame_lock);
		// pass 2: drawing to back buffer!
		hr = g_pd3dDevice->BeginScene();
		hr = g_pd3dDevice->SetPixelShader(NULL);
		CComPtr<IDirect3DSurface9> left_surface;
		CComPtr<IDirect3DSurface9> right_surface;
		hr = g_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
		hr = g_tex_rgb_right->GetSurfaceLevel(0, &right_surface);

		// test: NeedToHandleDisplayChange()?


		// set render target to back buffer
		CComPtr<IDirect3DSurface9> back_buffer;
		hr = g_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
		hr = g_pd3dDevice->SetRenderTarget(0, back_buffer);

		// back ground
		#ifdef DEBUG
		g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
		#else
		g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
		#endif

		// modify vertex back to single width
		g_quadVertices[0].x = -0.5; g_quadVertices[0].y = -0.5;
		g_quadVertices[1].x = g_active_pp.BackBufferWidth-0.5f; g_quadVertices[1].y = -0.5;
		g_quadVertices[2].x = -0.5; g_quadVertices[2].y = g_active_pp.BackBufferHeight-0.5f;
		g_quadVertices[3].x = g_active_pp.BackBufferWidth-0.5f; g_quadVertices[3].y = g_active_pp.BackBufferHeight-0.5f;
		g_quadVertices[0].tu = 0.0f; g_quadVertices[0].tv = 0.0f;
		g_quadVertices[1].tu = 1.0f; g_quadVertices[1].tv = 0.0f;
		g_quadVertices[2].tu = 0.0f; g_quadVertices[2].tv = 1.0f;
		g_quadVertices[3].tu = 1.0f; g_quadVertices[3].tv = 1.0f;
		void *pVertices = NULL;
		g_pVertexBuffer->Lock( 0, sizeof(g_quadVertices), (void**)&pVertices, D3DLOCK_DISCARD );
		memcpy( pVertices, g_quadVertices, sizeof(g_quadVertices) );
		g_pVertexBuffer->Unlock();

		// reset texture blending stages
		g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_DISABLE);


		if (output_mode == NV3D)
		{
			// StretchRect to backbuffer!, this is how 3D vision works
			RECT tar = {0,0, g_active_pp.BackBufferWidth*2, g_active_pp.BackBufferHeight};
			hr = g_pd3dDevice->StretchRect(g_sbs_surface, &tar, back_buffer, NULL, D3DTEXF_NONE);		//source is as previous, tag line not overwrited
		}

		else if (output_mode == anaglyph)
		{
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );			 // current = texture * diffuse(red)
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_RESULTARG, D3DTA_CURRENT);

			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 0 );
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_SPECULAR );
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG0, D3DTA_CURRENT );
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD ); // Modulate then add...
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_RESULTARG, D3DTA_CURRENT);     // current = texture * specular(cyan) + current
			
			g_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE);

			g_pd3dDevice->SetTexture( 0, g_tex_rgb_left );
			g_pd3dDevice->SetTexture( 1, g_tex_rgb_right );

			hr = g_pd3dDevice->SetStreamSource( 0, g_pVertexBuffer, 0, sizeof(QuadVertex) );
			hr = g_pd3dDevice->SetFVF( FVF_Flags );
			hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
		}

		else if (output_mode == masking)
		{
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );	// current = texture(mask)

			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 0 );
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SUBTRACT ); // subtract... to show only black 
			g_pd3dDevice->SetTextureStageState( 1, D3DTSS_RESULTARG, D3DTA_TEMP);	  // temp = texture(left) - current(mask)

			g_pd3dDevice->SetTextureStageState( 2, D3DTSS_TEXCOORDINDEX, 0 );
			g_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			g_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLORARG2, D3DTA_CURRENT );
			g_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLORARG0, D3DTA_TEMP );		 // arg3 = arg0...
			g_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD ); // Modulate then add... to show only white then add with temp(black)
			g_pd3dDevice->SetTextureStageState( 2, D3DTSS_RESULTARG, D3DTA_CURRENT);     // current = texture(right) * current(mask) + temp

			g_pd3dDevice->SetTexture( 0, g_tex_row_mask );
			g_pd3dDevice->SetTexture( 1, g_tex_rgb_left );
			g_pd3dDevice->SetTexture( 2, g_tex_rgb_right );

			hr = g_pd3dDevice->SetStreamSource( 0, g_pVertexBuffer, 0, sizeof(QuadVertex) );
			hr = g_pd3dDevice->SetFVF( FVF_Flags );
			hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
		}

		else if (output_mode == safe_interlace_row)
		{
			// very slow, but reliable;
			for(DWORD i=0; i<g_active_pp.BackBufferWidth; i+=2)
			{
				RECT src = {i, 0, i+1, g_active_pp.BackBufferHeight};
				RECT dst = src;
				g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer, &dst, D3DTEXF_NONE);

				src.left += g_active_pp.BackBufferWidth + 1;
				src.right += g_active_pp.BackBufferWidth + 1;
				dst.left += 1;
				dst.right += 1;
				g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer, &dst, D3DTEXF_NONE);
			}

		}

		else if (output_mode == safe_interlace_line)
		{
			// very slow, but reliable;
			for(DWORD i=0; i<g_active_pp.BackBufferHeight; i+=2)
			{
				RECT src = {0, i, g_active_pp.BackBufferWidth, i+1};
				RECT dst = src;
				g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer, &dst, D3DTEXF_NONE);

				src.left = g_active_pp.BackBufferWidth;
				src.right = g_active_pp.BackBufferWidth * 2;
				src.top = i+1;
				src.bottom = i+2;
				dst = src;
				dst.left = 0;
				dst.right = g_active_pp.BackBufferWidth;
				g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer, &dst, D3DTEXF_NONE);
			}
		}

		else if (output_mode == mono)
		{
			// cut left half
			RECT src = {0, 0, g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight};
			RECT dst = src;
			hr = g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer, &dst, D3DTEXF_NONE);
		}

		else if (output_mode == pageflipping)
		{
			RECT src = {0, 0, g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight};
			RECT dst = src;

			static bool left = true;
			if (!left)
			{
				src.left += g_active_pp.BackBufferWidth;
				src.right += g_active_pp.BackBufferWidth;
			}
			left = !left;
			hr = g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer, &dst, D3DTEXF_NONE);
		}

		else if (output_mode == dual_window)
		{
			RECT src = {0, 0, g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight};
			RECT dst = src;
			hr = g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer, &dst, D3DTEXF_NONE);

			src.left += g_active_pp.BackBufferWidth;
			src.right += g_active_pp.BackBufferWidth;
			CComPtr<IDirect3DSurface9> back_buffer2;
			g_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);
			hr = g_pd3dDevice->StretchRect(g_sbs_surface, &src, back_buffer2, &dst, D3DTEXF_NONE);
		}

		g_pd3dDevice->EndScene();
    }

	if (output_mode == dual_window)
	{
		if (g_swap1->Present(NULL, NULL, g_hWnd, NULL, D3DPRESENT_DONOTWAIT) == D3DERR_DEVICELOST)
			g_bDeviceLost = true;

		if (g_swap2->Present(NULL, NULL, g_hWnd2, NULL, D3DPRESENT_DONOTWAIT) == D3DERR_DEVICELOST)
			g_bDeviceLost = true;

	}

	else
	{
		if (g_pd3dDevice->Present( NULL, NULL, NULL, NULL ) ==  D3DERR_DEVICELOST)
			g_bDeviceLost = true;
	}


	char tmp[256];
	static int t = timeGetTime();
	sprintf(tmp, "%d\n", timeGetTime()-t);
	if (timeGetTime()-t > 18)
		OutputDebugStringA("lost sync\n");
	t = timeGetTime();
	OutputDebugStringA(tmp);

	return S_OK;
}
