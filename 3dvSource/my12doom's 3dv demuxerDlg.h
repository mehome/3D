// my12doom's 3dv demuxerDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"


// Cmy12dooms3dvdemuxerDlg �Ի���
class Cmy12dooms3dvdemuxerDlg : public CDialog
{
// ����
public:
	Cmy12dooms3dvdemuxerDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MY12DOOMS3DVDEMUXER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;
	
	void cancel_work();
	bool cancel;
	HANDLE m_thread;
	static DWORD WINAPI worker_thread(LPVOID param);

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	CProgressCtrl m_progress;
	afx_msg void OnBnClickedButton1();
};
