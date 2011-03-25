#include "srt_renderer.h"

CsrtRenderer::CsrtRenderer()
{
	m_font = NULL;
	m_font_color = RGB(255,255,255);
	m_lFontPointSize = 60;
	wcscpy(m_FontName, L"����");
	wcscpy(m_FontStyle, L"Regular");
	select_font(false);
	reset();
}
HRESULT CsrtRenderer::select_font(bool show_dlg)
{
	if (!show_dlg && m_font)
		return S_OK;

	CHOOSEFONTW cf={0};
	memset(&cf, 0, sizeof(cf));
	LOGFONTW lf={0}; 
	HDC hdc;
	LONG lHeight;

	// Convert requested font point size to logical units
	hdc = GetDC( NULL );
	lHeight = -MulDiv( m_lFontPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
	ReleaseDC( NULL, hdc );

	// Initialize members of the LOGFONT structure. 
	lstrcpynW(lf.lfFaceName, m_FontName, 32);
	lf.lfHeight = lHeight;      // Logical units
	lf.lfQuality = ANTIALIASED_QUALITY;

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
	m_lFontPointSize = cf.iPointSize / 10;  // Specified in 1/10 point units
	m_font_color = cf.rgbColors;
	m_font = CreateFontIndirectW(cf.lpLogFont); 

	return S_OK;
}
HRESULT CsrtRenderer::load_file(wchar_t *filename)
{
	return m_srt.load(filename);
}

HRESULT CsrtRenderer::reset()
{
	m_srt.init(20480, 2048*1024);
	m_last_found[0] = NULL;
	return S_OK;
}

HRESULT CsrtRenderer::seek()
{
	m_last_found[0] = NULL;
	return S_OK;
}

HRESULT CsrtRenderer::add_data(BYTE *data, int size, int start, int end)
{
	wchar_t *tmp = (wchar_t*)malloc(sizeof(wchar_t)*size);
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)data, size, tmp, size);
	return m_srt.direct_add_subtitle(tmp, start, end);
}

HRESULT CsrtRenderer::get_subtitle(int time, rendered_subtitle *out, int last_time/* =-1 */)
{
	if (NULL == out)
		return E_POINTER;

	wchar_t found[1024];
	m_srt.get_subtitle(time, time, found);
	if ((wcscmp(found, m_last_found) == 0) && (last_time != -1))
	{
		out->data = NULL;
		return S_FALSE;
	}

	else
	{
		wcscpy(m_last_found, found);
		// rendering
		if (found[0] == NULL)
		{
			out->width_pixel = out->height_pixel = 0;
			out->width = out->height = 0;
			out->data = NULL;
			return S_OK;
		}

		HDC hdc = GetDC(NULL);
		HDC hdcBmp = CreateCompatibleDC(hdc);

		HFONT hOldFont = (HFONT) SelectObject(hdcBmp, m_font);

		RECT rect = {0,0,0,0};
		DrawTextW(hdcBmp, found, (int)wcslen(found), &rect, DT_CENTER | DT_CALCRECT);
		out->pixel_type = out->pixel_type_RGB;
		out->height_pixel = rect.bottom - rect.top;
		out->width_pixel  = rect.right - rect.left;
		out->width = (double)out->width_pixel/1920;
		out->height = (double)out->height_pixel/1080;
		out->aspect = (double)16/9;
		out->delta = 0;

		out->left = 0.5 - out->width/2;
		out->top = 0.95 - out->height;

		HBITMAP hbm = CreateCompatibleBitmap(hdc, out->width_pixel, out->height_pixel);

		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

		RECT rcText;
		SetRect(&rcText, 0, 0, out->width_pixel, out->height_pixel);
		SetBkColor(hdcBmp, RGB(0, 0, 0));					// Pure black background
		SetTextColor(hdcBmp, RGB(255, 255, 255));			// white text for alpha

		DrawTextW(hdcBmp, found, (int)wcslen(found), &rect, DT_CENTER);

		out->data = (BYTE *) malloc(out->width_pixel * out->height_pixel * 4);
		GetBitmapBits(hbm, out->width_pixel * out->height_pixel * 4, out->data);


		BYTE *data = (BYTE*)out->data;
		DWORD *data_dw = (DWORD*)out->data;
		unsigned char color_r = (BYTE)(m_font_color       & 0xff);
		unsigned char color_g = (BYTE)((m_font_color>>8)  & 0xff);
		unsigned char color_b = (BYTE)((m_font_color>>16) & 0xff);
		DWORD color = (color_r<<16) | (color_g <<8) | (color_b);// reverse big endian

		for(int i=0; i<out->width_pixel * out->height_pixel; i++)
		{
			unsigned char alpha = data[i*4+2];
			data_dw[i] = color;
			data[i*4+3] = alpha;
		}

		DeleteObject(SelectObject(hdcBmp, hbmOld));
		SelectObject(hdc, hOldFont);
		DeleteObject(hbm);
		DeleteDC(hdcBmp);
		ReleaseDC(NULL, hdc);

		return S_OK;
	}
}