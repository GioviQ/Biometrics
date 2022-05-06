// Communication.cpp: implementation of the CCommunication class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Communication.h"
#include "Command.h"
#include "USBCommand.h"
#include "Device.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//void	DoEvents();

int BAUDRATES[] = {9600,19200,38400,57600,115200,230400,460800,921600};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/************************************************************************/
/************************************************************************/
CCommunication::CCommunication()
{
	m_hUsbHandle = INVALID_HANDLE_VALUE;
	m_hMainWnd = NULL;
}
/************************************************************************/
/************************************************************************/
CCommunication::~CCommunication()
{
	if (m_hUsbHandle != INVALID_HANDLE_VALUE)
	{

	}
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_TestConnection(void)
{
	BOOL	w_bRet;
	
	InitCmdPacket(CMD_TEST_CONNECTION, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);
	
	SEND_COMMAND(CMD_TEST_CONNECTION, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_SetParam(int p_nParamIndex, int p_nParamValue)
{
	BOOL	w_bRet;
	BYTE	w_abyData[5];
	
	w_abyData[0] = p_nParamIndex;
	memcpy(&w_abyData[1], &p_nParamValue, 4);
	
	InitCmdPacket(CMD_SET_PARAM, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 5);
 	 	
	SEND_COMMAND(CMD_SET_PARAM, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetParam(int p_nParamIndex, int* p_pnParamValue)
{
	BOOL	w_bRet;
	BYTE	w_byData;

	w_byData = p_nParamIndex;
	
	InitCmdPacket(CMD_GET_PARAM, m_bySrcDeviceID, m_byDstDeviceID, &w_byData, 1);
 	 	
	SEND_COMMAND(CMD_GET_PARAM, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(p_pnParamValue, g_pRcmPacket->m_abyData, 4);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int CCommunication::Run_GetDeviceInfo(char* p_szDevInfo)
{
	BOOL	w_bRet;
	WORD	w_wDevInfoLen;
	
	w_bRet = Run_Command_NP(CMD_GET_DEVICE_INFO);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;	
	
	w_wDevInfoLen = MAKEWORD(g_pRcmPacket->m_abyData[0], g_pRcmPacket->m_abyData[1]);
	
	RECEIVE_DATAPACKET(CMD_GET_DEVICE_INFO, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == false)
		return ERR_CONNECTION;
	
	if ( RESPONSE_RET != ERR_SUCCESS )	
		return RESPONSE_RET;
	
	memcpy(p_szDevInfo, g_pRcmPacket->m_abyData, w_wDevInfoLen);
	
	return ERR_SUCCESS;
}
/***************************************************************************/
/***************************************************************************/
int	CCommunication::Run_EnterISPMode(void)
{
	BOOL	w_bRet;
	
	InitCmdPacket(CMD_ENTER_ISPMODE, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);
	
	SEND_COMMAND(CMD_ENTER_ISPMODE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_SetIDNote(int p_nTmplNo, BYTE* pNote)
{
	BOOL	w_bRet = false;
	BYTE	w_abyData[ID_NOTE_SIZE+2];
	WORD	w_wData;
	
	//. Assemble command packet
	w_wData = ID_NOTE_SIZE + 2;
	InitCmdPacket(CMD_SET_ID_NOTE, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_wData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_SET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == false)
		return ERR_CONNECTION;
	
	if( RESPONSE_RET != ERR_SUCCESS)	
		return RESPONSE_RET;
	
	Sleep(10);
	
	//. Assemble data packet
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	memcpy(&w_abyData[2], pNote, ID_NOTE_SIZE);
	
	InitCmdDataPacket(CMD_SET_ID_NOTE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, ID_NOTE_SIZE+2);
	
	//. Send data packet to target
	SEND_DATAPACKET(CMD_SET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == false)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetIDNote(int p_nTmplNo, BYTE* pNote)
{
	BOOL		w_bRet = false;
	BYTE		w_abyData[2];
	int			w_nTemplateNo = 0;
	WORD		w_nCmdCks = 0, w_nSize = 0;
	
	//. Assemble command packet
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	InitCmdPacket(CMD_GET_ID_NOTE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_GET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == false)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	RECEIVE_DATAPACKET(CMD_GET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == false)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(pNote, &g_pRcmPacket->m_abyData[0], ID_NOTE_SIZE);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_SetModuleSN(BYTE* pModuleSN)
{
	BOOL	w_bRet = false;
	BYTE	w_abyData[MODULE_SN_LEN];
	WORD	w_wData;
	
	//. Assemble command packet
	w_wData = MODULE_SN_LEN;
	InitCmdPacket(CMD_SET_MODULE_SN, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_wData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_SET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == false)
		return ERR_CONNECTION;
	
	if( RESPONSE_RET != ERR_SUCCESS)	
		return RESPONSE_RET;
	
	Sleep(10);
	
	//. Assemble data packet
	memcpy(&w_abyData[0], pModuleSN, MODULE_SN_LEN);
	
	InitCmdDataPacket(CMD_SET_MODULE_SN, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, MODULE_SN_LEN);
	
	//. Send data packet to target
	SEND_DATAPACKET(CMD_SET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == false)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetModuleSN(BYTE* pModuleSN)
{
	BOOL		w_bRet = false;
	
	//. Assemble command packet
	InitCmdPacket(CMD_GET_MODULE_SN, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_GET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == false)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	RECEIVE_DATAPACKET(CMD_GET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == false)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(pModuleSN, &g_pRcmPacket->m_abyData[0], MODULE_SN_LEN);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetImage(void)
{
	BOOL	w_bRet;
	
	w_bRet = Run_Command_NP(CMD_GET_IMAGE);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_FingerDetect(int* p_pnDetectResult)
{
	BOOL	w_bRet;
	
	w_bRet = Run_Command_NP(CMD_FINGER_DETECT);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnDetectResult = g_pRcmPacket->m_abyData[0];
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
#define IMAGE_DATA_UINT	496
int	CCommunication::Run_UpImage(int p_nType, BYTE* p_pFpData, int* p_pnImgWidth, int* p_pnImgHeight)
{
	int		i, n, r, w, h, size;
	BOOL	w_bRet;
	BYTE	w_byData;

	w_byData = p_nType;

	InitCmdPacket(CMD_UP_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, &w_byData, 1);
	
	SEND_COMMAND(CMD_UP_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	w = MAKEWORD(g_pRcmPacket->m_abyData[0], g_pRcmPacket->m_abyData[1]);
	h = MAKEWORD(g_pRcmPacket->m_abyData[2], g_pRcmPacket->m_abyData[3]);
	
	size = w*h;

	n = (size)/IMAGE_DATA_UINT;
	r = (size)%IMAGE_DATA_UINT;

	if (m_nConnectionMode == SERIAL_CON_MODE)
	{
		for (i=0; i<n; i++)
		{
			RECEIVE_DATAPACKET(CMD_UP_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
			
			if(w_bRet == false)
				return ERR_CONNECTION;
			
			if (RESPONSE_RET != ERR_SUCCESS)
				return RESPONSE_RET;
			
			memcpy(&p_pFpData[i*IMAGE_DATA_UINT], &g_pRcmPacket->m_abyData[2], IMAGE_DATA_UINT);
			
			if (m_hMainWnd)
				SendMessage(m_hMainWnd, WM_UP_IMAGE_PROGRESS, (i+1)*IMAGE_DATA_UINT*100/(w*h), 1);
		}
		
		if (r > 0)
		{
			RECEIVE_DATAPACKET(CMD_UP_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
			
			if(w_bRet == false)
				return ERR_CONNECTION;
			
			if (RESPONSE_RET != ERR_SUCCESS)
				return RESPONSE_RET;
			
			memcpy(&p_pFpData[i*IMAGE_DATA_UINT], &g_pRcmPacket->m_abyData[2], r);
			
			if (m_hMainWnd)
				SendMessage(m_hMainWnd, WM_UP_IMAGE_PROGRESS, (i*IMAGE_DATA_UINT+r)*100/(w*h), 1);
		}	
	}
	else if (m_nConnectionMode == USB_CON_MODE)
	{
		w_bRet = USB_ReceiveImage(m_hUsbHandle, p_pFpData, w*h);
		
		if(w_bRet == false)
			return ERR_CONNECTION;
	}

	*p_pnImgWidth = w;
	*p_pnImgHeight = h;
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
#define DOWN_IMAGE_DATA_UINT	496
int	CCommunication::Run_DownImage(BYTE* p_pData, int p_nWidth, int p_nHeight)
{
	int		i, n, r, w, h;
	BOOL	w_bRet;
	BYTE	w_abyData[840];
	
	w = p_nWidth;
	h = p_nHeight;

	w_abyData[0] = LOBYTE(p_nWidth);
	w_abyData[1] = HIBYTE(p_nWidth);
	w_abyData[2] = LOBYTE(p_nHeight);
	w_abyData[3] = HIBYTE(p_nHeight);

	//. Assemble command packet
	InitCmdPacket(CMD_DOWN_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_DOWN_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	n = (w*h)/DOWN_IMAGE_DATA_UINT;
	r = (w*h)%DOWN_IMAGE_DATA_UINT;

	if (m_nConnectionMode == SERIAL_CON_MODE)
	{
		for (i=0; i<n; i++)
		{
			w_abyData[0] = LOBYTE(i);
			w_abyData[1] = HIBYTE(i);
			memcpy(&w_abyData[2], &p_pData[i*DOWN_IMAGE_DATA_UINT], DOWN_IMAGE_DATA_UINT);
			
			InitCmdDataPacket(CMD_DOWN_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2+DOWN_IMAGE_DATA_UINT);

			SEND_DATAPACKET(CMD_DOWN_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
			
			if (w_bRet == false)
				return ERR_CONNECTION;
			
			if (RESPONSE_RET != ERR_SUCCESS)
				return RESPONSE_RET;			
			
			if (m_hMainWnd)
				SendMessage(m_hMainWnd, WM_UP_IMAGE_PROGRESS, (i+1)*DOWN_IMAGE_DATA_UINT*100/(w*h), 0);

			Sleep(6);
		}
		
		if (r > 0)
		{
			w_abyData[0] = LOBYTE(i);
			w_abyData[1] = HIBYTE(i);
			memcpy(&w_abyData[2], &p_pData[i*DOWN_IMAGE_DATA_UINT], r);
			
			InitCmdDataPacket(CMD_DOWN_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2+r);

			SEND_DATAPACKET(CMD_DOWN_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
			
			if (w_bRet == false)
				return ERR_CONNECTION;
			
			if (RESPONSE_RET != ERR_SUCCESS)
				return RESPONSE_RET;
			
			if (m_hMainWnd)
				SendMessage(m_hMainWnd, WM_UP_IMAGE_PROGRESS, (i*DOWN_IMAGE_DATA_UINT+r)*100/(w*h), 0);
		}
	}
	else if (m_nConnectionMode == USB_CON_MODE)
	{
		w_bRet = USB_DownImage(m_hUsbHandle, p_pData, p_nWidth*p_nHeight);
		
		if(w_bRet != RT_OK)
			return ERR_CONNECTION;
	}

	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_SLEDControl(int p_nState)
{
	BOOL	w_bRet;
	BYTE	w_abyData[2];
	
	w_abyData[0] = LOBYTE(p_nState);
	w_abyData[1] = HIBYTE(p_nState);
	
	InitCmdPacket(CMD_SLED_CTRL, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_abyData, 2);
	
	SEND_COMMAND(CMD_SLED_CTRL, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_StoreChar(int p_nTmplNo, int p_nRamBufferID, int* p_pnDupTmplNo)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	w_abyData[2] = LOBYTE(p_nRamBufferID);
	w_abyData[3] = HIBYTE(p_nRamBufferID);
	
	InitCmdPacket(CMD_STORE_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_STORE_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
	{
		if (RESPONSE_RET == ERR_DUPLICATION_ID)
			*p_pnDupTmplNo = MAKEWORD(g_pRcmPacket->m_abyData[0], g_pRcmPacket->m_abyData[1]);

		return RESPONSE_RET;
	}
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_LoadChar(int p_nTmplNo, int p_nRamBufferID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	w_abyData[2] = LOBYTE(p_nRamBufferID);
	w_abyData[3] = HIBYTE(p_nRamBufferID);
	
	InitCmdPacket(CMD_LOAD_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_LOAD_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_UpChar(int p_nRamBufferID, BYTE* p_pbyTemplate)
{	
	BOOL		w_bRet = false;
	BYTE		w_abyData[2];
	int			w_nTemplateNo = 0;
	WORD		w_nCmdCks = 0, w_nSize = 0;
	
	//. Assemble command packet
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	InitCmdPacket(CMD_UP_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_UP_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == false)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	RECEIVE_DATAPACKET(CMD_UP_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == false)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(p_pbyTemplate, &g_pRcmPacket->m_abyData[0], GD_RECORD_SIZE);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_DownChar(int p_nRamBufferID, BYTE* p_pbyTemplate)
{
	BOOL	w_bRet = false;
	BYTE	w_abyData[GD_RECORD_SIZE+2];
	WORD	w_wData;
	
	//. Assemble command packet
	w_wData = GD_RECORD_SIZE + 2;
	InitCmdPacket(CMD_DOWN_CHAR, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_wData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_DOWN_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == false)
		return ERR_CONNECTION;
	
	if( RESPONSE_RET != ERR_SUCCESS)	
		return RESPONSE_RET;
	
	Sleep(10);
	
	//. Assemble data packet
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	memcpy(&w_abyData[2], p_pbyTemplate, GD_RECORD_SIZE);
		
	InitCmdDataPacket(CMD_DOWN_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, GD_RECORD_SIZE+2);
	
	//. Send data packet to target
	SEND_DATAPACKET(CMD_DOWN_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == false)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int CCommunication::Run_DelChar(int p_nSTmplNo, int p_nETmplNo)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);
	
	//. Assemble command packet
	InitCmdPacket(CMD_DEL_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_DEL_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetEmptyID(int p_nSTmplNo, int p_nETmplNo, int* p_pnEmptyID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];

	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);

	//. Assemble command packet
	InitCmdPacket(CMD_GET_EMPTY_ID, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_GET_EMPTY_ID, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if ( RESPONSE_RET != ERR_SUCCESS )	
		return RESPONSE_RET;
	
	*p_pnEmptyID = MAKEWORD(g_pRcmPacket->m_abyData[0], g_pRcmPacket->m_abyData[1]);
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetStatus(int p_nTmplNo, int* p_pnStatus)
{
	BOOL	w_bRet;
	BYTE	w_abyData[2];
	
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	
	InitCmdPacket(CMD_GET_STATUS, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	SEND_COMMAND(CMD_GET_STATUS, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if ( RESPONSE_RET != ERR_SUCCESS )	
		return RESPONSE_RET;
	
	*p_pnStatus = g_pRcmPacket->m_abyData[0];
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetBrokenID(int p_nSTmplNo, int p_nETmplNo, int* p_pnCount, int* p_pnFirstID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);

	//. Assemble command packet
	InitCmdPacket(CMD_GET_BROKEN_ID, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_GET_BROKEN_ID, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnCount		= MAKEWORD(g_pRcmPacket->m_abyData[0], g_pRcmPacket->m_abyData[1]);
	*p_pnFirstID	= MAKEWORD(g_pRcmPacket->m_abyData[2], g_pRcmPacket->m_abyData[3]);
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_GetEnrollCount(int p_nSTmplNo, int p_nETmplNo, int* p_pnEnrollCount)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);
	
	//. Assemble command packet
	InitCmdPacket(CMD_GET_ENROLL_COUNT, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_GET_ENROLL_COUNT, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if(RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnEnrollCount = MAKEWORD(g_pRcmPacket->m_abyData[0], g_pRcmPacket->m_abyData[1]);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_Generate(int p_nRamBufferID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[2];
	
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	
	InitCmdPacket(CMD_GENERATE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_GENERATE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet) 
		return ERR_CONNECTION;
	
	return RESPONSE_RET;		
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_Merge(int p_nRamBufferID, int p_nMergeCount)
{
	BOOL	w_bRet;
	BYTE	w_abyData[3];
	
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	w_abyData[2] = p_nMergeCount;
	
	InitCmdPacket(CMD_MERGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 3);
	
	SEND_COMMAND(CMD_MERGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet) 
		return ERR_CONNECTION;
	
	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_Match(int p_nRamBufferID0, int p_nRamBufferID1, int* p_pnLearnResult)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nRamBufferID0);
	w_abyData[1] = HIBYTE(p_nRamBufferID0);
	w_abyData[2] = LOBYTE(p_nRamBufferID1);
	w_abyData[3] = HIBYTE(p_nRamBufferID1);
	
	InitCmdPacket(CMD_MATCH, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_MATCH, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet) 
		return ERR_CONNECTION;

	if(RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;

	*p_pnLearnResult	= g_pRcmPacket->m_abyData[0];

	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_Search(int p_nRamBufferID, int p_nStartID, int p_nSearchCount, int* p_pnTmplNo, int* p_pnLearnResult)
{
	BOOL	w_bRet;
	BYTE	w_abyData[6];
	
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	w_abyData[2] = LOBYTE(p_nStartID);
	w_abyData[3] = HIBYTE(p_nStartID);
	w_abyData[4] = LOBYTE(p_nSearchCount);
	w_abyData[5] = HIBYTE(p_nSearchCount);
	
	InitCmdPacket(CMD_SEARCH, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 6);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_SEARCH, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if(RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnTmplNo			= MAKEWORD(g_pRcmPacket->m_abyData[0], g_pRcmPacket->m_abyData[1]);
	*p_pnLearnResult	= g_pRcmPacket->m_abyData[2];
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	CCommunication::Run_Verify(int p_nTmplNo, int p_nRamBufferID, int* p_pnLearnResult)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	w_abyData[2] = LOBYTE(p_nRamBufferID);
	w_abyData[3] = HIBYTE(p_nRamBufferID);
	
	//. Assemble command packet
	InitCmdPacket(CMD_VERIFY, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_VERIFY, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	*p_pnLearnResult = g_pRcmPacket->m_abyData[2];
	
	return RESPONSE_RET;
}
/***************************************************************************/
/***************************************************************************/
BOOL CCommunication::OpenSerialPort(CString p_strComPortIndex, int p_nBaudRateIndex)
{
	LONG    w_lRetCode = ERROR_SUCCESS;
	
	ASSERT(p_nBaudRateIndex >= 0);
	ASSERT(p_nBaudRateIndex <= BAUD921600);
	
	p_strComPortIndex = _T("\\\\.\\") + p_strComPortIndex;

	//. Attempt to open the serial port (COM1)
	w_lRetCode = g_Serial.Open(p_strComPortIndex, 2048, 2048,true);
	if (w_lRetCode != ERROR_SUCCESS){
		return false;
	}
	
	//. Setup the serial port (9600,8N1, which is the default setting)
	w_lRetCode = g_Serial.Setup((CSerial::EBaudrate)BAUDRATES[p_nBaudRateIndex], CSerial::EData8,CSerial::EParNone,CSerial::EStop1);
	if (w_lRetCode != ERROR_SUCCESS){
		return false;
	}
	
	//. Register only for the receive event
	w_lRetCode = g_Serial.SetMask(CSerial::EEventRecv);
	if (w_lRetCode != ERROR_SUCCESS){
		return false;
	}
	return true;
}
/************************************************************************
*      Test Connection with Target
************************************************************************/
BOOL CCommunication::EnableCommunicaton(int p_nDeviceID, BOOL p_bVerifyDeviceID, BYTE* p_pDevPwd, BOOL p_bMsgOut)
{
	return true;	
}
/************************************************************************
     p_nConMode = 0 : Open Serial Port
	 p_nParam1	= ComPort Index 
	 p_nParam2	= BaudRate Index
************************************************************************/
int CCommunication::InitConnection(int p_nConMode, CString p_strComPort, int p_nBaudRate, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	BOOL	w_blRet = false;
	CString w_strMsg, w_szDestIP;
	
	g_Serial.Close();
	
	m_bySrcDeviceID = p_bySrcDeviceID;
	m_byDstDeviceID = p_byDstDeviceID;

	m_nConnectionMode = p_nConMode;
	
	if (p_nConMode == SERIAL_CON_MODE)
	{
		w_blRet = OpenSerialPort(p_strComPort, p_nBaudRate);
		
		if( w_blRet != TRUE )
		{	
 			w_strMsg.Format(_T("Failed to open %s port!"), p_strComPort);
			//AfxMessageBox(w_strMsg);
			CloseConnection();
			return ERR_COM_OPEN_FAIL;
		}
	}
	else if (p_nConMode == USB_CON_MODE)
	{
		if ( OpenUSB( &m_hUsbHandle, p_byDstDeviceID ) == FALSE )
		{
			//AfxMessageBox(_T("Failed to open USB port!"));
			CloseConnection();
			return ERR_USB_OPEN_FAIL;
		}
	}

	return CONNECTION_SUCCESS;
}
/***************************************************************************/
/***************************************************************************/
BOOL CCommunication::Run_Command_NP( BYTE p_byCMD )
{
	BOOL w_bRet;

	//. Assemble command packet
	InitCmdPacket(p_byCMD, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);

 	SEND_COMMAND(p_byCMD, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	return w_bRet;
}
/***************************************************************************/
/***************************************************************************/
void CCommunication::CloseConnection() 
{
	if (m_nConnectionMode == SERIAL_CON_MODE)
	{
		//. Close serial port
		g_Serial.Close();
	}
	else if (m_nConnectionMode == USB_CON_MODE)
	{
		
	}
}
/***************************************************************************/
/***************************************************************************/
void CCommunication::SetIPandPort(CString strDestination, DWORD dwPort)
{
	m_strDestIp = strDestination;
	m_dwPort	= dwPort;		 
}
/***************************************************************************/
/***************************************************************************/
void CCommunication::SetCallbackWnd(HWND hWnd)
{
	m_hMainWnd = hWnd;
}