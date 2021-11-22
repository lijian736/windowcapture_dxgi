
// WindowCapturerDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "capture_window_dxgi.h"

// CWindowCapturerDlg �Ի���
class CWindowCapturerDlg : public CDialogEx
{
// ����
public:
	CWindowCapturerDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WINDOWCAPTURER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnBnClickedBtnCapture();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnDestroy();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	static DWORD WINAPI capture_thread(LPVOID ptr);
	void capture_thread_run();
	void on_capture(unsigned char* data, unsigned long len, unsigned int width, unsigned int height);

private:
	BOOL m_has_error;

	//the capture thread
	HANDLE m_capture_thread_handle;
	//the stop capture event
	HANDLE m_stop_capture_event_handle;
};
