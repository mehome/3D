class gpu_sample
{
public:
	gpu_sample(IMediaSample *memory_sample, CTextureAllocator *allocator, int width, int height, CLSID format, bool topdown_RGB32, bool do_cpu_test = false, bool remux_mode = false, D3DPOOL pool = D3DPOOL_SYSTEMMEM, DWORD PC_LEVEL = 0);
	~gpu_sample();
	HRESULT commit();		// it's just unlock textures
	HRESULT decommit();
	HRESULT convert_to_RGB32(IDirect3DDevice9 *device, IDirect3DPixelShader9 *ps_yv12, IDirect3DPixelShader9 *ps_nv12, IDirect3DPixelShader9 *ps_yuy2, IDirect3DVertexBuffer9 *vb, int time);
	HRESULT do_stereo_test(IDirect3DDevice9 *device, IDirect3DPixelShader9 *shader_sbs, IDirect3DPixelShader9 *shader_tb, IDirect3DVertexBuffer9 *vb);
	HRESULT get_strereo_test_result(IDirect3DDevice9 *device, int *out);		// S_FALSE: unkown, S_OK: out = (input_layout_types)

	bool m_ready;
	int m_width;
	int m_height;
	REFERENCE_TIME m_start;
	REFERENCE_TIME m_end;
	int m_fn;
	CLSID m_format;
	bool m_topdown;

	CPooledTexture *m_tex_gpu_RGB32;				// GPU RGB32 planes, in A8R8G8B8, full width
	CPooledTexture *m_tex_gpu_Y;					// GPU Y plane of YV12/NV12, in L8
	CPooledTexture *m_tex_gpu_YV12_UV;				// GPU UV plane of YV12, in L8, double height
	CPooledTexture *m_tex_gpu_NV12_UV;				// GPU UV plane of NV12, in A8L8
	CPooledTexture *m_tex_gpu_YUY2_UV;					// GPU YUY2 planes, in A8R8G8B8, half width
protected:
	bool is_ignored_line(int line);
	CTextureAllocator *m_allocator;
	bool m_prepared_for_rendering;
	bool m_converted;
	bool m_cpu_stereo_tested;
	input_layout_types m_cpu_tested_result;

	CPooledTexture *m_tex_RGB32;					// RGB32 planes, in A8R8G8B8, full width
	CPooledTexture *m_tex_Y;						// Y plane of YV12/NV12, in L8
	CPooledTexture *m_tex_YV12_UV;					// UV plane of YV12, in L8, double height
	CPooledTexture *m_tex_NV12_UV;					// UV plane of NV12, in A8L8
	CPooledTexture *m_tex_YUY2_UV;						// YUY2 planes, in A8R8G8B8, half width


	CPooledSurface *m_surf_YUY2;
	CPooledSurface *m_surf_YV12;
	CPooledSurface *m_surf_NV12;

	CPooledSurface *m_surf_gpu_YUY2;
	CPooledSurface *m_surf_gpu_YV12;
	CPooledSurface *m_surf_gpu_NV12;

	bool m_StretchRect;								// StretchRect level correct

	CPooledTexture *m_tex_stereo_test;
	CPooledTexture *m_tex_stereo_test_cpu;

	D3DPOOL m_pool;
	DWORD m_interlace_flags;
	CCritSec m_sample_lock;

};