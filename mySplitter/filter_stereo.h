#include <streams.h>
#include <dvdmedia.h>
#include "filter.h"
#include "image.h"
#include "splitbase.h"

class CYV12StereoMixer : public CSplitFilter, public IYV12Mixer, public image_processor_stereo
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IYV12StereoMixer and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Overrriden from CSplitFilter base class
	HRESULT Split(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckSplit(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming();

	// Implementation of  the custom IYV12StereoMixer interface
	HRESULT STDMETHODCALLTYPE SetCallback(IDWindowFilterCB * cb);
	HRESULT STDMETHODCALLTYPE SetMode(int mode, int extend, int max_mask_buffer = -1);	// ������input����֮ǰ�趨
	HRESULT STDMETHODCALLTYPE SetColor(DWORD color);									// ���ҽ�������һ֡��Ч
	HRESULT STDMETHODCALLTYPE Revert();													// ���ҽ�������һ֡��Ч
	HRESULT STDMETHODCALLTYPE GetLetterboxHeight(int *max_delta);						// ��úڱ��ܸ߶�
	HRESULT STDMETHODCALLTYPE SetLetterbox(int delta);									// �趨�Ϻڱ߱��ºڱ߿���٣������ɸ�����һ֡��Ч
	HRESULT STDMETHODCALLTYPE SetMask(unsigned char *mask,								//
		int width, int height,															//���࣬��һ���Զ�����
		int left, int top)	;															//
	HRESULT STDMETHODCALLTYPE SetMaskPos(int left, int top,	int offset);				// λ��Ҳ���Ե���Ϊ�ο�ϵ����ɫΪAYUV��0-255���ռ�
																						// offsetΪ����-���ۣ������ɸ�

private:
	REFERENCE_TIME m_this_stream_start;

	CCritSec m_YV12StereoMixerLock;
	bool m_revert;
	IDWindowFilterCB *m_cb;

	CYV12StereoMixer(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CYV12StereoMixer();
};