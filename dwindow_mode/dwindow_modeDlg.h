// dwindow_modeDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// Cdwindow_modeDlg �Ի���
class Cdwindow_modeDlg : public CDialog
{
// ����
public:
	Cdwindow_modeDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DWINDOW_MODE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRadio1();
	bool m_check_normal;
	bool m_check_bar;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
protected:
	CString m_server;
public:
	int m_port;
	CEdit m_server_edit;
	CEdit m_port_edit;
	afx_msg void OnBnClickedRadio2();
	afx_msg void OnBnClickedOk();
};
