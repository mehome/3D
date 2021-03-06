#ifndef AVSMAIN_H
#define AVSMAIN_H

// extern "C"
#ifdef __cplusplus
extern "C" {
#endif

// a global varable for load balance
extern float buffer_load;

// declaration
void* create_avs();
int set_avs_resolution(void *avs, int width, int height, int fps, int fpsdenumorator);
int insert_frame(void *avs, void **pY, void **pV, void **pU, int view_id, int frame_num);
int insert_offset_metadata(void *avs, BYTE *data, int count);
int close_avs(void *avs);		// just set no_more_data = 1
int view_count(void *avs);

// C++ classes
#ifdef __cplusplus
}
#include "CCritSec.h"
#include "avisynth.h"
#include "m2ts_scanner.h"

typedef struct buffer_unit_struct
{
	int frame_number;
	BYTE *data;
} buffer_unit;

// my12doom's offset metadata format:
typedef struct struct_my12doom_offset_metadata_header
{
	DWORD file_header;		// should be 'offs'
	DWORD version;
	DWORD point_count;
	DWORD fps_numerator;
	DWORD fps_denumerator;
} my12doom_offset_metadata_header;

// point data is stored in 8bit signed integer, upper 1bit is sign bit, lower 7bit is integer bits
// int value = (v&0x80)? -(v&7f):(v&7f);



class CFrameBuffer
{
public:
	buffer_unit* the_buffer;
	int m_unit_count;
	int m_width;
	int m_height;

	int m_frame_count;		// total frame count
	int m_max_fn_recieved;	// = -1, max fn inserted yet
	int m_item_count;		// = 0;
	bool m_discard_all;		// discard all incoming packets, mainly for exitting 

	CCritSec cs;

	// functions
	CFrameBuffer();
	~CFrameBuffer();
	int init(int width, int height, int buffer_unit_count, int frame_count);
	int insert(int n, const BYTE*buf);
	int insert(int n, const BYTE **pY, const BYTE **pV, const BYTE **pU);
	int insert_no_number(const BYTE **pY, const BYTE **pV, const BYTE **pU);		// ldecod use some stupid 2D memory layout
	int remove(int n, BYTE *buf, int stride, int offset);
	// return value:// 0-n : success 
	// -1  : wrong range, no such data
	// -2  : feeder busy, should wait and retry for data
};

class JMAvs: public IClip 
{
public:
	HANDLE m_decoding_thread;

	VideoInfo vi;
	int m_frame_count;
	int m_width;
	int m_height;
	int m_buffer_unit_count;
	my12doom_offset_metadata_header m_header;
	FILE *m_offset_file;
	bool m_swap_eyes;

	CFrameBuffer *left_buffer; 
	CFrameBuffer *right_buffer;

	bool m_decoding_done; // a temp buffer

	char m_m2ts_left[40960];
	char m_m2ts_right[40960];

	JMAvs();
	int avs_init(const char*m2ts_left, IScriptEnvironment* env, const char*m2ts_right = NULL, const char * offset_out = NULL,
		const int frame_count = -1, int buffer_count = 10,
		int fps_numerator = 24000, int fps_denominator = 1001,
		bool swap_eyes = false);
	int ldecod_init(int width, int height, int fps, int fpsdenumorator);
	int insert_offset(BYTE *data, int count);
	virtual ~JMAvs();

	// avisynth virtual functions
	bool __stdcall GetParity(int n){return false;}
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
	const VideoInfo& __stdcall GetVideoInfo(){return vi;}
	void __stdcall SetCacheHints(int cachehints,int frame_range) {};

	// key function
	void get_frame(int n, CFrameBuffer *the_buffer, BYTE*data, int stride, int offset);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif
#endif