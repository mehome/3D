#include "CFileBuffer.h"
/*
class CQueue
{
public:
	unsigned char *m_buffer;
	CQueue(int size)
	{
		m_buffer = (unsigned char*)malloc(size);
		LPos = RPos = 0;
		BUFFER_SIZE = size;
	}
	~CQueue()
	{
		free(m_buffer);
	}

	bool PutBuffer(BYTE*buff, DWORD size)
	{
		if (RPos - LPos + size > BUFFER_SIZE)
		{
			//buff满，丢弃
			return false;
		}
		mPut((const char*)buff, size, RPos);
		RPos += size;
		if (LPos > BUFFER_SIZE*2 && RPos > BUFFER_SIZE*2)
		{
			LPos %= BUFFER_SIZE*2;
			RPos %= BUFFER_SIZE*2;
		}
	}
	DWORD GetBuffer(BYTE*buff, DWORD size)
	{		
		size = min(RPos - LPos, size);
		if (size>0)
		{
			if (buff)
			{
				mGet((char*)buff, size, LPos);
			}
			LPos += size;
		}

		if (LPos > BUFFER_SIZE*2 && RPos > BUFFER_SIZE*2)
		{
			LPos %= BUFFER_SIZE*2;
			RPos %= BUFFER_SIZE*2;
		}
		return size;
	}

private:
	int BUFFER_SIZE;
	int LPos;//=0
	int RPos;//=0
	void mPut(const char *in, int size, int start)
	{
		if (size > BUFFER_SIZE|| size ==0) return ;
		int end = start+size;
		//循环判定
		if (end % BUFFER_SIZE > start % BUFFER_SIZE)
		{
			memcpy(m_buffer+start % BUFFER_SIZE, in, size);
		}
		else
		{
			memcpy(m_buffer+start % BUFFER_SIZE, in, BUFFER_SIZE - start % BUFFER_SIZE);
			memcpy(m_buffer, in + BUFFER_SIZE - start % BUFFER_SIZE, size - BUFFER_SIZE + start % BUFFER_SIZE);
		}
	}
	void mGet(char *in, int size, int start)
	{
		if (size > BUFFER_SIZE || size ==0) return ;
		int end = start+size;
		//循环判定
		if (end % BUFFER_SIZE > start % BUFFER_SIZE)
		{
			memcpy(in, m_buffer+start % BUFFER_SIZE, size);
		}
		else
		{
			memcpy(in,m_buffer+start % BUFFER_SIZE, BUFFER_SIZE - start % BUFFER_SIZE);
			memcpy( in + BUFFER_SIZE - start % BUFFER_SIZE,m_buffer, size - BUFFER_SIZE + start % BUFFER_SIZE);

		}
	}
	void ResetBuffer();
};
*/

CFileBuffer::CFileBuffer(int buffer_size):
m_buffer_size(buffer_size),
m_data_size(0),
m_data_start(0),
no_more_data(false),
no_more_remove(false)
{
	m_the_buffer = new BYTE[buffer_size];
}

CFileBuffer::~CFileBuffer()
{
	delete [] m_the_buffer;
}

void CFileBuffer::mPut(const unsigned char *in, int size, int start)
{
	if (size > m_buffer_size|| size ==0) return ;
	int end = start+size;
	//循环判定
	if (end % m_buffer_size > start % m_buffer_size)
	{
		memcpy(m_the_buffer+start % m_buffer_size, in, size);
	}
	else
	{
		memcpy(m_the_buffer+start % m_buffer_size, in, m_buffer_size - start % m_buffer_size);
		memcpy(m_the_buffer, in + m_buffer_size - start % m_buffer_size, size - m_buffer_size + start % m_buffer_size);
	}
}
void CFileBuffer::mGet(unsigned char *in, int size, int start)
{
	if (size > m_buffer_size || size ==0) return ;
	int end = start+size;
	//循环判定
	if (end % m_buffer_size > start % m_buffer_size)
	{
		memcpy(in, m_the_buffer+start % m_buffer_size, size);
	}
	else
	{
		memcpy(in,m_the_buffer+start % m_buffer_size, m_buffer_size - start % m_buffer_size);
		memcpy( in + m_buffer_size - start % m_buffer_size,m_the_buffer, size - m_buffer_size + start % m_buffer_size);
	}
}

void CFileBuffer::resort()
{
	unsigned char * tmp = (unsigned char*) malloc(m_data_size);
	mGet(tmp, m_data_size, m_data_start);
	mPut(tmp, m_data_size, 0);
	m_data_start = 0;
	free(tmp);
}
int CFileBuffer::wait_for_data(int size)
{
retry:
	cs.Lock();	
	if (no_more_data)
	{
		if (m_data_start + size >= m_buffer_size)
			resort();
		cs.Unlock();
		return min(size, m_data_size);
	}
	
	if (m_data_size < size)
	{
		cs.Unlock();
		Sleep(10);
		goto retry;
	}

	// see if need resort
	if (m_data_start + size >= m_buffer_size)
		resort();

	cs.Unlock();

	return size;
}

int CFileBuffer::remove_data(int size)
{
	cs.Lock();

	size = min(size, m_data_size);
	//memmove(m_the_buffer, m_the_buffer+size, m_data_size-size);
	m_data_size -= size;
	m_data_start += size;
	m_data_start %= m_buffer_size;

	cs.Unlock();

	return 0;
}

int CFileBuffer::insert(int size, const BYTE*buf)
{
retry:
	cs.Lock();

	if (no_more_remove)
	{
		cs.Unlock();
		return 0;
	}

	if (size + m_data_size > m_buffer_size)
	{
		cs.Unlock();		// wait for enough space
		Sleep(10);
		goto retry;
	}

	//memcpy(m_the_buffer+m_data_size, buf, size);
	mPut(buf, size, m_data_start + m_data_size);

	m_data_size += size;

	cs.Unlock();
	return 0;
}

int CFileBuffer::remove(int size, BYTE *buf)
{
	cs.Lock();

	if (m_data_size < size)
	{
		cs.Unlock();
		return -1;	// not enough data, please retry
	}

	//memcpy(buf, m_the_buffer, size);
	//memmove(m_the_buffer, m_the_buffer+size, m_data_size-size);
	mGet(buf, size, m_data_start);
	m_data_size -= size;
	m_data_start += size;
	m_data_start %= m_buffer_size;

	cs.Unlock();

	return 0;
}

int CFileBuffer::block_remove(int size, BYTE *buf)
{

retry:
	int result;
	result = remove(size, buf);

	if (result == 0)
		return size;		//remove OK

	if (result == -1 && !no_more_data) 
	{
		// wait for data
		Sleep(10);
		goto retry;
	}

	if (result == -1 && no_more_data)
	{
		// get the only data
		memcpy(buf, m_the_buffer, m_data_size);

		int got = m_data_size;
		m_data_size = 0;

		return got;
	}

	return 0;
}
