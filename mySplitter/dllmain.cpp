#ifdef _DEBUG
#pragma comment( lib, "strmbasd" )
#else
#pragma comment( lib, "strmbase" )
#endif

#pragma comment( lib, "strmiids" )
#pragma comment( lib, "winmm" )

#include <tchar.h>
#include <streams.h>
#include <initguid.h>

#include "filter_stereo.h"
#include "filter_mono.h"
#include "extender.h"
#include "ssp.h"
#include "audio_downmix.h"
#include "j2k_decoder.h"
#include "hevcDecoder.h"

//////////////////////////////////////////////////////////////////////////
// This file contains the standard COM glue code required for registering the 
// YV12StereoMixer filter 
//////////////////////////////////////////////////////////////////////////

#define g_wszYV12StereoMixer L"YV12 Stereo Mixer"
#define g_wszYV12MonoMixer   L"YV12 Mono Mixer"
#define g_wszDWindowExtenderMono   L"DWindow Extender Mono"
#define g_wszDWindowExtenderStereo   L"DWindow Extender Stereo"
#define g_wszDWindowSSP L"DWindow SSP filter"
#define g_wszDWindowAudioDownmix L"DWindow Audio Downmix"

// Filter setup data
const AMOVIESETUP_MEDIATYPE sudPinTypes = { &MEDIATYPE_Video, &MEDIASUBTYPE_NULL};
const AMOVIESETUP_MEDIATYPE sudPinTypesSSP = { &MEDIATYPE_Video, &MEDIASUBTYPE_YUY2};
const AMOVIESETUP_MEDIATYPE sudPinTypesAudio = {&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL};

const AMOVIESETUP_PIN sudYV12StereoMixerPins[] =
{
    { 
        L"Input",             // Pins string name
        FALSE,                // Is it rendered
        FALSE,                // Is it an output
        FALSE,                // Are we allowed none
        FALSE,                // And allowed many
        &CLSID_NULL,          // Connects to filter
        NULL,                 // Connects to pin
        1,                    // Number of types
        &sudPinTypes          // Pin information
    },
    { 
        L"Output1",            // Pins string name
		FALSE,                // Is it rendered
        TRUE,                 // Is it an output
        FALSE,                // Are we allowed none
        FALSE,                // And allowed many
        &CLSID_NULL,          // Connects to filter
        NULL,                 // Connects to pin
        1,                    // Number of types
        &sudPinTypes          // Pin information
    },
	{ 
		L"Output2",            // Pins string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	}
};


const AMOVIESETUP_PIN sudDWindowExtenderStereoPins[] =
{
	{ 
		L"Input",             // Pins string name
		FALSE,                // Is it rendered
		FALSE,                // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	},
	{ 
		L"Output1",           // Pins string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	},
	{ 
		L"Output2",           // Pins string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	}
};

const AMOVIESETUP_PIN sudYV12MonoMixerPins[] =
{
	{ 
		L"Input",             // Pins string name
		FALSE,                // Is it rendered
		FALSE,                // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	},
	{ 
		L"Output",            // Pins string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	}
};


const AMOVIESETUP_PIN sudDWindowExtenderMonoPins[] =
{
	{ 
		L"Input",             // Pins string name
		FALSE,                // Is it rendered
		FALSE,                // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	},
	{ 
		L"Output",            // Pins string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypes          // Pin information
	}
};

const AMOVIESETUP_PIN sudDWindowSSPPins[] =
{
	{ 
		L"Input",             // Pins string name
		FALSE,                // Is it rendered
		FALSE,                // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypesSSP          // Pin information
	},
	{ 
		L"Output",            // Pins string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		1,                    // Number of types
		&sudPinTypesSSP          // Pin information
	}
};


const AMOVIESETUP_PIN sudDWindowAudiodownmixPins[] =
{
	{ 
		L"Input",             // Pins string name
			FALSE,                // Is it rendered
			FALSE,                // Is it an output
			FALSE,                // Are we allowed none
			FALSE,                // And allowed many
			&CLSID_NULL,          // Connects to filter
			NULL,                 // Connects to pin
			1,                    // Number of types
			&sudPinTypesAudio     // Pin information
	},
	{ 
		L"Output",				  // Pins string name
			FALSE,                // Is it rendered
			TRUE,                 // Is it an output
			FALSE,                // Are we allowed none
			FALSE,                // And allowed many
			&CLSID_NULL,          // Connects to filter
			NULL,                 // Connects to pin
			1,                    // Number of types
			&sudPinTypesAudio     // Pin information
		}
};
const AMOVIESETUP_FILTER sudYV12StereoMixer =
{
    &CLSID_YV12StereoMixer,				// Filter CLSID
    g_wszYV12StereoMixer,				// String name
    MERIT_DO_NOT_USE,					// Filter merit
    3,									// Number of pins
    sudYV12StereoMixerPins,				// Pin information
};


const AMOVIESETUP_FILTER sudYV12MonoMixer =
{
	&CLSID_YV12MonoMixer,				// Filter CLSID
	g_wszYV12MonoMixer,					// String name
	MERIT_DO_NOT_USE,					// Filter merit
	2,									// Number of pins
	sudYV12MonoMixerPins,				// Pin information
};


const AMOVIESETUP_FILTER sudDWindowExtenderMono =
{
	&CLSID_DWindowMono,					// Filter CLSID
	g_wszDWindowExtenderMono,			// String name
	MERIT_DO_NOT_USE,					// Filter merit
	2,									// Number of pins
	sudDWindowExtenderMonoPins,			// Pin information
};

const AMOVIESETUP_FILTER sudDWindowExtenderStereo =
{
	&CLSID_DWindowStereo,				// Filter CLSID
	g_wszDWindowExtenderStereo,			// String name
	MERIT_DO_NOT_USE,					// Filter merit
	3,									// Number of pins
	sudDWindowExtenderStereoPins,		// Pin information
};

const AMOVIESETUP_FILTER sudDWindowSSP =
{
	&CLSID_DWindowSSP,					// Filter CLSID
	g_wszDWindowSSP,					// String name
	MERIT_DO_NOT_USE,					// Filter merit
	2,									// Number of pins
	sudDWindowSSPPins,					// Pin information
};

const AMOVIESETUP_FILTER sudDWindowAudio =
{
	&CLSID_DWindowAudioDownmix,			// Filter CLSID
	g_wszDWindowAudioDownmix,			// String name
	MERIT_DO_NOT_USE,					// Filter merit
	2,									// Number of pins
	sudDWindowAudiodownmixPins,					// Pin information
};

const AMOVIESETUP_FILTER sudJ2KDecoder =
{
	&CLSID_my12doomJ2KDecoder,			// Filter CLSID
	L"my12doom's JPEG2000 Decoder",			// String name
	MERIT_DO_NOT_USE,					// Filter merit
	2,									// Number of pins
	sudDWindowExtenderMonoPins,					// Pin information
};

const AMOVIESETUP_FILTER sudHEVCDecoder =
{
	&CLSID_OpenHEVCDecoder,			// Filter CLSID
	L"DWindow HEVC/H265 Decoder",			// String name
	MERIT_NORMAL,					// Filter merit
	2,									// Number of pins
	sudDWindowExtenderMonoPins,					// Pin information
};

CFactoryTemplate g_Templates[] = 
{
    //{ g_wszYV12StereoMixer, &CLSID_YV12StereoMixer, CYV12StereoMixer::CreateInstance, NULL, &sudYV12StereoMixer },
	//{ g_wszYV12MonoMixer,	&CLSID_YV12MonoMixer,	CYV12MonoMixer::CreateInstance,   NULL, &sudYV12MonoMixer	},
	{ g_wszDWindowExtenderMono,	&CLSID_DWindowMono,	CDWindowExtenderMono::CreateInstance,   NULL, &sudDWindowExtenderMono	},
	{ g_wszDWindowExtenderStereo,	&CLSID_DWindowStereo,	CDWindowExtenderStereo::CreateInstance,   NULL, &sudDWindowExtenderStereo	},
	{ g_wszDWindowSSP,	&CLSID_DWindowSSP,	CDWindowSSP::CreateInstance,   NULL, &sudDWindowSSP	},
	{ g_wszDWindowAudioDownmix,	&CLSID_DWindowAudioDownmix,	CDWindowAudioDownmix::CreateInstance,   NULL, &sudDWindowAudio	},
#ifndef NOJ2K
	{ L"my12doom's JPEG2000 Decoder",	&CLSID_my12doomJ2KDecoder,	CJ2kDecoder::CreateInstance,   NULL, &sudJ2KDecoder	},
#endif
	{ L"DWindow OpenHEVC Decoder",	&CLSID_OpenHEVCDecoder,	CHEVCDecoder::CreateInstance,   NULL, &sudHEVCDecoder	},

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    

////////////////////////////////////////////////////////////////////////
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
#ifndef DEBUG
// 	return E_FAIL;
#endif
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

