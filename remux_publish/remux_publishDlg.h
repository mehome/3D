// remux_publishDlg.h : ͷ�ļ�
//

#pragma once


// Cremux_publishDlg �Ի���
class Cremux_publishDlg : public CDialog
{
// ����
public:
	Cremux_publishDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_REMUX_PUBLISH_DIALOG };

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


	// remux control:
	wchar_t pathname[MAX_PATH];
	int check_result;
	void publish_signature(const DWORD *checksum, DWORD *signature);
	void check_file();
public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBnClickedButton2();
};
