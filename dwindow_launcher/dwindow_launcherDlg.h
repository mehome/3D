// dwindow_launcherDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// Cdwindow_launcherDlg �Ի���
class Cdwindow_launcherDlg : public CDialog
{
// ����
public:
	Cdwindow_launcherDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DWINDOW_LAUNCHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	bool kill_ssp(bool kill);
	bool check_module();

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton2();
	CButton m_regsvr_button;
	afx_msg void OnBnClickedButton5();
};
