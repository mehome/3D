/* 
 * (C) 2003-2006 Gabest
 * (C) 2006-2010 see AUTHORS
 *  http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <tchar.h>
#include <mmreg.h>
#include <initguid.h>
#include <dmodshow.h>
#include "MpegSplitter.h"
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"


#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
//	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD}, // cdxa filter should take care of this
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PROGRAM},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_TRANSPORT},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PVA},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    {L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesIn), sudPinTypesIn},
    {L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL},
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	//{&__uuidof(CMpegSplitterFilter), L"MPC - Mpeg Splitter (Gabest)", MERIT_NORMAL+1, countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMpegSourceFilter), L"my12doom's SSIF Source", MERIT_UNLIKELY, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMpegSourceFilter>, NULL, &sudFilter[0]},
	//{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMpegSplitterFilter>, NULL, &sudFilter[0]},
	//{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMpegSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	/*
	DeleteRegKey(_T("Media Type\\Extensions\\"), _T(".ts"));

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG1System, _T("0,16,FFFFFFFFF100010001800001FFFFFFFF,000001BA2100010001800001000001BB"), NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_PROGRAM, _T("0,5,FFFFFFFFC0,000001BA40"), NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_PVA, _T("0,8,fffffc00ffe00000,4156000055000000"), NULL);

	CAtlList<CString> chkbytes;
	chkbytes.AddTail(_T("0,1,,47,188,1,,47,376,1,,47"));
	chkbytes.AddTail(_T("4,1,,47,196,1,,47,388,1,,47"));
	chkbytes.AddTail(_T("0,4,,54467263,1660,1,,47")); // TFrc
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_TRANSPORT, chkbytes, NULL);	
	*/

	DeleteRegKey(_T("Media Type\\Extensions\\"), _T(".ssif"));
	DeleteRegKey(_T("Media Type\\Extensions\\"), _T(".mpls"));

	SetRegKeyValue(_T("Media Type\\Extensions\\"), _T(".ssif"), _T("Source Filter"), CStringFromGUID(__uuidof(CMpegSourceFilter)));
	SetRegKeyValue(_T("Media Type\\Extensions\\"), _T(".mpls"), _T("Source Filter"), CStringFromGUID(__uuidof(CMpegSourceFilter)));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
//	UnRegisterSourceFilter(MEDIASUBTYPE_MPEG1System);
//	UnRegisterSourceFilter(MEDIASUBTYPE_MPEG2_PROGRAM);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../FilterApp.h"

CFilterApp theApp;

#endif

template <typename t_CType>
t_CType GetFormatHelper(t_CType &_pInfo, const CMediaType *_pFormat)
{
	ASSERT(_pFormat->cbFormat >= sizeof(*_pInfo));
	_pInfo = (t_CType)_pFormat->pbFormat;
	return _pInfo;
}

static int GetHighestBitSet32(unsigned long _Value)
{
	unsigned long Ret;
	unsigned char bNonZero = _BitScanReverse(&Ret, _Value);
	if (bNonZero)
		return Ret;
	else
		return -1;
}

CString FormatBitrate(double _Bitrate)
{
	CString Temp;
	if (_Bitrate > 20000000) // More than 2 mbit
		Temp.Format(L"%.2f mbit/s", double(_Bitrate)/1000000.0);
	else
		Temp.Format(L"%.1f kbit/s", double(_Bitrate)/1000.0);

	return Temp;
}

CString FormatString(const wchar_t *pszFormat, ... )
{
	CString Temp;
	ATLASSERT( AtlIsValidString( pszFormat ) );

	va_list argList;
	va_start( argList, pszFormat );
	Temp.FormatV( pszFormat, argList );
	va_end( argList );

	return Temp;
}

CString GetMediaTypeDesc(const CMediaType *_pMediaType, const CHdmvClipInfo::Stream *pClipInfo, int _PresentationType)
{
	const WCHAR *pPresentationDesc = NULL;

	if (pClipInfo)
		pPresentationDesc = StreamTypeToName(pClipInfo->m_Type);
	else
		pPresentationDesc = StreamTypeToName((PES_STREAM_TYPE)_PresentationType);

	CString MajorType;
	CAtlList<CString> Infos;

	if (_pMediaType->majortype == MEDIATYPE_Video)
	{
		MajorType = "Video";

		if (pClipInfo)
		{
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);

			if (!name.IsEmpty())
				Infos.AddTail(name);
		}

		const VIDEOINFOHEADER *pVideoInfo = NULL;
		const VIDEOINFOHEADER2 *pVideoInfo2 = NULL;

		if (_pMediaType->formattype == FORMAT_MPEGVideo)
		{
			Infos.AddTail(L"MPEG");

			const MPEG1VIDEOINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo = &pInfo->hdr;

		}
		else if (_pMediaType->formattype == FORMAT_MPEG2_VIDEO)
		{
			const MPEG2VIDEOINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo2 = &pInfo->hdr;

			bool bIsAVC = false;

			if (pInfo->hdr.bmiHeader.biCompression == '1CVA')
			{
				bIsAVC = true;
				Infos.AddTail(L"AVC (H.264)");
			}
			else if (pInfo->hdr.bmiHeader.biCompression == 0)
				Infos.AddTail(L"MPEG2");
			else
			{
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (pInfo->hdr.bmiHeader.biCompression >> 0) & 0xFF;
				Temp[1] = (pInfo->hdr.bmiHeader.biCompression >> 8) & 0xFF;
				Temp[2] = (pInfo->hdr.bmiHeader.biCompression >> 16) & 0xFF;
				Temp[3] = (pInfo->hdr.bmiHeader.biCompression >> 24) & 0xFF;
				Infos.AddTail(Temp);
			}

			switch (pInfo->dwProfile)
			{
			case AM_MPEG2Profile_Simple:			Infos.AddTail(L"Simple Profile"); break;
			case AM_MPEG2Profile_Main:				Infos.AddTail(L"Main Profile"); break;
			case AM_MPEG2Profile_SNRScalable:		Infos.AddTail(L"SNR Scalable Profile"); break;
			case AM_MPEG2Profile_SpatiallyScalable:	Infos.AddTail(L"Spatially Scalable Profile"); break;
			case AM_MPEG2Profile_High:				Infos.AddTail(L"High Profile"); break;
			default:
				if (pInfo->dwProfile)
				{
					if (bIsAVC)
					{
						switch (pInfo->dwProfile)
						{
							case 44:	Infos.AddTail(L"CAVLC Profile"); break;
							case 66:	Infos.AddTail(L"Baseline Profile"); break;
							case 77:	Infos.AddTail(L"Main Profile"); break;
							case 88:	Infos.AddTail(L"Extended Profile"); break;
							case 100:	Infos.AddTail(L"High Profile"); break;
							case 110:	Infos.AddTail(L"High 10 Profile"); break;
							case 122:	Infos.AddTail(L"High 4:2:2 Profile"); break;
							case 244:	Infos.AddTail(L"High 4:4:4 Profile"); break;

							default:	Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile)); break;
						}
					}
					else
						Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
				}
				break;
			}

			switch (pInfo->dwLevel)
			{
			case AM_MPEG2Level_Low:			Infos.AddTail(L"Low Level"); break;
			case AM_MPEG2Level_Main:		Infos.AddTail(L"Main Level"); break;
			case AM_MPEG2Level_High1440:	Infos.AddTail(L"High1440 Level"); break;
			case AM_MPEG2Level_High:		Infos.AddTail(L"High Level"); break;
			default:
				if (pInfo->dwLevel)
				{
					if (bIsAVC)
						Infos.AddTail(FormatString(L"Level %1.1f", double(pInfo->dwLevel)/10.0));
					else
						Infos.AddTail(FormatString(L"Level %d", pInfo->dwLevel));
				}
				break;
			}
		}
		else if (_pMediaType->formattype == FORMAT_VIDEOINFO2)
		{
			const VIDEOINFOHEADER2 *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo2 = pInfo;
			bool bIsVC1 = false;

			DWORD CodecType = pInfo->bmiHeader.biCompression;
			if (CodecType == '1CVW')
			{
				bIsVC1 = true;
				Infos.AddTail(L"VC-1");
			}
			else if (CodecType)
			{
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (CodecType >> 0) & 0xFF;
				Temp[1] = (CodecType >> 8) & 0xFF;
				Temp[2] = (CodecType >> 16) & 0xFF;
				Temp[3] = (CodecType >> 24) & 0xFF;
				Infos.AddTail(Temp);
			}
		}
		else if (_pMediaType->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
		{
			Infos.AddTail(L"DVD Sub Picture");
		}
		else if (_pMediaType->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE)
		{
			Infos.AddTail(L"SVCD Sub Picture");
		}
		else if (_pMediaType->subtype == MEDIASUBTYPE_CVD_SUBPICTURE)
		{
			Infos.AddTail(L"CVD Sub Picture");
		}

		if (pVideoInfo2)
		{
			if (pVideoInfo2->bmiHeader.biWidth && pVideoInfo2->bmiHeader.biHeight)
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo2->bmiHeader.biWidth, pVideoInfo2->bmiHeader.biHeight));
			if (pVideoInfo2->AvgTimePerFrame)
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo2->AvgTimePerFrame)));
			if (pVideoInfo2->dwBitRate)
				Infos.AddTail(FormatBitrate(pVideoInfo2->dwBitRate));
		}
		else if (pVideoInfo)
		{
			if (pVideoInfo->bmiHeader.biWidth && pVideoInfo->bmiHeader.biHeight)
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight));
			if (pVideoInfo->AvgTimePerFrame)
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo->AvgTimePerFrame)));
			if (pVideoInfo->dwBitRate)
				Infos.AddTail(FormatBitrate(pVideoInfo->dwBitRate));
		}
		
	}
	else if (_pMediaType->majortype == MEDIATYPE_Audio)
	{
		MajorType = "Audio";
		if (pClipInfo)
		{
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty())
				Infos.AddTail(name);
		}
		if (_pMediaType->formattype == FORMAT_WaveFormatEx)
		{
			const WAVEFORMATEX *pInfo = GetFormatHelper(pInfo, _pMediaType);

			if (_pMediaType->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
			{
				Infos.AddTail(L"DVD LPCM");
			}
			else if (_pMediaType->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO)
			{
				const WAVEFORMATEX_HDMV_LPCM *pInfoHDMV = GetFormatHelper(pInfoHDMV, _pMediaType);
				UNUSED_ALWAYS(pInfoHDMV);
				Infos.AddTail(L"HDMV LPCM");
			}
			if (_pMediaType->subtype == MEDIASUBTYPE_DOLBY_DDPLUS)
			{
				Infos.AddTail(L"Dolby Digital Plus");
			}
			else
			{
				switch (pInfo->wFormatTag)
				{
				case WAVE_FORMAT_PS2_PCM:
					{
						Infos.AddTail(L"PS2 PCM");
					}
					break;
				case WAVE_FORMAT_PS2_ADPCM:
					{
						Infos.AddTail(L"PS2 ADPCM");
					}
					break;
				case WAVE_FORMAT_DVD_DTS:
					{
						if (pPresentationDesc)
							Infos.AddTail(pPresentationDesc);
						else
							Infos.AddTail(L"DTS");
					}
					break;
				case WAVE_FORMAT_DOLBY_AC3:
					{
						if (pPresentationDesc)
							Infos.AddTail(pPresentationDesc);
						else
							Infos.AddTail(L"Dolby Digital");
					}
					break;
				case WAVE_FORMAT_AAC:
					{
						Infos.AddTail(L"AAC");
					}
					break;
				case WAVE_FORMAT_MP3:
					{
						Infos.AddTail(L"MP3");
					}
					break;
				case WAVE_FORMAT_MPEG:
					{
						const MPEG1WAVEFORMAT* pInfoMPEG1 = GetFormatHelper(pInfoMPEG1, _pMediaType);

						int layer = GetHighestBitSet32(pInfoMPEG1->fwHeadLayer) + 1;
						Infos.AddTail(FormatString(L"MPEG1 - Layer %d", layer));
					}
					break;
				}
			}

			if (pClipInfo && (pClipInfo->m_SampleRate == BDVM_SampleRate_48_192 || pClipInfo->m_SampleRate == BDVM_SampleRate_48_96))
			{
				switch (pClipInfo->m_SampleRate)
				{
				case BDVM_SampleRate_48_192:
					Infos.AddTail(FormatString(L"192(48) kHz"));
					break;
				case BDVM_SampleRate_48_96:
					Infos.AddTail(FormatString(L"96(48) kHz"));
					break;
				}
			}
			else if (pInfo->nSamplesPerSec)
				Infos.AddTail(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec)/1000.0));
			if (pInfo->nChannels)
				Infos.AddTail(FormatString(L"%d chn", pInfo->nChannels));
			if (pInfo->wBitsPerSample)
				Infos.AddTail(FormatString(L"%d bit", pInfo->wBitsPerSample));
			if (pInfo->nAvgBytesPerSec)
				Infos.AddTail(FormatBitrate(pInfo->nAvgBytesPerSec * 8));

		}
	}
	else if (_pMediaType->majortype == MEDIATYPE_Subtitle)
	{
		MajorType = "Subtitle";

		if (pPresentationDesc)
			Infos.AddTail(pPresentationDesc);

		if (_pMediaType->cbFormat == sizeof(SUBTITLEINFO))
		{
			const SUBTITLEINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);
			CString name = ISO6392ToLanguage(pInfo->IsoLang);

			if (pInfo->TrackName[0])
				Infos.AddHead(pInfo->TrackName);
			if (!name.IsEmpty())
				Infos.AddHead(name);
		}
		else
		{
			if (pClipInfo)
			{
				CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
				if (!name.IsEmpty())
					Infos.AddHead(name);
			}
		}
	}

	if (!Infos.IsEmpty())
	{
		CString Ret;

		Ret += MajorType;
		Ret += " - ";

		bool bFirst = true;

		for(POSITION pos = Infos.GetHeadPosition(); pos; Infos.GetNext(pos))
		{
			CString& String = Infos.GetAt(pos);

			if (bFirst)
				Ret += String;
			else
				Ret += L", " + String;

			bFirst = false;
		}

		return Ret;
	}
	return CString();
}

//
// CMpegSplitterFilter
//

CMpegSplitterFilter::CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseSplitterFilter(NAME("CMpegSplitterFilter"), pUnk, phr, clsid)
	, m_pPipoBimbo(false)
	, m_rtPlaylistDuration(0)
	, m_fHasAccessUnitDelimiters(false)
	, m_rtMaxShift(50000000)
	, m_mvc_found(false)
	, m_dummy_exit(false)
	, m_for_encoding(false)
{
	TCHAR exe[MAX_PATH];
	GetModuleFileName (NULL, exe, MAX_PATH);
	for(int i=_tcslen(exe)-1; i>0; i--)
		if (exe[i] == _T('\\'))
		{
			_tcscpy(exe, exe+i+1);
			break;
		}

	for(int i=0; i<_tcslen(exe); i++)
		exe[i] = tolower(exe[i]);

	if (_tcscmp(exe, _T("x264.exe")) == 0 || 
		_tcscmp(exe, _T("avsutil.exe")) == 0 || 
		_tcscmp(exe, _T("avsdaemon32.exe")) == 0 ||
		_tcscmp(exe, _T("virtualdub.exe")) == 0) 
	{
		_tprintf(_T("SsifSource: %s detected, right eye queue disabled.\n"), exe);
		m_for_encoding = true;
	}
	else
	{
		_tprintf(_T("SsifSource: %s detected, right eye queue enabled.\n"), exe);
	}


	m_dummy_thread = CreateThread(0,0,dummy_thread, this, NULL, NULL);
}

CMpegSplitterFilter::~CMpegSplitterFilter()
{
	m_dummy_exit = true;
	ResumeThread(m_dummy_thread);
	if (WAIT_TIMEOUT == WaitForSingleObject(m_dummy_thread, 200))
	{
		ResumeThread(m_dummy_thread);
		if (WAIT_TIMEOUT == WaitForSingleObject(m_dummy_thread, 1000))
			TerminateThread(m_dummy_thread, 0);
	}
}

HRESULT CMpegSplitterFilter::IsMVC()
{
	if (m_mvc_found)
		return S_OK;
	else
		return S_FALSE;
}

HRESULT CMpegSplitterFilter::GetPD10(BOOL *Enabled)
{
	if (!Enabled)
		return E_POINTER;

	if (m_mvc_found)
		return S_OK;
	else
		return S_FALSE;
}
HRESULT CMpegSplitterFilter::SetPD10(BOOL Enable)
{
	if (m_mvc_found && Enable)
		return S_OK;
	else if (m_mvc_found && !Enable)
		return S_FALSE;
	else if (!m_mvc_found && Enable)
		return E_FAIL;
	else if (!m_mvc_found && !Enable)
		return S_OK;

	return E_FAIL;
}
HRESULT CMpegSplitterFilter::BeforeShow()
{
	m_traymenu->m_Menu.CreatePopupMenu();

	WCHAR *name = NULL;
	DWORD enabled = 0;
	DWORD group = 0;
	DWORD last_group = 0;

	DWORD nStreams = 0;
	Count(&nStreams);
	for(int i=0; i<nStreams; i++)
	{
		Info(i, NULL, &enabled, NULL, &group, &name, NULL, NULL);
		if (name)
		{
			if (group != last_group)
				m_traymenu->m_Menu.AppendMenuW(MF_SEPARATOR);
			
			int flag = MF_STRING;

			if(enabled)
				flag |= MF_CHECKED;
			
			if (wcsstr(name, L"Right Eye"))
				flag |= MF_GRAYED;

			m_traymenu->m_Menu.AppendMenuW(flag, i, name);
			CoTaskMemFree (name);

			last_group = group;
		}
	}

	return S_OK;
}
HRESULT CMpegSplitterFilter::Click(int id)
{
	Enable(id, AMSTREAMSELECTENABLE_ENABLE);

	return S_OK;
}


STDMETHODIMP CMpegSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    return 
		QI(IMVC)
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CMpegSplitterFilter::GetClassID(CLSID* pClsID)
{
	CheckPointer (pClsID, E_POINTER);

	if (m_pPipoBimbo)
	{
		memcpy (pClsID, &CLSID_WMAsfReader, sizeof (GUID));
		return S_OK;
	}
	else
		return __super::GetClassID(pClsID);
}

void CMpegSplitterFilter::ReadClipInfo(LPCOLESTR pszFileName)
{
	if (wcslen (pszFileName) > 0)
	{
		WCHAR		Drive[_MAX_DRIVE];
		WCHAR		Dir[_MAX_PATH];
		WCHAR		Filename[_MAX_PATH];
		WCHAR		Ext[_MAX_EXT];
		
		if (_wsplitpath_s (pszFileName, Drive, countof(Drive), Dir, countof(Dir), Filename, countof(Filename), Ext, countof(Ext)) == 0)
		{
			CString	strClipInfo;

			if (Drive[0])
				strClipInfo.Format (_T("%s\\%s\\..\\CLIPINF\\%s.clpi"), Drive, Dir, Filename);
			else
				strClipInfo.Format (_T("%s\\..\\CLIPINF\\%s.clpi"), Dir, Filename);

			CString ssif = strClipInfo;
			ssif.MakeLower();

			if (ssif.Find(_T("ssif")) >= 0)
				strClipInfo.Replace(_T("..\\"), _T("..\\..\\"));

			m_ClipInfo.ReadInfo (strClipInfo);
		}
	}
}

STDMETHODIMP CMpegSplitterFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	HRESULT		hr;
	
	return __super::Load (pszFileName, pmt);
}

HRESULT CMpegSplitterFilter::DemuxNextPacket(REFERENCE_TIME rtStartOffset)
{
	HRESULT hr;
	BYTE b;

	if(m_pFile->m_type == CMpegSplitterFile::ps || m_pFile->m_type == CMpegSplitterFile::es)
	{
		if(!m_pFile->NextMpegStartCode(b))
			return S_FALSE;

		if(b == 0xba) // program stream header
		{
			CMpegSplitterFile::pshdr h;
			if(!m_pFile->Read(h))
				return S_FALSE;
		}
		else if(b == 0xbb) // program stream system header
		{
			CMpegSplitterFile::pssyshdr h;
			if(!m_pFile->Read(h))
				return S_FALSE;
		}
#if (EVO_SUPPORT == 0)
		else if(b >= 0xbd && b < 0xf0) // pes packet
#else
		else if((b >= 0xbd && b < 0xf0) || (b == 0xfd)) // pes packet
#endif
		{
			CMpegSplitterFile::peshdr h;

			if(!m_pFile->Read(h, b) || !h.len) return S_FALSE;

			if(h.type == CMpegSplitterFile::mpeg2 && h.scrambling)
			{
				ASSERT(0);
				return E_FAIL;
			}

			__int64 pos = m_pFile->GetPos();

			DWORD TrackNumber = m_pFile->AddStream(0, b, h.len);

			if(GetOutputPin(TrackNumber))
			{
				CAutoPtr<Packet> p(DNew Packet());

				p->TrackNumber = TrackNumber;
				p->bSyncPoint = !!h.fpts;
				p->bAppendable = !h.fpts;
				p->rtStart = h.fpts ? (h.pts - rtStartOffset) : Packet::INVALID_TIME;
				p->rtStop = p->rtStart+1;
				p->SetCount(h.len - (size_t)(m_pFile->GetPos() - pos));

				m_pFile->ByteRead(p->GetData(), h.len - (m_pFile->GetPos() - pos));

				hr = DeliverPacket(p);
			}
			m_pFile->Seek(pos + h.len);
		}
	}
	else if(m_pFile->m_type == CMpegSplitterFile::ts)
	{
		CMpegSplitterFile::trhdr h;

		if(!m_pFile->Read(h)) 
			return S_FALSE;


		__int64 pos = m_pFile->GetPos();

		if(h.payload && h.payloadstart)
			m_pFile->UpdatePrograms(h);

		if(h.payload && h.pid >= 16 && h.pid < 0x1fff && !h.scrambling)
		{
			DWORD TrackNumber = h.pid;

			CMpegSplitterFile::peshdr h2;

			if(h.payloadstart && m_pFile->NextMpegStartCode(b, 4) && m_pFile->Read(h2, b)) // pes packet
			{
				if(h2.type == CMpegSplitterFile::mpeg2 && h2.scrambling)
				{
					ASSERT(0);
					return E_FAIL;
				}
				TrackNumber = m_pFile->AddStream(h.pid, b, h.bytes - (DWORD)(m_pFile->GetPos() - pos));
			}

			if(GetOutputPin(TrackNumber) || (m_mvc_found && TrackNumber == 0x1012))
			{
				CAutoPtr<Packet> p(DNew Packet());

				p->TrackNumber = TrackNumber;
				p->bSyncPoint = !!h2.fpts;
				p->bAppendable = !h2.fpts;

				if (h.fPCR)
				{
					CRefTime rtNow;
					StreamTime(rtNow);
					TRACE ("Now=%S   PCR=%S\n", ReftimeToString(rtNow.m_time), ReftimeToString(h.PCR));
				}
				if (h2.fpts && h.pid == 241)
				{
					TRACE ("Sub=%S\n", ReftimeToString(h2.pts - rtStartOffset));
				}

				p->rtStart = h2.fpts ? (h2.pts - rtStartOffset) : Packet::INVALID_TIME;
				p->rtStop = p->rtStart+1;
				p->SetCount(h.bytes - (size_t)(m_pFile->GetPos() - pos));

				int nBytes = int(h.bytes - (m_pFile->GetPos() - pos));
				m_pFile->ByteRead(p->GetData(), nBytes);

				if (m_mvc_found && TrackNumber == 0x1012)
				{
					if (m_for_encoding)
						// for encoding ,no queue
						hr = dummy_deliver_packet(p);
					else
					{
						// for player, queue
						bool sleep = false;
						retry:
						if (sleep)
							Sleep(1);
						{
							CAutoLock queuelock(&m_queuelock);
							if (m_queue.GetCount() > 1024*1024*30/188)
							{
								sleep = true;
								goto retry;
							}
							hr = S_OK;
							m_queue.AddTail(p);
							ResumeThread(m_dummy_thread);
						}
					}

				}
				else
					hr = DeliverPacket(p);
			}
		}

		m_pFile->Seek(h.next);
	}
	else if(m_pFile->m_type == CMpegSplitterFile::pva)
	{
		CMpegSplitterFile::pvahdr h;
		if(!m_pFile->Read(h))
			return S_FALSE;

		DWORD TrackNumber = h.streamid;

		__int64 pos = m_pFile->GetPos();

		if(GetOutputPin(TrackNumber))
		{
			CAutoPtr<Packet> p(DNew Packet());

			p->TrackNumber = TrackNumber;
			p->bSyncPoint = !!h.fpts;
			p->bAppendable = !h.fpts;
			p->rtStart = h.fpts ? (h.pts - rtStartOffset) : Packet::INVALID_TIME;
			p->rtStop = p->rtStart+1;
			p->SetCount(h.length);

			m_pFile->ByteRead(p->GetData(), h.length);
			hr = DeliverPacket(p);
		}

		m_pFile->Seek(pos + h.length);
	}

	return S_OK;
}

//

HRESULT CMpegSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();

	ReadClipInfo (GetPartFilename(pAsyncReader));
	m_pFile.Attach(DNew CMpegSplitterFile(pAsyncReader, hr, m_ClipInfo.IsHdmv(), m_ClipInfo));

	if(!m_pFile) return E_OUTOFMEMORY;

	if(FAILED(hr))
	{
		m_pFile.Free();
		return hr;
	}

	// check for mvc ext stream
	CMpegSplitterFile::CStreamList &streamlist = m_pFile->m_streams[CMpegSplitterFile::video];
	for(POSITION pos = streamlist.GetHeadPosition(); pos; streamlist.GetNext(pos))
	{
		CMpegSplitterFile::stream &s = streamlist.GetAt(pos);
		if (s.pid == 0x1012)
		{
			m_mvc_found = true;
		}
	}

	//if (!m_mvc_found)
	//	return E_FAIL;

	//m_traymenu.Attach(DNew TrayMenu(this, m_fn));

	// Create
	if (m_ClipInfo.IsHdmv())
	{
		for (int i=0; i<m_ClipInfo.GetStreamNumber(); i++)
		{
			CHdmvClipInfo::Stream* stream = m_ClipInfo.GetStreamByIndex (i);
			if (stream->m_Type == PRESENTATION_GRAPHICS_STREAM)
			{
				m_pFile->AddHdmvPGStream (stream->m_PID, stream->m_LanguageCode);
			}
		}
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	for(int i = 0; i < countof(m_pFile->m_streams); i++)
	{
		POSITION pos = m_pFile->m_streams[i].GetHeadPosition();
		while(pos)
		{
			CMpegSplitterFile::stream& s = m_pFile->m_streams[i].GetNext(pos);
			CAtlArray<CMediaType> mts;
			mts.Add(s.mt);
			
			CStringW name = CMpegSplitterFile::CStreamList::ToString(i);
			CStringW str;

			if (i == CMpegSplitterFile::subpic && s.pid == NO_SUBTITLE_PID)
			{
				str	= _T("No subtitles");
			}
			else
			{
				int iProgram;
				const CHdmvClipInfo::Stream *pClipInfo;
				const CMpegSplitterFile::program * pProgram = m_pFile->FindProgram(s.pid, iProgram, pClipInfo);
				const wchar_t *pStreamName = NULL;
				int StreamType = pClipInfo ? pClipInfo->m_Type : pProgram ? pProgram->streams[iProgram].type : 0;
				pStreamName = StreamTypeToName((PES_STREAM_TYPE)StreamType);

				CString FormatDesc = GetMediaTypeDesc(&s.mt, pClipInfo, StreamType);

				if (!FormatDesc.IsEmpty())
					str.Format(L"%s (%04x,%02x,%02x)", FormatDesc.GetString(), s.pid, s.pesid, s.ps1id); // TODO: make this nicer
				else if (pStreamName)
					str.Format(L"%s - %s (%04x,%02x,%02x)", name, pStreamName, s.pid, s.pesid, s.ps1id); // TODO: make this nicer
				else
					str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id); // TODO: make this nicer

				if (m_mvc_found && s.pid == 0x1011)
				{
					str.Replace(_T("AVC"), _T("MVC"));
					//str += _T("(MVC)");
				}
				else if (m_mvc_found && s.pid == 0x1012)
				{
					str.Replace(_T("AVC"), _T("MVC"));
					str += _T("(Right Eye Only)");
				}

			}
			
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CMpegSplitterOutputPin(mts, str, this, this, &hr));
			if (i == CMpegSplitterFile::subpic)
			{
				(static_cast<CMpegSplitterOutputPin*>(pPinOut.m_p))->SetMaxShift (_I64_MAX);
			}
			if(S_OK == AddOutputPin(s, pPinOut))
				break;
		}
	}

	if(m_rtPlaylistDuration)
	{
		m_rtNewStop = m_rtStop = m_rtDuration = m_rtPlaylistDuration;
	}
	else if(m_pFile->IsRandomAccess() && m_pFile->m_rate)
	{
		m_rtNewStop = m_rtStop = m_rtDuration = 10000000i64 * m_pFile->GetLength() / m_pFile->m_rate;
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CMpegSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CMpegSplitterFilter");
	if(!m_pFile) return(false);

	m_rtStartOffset = 0;

	return(true);
}

void CMpegSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	{
		CAutoLock queuelock(&m_queuelock);
		m_queue.RemoveAll();
	}

	CAtlList<CMpegSplitterFile::stream>* pMasterStream = m_pFile->GetMasterStream();

	if(!pMasterStream)
	{
		ASSERT(0);
		return;
	}

	if(m_pFile->IsStreaming())
	{
		m_pFile->Seek(max(0, m_pFile->GetLength() - 100*1024));
		m_rtStartOffset = m_pFile->m_rtMin + m_pFile->NextPTS(pMasterStream->GetHead());
		return;
	}

	REFERENCE_TIME rtPreroll = 10000000;
	
	if(rt <= rtPreroll || m_rtDuration <= 0)
	{
		m_pFile->Seek(0);
	}
	else
	{
		__int64 len = m_pFile->GetLength();
		__int64 seekpos = (__int64)(1.0*rt/m_rtDuration*len);
		__int64 minseekpos = _I64_MAX;

		REFERENCE_TIME rtmax = rt - rtPreroll;
		REFERENCE_TIME rtmin = rtmax - 5000000;

		if(m_rtStartOffset == 0)
		for(int i = 0; i < countof(m_pFile->m_streams)-1; i++)
		{
			POSITION pos = m_pFile->m_streams[i].GetHeadPosition();
			while(pos)
			{
				DWORD TrackNum = m_pFile->m_streams[i].GetNext(pos);

				CBaseSplitterOutputPin* pPin = GetOutputPin(TrackNum);
				if(pPin && pPin->IsConnected())
				{
					m_pFile->Seek(seekpos);

					REFERENCE_TIME pdt = _I64_MIN;

					for(int j = 0; j < 10; j++)
					{
						REFERENCE_TIME rt = m_pFile->NextPTS(TrackNum);

						if(rt < 0) break;

						REFERENCE_TIME dt = rt - rtmax;
						if(dt > 0 && dt == pdt) dt = 10000000i64;


						if(rtmin <= rt && rt <= rtmax || pdt > 0 && dt < 0)
						{
							minseekpos = min(minseekpos, m_pFile->GetPos());
							break;
						}

						m_pFile->Seek(m_pFile->GetPos() - (__int64)(1.0*dt/m_rtDuration*len));
		
						pdt = dt;
					}
				}
			}
		}

		if(minseekpos != _I64_MAX)
		{
			seekpos = minseekpos;
		}
		else
		{
			// this file is probably screwed up, try plan B, seek simply by bitrate

			rt -= rtPreroll;
			seekpos = (__int64)(1.0*rt/m_rtDuration*len);
			m_pFile->Seek(seekpos);
			m_rtStartOffset = m_pFile->m_rtMin + m_pFile->NextPTS(pMasterStream->GetHead()) - rt;
		}

		m_pFile->Seek(seekpos);
	}
}

bool CMpegSplitterFilter::DemuxLoop()
{
	REFERENCE_TIME rtStartOffset = m_rtStartOffset ? m_rtStartOffset : m_pFile->m_rtMin;

	HRESULT hr = S_OK;
	BOOL check = CheckRequest(NULL);
	int n = 0;
	while(SUCCEEDED(hr) && !check)
	{
		n++;
		if((hr = m_pFile->HasMoreData(1024*500)) == S_OK)
			if((hr = DemuxNextPacket(rtStartOffset)) == S_FALSE)
				Sleep(1);

		//if ((n&0xff) == 0)
			check = CheckRequest(NULL);
	}

	return(true);
}

HRESULT find_main_movie(wchar_t *in, wchar_t *out)
{
	wchar_t in2[MAX_PATH];
	wcsncpy(in2, in, MAX_PATH);
	in2[MAX_PATH-1] = NULL;
	while (in2[wcslen(in2)-1] == L'\\') in2[wcslen(in2)-1] = NULL;


	CHdmvClipInfo	clipinfo;
	CString main_file;
	CAtlList<CHdmvClipInfo::PlaylistItem> item;
	HRESULT hr = clipinfo.FindMainMovie(W2T(in2), main_file, item);

	USES_CONVERSION;
	wcsncpy(out, T2W(main_file.GetBuffer()), MAX_PATH);
	out[MAX_PATH-1] = NULL;
	return hr;
}

bool CMpegSplitterFilter::BuildPlaylist(LPCTSTR pszFileName, CAtlList<CHdmvClipInfo::PlaylistItem>& Items)
{
	m_rtPlaylistDuration = 0;
	return SUCCEEDED (m_ClipInfo.ReadPlaylist (pszFileName, m_rtPlaylistDuration, Items)) ? true : false;
}

// IAMStreamSelect

STDMETHODIMP CMpegSplitterFilter::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	*pcStreams = 0;

	for(int i = 0; i < countof(m_pFile->m_streams); i++)
		(*pcStreams) += m_pFile->m_streams[i].GetCount();

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::Enable(long lIndex, DWORD dwFlags)
{
	if(!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
		return E_NOTIMPL;

	WCHAR *name;
	DWORD group = 0;
	Info(lIndex, NULL, NULL, NULL, &group, &name, NULL, NULL);

	HRESULT hr = S_OK;
	if (group != m_pFile->audio && group != m_pFile->subpic)
	{
		if(wcsstr(name, L"Right Eye"))
			hr = E_FAIL;
		else
			hr = EnableCore(lIndex, dwFlags);
	}
	else
	{
		// QI some interface
		CComQIPtr<IBaseFilter, &IID_IBaseFilter> this_filter(this);
		FILTER_INFO fi;
		this_filter->QueryFilterInfo(&fi);
		CComPtr<IFilterGraph> graph;
		graph.Attach(fi.pGraph);
		CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> gb(graph);
		CComQIPtr<IMediaControl, &IID_IMediaControl> mc(graph);

		// stop the graph
		OAFilterState state_before, state;
		mc->GetState(INFINITE, &state_before);
		if (state_before != State_Stopped)
		{
			mc->Stop();
			mc->GetState(INFINITE, &state);
		}

		// find output pin
		CComPtr<IEnumPins> ep;
		this_filter->EnumPins(&ep);
		CComPtr<IPin> outpin;
		while(S_OK == ep->Next(1, &outpin, NULL))
		{
			PIN_INFO pi;
			outpin->QueryPinInfo(&pi);
			if (pi.pFilter) pi.pFilter->Release();

			if ((wcsstr(pi.achName, L"Audio")&& group == m_pFile->audio) ||
				(wcsstr(pi.achName, L"Subtitle")&& group == m_pFile->subpic))
				break;

			outpin = NULL;
		}

		// reconnect/rerender it
		CComPtr<IPin> connected;
		outpin->ConnectedTo(&connected);
		if (connected)
		{
			if (group == m_pFile->audio)
			{
				// disconnect and render it
				RemoveDownstream(connected);
				gb->Render(outpin);
			}
			else if (group == m_pFile->subpic)
			{
				// disconnect and reconnect it
				gb->Disconnect(connected);
				gb->Disconnect(outpin);
				hr = EnableCore(lIndex, dwFlags);
				hr = gb->ConnectDirect(outpin, connected, NULL);
			}


		}
		else
			hr = EnableCore(lIndex, dwFlags);

		// run graph if needed
		if (state_before == State_Running) 
			mc->Run();
		else if (state_before == State_Paused)
			mc->Pause();

	}

	CoTaskMemFree(name);

	return hr;
}

HRESULT CMpegSplitterFilter::RemoveDownstream(CComPtr<IPin> &input_pin)
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

STDMETHODIMP CMpegSplitterFilter::EnableCore(long lIndex, DWORD dwFlags)
{
	CAutoLock cAutoLock(this);
	if(!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
		return E_NOTIMPL;

	for(int i = 0, j = 0; i < countof(m_pFile->m_streams); i++)
	{
		int cnt = m_pFile->m_streams[i].GetCount();
		
		if(lIndex >= j && lIndex < j+cnt)
		{
			lIndex -= j;

			POSITION pos = m_pFile->m_streams[i].FindIndex(lIndex);
			if(!pos) return E_UNEXPECTED;

			CMpegSplitterFile::stream& to = m_pFile->m_streams[i].GetAt(pos);

			pos = m_pFile->m_streams[i].GetHeadPosition();
			while(pos)
			{
				CMpegSplitterFile::stream& from = m_pFile->m_streams[i].GetNext(pos);
				if(!GetOutputPin(from)) continue;

				HRESULT hr;
				if(FAILED(hr = RenameOutputPin(from, to, &to.mt)))
					return hr;

				// Don't rename other pin for Hdmv!
				int iProgram;
				const CHdmvClipInfo::Stream *pClipInfo;
				const CMpegSplitterFile::program* p = m_pFile->FindProgram(to.pid, iProgram, pClipInfo);

				if(p!=NULL && !m_ClipInfo.IsHdmv() && !m_pFile->IsHdmv())
				{
					for(int k = 0; k < countof(m_pFile->m_streams); k++)
					{
						if(k == i) continue;

						pos = m_pFile->m_streams[k].GetHeadPosition();
						while(pos)
						{
							CMpegSplitterFile::stream& from = m_pFile->m_streams[k].GetNext(pos);
							if(!GetOutputPin(from)) continue;

							for(int l = 0; l < countof(p->streams); l++)
							{
								if(const CMpegSplitterFile::stream* s = m_pFile->m_streams[k].FindStream(p->streams[l].pid))
								{
									if(from != *s)
										hr = RenameOutputPin(from, *s, &s->mt);
									break;
								}
							}
						}
					}
				}

				return S_OK;
			}
		}

		j += cnt;
	}

	return S_FALSE;
}

LONGLONG GetMediaTypeQuality(const CMediaType *_pMediaType, int _PresentationFormat)
{
	if (_pMediaType->formattype == FORMAT_WaveFormatEx)
	{
		__int64 Ret = 0;

		const WAVEFORMATEX *pInfo = GetFormatHelper(pInfo, _pMediaType);
		int TypePriority = 0;

		if (_pMediaType->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
		{
			TypePriority = 12;			
		}
		else if (_pMediaType->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO)
		{
			TypePriority = 12;
		}
		else
		{
			if (_PresentationFormat == AUDIO_STREAM_DTS_HD_MASTER_AUDIO)
				TypePriority = 12;
			else if (_PresentationFormat == AUDIO_STREAM_DTS_HD)
				TypePriority = 11;
			else if (_PresentationFormat == AUDIO_STREAM_AC3_TRUE_HD)
				TypePriority = 12;
			else if (_PresentationFormat == AUDIO_STREAM_AC3_PLUS)
				TypePriority = 10;
			else
			{
				switch (pInfo->wFormatTag)
				{
				case WAVE_FORMAT_PS2_PCM:
					{
						TypePriority = 12;
					}
					break;
				case WAVE_FORMAT_PS2_ADPCM:
					{
						TypePriority = 4;
					}
					break;
				case WAVE_FORMAT_DVD_DTS:
					{
						TypePriority = 9;
					}
					break;
				case WAVE_FORMAT_DOLBY_AC3:
					{
						TypePriority = 8;
					}
					break;
				case WAVE_FORMAT_AAC:
					{
						TypePriority = 7;
					}
					break;
				case WAVE_FORMAT_MP3:
					{
						TypePriority = 6;
					}
					break;
				case WAVE_FORMAT_MPEG:
					{
						TypePriority = 5;
					}
					break;
				}
			}
		}

		Ret += __int64(TypePriority) * 100000000i64 * 1000000000i64;

		Ret += __int64(pInfo->nChannels) * 1000000i64 * 1000000000i64;
		Ret += __int64(pInfo->nSamplesPerSec) * 10i64  * 1000000000i64;
		Ret += __int64(pInfo->wBitsPerSample)  * 10000000i64;
		Ret += __int64(pInfo->nAvgBytesPerSec);

		return Ret;
	}

	return 0;
}

bool CMpegSplitterFile::stream::operator < (const stream &_Other) const
{

	if (mt.majortype == MEDIATYPE_Audio && _Other.mt.majortype == MEDIATYPE_Audio)
	{
		int iProgram0;
		const CHdmvClipInfo::Stream *pClipInfo0;
		const CMpegSplitterFile::program * pProgram0 = m_pFile->FindProgram(pid, iProgram0, pClipInfo0);
		int StreamType0 = pClipInfo0 ? pClipInfo0->m_Type : pProgram0 ? pProgram0->streams[iProgram0].type : 0;
		int iProgram1;
		const CHdmvClipInfo::Stream *pClipInfo1;
		const CMpegSplitterFile::program * pProgram1 = m_pFile->FindProgram(_Other.pid, iProgram1, pClipInfo1);
		int StreamType1 = pClipInfo1 ? pClipInfo1->m_Type : pProgram1 ? pProgram1->streams[iProgram1].type : 0;
		
		if (mt.formattype == FORMAT_WaveFormatEx && _Other.mt.formattype != FORMAT_WaveFormatEx)
			return true;
		if (mt.formattype != FORMAT_WaveFormatEx && _Other.mt.formattype == FORMAT_WaveFormatEx)
			return false;

		LONGLONG Quality0 = GetMediaTypeQuality(&mt, StreamType0);
		LONGLONG Quality1 = GetMediaTypeQuality(&_Other.mt, StreamType1);
		if (Quality0 > Quality1)
			return true;
		if (Quality0 < Quality1)
			return false;
	}
	DWORD DefaultFirst = *this;
	DWORD DefaultSecond = _Other;
	return DefaultFirst < DefaultSecond;
}

DWORD WINAPI CMpegSplitterFilter::dummy_thread(LPVOID lpParame)
{
	CMpegSplitterFilter *_this = (CMpegSplitterFilter*) lpParame;
	CAutoPtr<Packet> p;

sleep:
	SuspendThread(_this->m_dummy_thread);
	if (_this->m_dummy_exit)
		return 0;
next:
	{ 
		CAutoLock queuelock(&(_this->m_queuelock));
		if (_this->m_queue.IsEmpty())
			goto sleep;
		p = _this->m_queue.RemoveHead();
	}

	_this->dummy_deliver_packet(p);

	goto next;

	return 0;
}


HRESULT CMpegSplitterFilter::dummy_deliver_packet(CAutoPtr<Packet> p)
{
	CAutoLock dummylock(&m_dummylock);
	// copied from CBaseSplitterFilter
	if(p->rtStart != Packet::INVALID_TIME)
	{
		m_rtCurrent = p->rtStart;

		p->rtStart -= m_rtStart;
		p->rtStop -= m_rtStart;

		ASSERT(p->rtStart <= p->rtStop);
	}


	if(p->rtStart != Packet::INVALID_TIME)
	{
		REFERENCE_TIME rt = p->rtStart + m_rtOffset;

		// Filter invalid PTS (if too different from previous packet)
		if(m_rtPrev != Packet::INVALID_TIME)
		if(_abs64(rt - m_rtPrev) > m_rtMaxShift)
			m_rtOffset += m_rtPrev - rt;

		p->rtStart += m_rtOffset;
		p->rtStop += m_rtOffset;

		m_rtPrev = p->rtStart;
	}

	//if(p->pmt->subtype == FOURCCMap('1CVA') || p->pmt->subtype == FOURCCMap('1cva'))
	if(true)
	{

		if(!m_p)		// if this is the first packet
		{
			m_p.Attach(DNew Packet());
			m_p->TrackNumber = p->TrackNumber;
			m_p->bDiscontinuity = p->bDiscontinuity;
			p->bDiscontinuity = FALSE;

			m_p->bSyncPoint = p->bSyncPoint;
			p->bSyncPoint = FALSE;

			m_p->rtStart = p->rtStart;
			p->rtStart = Packet::INVALID_TIME;

			m_p->rtStop = p->rtStop;
			p->rtStop = Packet::INVALID_TIME;
		}

		m_p->Append(*p);

		BYTE* start = m_p->GetData();
		BYTE* end = start + m_p->GetCount();

		while(start <= end-4 && *(DWORD*)start != 0x01000000) start++;

		while(start <= end-4)
		{
			BYTE* next = start+1;
			next = max(next, end - p->GetCount()-4);	//my12doom's speed skip code

			while(next <= end-4 && *(DWORD*)next != 0x01000000) next++;

			if(next >= end-4) 
				break;

			int size = next - start;

			CH264Nalu			Nalu;
			Nalu.SetBuffer (start, size, 0);

			CAutoPtr<Packet> p2;

			while (Nalu.ReadNext())
			{
				DWORD	dwNalLength = 
					((Nalu.GetDataLength() >> 24) & 0x000000ff) |
					((Nalu.GetDataLength() >>  8) & 0x0000ff00) |
					((Nalu.GetDataLength() <<  8) & 0x00ff0000) |
					((Nalu.GetDataLength() << 24) & 0xff000000);		//reverse byte order

				CAutoPtr<Packet> p3(DNew Packet());
				
				p3->SetCount (Nalu.GetDataLength()+sizeof(dwNalLength));
				memcpy (p3->GetData(), &dwNalLength, sizeof(dwNalLength));
				memcpy (p3->GetData()+sizeof(dwNalLength), Nalu.GetDataBuffer(), Nalu.GetDataLength());
				
				if (p2 == NULL)
					p2 = p3;
				else
					p2->Append(*p3);
			}

			// now p2 is all nalus recieved yet
			p2->TrackNumber = m_p->TrackNumber;
			p2->bDiscontinuity = m_p->bDiscontinuity;
			m_p->bDiscontinuity = FALSE;

			p2->bSyncPoint = m_p->bSyncPoint;
			m_p->bSyncPoint = FALSE;

			p2->rtStart = m_p->rtStart; m_p->rtStart = Packet::INVALID_TIME;
			p2->rtStop = m_p->rtStop; m_p->rtStop = Packet::INVALID_TIME;

			p2->pmt = m_p->pmt; m_p->pmt = NULL;

			m_pl.AddTail(p2);

			if(p->rtStart != Packet::INVALID_TIME)
			{
				m_p->rtStart = p->rtStart;
				m_p->rtStop = p->rtStop;
				p->rtStart = Packet::INVALID_TIME;
			}
			if(p->bDiscontinuity)
			{
				m_p->bDiscontinuity = p->bDiscontinuity;
				p->bDiscontinuity = FALSE;
			}
			if(p->bSyncPoint)
			{
				m_p->bSyncPoint = p->bSyncPoint;
				p->bSyncPoint = FALSE;
			}
			if(m_p->pmt)
				DeleteMediaType(m_p->pmt);
			
			m_p->pmt = p->pmt;
			p->pmt = NULL;

			start = next;
		}

		if(start > m_p->GetData())
		{
			m_p->RemoveAt(0, start - m_p->GetData());
		}

		for(POSITION pos = m_pl.GetHeadPosition(); pos; m_pl.GetNext(pos))
		{
			if(pos == m_pl.GetHeadPosition()) 
				continue;

			Packet* pPacket = m_pl.GetAt(pos);
			BYTE* pData = pPacket->GetData();

			if((pData[4]&0x1f) == 0x18 && !m_fHasAccessUnitDelimiters)
			{
				m_fHasAccessUnitDelimiters = true;
				printf("right eye delimeter detected.\n");
			}

			if((pData[4]&0x1f) == 0x18 || !m_fHasAccessUnitDelimiters && pPacket->rtStart != Packet::INVALID_TIME)
			{
				p = m_pl.RemoveHead();

				while(pos != m_pl.GetHeadPosition())
				{
					CAutoPtr<Packet> p2 = m_pl.RemoveHead();
					p->Append(*p2);
				}

				HRESULT hr = ((CMpegSplitterOutputPin*)GetOutputPin(0x1011)) ->DeliverMVCPacket (p);

				if(hr != S_OK) return hr;
			}
		}

		return S_OK;		// packet cached in output pin;
	}
	else
	{
		m_p.Free();
		m_pl.RemoveAll();
	}

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	for(int i = 0, j = 0; i < countof(m_pFile->m_streams); i++)
	{
		int cnt = m_pFile->m_streams[i].GetCount();
		
		if(lIndex >= j && lIndex < j+cnt)
		{
			lIndex -= j;
			
			POSITION pos = m_pFile->m_streams[i].FindIndex(lIndex);
			if(!pos) return E_UNEXPECTED;

			CMpegSplitterFile::stream&	s = m_pFile->m_streams[i].GetAt(pos);
			CHdmvClipInfo::Stream*		pStream = m_ClipInfo.FindStream (s.pid);

			if(ppmt) *ppmt = CreateMediaType(&s.mt);
			if(pdwFlags) *pdwFlags = GetOutputPin(s) ? (AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE) : 0;
			if(plcid) *plcid = pStream ? pStream->m_LCID : 0;
			if(pdwGroup) *pdwGroup = i;
			if(ppObject) *ppObject = NULL;
			if(ppUnk) *ppUnk = NULL;

			
			if(ppszName)
			{
				CStringW name = CMpegSplitterFile::CStreamList::ToString(i);

				CStringW str;

				if (i == CMpegSplitterFile::subpic && s.pid == NO_SUBTITLE_PID)
				{
					str		= _T("No subtitles");
					*plcid	= (LCID)LCID_NOSUBTITLES;
				}
				else
				{
					int iProgram;
					const CHdmvClipInfo::Stream *pClipInfo;
					const CMpegSplitterFile::program * pProgram = m_pFile->FindProgram(s.pid, iProgram, pClipInfo);
					const wchar_t *pStreamName = NULL;
					int StreamType = pClipInfo ? pClipInfo->m_Type : pProgram ? pProgram->streams[iProgram].type : 0;
					pStreamName = StreamTypeToName((PES_STREAM_TYPE)StreamType);

					CString FormatDesc = GetMediaTypeDesc(&s.mt, pClipInfo, StreamType);

					if (!FormatDesc.IsEmpty())
						str.Format(L"%s (%04x,%02x,%02x)", FormatDesc.GetString(), s.pid, s.pesid, s.ps1id); // TODO: make this nicer
					else if (pStreamName)
						str.Format(L"%s - %s (%04x,%02x,%02x)", name, pStreamName, s.pid, s.pesid, s.ps1id); // TODO: make this nicer
					else
						str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id); // TODO: make this nicer

					if (m_mvc_found && s.pid == 0x1011)
					{
						//str += _T("(MVC)");
						str.Replace(_T("AVC"), _T("MVC"));
					}
					else if (m_mvc_found && s.pid == 0x1012)
					{
						str.Replace(_T("AVC"), _T("MVC"));
						str += _T("(Right Eye Only)");
					}
				}

				*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
				if(*ppszName == NULL) return E_OUTOFMEMORY;

				wcscpy_s(*ppszName, str.GetLength()+1, str);
			}
		}

		j += cnt;
	}

	return S_OK;
}

//
// CMpegSourceFilter
//

CMpegSourceFilter::CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CMpegSplitterFilter(pUnk, phr, clsid)
{
	m_pInput.Free();
}

//
// CMpegSplitterOutputPin
//

CMpegSplitterOutputPin::CMpegSplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
	, m_fHasAccessUnitDelimiters(false)
	, m_rtMaxShift(50000000)
	, m_bFilterDTSMA(false)
{
}

CMpegSplitterOutputPin::~CMpegSplitterOutputPin()
{
}

HRESULT CMpegSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	{
		CAutoLock cAutoLock(this);
		m_rtPrev = Packet::INVALID_TIME;
		m_rtOffset = 0;
	}

	{
		CAutoLock cAutoLock2(&((CMpegSplitterFilter*)m_pFilter)->m_dummylock);
		((CMpegSplitterFilter*)m_pFilter) -> m_rtPrev = Packet::INVALID_TIME;
		((CMpegSplitterFilter*)m_pFilter) -> m_rtOffset = 0;
	}

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CMpegSplitterOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(this);
		m_p.Free();
		m_pl.RemoveAll();
	}

	{
		CAutoLock cAutoLock2(&((CMpegSplitterFilter*)m_pFilter)->m_dummylock);
		((CMpegSplitterFilter*)m_pFilter) ->m_p.Free();
		((CMpegSplitterFilter*)m_pFilter) ->m_pl.RemoveAll();

		m_q1011.RemoveAll();
		m_q1012.RemoveAll();

	}

	return __super::DeliverEndFlush();
}

HRESULT CMpegSplitterOutputPin::DeliverMVCPacket(CAutoPtr<Packet> p)
{
	CAutoLock cAutoLock(this);

	// add to cache
	TCHAR str[102400];
	if (p->TrackNumber == 0x1011)
		m_q1011.AddTail(p);
	else
	{
		p->TrackNumber = 0x1011;
		m_q1012.AddTail(p);
	}

//find_match:

	// find matching packets
	bool matched = false;
	REFERENCE_TIME matched_time = Packet::INVALID_TIME;

	for(POSITION for_1011 = m_q1011.GetHeadPosition(); for_1011; m_q1011.GetNext(for_1011))
	{
		Packet *ele1011 = m_q1011.GetAt(for_1011);
		for(POSITION for_1012 = m_q1012.GetHeadPosition(); for_1012; m_q1012.GetNext(for_1012))
		{
			Packet *ele1012 = m_q1012.GetAt(for_1012);
			if (ele1011->rtStart == ele1012->rtStart
				&& ele1011->rtStop == ele1012->rtStop
				//&& ele1011->rtStart != Packet::INVALID_TIME
				)
			{
				matched_time = ele1011->rtStart;
				matched = true;
				break;
			}
		}
	}

	//if (matched_time != Packet::INVALID_TIME)
	if (matched)
	{
		CAutoPtr<Packet> double_packet;
		// delete every packet before matched packet
		HRESULT hr;
		while(true)
		{
			CAutoPtr<Packet> p2 = m_q1011.RemoveHead();
			if (p2->rtStart == matched_time)
			{
				double_packet = p2;

				break;
			}
			else
				_tprintf(_T("warning: dropping unmatched packets(left eye, %s - %s, %d bytes)\n"), (LPCTSTR)ReftimeToString(p2->rtStart), (LPCTSTR)ReftimeToString(p2->rtStop), p2->GetDataSize());
		}
		while(true)
		{
			CAutoPtr<Packet> p2 = m_q1012.RemoveHead();
			if (p2->rtStart == matched_time)
			{
				double_packet->Append(*p2);

				break;
			}
			else
				_tprintf(_T("warning: dropping unmatched packets(right eye, %s - %s, %d bytes)\n"), (LPCTSTR)ReftimeToString(p2->rtStart), (LPCTSTR)ReftimeToString(p2->rtStop), p2->GetDataSize());

		}

		// frame number debug:
		int fn = (int)((double)(double_packet->rtStart)/10000*120/1001 + 0.5);
		//printf("debug: fn = %d\n", fn);


		if (double_packet->rtStart == Packet::INVALID_TIME)
			printf("warning: delivering packet with start = INVALID_TIME");
		if (double_packet->rtStop == Packet::INVALID_TIME)
			printf("warning: delivering packet with stop  = INVALID_TIME");

		return __super::DeliverPacket(double_packet);
	}
	return S_OK;
}


HRESULT CMpegSplitterOutputPin::DeliverPacket(CAutoPtr<Packet> p)
{
	CAutoLock cAutoLock(this);

	if(p->rtStart != Packet::INVALID_TIME)
	{
		REFERENCE_TIME rt = p->rtStart + m_rtOffset;

		// Filter invalid PTS (if too different from previous packet)
		if(m_rtPrev != Packet::INVALID_TIME)
		if(_abs64(rt - m_rtPrev) > m_rtMaxShift)
			m_rtOffset += m_rtPrev - rt;

		p->rtStart += m_rtOffset;
		p->rtStop += m_rtOffset;

		m_rtPrev = p->rtStart;
	}

	
	if (p->pmt)
	{
		if (*((CMediaType *)p->pmt) != m_mt)
			SetMediaType ((CMediaType*)p->pmt);
	}

	if(m_mt.subtype == MEDIASUBTYPE_AAC) // special code for aac, the currently available decoders only like whole frame samples
	{
		if(m_p && m_p->GetCount() == 1 && m_p->GetAt(0) == 0xff	&& !(!p->IsEmpty() && (p->GetAt(0) & 0xf6) == 0xf0))
			m_p.Free();

		if(!m_p)
		{
			BYTE* base = p->GetData();
			BYTE* s = base;
			BYTE* e = s + p->GetCount();

			for(; s < e; s++)
			{
				if(*s != 0xff) continue;

				if(s == e-1 || (s[1]&0xf6) == 0xf0)
				{
					memmove(base, s, e - s);
					p->SetCount(e - s);
					m_p = p;
					break;
				}
			}
		}
		else
		{
			m_p->Append(*p);
		}

		while(m_p && m_p->GetCount() > 9)
		{
			BYTE* base = m_p->GetData();
			BYTE* s = base;
			BYTE* e = s + m_p->GetCount();
			int len = ((s[3]&3)<<11)|(s[4]<<3)|(s[5]>>5);
			bool crc = !(s[1]&1);
			s += 7; len -= 7;
			if(crc) s += 2, len -= 2;

			if(e - s < len)
			{
				break;
			}

			if(len <= 0 || e - s >= len + 2 && (s[len] != 0xff || (s[len+1]&0xf6) != 0xf0))
			{
				m_p.Free();
				break;
			}

			CAutoPtr<Packet> p2(DNew Packet());

			p2->TrackNumber = m_p->TrackNumber;
			p2->bDiscontinuity |= m_p->bDiscontinuity;
			m_p->bDiscontinuity = false;

			p2->bSyncPoint = m_p->rtStart != Packet::INVALID_TIME;
			p2->rtStart = m_p->rtStart;
			m_p->rtStart = Packet::INVALID_TIME;

			p2->rtStop = m_p->rtStop;
			m_p->rtStop = Packet::INVALID_TIME;
			p2->pmt = m_p->pmt; m_p->pmt = NULL;
			p2->SetData(s, len);

			s += len;
			memmove(base, s, e - s);
			m_p->SetCount(e - s);

			HRESULT hr = __super::DeliverPacket(p2);
			if(hr != S_OK) return hr;
		}

		if(m_p && p)
		{
			if(!m_p->bDiscontinuity) m_p->bDiscontinuity = p->bDiscontinuity;
			if(!m_p->bSyncPoint) m_p->bSyncPoint = p->bSyncPoint;
			if(m_p->rtStart == Packet::INVALID_TIME) m_p->rtStart = p->rtStart, m_p->rtStop = p->rtStop;
			if(m_p->pmt) DeleteMediaType(m_p->pmt);
			
			m_p->pmt = p->pmt;
			p->pmt = NULL;
		}

		return S_OK;
	}
	else if(m_mt.subtype == FOURCCMap('1CVA') || m_mt.subtype == FOURCCMap('1cva')) // just like aac, this has to be starting nalus, more can be packed together
	{
		if(!m_p)		// if this is the first packet
		{
			m_p.Attach(DNew Packet());
			m_p->TrackNumber = p->TrackNumber;
			m_p->bDiscontinuity = p->bDiscontinuity;
			p->bDiscontinuity = FALSE;

			m_p->bSyncPoint = p->bSyncPoint;
			p->bSyncPoint = FALSE;

			m_p->rtStart = p->rtStart;
			p->rtStart = Packet::INVALID_TIME;

			m_p->rtStop = p->rtStop;
			p->rtStop = Packet::INVALID_TIME;
		}

		m_p->Append(*p);

		BYTE* start = m_p->GetData();
		BYTE* end = start + m_p->GetCount();

		while(start <= end-4 && *(DWORD*)start != 0x01000000) start++;

		while(start <= end-4)
		{
			BYTE* next = start+1;
			next = max(next, end - p->GetCount()-4);		//my12doom's speed skip code

			while(next <= end-4 && *(DWORD*)next != 0x01000000) next++;

			if(next >= end-4) 
				break;

			int size = next - start;

			CH264Nalu			Nalu;
			Nalu.SetBuffer (start, size, 0);

			CAutoPtr<Packet> p2;

			while (Nalu.ReadNext())
			{
				DWORD	dwNalLength = 
					((Nalu.GetDataLength() >> 24) & 0x000000ff) |
					((Nalu.GetDataLength() >>  8) & 0x0000ff00) |
					((Nalu.GetDataLength() <<  8) & 0x00ff0000) |
					((Nalu.GetDataLength() << 24) & 0xff000000);		//reverse byte order

				CAutoPtr<Packet> p3(DNew Packet());
				
				p3->SetCount (Nalu.GetDataLength()+sizeof(dwNalLength));
				memcpy (p3->GetData(), &dwNalLength, sizeof(dwNalLength));
				memcpy (p3->GetData()+sizeof(dwNalLength), Nalu.GetDataBuffer(), Nalu.GetDataLength());
				
				if (p2 == NULL)
					p2 = p3;
				else
					p2->Append(*p3);
			}

			// now p2 is all nalus recieved yet
			p2->TrackNumber = m_p->TrackNumber;
			p2->bDiscontinuity = m_p->bDiscontinuity;
			m_p->bDiscontinuity = FALSE;

			p2->bSyncPoint = m_p->bSyncPoint;
			m_p->bSyncPoint = FALSE;

			p2->rtStart = m_p->rtStart; m_p->rtStart = Packet::INVALID_TIME;
			p2->rtStop = m_p->rtStop; m_p->rtStop = Packet::INVALID_TIME;

			p2->pmt = m_p->pmt; m_p->pmt = NULL;

			m_pl.AddTail(p2);

			if(p->rtStart != Packet::INVALID_TIME)
			{
				m_p->rtStart = p->rtStart;
				m_p->rtStop = p->rtStop;
				p->rtStart = Packet::INVALID_TIME;
			}
			if(p->bDiscontinuity)
			{
				m_p->bDiscontinuity = p->bDiscontinuity;
				p->bDiscontinuity = FALSE;
			}
			if(p->bSyncPoint)
			{
				m_p->bSyncPoint = p->bSyncPoint;
				p->bSyncPoint = FALSE;
			}
			if(m_p->pmt)
				DeleteMediaType(m_p->pmt);
			
			m_p->pmt = p->pmt;
			p->pmt = NULL;

			start = next;
		}
		if(start > m_p->GetData())
		{
			m_p->RemoveAt(0, start - m_p->GetData());
		}

		for(POSITION pos = m_pl.GetHeadPosition(); pos; m_pl.GetNext(pos))
		{
			if(pos == m_pl.GetHeadPosition()) 
				continue;

			Packet* pPacket = m_pl.GetAt(pos);
			BYTE* pData = pPacket->GetData();

			if((pData[4]&0x1f) == 0x09) m_fHasAccessUnitDelimiters = true;

			if((pData[4]&0x1f) == 0x09 || !m_fHasAccessUnitDelimiters && pPacket->rtStart != Packet::INVALID_TIME)
			{
				p = m_pl.RemoveHead();

				while(pos != m_pl.GetHeadPosition())
				{
					CAutoPtr<Packet> p2 = m_pl.RemoveHead();
					p->Append(*p2);
				}

				// p is a delimeter now			
				HRESULT hr = S_OK;
				if ( ((CMpegSplitterFilter*)m_pFilter)->m_mvc_found
					&& (p->TrackNumber == 0x1011 ))
				{
					hr = DeliverMVCPacket(p);
				}
				else
					hr = __super::DeliverPacket(p);

				if(hr != S_OK) return hr;
			}
		}

		return S_OK;		// packet cached in output pin;
	}
	else if(m_mt.subtype == FOURCCMap('1CVW') || m_mt.subtype == FOURCCMap('1cvw')) // just like aac, this has to be starting nalus, more can be packed together
	{
		if(!m_p)
		{
			m_p.Attach(DNew Packet());
			m_p->TrackNumber = p->TrackNumber;
			m_p->bDiscontinuity = p->bDiscontinuity;
			p->bDiscontinuity = FALSE;

			m_p->bSyncPoint = p->bSyncPoint;
			p->bSyncPoint = FALSE;

			m_p->rtStart = p->rtStart;
			p->rtStart = Packet::INVALID_TIME;

			m_p->rtStop = p->rtStop;
			p->rtStop = Packet::INVALID_TIME;
		}

		m_p->Append(*p);

		BYTE* start = m_p->GetData();
		BYTE* end = start + m_p->GetCount();

		bool bSeqFound = false;
		while(start <= end-4)
		{
			if (*(DWORD*)start == 0x0D010000)
			{
				bSeqFound = true;
				break;
			}
			else if (*(DWORD*)start == 0x0F010000)
				break;
			start++;
		}

		while(start <= end-4)
		{
			BYTE* next = start+1;

			while(next <= end-4)
			{
				if (*(DWORD*)next == 0x0D010000)
				{
					if (bSeqFound) break;
					bSeqFound = true;
				}
				else if (*(DWORD*)next == 0x0F010000)
					break;
				next++;
			}

			if(next >= end-4) break;

			int size = next - start - 4;
			UNUSED_ALWAYS(size);

			CAutoPtr<Packet> p2(DNew Packet());
			p2->TrackNumber = m_p->TrackNumber;
			p2->bDiscontinuity = m_p->bDiscontinuity;
			m_p->bDiscontinuity = FALSE;

			p2->bSyncPoint = m_p->bSyncPoint;
			m_p->bSyncPoint = FALSE;

			p2->rtStart = m_p->rtStart;
			m_p->rtStart = Packet::INVALID_TIME;

			p2->rtStop = m_p->rtStop;
			m_p->rtStop = Packet::INVALID_TIME;

			p2->pmt = m_p->pmt;
			m_p->pmt = NULL;

			p2->SetData(start, next - start);

			HRESULT hr = __super::DeliverPacket(p2);
			if(hr != S_OK) return hr;

			if(p->rtStart != Packet::INVALID_TIME)
			{
				m_p->rtStart = p->rtStart;
				m_p->rtStop = p->rtStop;
				p->rtStart = Packet::INVALID_TIME;
			}
			if(p->bDiscontinuity)
			{
				m_p->bDiscontinuity = p->bDiscontinuity;
				p->bDiscontinuity = FALSE;
			}
			if(p->bSyncPoint)
			{
				m_p->bSyncPoint = p->bSyncPoint;
				p->bSyncPoint = FALSE;
			}
			if(m_p->pmt)
				DeleteMediaType(m_p->pmt);
			
			m_p->pmt = p->pmt;
			p->pmt = NULL;

			start = next;
			bSeqFound = (*(DWORD*)start == 0x0D010000);
		}

		if(start > m_p->GetData())
		{
			m_p->RemoveAt(0, start - m_p->GetData());
		}

		return S_OK;
	}
	// DTS HD MA data is causing trouble with some filters, lets just remove it
	else if (m_bFilterDTSMA && ((m_mt.subtype == MEDIASUBTYPE_DTS || m_mt.subtype == MEDIASUBTYPE_WAVE_DTS)))
	{
		BYTE* start = p->GetData();
		BYTE* end = start + p->GetCount();
		if (end - start < 4 && !p->pmt)
			return S_OK;  // Should be invalid packet

		BYTE* hdr = start;


		int Type;
		  // 16 bits big endian bitstream
		  if      (hdr[0] == 0x7f && hdr[1] == 0xfe &&
				   hdr[2] == 0x80 && hdr[3] == 0x01)
			Type = 16 + 32;

		  // 16 bits low endian bitstream
		  else if (hdr[0] == 0xfe && hdr[1] == 0x7f &&
				   hdr[2] == 0x01 && hdr[3] == 0x80)
			Type = 16;

		  // 14 bits big endian bitstream
		  else if (hdr[0] == 0x1f && hdr[1] == 0xff &&
				   hdr[2] == 0xe8 && hdr[3] == 0x00 &&
				   hdr[4] == 0x07 && (hdr[5] & 0xf0) == 0xf0)
			Type = 14 + 32;

		  // 14 bits low endian bitstream
		  else if (hdr[0] == 0xff && hdr[1] == 0x1f &&
				   hdr[2] == 0x00 && hdr[3] == 0xe8 &&
				  (hdr[4] & 0xf0) == 0xf0 && hdr[5] == 0x07)
			Type = 14;

		  // no sync
		  else if (!p->pmt)
		  {
			  return S_OK;
		  }
	}
	else if (m_mt.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO)
	{
		BYTE* start = p->GetData();
		p->SetData(start + 4, p->GetCount() - 4);
	}
	else
	{
		m_p.Free();
		m_pl.RemoveAll();
	}

	return __super::DeliverPacket(p);
}


STDMETHODIMP CMpegSplitterOutputPin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
	HRESULT		hr;
	PIN_INFO	PinInfo;
	GUID		FilterClsid;

	if (SUCCEEDED (pReceivePin->QueryPinInfo (&PinInfo)))
	{
		if (SUCCEEDED (PinInfo.pFilter->GetClassID(&FilterClsid)))
		{
			if (FilterClsid == CLSID_DMOWrapperFilter)
				(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(true);
			// my12doom: it support now
			// AC3 Filter did not support DTS-MA
			else if (FilterClsid == CLSID_AC3Filter)
				m_bFilterDTSMA = true;

			if (FilterClsid != CLSID_AC3Filter)
				m_bFilterDTSMA = false;
		}
		PinInfo.pFilter->Release();
	}

	hr = __super::Connect (pReceivePin, pmt);
	(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(false);
	return hr;
}
