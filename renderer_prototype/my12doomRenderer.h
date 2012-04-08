#pragma  once
#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>
#include "..\AESFile\rijndael.h"
#include "my12doomUI.h"
#include "TextureAllocator.h"
#include "..\dwindow\nvapi.h"
#include "AtiDx9Stereo.h"

struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_my12doomRenderer;
#define WM_NV_NOTIFY (WM_USER+10086)
#define PCLEVELTEST_TESTED 1
#define PCLEVELTEST_YV12 2
#define PCLEVELTEST_NV12 4
#define PCLEVELTEST_YUY2 8

const int fade_in_out_time = 500;
void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName);

// this renderer must have a valid first window, if not, connection will fail.
// setting window to invalid window during 
// you can enter dual projector mode without a second window, but you won't get second image until you set a second window

enum output_mode_types
{
	NV3D, masking, anaglyph, mono, pageflipping, iz3d,
	dual_window, out_sbs, out_tb,
	out_hsbs, out_htb, 
	out_hd3d, output_mode_types_max
};

#ifndef def_input_layout_types
#define def_input_layout_types
enum input_layout_types
{
	side_by_side, top_bottom, mono2d, input_layout_types_max, input_layout_auto
};
#endif

enum mask_mode_types
{
	row_interlace, line_interlace, checkboard_interlace, mask_mode_types_max
};

enum aspect_mode_types
{
	aspect_letterbox,
	aspect_stretch,
	aspect_horizontal_fill,
	aspect_vertical_fill,
	aspect_mode_types_max,
};



class Imy12doomRendererCallback
{
public:
	virtual HRESULT SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn) = 0;
};

class my12doomRenderer;
class DRendererInputPin : public CRendererInputPin
{
public:
	DRendererInputPin(__inout CBaseRenderer *pRenderer,	__inout HRESULT *phr, __in_opt LPCWSTR Name) : CRendererInputPin(pRenderer, phr, Name){}
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP BeginFlush(void);
	STDMETHODIMP GetAllocator(__deref_out IMemAllocator ** ppAllocator)
	{
		return __super::GetAllocator(ppAllocator);
	}

	STDMETHODIMP NotifyAllocator(
		IMemAllocator * pAllocator,
		BOOL bReadOnly)
	{
		return __super::NotifyAllocator(pAllocator, bReadOnly);
	}
protected:
	friend class my12doomRenderer;
};


typedef struct _dummy_packet
{
	REFERENCE_TIME start;
	REFERENCE_TIME end;
} dummy_packet;
#define my12doom_queue_size 16

class DBaseVideoRenderer: public CBaseVideoRenderer
{
public:
	DBaseVideoRenderer(REFCLSID clsid,LPCTSTR name , LPUNKNOWN pUnk,HRESULT *phr)
		: CBaseVideoRenderer(clsid, name, pUnk, phr){};
	virtual CBasePin * GetPin(int n);
	virtual HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	virtual HRESULT BeginFlush(void);
protected:
	friend class DRendererInputPin;
	REFERENCE_TIME m_thisstream;
};

class gpu_sample
{
public:
	gpu_sample(IMediaSample *memory_sample, CTextureAllocator *allocator, int width, int height, CLSID format, bool topdown_RGB32, bool do_cpu_test = false, bool remux_mode = false, D3DPOOL pool = D3DPOOL_SYSTEMMEM, DWORD PC_LEVEL = 0);
	~gpu_sample();
	bool is_ignored_line(int line);
	HRESULT prepare_rendering();		// it's just unlock textures
	HRESULT convert_to_RGB32(IDirect3DDevice9 *device, IDirect3DPixelShader9 *ps_yv12, IDirect3DPixelShader9 *ps_nv12, IDirect3DPixelShader9 *ps_yuy2, IDirect3DVertexBuffer9 *vb, int time);
	HRESULT do_stereo_test(IDirect3DDevice9 *device, IDirect3DPixelShader9 *shader_sbs, IDirect3DPixelShader9 *shader_tb, IDirect3DVertexBuffer9 *vb);
	HRESULT get_strereo_test_result(IDirect3DDevice9 *device, int *out);		// S_FALSE: unkown, S_OK: out = (input_layout_types)

	CTextureAllocator *m_allocator;
	bool m_ready;
	bool m_prepared_for_rendering;
	bool m_converted;
	CLSID m_format;
	REFERENCE_TIME m_start;
	REFERENCE_TIME m_end;
	bool m_cpu_stereo_tested;
	input_layout_types m_cpu_tested_result;

	CPooledTexture *m_tex_RGB32;					// RGB32 planes, in A8R8G8B8, full width
	CPooledTexture *m_tex_YUY2;						// YUY2 planes, in A8R8G8B8, half width
	CPooledTexture *m_tex_Y;						// Y plane of YV12/NV12, in L8
	CPooledTexture *m_tex_YV12_UV;					// UV plane of YV12, in L8, double height
	CPooledTexture *m_tex_NV12_UV;					// UV plane of NV12, in A8L8

	CPooledTexture *m_tex_gpu_RGB32;				// GPU RGB32 planes, in A8R8G8B8, full width
	CPooledTexture *m_tex_gpu_YUY2;					// GPU YUY2 planes, in A8R8G8B8, half width
	CPooledTexture *m_tex_gpu_Y;					// GPU Y plane of YV12/NV12, in L8
	CPooledTexture *m_tex_gpu_YV12_UV;				// GPU UV plane of YV12, in L8, double height
	CPooledTexture *m_tex_gpu_NV12_UV;				// GPU UV plane of NV12, in A8L8

	CPooledSurface *m_surf_YUY2;
	CPooledSurface *m_surf_YV12;
	CPooledSurface *m_surf_NV12;
	bool m_StretchRect;								// StretchRect level correct

	CPooledTexture *m_tex_stereo_test;
	CPooledTexture *m_tex_stereo_test_cpu;

	int m_width;
	int m_height;
	bool m_topdown;
	D3DPOOL m_pool;
	DWORD m_interlace_flags;
	int m_fn;
};

class my12doom_auto_shader
{
public:
	my12doom_auto_shader();
	HRESULT set_source(IDirect3DDevice9 *device, const DWORD *data, int datasize, bool is_ps, DWORD *aes_key);
	~my12doom_auto_shader();

	HRESULT invalid();
	HRESULT restore();
	operator IDirect3DPixelShader9*();
	operator IDirect3DVertexShader9*();

protected:
	DWORD *m_data;
	int m_datasize;
	DWORD *m_key;
	bool m_has_key;
	bool m_is_ps;
	CComPtr<IDirect3DPixelShader9> m_ps;
	CComPtr<IDirect3DVertexShader9> m_vs;
	IDirect3DDevice9 *m_device;
};

class my12doomRendererDShow : public DBaseVideoRenderer
{
public:
	my12doomRendererDShow(LPUNKNOWN pUnk,HRESULT *phr, my12doomRenderer *owner, int id);
	~my12doomRendererDShow();

	bool is_connected();

protected:
	friend class my12doomRenderer;
	HRESULT drop_packets(int num = -1);		//...just for invalidGPUObjects lost...
	HRESULT set_queue_size(int num){m_queue_size = min(num, my12doom_queue_size); return S_OK; }

	// dshow functions
	HRESULT CheckMediaType(const CMediaType *pmt );
	HRESULT SetMediaType(const CMediaType *pmt );
	HRESULT DoRenderSample(IMediaSample *pMediaSample);
	void OnReceiveFirstSample(IMediaSample *pMediaSample);
	HRESULT	BreakConnect();
	HRESULT CompleteConnect(IPin *pRecievePin);
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT Receive(IMediaSample *sample);
	HRESULT SuperReceive(IMediaSample *sample){return __super::Receive(sample);}
	HRESULT ShouldDrawSampleNow(IMediaSample *pMediaSample,
		__inout REFERENCE_TIME *ptrStart,
		__inout REFERENCE_TIME *ptrEnd);			// override to send Quality Message only when queue empty
	HRESULT ShouldDrawSampleNow(gpu_sample *pMediaSample);		//mine

	// dshow variables
	REFERENCE_TIME m_time;
	GUID m_format;
	GUID m_formattype;

	// queue
	static DWORD WINAPI queue_thread(LPVOID param);
	CCritSec m_queue_lock;
	dummy_packet m_queue[my12doom_queue_size];
	int m_queue_size;
	int m_queue_count;
	bool m_queue_exit;
	CComPtr<IMemAllocator> m_allocator;
	HANDLE m_queue_thread;

	// variables for contact to owner
	my12doomRenderer *m_owner;
	int m_id;
};




enum vertex_types
{
	vertex_pass1_types_count = 5,
	vertex_point_per_type = 4,

	vertex_pass1_whole = 0,
	vertex_pass1_left = 4,
	vertex_pass1_right = 8,
	vertex_pass1_top = 12,
	vertex_pass1_bottom = 16,

	vertex_pass2_main = 20,
	vertex_pass2_second = 24,
	vertex_pass3 = 28,

	vertex_bmp = 32,
	vertex_bmp2 = 36,

	vertex_ui = 40,

	vertex_test_sbs = 44,
	vertex_test_tb = 48,
	vertex_pass2_main_r = 52,


	vertex_total = 56,
};

#define stereo_test_texture_size 64

class my12doomRenderer
{
public:
	my12doomRenderer(HWND hwnd, HWND hwnd2 = NULL);
	~my12doomRenderer();
	CComPtr<IBaseFilter> m_dshow_renderer1;
	CComPtr<IBaseFilter> m_dshow_renderer2;
	AESCryptor m_AES;
	unsigned char m_key[32];
	REFERENCE_TIME m_frame_length;
	bool m_deinterlace;
	bool m_forced_deinterlace;
	bool m_remux_mode;

	// color adjust controll
	double m_saturation1;
	double m_luminance1;
	double m_hue1;
	double m_contrast1;
	double m_saturation2;
	double m_luminance2;
	double m_hue2;
	double m_contrast2;

	// public functions
	HRESULT pump();
	HRESULT repaint_video();
	HRESULT NV3D_notify(WPARAM wparam);
	HRESULT reset();
	int hittest(int x, int y, double*outv){int o = -1; if(m_uidrawer) m_uidrawer->hittest(x, y, &o, outv); return o;}


	// settings SET function
	HRESULT set_input_layout(int layout);
	HRESULT set_output_mode(int mode);
	HRESULT set_mask_mode(int mode);
	HRESULT set_mask_color(int id, DWORD color);
	HRESULT set_swap_eyes(bool swap);
	HRESULT set_fullscreen(bool full);
	HRESULT set_movie_pos(int dimention, double offset);		// dimention1 = x, dimention2 = y
	HRESULT set_aspect(double aspect);
	HRESULT set_window(HWND wnd, HWND wnd2);
	HRESULT set_bmp(void* data, int width, int height, float fwidth, float fheight, float fleft, float ftop);
	HRESULT set_bmp_offset(double offset);
	HRESULT set_parallax(double parallax);
	HRESULT set_ui_visible(bool visible);
	HRESULT set_callback(Imy12doomRendererCallback *cb){m_cb = cb; return S_OK;}
	HRESULT set_2dto3d(bool convert){m_convert3d = convert;}
	HRESULT set_aspect_mode(int mode);


	// settings GET function
	DWORD get_mask_color(int id);
	bool get_swap_eyes();
	input_layout_types get_input_layout();
	output_mode_types get_output_mode();
	mask_mode_types get_mask_mode();
	bool get_fullscreen();
	double get_offset(int dimention);
	double get_aspect();
	bool is_connected(int id){return (id?m_dsr1:m_dsr0)->is_connected();}
	double get_bmp_offset(){return m_bmp_offset;}
	double get_parallax(){return m_parallax;}
	bool get_2dto3d(){return m_convert3d;}
	aspect_mode_types get_aspect_mode(){return m_aspect_mode;}

protected:

	double m_parallax;
	bool m_showui;
	bool m_has_subtitle;
	int m_ui_visible_last_change_time;
	int m_last_ui_draw;
	int m_bmp_width, m_bmp_height;
	float m_bmp_fleft, m_bmp_ftop, m_bmp_fwidth, m_bmp_fheight;
	double m_bmp_offset_x /*= -0.0*/;
	double m_bmp_offset_y /*= 0.0*/;
	double m_bmp_offset;
	double m_source_aspect /*= (double)m_lVidWidth / m_lVidHeight*/;
	double m_forced_aspect /* = -1 */;
	int m_pass1_width;
	int m_pass1_height;
	Imy12doomRendererCallback *m_cb;
	bool m_revert_RGB32;
	REFERENCE_TIME m_last_frame_time;

protected:
	friend class my12doomRendererDShow;

	// dshow related things
	bool m_recreating_dshow_renderer;
	my12doomRendererDShow * m_dsr0;
	my12doomRendererDShow * m_dsr1;
	HRESULT CheckMediaType(const CMediaType *pmt, int id);
	HRESULT SetMediaType(const CMediaType *pmt, int id);
	HRESULT	BreakConnect(int id);
	HRESULT CompleteConnect(IPin *pRecievePin, int id);
	HRESULT DataPreroll(int id, IMediaSample *media_sample);
	HRESULT DoRender(int id, IMediaSample *media_sample);
	LONG m_lVidWidth;   // Video width
	LONG m_lVidHeight;  // Video Height
	CGenericList<gpu_sample> m_left_queue;
	CGenericList<gpu_sample> m_right_queue;
	CCritSec m_queue_lock;
	gpu_sample * m_sample2render_1;
	gpu_sample * m_sample2render_2;
	gpu_sample * m_last_rendered_sample1;
	gpu_sample * m_last_rendered_sample2;
	CCritSec m_packet_lock;
	CCritSec m_rendered_packet_lock;
	CCritSec m_pool_lock;

	// dx9 functions and variables
	enum device_state
	{
		fine,							// device is fine
		need_resize_back_buffer,		// just resize back buffer and recaculate vertex
		need_reset_object,				// objects size changed, should recreate objects
		need_reset,						// reset requested by program, usually to change back buffer size, but program can continue rendering without reset
		device_lost,					// device lost, can't continue
		need_create,					// device not created, or need to recreate, can't continue
		create_failed,					// 
		device_state_max,				// not used
	} m_device_state;					// need_create

	HRESULT create_render_targets();
	HRESULT delete_render_targets();
	HRESULT fix_nv3d_bug();				// do this after every CreateRenderTarget
	HRESULT handle_device_state();							//handle device create/recreate/lost/reset
	HRESULT create_render_thread();
	HRESULT terminate_render_thread();
	HRESULT invalidate_gpu_objects();
	HRESULT invalidate_cpu_objects();
	HRESULT restore_gpu_objects();
	HRESULT restore_cpu_objects();
	HRESULT render_nolock(bool forced = false);
	HRESULT draw_movie(IDirect3DSurface9 *surface, bool left_eye);
	HRESULT draw_bmp(IDirect3DSurface9 *surface, bool left_eye);
	HRESULT draw_ui(IDirect3DSurface9 *surface);
	HRESULT adjust_temp_color(IDirect3DSurface9 *surface_to_adjust, bool left);
#ifdef DEBUG
	HRESULT clear(IDirect3DSurface9 *surface, DWORD color = D3DCOLOR_XRGB(255,128,0));
#else
	HRESULT clear(IDirect3DSurface9 *surface, DWORD color = D3DCOLOR_XRGB(0,0,0));
#endif

	HRESULT render(bool forced = false);
	static DWORD WINAPI render_thread(LPVOID param);
	static DWORD WINAPI test_thread(LPVOID param);

	// dx9 helper functions
	HRESULT load_image(int id = -1, bool forced = false);		// -1 = both dshow renderer
	HRESULT load_image_convert(gpu_sample * sample1, gpu_sample *sample2);
	HRESULT calculate_vertex();
	HRESULT generate_mask();
	HRESULT set_device_state(device_state new_state);
	HRESULT backup_rgb();
	HRESULT restore_rgb();
	HRESULT test_PC_level();		// test hardware YUV-RGB conversion level
	DWORD m_PC_level;				// 0

	friend class dx_player;
	bool m_backuped;
	int m_last_reset_time;

	// AMD HD3D functions and variables
	CComPtr <IDirect3D9> m_HD3DCommSurface;
	int m_HD3DStereoModesCount;
	int m_AMDlineoffset;
	D3DDISPLAYMODE m_HD3DStereoModes[100];
	HRESULT AMD_one_time_init();
	HRESULT AMD_restore_objects();
	HRESULT AMD_invalidate_objects();
	HRESULT AMDSetStereoPresentParameters();
	HRESULT AMDDrawStereo(IDirect3DSurface9 *left_surface, IDirect3DSurface9 *right_surface);
	HRESULT AMDSendStereoCommand(ATIDX9STEREOCOMMAND stereoCommand, BYTE *pOutBuffer, 
								DWORD dwOutBufferSize, BYTE *pInBuffer, 
								DWORD dwInBufferSize);

	// variables
	void init_variables();
	bool m_nv3d_enabled;			// false if ATI card
	bool m_nv3d_actived;
	bool m_nv3d_windowed;			// false if driver does not support windowed 3d vision
	NvDisplayHandle m_nv3d_display;
	DWORD m_nv_pageflip_counter;
	int m_pageflip_frames;

	MyVertex m_vertices[vertex_total];
	MyVertex_subtitle m_vertices_subtitle[vertex_total];
	int m_pageflipping_start;
	bool m_swapeyes;
	output_mode_types m_output_mode;
	input_layout_types m_input_layout;
	bool m_convert3d;			// = false
	aspect_mode_types m_aspect_mode;
	mask_mode_types m_mask_mode;
	HWND m_hWnd;
	HWND m_hWnd2;
	CComPtr<IDirect3D9>		m_D3D;
	CComPtr<IDirect3D9Ex>	m_D3DEx;
	CComPtr<IDirect3DDevice9> m_Device;
	CComPtr<IDirect3DDevice9Ex> m_DeviceEx;
	CComPtr<IDirect3DSwapChain9> m_swap1;
	CComPtr<IDirect3DSwapChain9> m_swap2;
	//CComPtr<IDirect3DQuery9> m_d3d_query;
	D3DPRESENT_PARAMETERS   m_new_pp;
	D3DPRESENT_PARAMETERS   m_active_pp;
	D3DPRESENT_PARAMETERS   m_active_pp2;
	D3DDISPLAYMODE m_d3ddm;
	HANDLE m_device_not_reseting;
	CCritSec m_frame_lock;
	CCritSec m_device_lock;
	HANDLE m_render_event;
	int m_device_threadid;
	CTextureAllocator *m_pool;

	CComPtr<IDirect3DVertexBuffer9> g_VertexBuffer;
	CComPtr<IDirect3DVertexBuffer9> m_vertex_subtitle;
	bool m_vertex_changed;
	CComPtr <IDirect3DVertexShader9> m_vs_subtitle;
	CComPtr <IDirect3DPixelShader9> m_ps_yv12;
	CComPtr <IDirect3DPixelShader9> m_ps_nv12;
	CComPtr <IDirect3DPixelShader9> m_ps_yuy2;
	CComPtr <IDirect3DPixelShader9> m_ps_anaglyph;
	CComPtr <IDirect3DPixelShader9> m_ps_iz3d_back;
	CComPtr <IDirect3DPixelShader9> m_ps_iz3d_front;
	CComPtr <IDirect3DPixelShader9> m_ps_test;
	CComPtr <IDirect3DPixelShader9> m_ps_test_tb;
	CComPtr <IDirect3DPixelShader9> m_ps_test_tb2;
	CComPtr <IDirect3DPixelShader9> m_ps_test_sbs;
	CComPtr <IDirect3DPixelShader9> m_ps_test_sbs2;
	CComPtr <IDirect3DPixelShader9> m_ps_color_adjust;
	CComPtr <IDirect3DPixelShader9> m_ps_bmp_lanczos;
	CComPtr <IDirect3DPixelShader9> m_ps_bmp_blur;

	my12doom_auto_shader m_red_blue;

	/*
	CComPtr<IDirect3DTexture9> m1_tex_RGB32;						// RGB32 planes, in A8R8G8B8, full width
	CComPtr<IDirect3DTexture9> m1_tex_YUY2;						// YUY2 planes, in A8R8G8B8, half width
	CComPtr<IDirect3DTexture9> m1_tex_Y;							// Y plane of YV12/NV12, in L8
	CComPtr<IDirect3DTexture9> m1_tex_YV12_UV;					// UV plane of YV12, in L8, double height
	CComPtr<IDirect3DTexture9> m1_tex_NV12_UV;					// UV plane of NV12, in A8L8

	CComPtr<IDirect3DTexture9> m2_tex_RGB32;						// RGB32 planes, in A8R8G8B8, full width
	CComPtr<IDirect3DTexture9> m2_tex_YUY2;						// YUY2 planes, in A8R8G8B8, half width
	CComPtr<IDirect3DTexture9> m2_tex_Y;							// Y plane of YV12/NV12, in L8
	CComPtr<IDirect3DTexture9> m2_tex_YV12_UV;					// UV plane of YV12, in L8, double height
	CComPtr<IDirect3DTexture9> m2_tex_NV12_UV;					// UV plane of NV12, in A8L8
	*/

	// TV - PC level test surfaces
	CComPtr<IDirect3DSurface9> m_PC_level_test;

	CComPtr<IDirect3DTexture9> m_tex_rgb_left;				// source texture, converted to RGB32
	CComPtr<IDirect3DTexture9> m_tex_rgb_right;
	CComPtr<IDirect3DSurface9> m_deinterlace_surface;		// surface for deinterlace
	CComPtr<IDirect3DSurface9> m_surface_rgb_backup_full;	// back up converted RGB32 texture when stop, to restore after device lost or reset
	CComPtr<IDirect3DTexture9> m_tex_rgb_full;
	RECT m_window_rect;
	CComPtr<IDirect3DTexture9> m_tex_mask;					// mask txture
	CComPtr<IDirect3DTexture9> m_mask_temp_left;			// two temp texture, you may need it in some case
	CComPtr<IDirect3DTexture9> m_mask_temp_right;

	CComPtr<IDirect3DTexture9> m_tex_bmp;
	CComPtr<IDirect3DSurface9> m_tex_bmp_mem;
	D3DLOCKED_RECT m_bmp_locked_rect;
	bool m_bmp_changed;
	CCritSec m_bmp_lock;

	CComPtr<IDirect3DSurface9> m_nv3d_surface;				// nv3d temp surface


	CComPtr<IDirect3DSurface9> m_just_a_test_surface;

	// input layout detector
	input_layout_types get_active_input_layout();
	double get_active_aspect();
	CComPtr<IDirect3DSurface9> m_stereo_test_gpu;
	CComPtr<IDirect3DSurface9> m_stereo_test_cpu;
	int m_sbs;
	int m_normal;
	int m_tb;
	input_layout_types m_layout_detected;
	bool m_no_more_detect;

	// render thread variables
	CCritSec m_thread_lock;
	HANDLE m_render_thread;
	DWORD m_render_thread_id;
	bool m_render_thread_exit;
	DWORD m_color1;
	DWORD m_color2;
	LONG m_style, m_exstyle;
	RECT m_window_pos;


	// test draw ui
	ui_drawer_base *m_uidrawer;

	REFERENCE_TIME m_total_time;

public:
	float m_volume;
};

/*
main gpu:

m_mask_temp_left
m_mask_temp_right
m_tex_rgb_full
m_tex_rgb_left
m_tex_rgb_right
m_tex_bmp
m1_tex_YUY2
m2_tex_YUY2
m1_tex_RGB32
m2_tex_RGB32
m1_tex_Y
m1_tex_NV12_UV
m2_tex_Y
m2_tex_NV12_UV
m1_tex_Y
m1_tex_YV12_UV
m2_tex_Y
m2_tex_YV12_UV

g_VertexBuffer
m_vertex_subtitle


main cpu:
m_tex_mask
m_tex_bmp_mem
m_mem
m_surface_rgb_backup_full

*/