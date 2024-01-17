
// MultiGattDlg.h : header file
//

#pragma once

#include "ClientWatcher.h"


// CMultiGattDlg dialog
class CMultiGattDlg : public CDialogEx
{
// Construction
public:
	CMultiGattDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MULTIGATT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	CButton btStart;
	CButton btStop;
	CButton btDisconnect;
	CListCtrl lvDevices;
	CEdit edData;
	CButton btRead;
	CButton btSend;
	CListBox lbLog;

private:
	typedef enum {
		dsFound,
		dsConnecting,
		dsConnected
	} TDeviceStatus;

	CwclBluetoothManager* FManager;
	CClientWatcher*	FWatcher;

	void UpdateButtons();

	void ManagerAfterOpen(void* Sender);
	void ManagerBeforeClose(void* Sender);
	void ManagerClosed(void* Sender);

	void WatcherClientDisconnected(const __int64 Address, const int Reason);
	void WatcherConnectionCompleted(const __int64 Address, const int Result);
	void WatcherConnectionStarted(const __int64 Address, const int Result);
	void WatcherDeviceFound(const __int64 Address, const tstring& Name);
	void WatcherStopped(void* Sender);
	void WatcherStarted(void* Sender);
	void WatcherValueChanged(const __int64 Address, const unsigned char* Value,
		const unsigned long Length);

public:
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonDisconnect();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonRead();
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnLvnItemchangedListDevices(NMHDR *pNMHDR, LRESULT *pResult);
};
