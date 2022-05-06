/********************************************************************
	created:	2012/01/26
	file base:	Command
	file ext:	cpp
	author:		SYFP Team

	purpose:	통신파케트 관련 구조체 및 함수들을 정의한다.
*********************************************************************/

#include "stdafx.h"
#include "Define.h"
#include "Command.h"


CSerial			g_Serial;
DWORD			g_dwPacketSize = 0;
BYTE			g_Packet[1024 * 64];
PST_CMD_PACKET	g_pCmdPacket = (PST_CMD_PACKET)g_Packet;
PST_RCM_PACKET	g_pRcmPacket = (PST_RCM_PACKET)g_Packet;

/***************************************************************************/
/***************************************************************************/
WORD GetCheckSum(BOOL bCmdData)
{
	WORD	w_Ret = 0;

	//w_Ret = g_pCmdPacket->;

	return w_Ret;
}
/***************************************************************************/
/***************************************************************************/
BOOL CheckReceive(BYTE* p_pbyPacket, DWORD p_dwPacketLen, WORD p_wPrefix, WORD p_wCMDCode)
{
	int				i;
	WORD			w_wCalcCheckSum, w_wCheckSum;
	ST_RCM_PACKET*	w_pstRcmPacket;

	w_pstRcmPacket = (ST_RCM_PACKET*)p_pbyPacket;

	//. Check prefix code
	if (p_wPrefix != w_pstRcmPacket->m_wPrefix)
		return FALSE;

	//. Check checksum
	w_wCheckSum = MAKEWORD(p_pbyPacket[p_dwPacketLen - 2], p_pbyPacket[p_dwPacketLen - 1]);
	w_wCalcCheckSum = 0;
	for (i = 0; i < p_dwPacketLen - 2; i++)
	{
		w_wCalcCheckSum = w_wCalcCheckSum + p_pbyPacket[i];
	}

	if (w_wCheckSum != w_wCalcCheckSum)
		return FALSE;

	if (p_wCMDCode != w_pstRcmPacket->m_wCMDCode)
	{
		return FALSE;
	}

	return TRUE;
}
/***************************************************************************/
/***************************************************************************/
void InitCmdPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen)
{
	int		i;
	WORD	w_wCheckSum;

	memset(g_Packet, 0, sizeof(g_Packet));
	g_pCmdPacket->m_wPrefix = CMD_PREFIX_CODE;
	g_pCmdPacket->m_bySrcDeviceID = p_bySrcDeviceID;
	g_pCmdPacket->m_byDstDeviceID = p_byDstDeviceID;
	g_pCmdPacket->m_wCMDCode = p_wCMDCode;
	g_pCmdPacket->m_wDataLen = p_wDataLen;

	if (p_wDataLen)
		memcpy(g_pCmdPacket->m_abyData, p_pbyData, p_wDataLen);

	w_wCheckSum = 0;

	for (i = 0; i < sizeof(ST_CMD_PACKET) - 2; i++)
	{
		w_wCheckSum = w_wCheckSum + g_Packet[i];
	}

	g_pCmdPacket->m_wCheckSum = w_wCheckSum;

	g_dwPacketSize = sizeof(ST_CMD_PACKET);
}
/***************************************************************************/
/***************************************************************************/
void	InitCmdDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen)
{
	int		i;
	WORD	w_wCheckSum;

	g_pCmdPacket->m_wPrefix = CMD_DATA_PREFIX_CODE;
	g_pCmdPacket->m_bySrcDeviceID = p_bySrcDeviceID;
	g_pCmdPacket->m_byDstDeviceID = p_byDstDeviceID;
	g_pCmdPacket->m_wCMDCode = p_wCMDCode;
	g_pCmdPacket->m_wDataLen = p_wDataLen;

	memcpy(&g_pCmdPacket->m_abyData[0], p_pbyData, p_wDataLen);

	//. Set checksum
	w_wCheckSum = 0;

	for (i = 0; i < (p_wDataLen + 8); i++)
	{
		w_wCheckSum = w_wCheckSum + g_Packet[i];
	}

	g_Packet[p_wDataLen + 8] = LOBYTE(w_wCheckSum);
	g_Packet[p_wDataLen + 9] = HIBYTE(w_wCheckSum);

	g_dwPacketSize = p_wDataLen + 10;
}
/***************************************************************************/
/***************************************************************************/
BOOL SendCommand(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nSendCnt = 0;
	LONG	w_nResult = 0;

	g_Serial.Purge();

	//::SendMessage(g_hMainWnd, WM_CMD_PACKET_HOOK, 0, 0);

	w_nResult = g_Serial.Write(g_Packet, g_dwPacketSize, &w_nSendCnt, NULL, COMM_TIMEOUT);

	if (ERROR_SUCCESS != w_nResult)
	{
		return FALSE;
	}

	return ReceiveAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID);
}
/***************************************************************************/
/***************************************************************************/
BOOL ReceiveAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nAckCnt = 0;
	LONG	w_nResult = 0;
	DWORD	w_dwTimeOut = COMM_TIMEOUT;

	if (p_wCMDCode == CMD_TEST_CONNECTION)
		w_dwTimeOut = 2000;

l_read_packet:

	w_nResult = g_Serial.Read(g_Packet, sizeof(ST_RCM_PACKET), &w_nAckCnt, NULL, w_dwTimeOut);

	if (ERROR_SUCCESS != w_nResult)
	{
		return FALSE;
	}

	g_dwPacketSize = sizeof(ST_RCM_PACKET);

	//::SendMessage(g_hMainWnd, WM_RCM_PACKET_HOOK, 0, 0);

	if (!CheckReceive(g_Packet, sizeof(ST_RCM_PACKET), RCM_PREFIX_CODE, p_wCMDCode))
		return FALSE;

	if (g_pCmdPacket->m_byDstDeviceID != p_bySrcDeviceID)
	{
		goto l_read_packet;
	}

	return TRUE;
}
/***************************************************************************/
/***************************************************************************/
BOOL ReceiveDataAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nAckCnt = 0;
	LONG	w_nResult = 0;
	DWORD	w_dwTimeOut = COMM_TIMEOUT;

	w_nResult = g_Serial.Read(g_Packet, 8, &w_nAckCnt, NULL, w_dwTimeOut);

	if (ERROR_SUCCESS != w_nResult)
	{
		::MessageBox(NULL, _T("Please check connection with device"), _T("Err"), MB_ICONWARNING);
		return false;
	}

	w_nResult = g_Serial.Read(g_Packet + 8, g_pRcmPacket->m_wDataLen + 2, &w_nAckCnt, NULL, w_dwTimeOut);

	g_dwPacketSize = g_pRcmPacket->m_wDataLen + 10;

	//::SendMessage(g_hMainWnd, WM_RCM_PACKET_HOOK, 0, 0);

	return CheckReceive(g_Packet, 10 + g_pRcmPacket->m_wDataLen, RCM_DATA_PREFIX_CODE, p_wCMDCode);
}
/***************************************************************************/
/***************************************************************************/
BOOL SendDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nSendCnt = 0;
	LONG	w_nResult = 0;

	//::SendMessage(g_hMainWnd, WM_CMD_PACKET_HOOK, 0, 0);

	w_nResult = g_Serial.Write(g_Packet, g_dwPacketSize, &w_nSendCnt, NULL, COMM_TIMEOUT);

	if (ERROR_SUCCESS != w_nResult)
	{
		::MessageBox(NULL, _T("Fail in sending a command packet !"), _T("Communication Error"), MB_ICONWARNING);
		return FALSE;
	}

	return ReceiveDataAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID);
}
/***************************************************************************/
/***************************************************************************/
BOOL ReceiveDataPacket(WORD	p_wCMDCode, BYTE p_byDataLen, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	return ReceiveDataAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID);
}
/***************************************************************************/
/***************************************************************************/
CString GetErrorMsg(DWORD p_dwErrorCode)
{
	CString w_ErrMsg;

	switch (LOBYTE(p_dwErrorCode))
	{
	case ERROR_SUCCESS:
		w_ErrMsg = _T("Succcess");
		break;
	case ERR_VERIFY:
		w_ErrMsg = _T("Verify NG");
		break;
	case ERR_IDENTIFY:
		w_ErrMsg = _T("Identify NG");
		break;
	case ERR_EMPTY_ID_NOEXIST:
		w_ErrMsg = _T("Empty Template no Exist");
		break;
	case ERR_BROKEN_ID_NOEXIST:
		w_ErrMsg = _T("Broken Template no Exist");
		break;
	case ERR_TMPL_NOT_EMPTY:
		w_ErrMsg = _T("Template of this ID Already Exist");
		break;
	case ERR_TMPL_EMPTY:
		w_ErrMsg = _T("This Template is Already Empty");
		break;
	case ERR_INVALID_TMPL_NO:
		w_ErrMsg = _T("Invalid Template No");
		break;
	case ERR_ALL_TMPL_EMPTY:
		w_ErrMsg = _T("All Templates are Empty");
		break;
	case ERR_INVALID_TMPL_DATA:
		w_ErrMsg = _T("Invalid Template Data");
		break;
	case ERR_DUPLICATION_ID:
		w_ErrMsg = _T("Duplicated ID : ");
		break;
	case ERR_BAD_QUALITY:
		w_ErrMsg = _T("Bad Quality Image");
		break;
	case ERR_MERGE_FAIL:
		w_ErrMsg = _T("Merge failed");
		break;
	case ERR_NOT_AUTHORIZED:
		w_ErrMsg = _T("Device not authorized.");
		break;
	case ERR_MEMORY:
		w_ErrMsg = _T("Memory Error ");
		break;
	case ERR_INVALID_PARAM:
		w_ErrMsg = _T("Invalid Parameter");
		break;
	case ERR_GEN_COUNT:
		w_ErrMsg = _T("Generation Count is invalid");
		break;
	case ERR_INVALID_BUFFER_ID:
		w_ErrMsg = _T("Ram Buffer ID is invalid.");
		break;
	case ERR_INVALID_OPERATION_MODE:
		w_ErrMsg = _T("Invalid Operation Mode!");
		break;
	case ERR_FP_NOT_DETECTED:
		w_ErrMsg = _T("Finger is not detected.");
		break;
	default:
		w_ErrMsg.Format(_T("Fail, error code=%d"), p_dwErrorCode);
		break;
	}

	return w_ErrMsg;
}
