#include <streams.h>
#include <dvdmedia.h>
#include "filter.h"
#include "image.h"

class CYV12MonoMixer : public CTransformFilter, public IYV12Mixer, public image_processor_mono
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IYV12MonoMixer and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// CTransformFilter Bug FIX
	STDMETHODIMP Pause(){CAutoLock lock_it(m_pLock);return CTransformFilter::Pause();}
	STDMETHODIMP Stop(){CAutoLock lock_it(m_pLock);return CTransformFilter::Stop();}

	// Overrriden from CSplitFilter base class
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming();


	// Implementation of  the custom IYV12MonoMixer interface
	HRESULT STDMETHODCALLTYPE SetCallback(IDWindowFilterCB * cb);
	HRESULT STDMETHODCALLTYPE SetMode(int mode, int extend, int max_mask_buffer = -1);	// ������input����֮ǰ�趨
	HRESULT STDMETHODCALLTYPE SetColor(DWORD color);									// 
	HRESULT STDMETHODCALLTYPE Revert();													// ��Ч
	HRESULT STDMETHODCALLTYPE GetLetterboxHeight(int *max_delta);						// ��úڱ��ܸ߶�
	HRESULT STDMETHODCALLTYPE SetLetterbox(int delta);									// �趨�Ϻڱ߱��ºڱ߿���٣������ɸ�����һ֡��Ч
	HRESULT STDMETHODCALLTYPE SetMask(unsigned char *mask,								//
		int width, int height,															//
		int left, int top)	;															//
	HRESULT STDMETHODCALLTYPE SetMaskPos(int left, int top,int offset);					// λ��Ҳ���Ե���Ϊ�ο�ϵ����ɫΪAYUV��0-255���ռ�
																						// offsetΪ����-���ۣ������ɸ�

private:
	REFERENCE_TIME m_this_stream_start;

	CCritSec m_YV12MonoMixerLock;
	IDWindowFilterCB *m_cb;

	CYV12MonoMixer(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CYV12MonoMixer();
};