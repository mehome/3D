// active_dwindowDlg.h : ͷ�ļ�
//

#pragma once


// Cactive_dwindowDlg �Ի���
class Cactive_dwindowDlg : public CDialog
{
// ����
public:
	Cactive_dwindowDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_ACTIVE_DWINDOW_DIALOG };

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
	afx_msg void OnBnClickedButton1();
	CString m_password;
	afx_msg void OnBnClickedButton2();
};
