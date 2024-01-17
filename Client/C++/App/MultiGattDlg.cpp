
// MultiGattDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MultiGatt.h"
#include "MultiGattDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

__int64 StrToInt64(const CString& s)
{
	return _tcstoi64(s, NULL, 16);
}

CString IntToHex(const int i)
{
	CString s;
	s.Format(_T("%.8X"), i);
	return s;
}

CString IntToHex(const __int64 i)
{
	CString s;
	s.Format(_T("%.4X%.8X"), static_cast<INT32>((i >> 32) & 0x00000FFFF),
		static_cast<INT32>(i) & 0xFFFFFFFF);
	return s;
}

CString IntToStr(const unsigned long i)
{
	CString s;
	s.Format(_T("%d"), i);
	return s;
}

// CMultiGattDlg dialog



CMultiGattDlg::CMultiGattDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MULTIGATT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMultiGattDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_START, btStart);
	DDX_Control(pDX, IDC_BUTTON_STOP, btStop);
	DDX_Control(pDX, IDC_BUTTON_DISCONNECT, btDisconnect);
	DDX_Control(pDX, IDC_LIST_DEVICES, lvDevices);
	DDX_Control(pDX, IDC_EDIT_DATA, edData);
	DDX_Control(pDX, IDC_BUTTON_READ, btRead);
	DDX_Control(pDX, IDC_BUTTON_SEND, btSend);
	DDX_Control(pDX, IDC_LIST_LOG, lbLog);
}

BEGIN_MESSAGE_MAP(CMultiGattDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CMultiGattDlg::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CMultiGattDlg::OnBnClickedButtonStop)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CMultiGattDlg::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_START, &CMultiGattDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_READ, &CMultiGattDlg::OnBnClickedButtonRead)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CMultiGattDlg::OnBnClickedButtonSend)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DEVICES, &CMultiGattDlg::OnLvnItemchangedListDevices)
END_MESSAGE_MAP()


// CMultiGattDlg message handlers

BOOL CMultiGattDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	lvDevices.InsertColumn(0, _T("Address"), 0, 120);
	lvDevices.InsertColumn(1, _T("Name"), 0, 120);
	lvDevices.InsertColumn(2, _T("Status"), 0, 120);

	// Do not forget to change synchronization method.
	CwclMessageBroadcaster::SetSyncMethod(skThread);

	FManager = new CwclBluetoothManager();
	__hook(&CwclBluetoothManager::AfterOpen, FManager, &CMultiGattDlg::ManagerAfterOpen);
	__hook(&CwclBluetoothManager::BeforeClose, FManager, &CMultiGattDlg::ManagerBeforeClose);
	__hook(&CwclBluetoothManager::OnClosed, FManager, &CMultiGattDlg::ManagerClosed);

	FWatcher = new CClientWatcher();
	__hook(&CClientWatcher::OnClientDisconnected, FWatcher, &CMultiGattDlg::WatcherClientDisconnected);
	__hook(&CClientWatcher::OnConnectionCompleted, FWatcher, &CMultiGattDlg::WatcherConnectionCompleted);
	__hook(&CClientWatcher::OnConnectionStarted, FWatcher, &CMultiGattDlg::WatcherConnectionStarted);
	__hook(&CClientWatcher::OnDeviceFound, FWatcher, &CMultiGattDlg::WatcherDeviceFound);
	__hook(&CClientWatcher::OnValueChanged, FWatcher, &CMultiGattDlg::WatcherValueChanged);
	__hook(&CClientWatcher::OnStarted, FWatcher, &CMultiGattDlg::WatcherStarted);
	__hook(&CClientWatcher::OnStopped, FWatcher, &CMultiGattDlg::WatcherStopped);

	UpdateButtons();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMultiGattDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMultiGattDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMultiGattDlg::UpdateButtons()
{
	btStart.EnableWindow(!FWatcher->Monitoring);
	btStop.EnableWindow(FWatcher->Monitoring);

	POSITION Pos = lvDevices.GetFirstSelectedItemPosition();
	int Item = -1;
	if (Pos != NULL)
		Item = lvDevices.GetNextSelectedItem(Pos);

	TDeviceStatus Status = dsFound;
	if (Item != -1)
		Status = (TDeviceStatus)lvDevices.GetItemData(Item);

	btDisconnect.EnableWindow(Status == dsConnected);
	btRead.EnableWindow(Status == dsConnected);
	btSend.EnableWindow(Status == dsConnected && edData.GetWindowTextLength() > 0);
}

void CMultiGattDlg::OnBnClickedButtonClear()
{
	lbLog.ResetContent();
}

void CMultiGattDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	OnBnClickedButtonStop();

	__unhook(FWatcher);
	delete FWatcher;
	__unhook(FManager);
	delete FManager;
}

void CMultiGattDlg::OnBnClickedButtonDisconnect()
{
	POSITION Pos = lvDevices.GetFirstSelectedItemPosition();
	if (Pos != NULL)
	{
		int Item = lvDevices.GetNextSelectedItem(Pos);
		__int64 Address = StrToInt64(lvDevices.GetItemText(Item, 0));
		int Res = FWatcher->Disconnect(Address);
		if (Res != WCL_E_SUCCESS)
			AfxMessageBox(_T("Disconnect failed: 0x") + IntToHex(Res));
		UpdateButtons();
	}
}

void CMultiGattDlg::OnBnClickedButtonStart()
{

	int Res = FManager->Open();
	if (Res != WCL_E_SUCCESS)
		AfxMessageBox(_T("Open Bluetooth Manager failed: 0x") + IntToHex(Res));
	else
	{
		CwclBluetoothRadio* Radio;
		Res = FManager->GetLeRadio(Radio);
		if (Res != WCL_E_SUCCESS)
			AfxMessageBox(_T("Get working radio failed: 0x") + IntToHex(Res));
		else
		{
			Res = FWatcher->Start(Radio);
			if (Res != WCL_E_SUCCESS)
				AfxMessageBox(_T("Start Watcher failed: 0x") + IntToHex(Res));
		}

		if (Res != WCL_E_SUCCESS)
			FManager->Close();
	}
	UpdateButtons();
}

void CMultiGattDlg::OnBnClickedButtonStop()
{
	FWatcher->Stop();
	UpdateButtons();
}

void CMultiGattDlg::OnBnClickedButtonRead()
{
	POSITION Pos = lvDevices.GetFirstSelectedItemPosition();
	if (Pos != NULL)
	{
		int Item = lvDevices.GetNextSelectedItem(Pos);
		__int64 Address = StrToInt64(lvDevices.GetItemText(Item, 0));

		unsigned char* Data;
		unsigned long Length;
		int Res = FWatcher->ReadData(Address, Data, Length);
		if (Res != WCL_E_SUCCESS)
			AfxMessageBox(_T("Read failed: 0x") + IntToHex(Res));
		else
		{
			if (Data == NULL || Length == 0)
				AfxMessageBox(_T("Data is empty"));
			else
			{
				CStringA s;
				for (unsigned long i = 0; i < Length; i++)
					s += (char)Data[i];
				MessageBoxA(this->m_hWnd, "Data read: " + s, "Received data", 0);
			}

			if (Data != NULL)
				free(Data);
		}
	}
}

void CMultiGattDlg::OnBnClickedButtonSend()
{
	POSITION Pos = lvDevices.GetFirstSelectedItemPosition();
	if (Pos != NULL)
	{
		int Item = lvDevices.GetNextSelectedItem(Pos);
		__int64 Address = StrToInt64(lvDevices.GetItemText(Item, 0));

		CString s;
		edData.GetWindowText(s);
		unsigned long Length = s.GetLength() + 1;
		unsigned char* Data = (unsigned char*)malloc(Length);
		for (unsigned long i = 0; i < Length - 1; i++)
			Data[i] = (char)s[i + 1];
		int Res = FWatcher->WriteData(Address, Data, Length);
		if (Res != WCL_E_SUCCESS)
			AfxMessageBox(_T("Write failed: 0x") + IntToHex(Res));
		free(Data);
	}
}

void CMultiGattDlg::OnLvnItemchangedListDevices(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	//UpdateButtons();

	*pResult = 0;
}

void CMultiGattDlg::ManagerAfterOpen(void* Sender)
{
	// SYNC
	lbLog.AddString(_T("Bluetooth Manager opened"));
}

void CMultiGattDlg::ManagerBeforeClose(void* Sender)
{
	// SYNC
	lbLog.AddString(_T("Bluetooth Manager is closing"));
}

void CMultiGattDlg::ManagerClosed(void* Sender)
{
	// SYNC
	lbLog.AddString(_T("Bluetooth Manager closed"));
}

void CMultiGattDlg::WatcherClientDisconnected(const __int64 Address, const int Reason)
{
	// SYNC
	lbLog.AddString(_T("Device ") + IntToHex(Address) + _T(" disconnected: 0x") + IntToHex(Reason));
	if (lvDevices.GetItemCount() > 0)
	{
		for (int Item = 0; Item < lvDevices.GetItemCount(); Item++)
		{
			__int64 DeviceAddress = StrToInt64(lvDevices.GetItemText(Item, 0));
			if (DeviceAddress = Address)
			{
				lvDevices.DeleteItem(Item);
				UpdateButtons();
				break;
			}
		}
	}
}

void CMultiGattDlg::WatcherConnectionCompleted(const __int64 Address, const int Result)
{
	// SYNC
	lbLog.AddString(_T("Connection to ") + IntToHex(Address) + _T(" completed: 0x") + IntToHex(Result));
	if (lvDevices.GetItemCount() > 0)
	{
		for (int Item = 0; Item < lvDevices.GetItemCount(); Item++)
		{
			__int64 DeviceAddress = StrToInt64(lvDevices.GetItemText(Item, 0));
			if (DeviceAddress == Address)
			{
				if (Result != WCL_E_SUCCESS)
					lvDevices.DeleteItem(Item);
				else
				{
					lvDevices.SetItemText(Item, 2, _T("Connected"));
					lvDevices.SetItemData(Item, (DWORD_PTR)dsConnected);
				}
				UpdateButtons();
				break;
			}
		}
	}
}

void CMultiGattDlg::WatcherConnectionStarted(const __int64 Address, const int Result)
{
	// SYNC
	lbLog.AddString(_T("Connection to ") + IntToHex(Address) + _T(" started: 0x") + IntToHex(Result));
	if (lvDevices.GetItemCount() > 0)
	{
		for (int Item = 0; Item < lvDevices.GetItemCount(); Item++)
		{
			__int64 DeviceAddress = StrToInt64(lvDevices.GetItemText(Item, 0));
			if (DeviceAddress == Address)
			{
				if (Result != WCL_E_SUCCESS)
					lvDevices.DeleteItem(Item);
				else
				{
					lvDevices.SetItemText(Item, 2, _T("Connecting..."));
					lvDevices.SetItemData(Item, (DWORD_PTR)dsConnecting);
				}
				UpdateButtons();
				break;
			}
		}
	}
}

void CMultiGattDlg::WatcherDeviceFound(const __int64 Address, const tstring& Name)
{
	// SYNC
	lbLog.AddString(_T("Device ") + IntToHex(Address) + _T(" found: ") + CString(Name.c_str()));
	int Item = lvDevices.GetItemCount();
	lvDevices.InsertItem(Item, IntToHex(Address));
	lvDevices.SetItemText(Item, 1, Name.c_str());
	lvDevices.SetItemText(Item, 2, _T("Found..."));
	lvDevices.SetItemData(Item, (DWORD_PTR)dsFound);
}

void CMultiGattDlg::WatcherStopped(void* Sender)
{
	// SYNC
	lvDevices.DeleteAllItems();
	lbLog.AddString(_T("Client watcher stopped"));
	FManager->Close();
	UpdateButtons();
}

void CMultiGattDlg::WatcherStarted(void* Sender)
{
	// SYNC
	lvDevices.DeleteAllItems();
	lbLog.AddString(_T("Client watcher started"));
	UpdateButtons();
}

void CMultiGattDlg::WatcherValueChanged(const __int64 Address, const unsigned char* Value,
	const unsigned long Length)
{
	// SYNC
	if (Length > 3)
	{
		unsigned long Val = (Value[3] << 24) | (Value[2] << 16) |
			(Value[1] << 8) | Value[0];
		lbLog.AddString(_T("Data received from ") + IntToHex(Address) + _T(": ") + IntToStr(Val));
	}
	else
		lbLog.AddString(_T("Empty data received"));
}
