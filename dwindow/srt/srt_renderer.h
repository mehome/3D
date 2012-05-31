#pragma once
#include "..\CSubtitle.h"
#include "srt_parser.h"

class CsrtRenderer : public CSubtitleRenderer
{
public:
	CsrtRenderer(HFONT font, DWORD fontcolor);
	~CsrtRenderer();
	virtual HRESULT load_file(wchar_t *filename);										//maybe you don't need this?
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);	// get subtitle on a time point, 
																						// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek();																// just clear current incompleted data, to support dshow seeking.

	virtual HRESULT set_font_color(DWORD newcolor);										// color format is 8bit BGRA, use RGB() macro, for text based subtitle only
	virtual HRESULT set_font(HFONT newfont);											// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.
	virtual HRESULT set_output_aspect(double aspect);									// set the rendered output aspect, to suit movies's aspect
protected:
	srt_parser m_srt;
	wchar_t m_last_found[1024];

	HRESULT render(const wchar_t *text, rendered_subtitle *out, bool has_offset, int offset);

	// font
	double m_aspect;		// default = 16/9
	HFONT m_font;
	DWORD m_font_color;
};

class CAssRenderer : public CsrtRenderer
{
public:
	CAssRenderer(HFONT font, DWORD fontcolor): CsrtRenderer(font, fontcolor){}
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);					// override this function to provide dshow support, which remove leading tags
};