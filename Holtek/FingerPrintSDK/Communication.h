// Communication.h: interface for the CCommunication class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMUNICATION_H__B10E77AC_5BA1_470E_8B25_18DBA85D2902__INCLUDED_)
#define AFX_COMMUNICATION_H__B10E77AC_5BA1_470E_8B25_18DBA85D2902__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Command.h"
#include "Define.h"
 
class CCommunication  
{
private:
	int		m_nConnectionMode;
	BYTE	m_bySrcDeviceID;
	BYTE	m_byDstDeviceID;
	HWND	m_hMainWnd;
	HANDLE	m_hUsbHandle;
	
	DWORD	m_dwPort;
	CString	m_strDestIp;

public:
	CCommunication();
	virtual ~CCommunication();
	
	int		Run_TestConnection(void);	
	int		Run_SetParam(int p_nParamIndex, int p_nParamValue);
	int		Run_GetParam(int p_nParamIndex, int* p_pnParamValue);
	int 	Run_GetDeviceInfo(char* p_pszDevInfo);
	int		Run_EnterISPMode(void);
	int		Run_SetIDNote(int p_nTmplNo, BYTE* pNote);
	int		Run_GetIDNote(int p_nTmplNo, BYTE* pNote);
	int		Run_SetModuleSN(BYTE* pModuleSN);
	int		Run_GetModuleSN(BYTE* pModuleSN);

	int		Run_GetImage(void);
	int		Run_FingerDetect(int* p_pnDetectResult);
	int		Run_UpImage(int p_nType, BYTE* p_pData, int* p_pnImgWidth, int* p_pnImgHeight);
	int		Run_DownImage(BYTE* p_pData, int p_nWidth, int p_nHeight);
	int		Run_SLEDControl(int p_nState);

	int		Run_StoreChar(int p_nTmplNo, int p_nRamBufferID, int* p_pnDupTmplNo);
	int		Run_LoadChar(int p_nTmplNo, int p_nRamBufferID);
	int		Run_UpChar(int p_nRamBufferID, BYTE* p_pbyTemplate);
	int		Run_DownChar(int p_nRamBufferID, BYTE* p_pbyTemplate);
	
	int 	Run_DelChar(int p_nSTmplNo, int p_nETmplNo);
	int		Run_GetEmptyID(int p_nSTmplNo, int p_nETmplNo, int* p_pnEmptyID);
	int		Run_GetStatus(int p_nTmplNo, int* p_pnStatus);
	int		Run_GetBrokenID(int p_nSTmplNo, int p_nETmplNo, int* p_pnCount, int* p_pnFirstID);
	int		Run_GetEnrollCount(int p_nSTmplNo, int p_nETmplNo, int* p_pnEnrollCount);

	int		Run_Generate(int p_nRamBufferID);
	int		Run_Merge(int p_nRamBufferID, int p_nMergeCount);
	int		Run_Match(int p_nRamBufferID0, int p_nRamBufferID1, int* p_pnLearnResult);
	int		Run_Search(int p_nRamBufferID, int p_nStartID, int p_nSearchCount, int* p_pnTmplNo, int* p_pnLearnResult);
	int		Run_Verify(int p_nTmplNo, int p_nRamBufferID, int* p_pnLearnResult);
			
	int		InitConnection(int p_nConMode, CString p_strComPort, int p_nBaudRate, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);

	BOOL	EnableCommunicaton(int p_nDevNum, BOOL p_bVerifyDeviceID, BYTE* p_pDevPwd, BOOL p_bMsgOut);	
	BOOL	OpenSerialPort(CString p_strComPortIndex, int p_nBaudRateIndex);		
	BOOL	Run_Command_NP( BYTE p_byCMD );	

	void	CloseConnection();	
	void	SetIPandPort(CString strDestination, DWORD dwPort);
	void	SetCallbackWnd(HWND hWnd);
};

#endif // !defined(AFX_COMMUNICATION_H__B10E77AC_5BA1_470E_8B25_18DBA85D2902__INCLUDED_)
