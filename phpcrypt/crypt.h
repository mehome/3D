// crypt.h : Ccrypt ������

#pragma once
#include "resource.h"       // ������

#include "phpcrypt_i.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Windows CE ƽ̨(�粻�ṩ��ȫ DCOM ֧�ֵ� Windows Mobile ƽ̨)���޷���ȷ֧�ֵ��߳� COM ���󡣶��� _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA ��ǿ�� ATL ֧�ִ������߳� COM ����ʵ�ֲ�����ʹ���䵥�߳� COM ����ʵ�֡�rgs �ļ��е��߳�ģ���ѱ�����Ϊ��Free����ԭ���Ǹ�ģ���Ƿ� DCOM Windows CE ƽ̨֧�ֵ�Ψһ�߳�ģ�͡�"
#endif



// Ccrypt
#include "..\libchecksum\libchecksum.h"

class ATL_NO_VTABLE Ccrypt :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Ccrypt, &CLSID_crypt>,
	public IDispatchImpl<Icrypt, &IID_Icrypt, &LIBID_phpcryptLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	Ccrypt()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CRYPT)


BEGIN_COM_MAP(Ccrypt)
	COM_INTERFACE_ENTRY(Icrypt)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

	STDMETHOD(test)(BSTR* ret);
	STDMETHOD(test2)(BSTR* rtn);
	STDMETHOD(get_passkey)(BSTR input, BSTR* ret);
	STDMETHOD(get_hash)(BSTR input, BSTR* rtn);
	STDMETHOD(get_key)(BSTR input, BSTR* ret);
	STDMETHOD(decode_message)(BSTR input, BSTR* ret);
	STDMETHOD(AES)(BSTR data, BSTR key, BSTR* ret);
protected:
public:
	STDMETHOD(gen_key)(void);
	STDMETHOD(gen_keys)(BSTR passkey, DATE time_start, DATE time_end, BSTR* out);
	STDMETHOD(gen_keys_int)(BSTR passkey, ULONG time_start, ULONG time_end, BSTR* out);
	STDMETHOD(genkeys)(BSTR passkey, LONG time_start, LONG time_end, BSTR* out);
	STDMETHOD(decode_binarystring)(BSTR in, BSTR* out);
	STDMETHOD(SHA1)(BSTR in, BSTR* out);
	STDMETHOD(genkeys2)(BSTR passkey, LONG time_start, LONG time_end, LONG max_bar_user, BSTR* out);
	STDMETHOD(genkey3)(BSTR passkey, LONG time_start, BSTR time_end, LONG max_bar_user, BSTR* out);
	STDMETHOD(genkey4)(BSTR passkey, LONG time_start, LONG time_end, LONG max_bar_user, LONG user_type, BSTR* out);
	STDMETHOD(genkey5)(BSTR passkey, LONG time_start, LONG time_end, LONG max_bar_user, LONG user_type, LONG user_rights, BSTR* out);
	STDMETHOD(gen_freekey)(LONG time_start, LONG time_end, BSTR* out);
};

OBJECT_ENTRY_AUTO(__uuidof(crypt), Ccrypt)
