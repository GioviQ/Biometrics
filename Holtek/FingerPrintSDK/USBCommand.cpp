#include "StdAfx.h"
#include "Define.h"
#include "command.h"
#include "USBCommand.h"
#include "Device.h"
#include "Communication.h"
#include <MMSystem.h>

#define COMM_SLEEP_TIME		(40)
/***************************************************************************/
/***************************************************************************/
bool OpenUSB( HANDLE* pHandle, int p_nDeviceID )
{ 	
	BOOL	w_bRet;
	HANDLE  hUDisk= INVALID_HANDLE_VALUE;
    CHAR	strDriver[25]; 
	int		Driver;
	
	CString strPath;
	
	for ( Driver='C'; Driver<='Z'; Driver++ )
	{
		strPath.Format( _T("%c:"),Driver );
		int type = GetDriveType( strPath );
		if( type==DRIVE_REMOVABLE || type==DRIVE_CDROM )
		{
			sprintf(strDriver,"\\\\.\\%c:",Driver);	
			hUDisk = CreateFileA(strDriver,
								GENERIC_WRITE | GENERIC_READ,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								0,
								NULL);	
			
			if(hUDisk==INVALID_HANDLE_VALUE) continue;

 			InitCmdPacket(CMD_TEST_CONNECTION, 0, p_nDeviceID, NULL, 0);
 			
 			w_bRet = USB_SendPacket(hUDisk, CMD_TEST_CONNECTION, 0, p_nDeviceID);
			
			if ( !w_bRet )
				continue;
			
			if(RESPONSE_RET != ERR_SUCCESS)
				continue;
					
			//if(g_pRcmPacket->m_abyData[0] == p_nDeviceID)
			{
				*pHandle=hUDisk;
				return TRUE;
			}			
		}
	}
	return FALSE; 	
} 
/***************************************************************************/
/***************************************************************************/
bool CloseUsb( HANDLE *pHandle )
{
	if( *pHandle==NULL || *pHandle==INVALID_HANDLE_VALUE )
		return TRUE;

	CloseHandle( *pHandle );

	*pHandle = INVALID_HANDLE_VALUE;

	return TRUE;
}
/***************************************************************************/
/***************************************************************************/
bool USB_SendPacket( HANDLE hHandle, BYTE p_byCMD, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID )
{
	DWORD	w_nSendCnt = 0;
	LONG	w_nResult = 0;
	BYTE	btCDB[8] = {0};
	
	//::SendMessage(g_hMainWnd, WM_CMD_PACKET_HOOK, 0, 0);

	btCDB[0] = 0xEF; btCDB[1] = 0x11; btCDB[4] = (BYTE)g_dwPacketSize;

	if( !USBSCSIWrite( hHandle, btCDB, sizeof(btCDB), (PBYTE)g_Packet, g_dwPacketSize, SCSI_TIMEOUT ) )
		return FALSE;

	return USB_ReceiveAck( hHandle, p_byCMD );
}
/***************************************************************************/
/***************************************************************************/
bool USB_ReceiveAck(HANDLE hHandle, BYTE p_byCMD)
{
	DWORD	w_nAckCnt = 0, w_nReadCount = 0,  c = 0;
	LONG	w_nResult = 0;
	BYTE	btCDB[8] = {0};
	BYTE	w_WaitPacket[CMD_PACKET_LEN];
	DWORD	nLen;
	DWORD	w_dwTimeOut = SCSI_TIMEOUT;

	if (p_byCMD == CMD_TEST_CONNECTION)
	{
		w_dwTimeOut = 3;		
	}

	w_nReadCount = GetReadWaitTime(p_byCMD);

	c = 0;
	memset(w_WaitPacket, 0xAF, CMD_PACKET_LEN);
	do 
	{
		memset(g_Packet,0,sizeof(g_Packet));
		btCDB[0] = 0xEF; btCDB[1] = 0x12;
		nLen = sizeof(ST_RCM_PACKET);
		if( !USBSCSIRead( hHandle, btCDB, sizeof(btCDB), g_Packet, &nLen, w_dwTimeOut ) )
			return false;

		Sleep(COMM_SLEEP_TIME);
		
		c++;
	
		if ( c > w_nReadCount)
		{
			return false;
		}
	} while ( !memcmp(g_Packet, w_WaitPacket, CMD_PACKET_LEN) );

	g_dwPacketSize = nLen;	
	//::SendMessage(g_hMainWnd, WM_RCM_PACKET_HOOK, 0, 0);

	if( !CheckReceive(g_Packet, sizeof(ST_RCM_PACKET), RCM_PREFIX_CODE, p_byCMD ) )
		return false;
		
	return true;
}
/***************************************************************************/
/***************************************************************************/
bool USB_ReceiveDataAck( HANDLE hHandle, BYTE p_byCMD)
{
	DWORD	w_nAckCnt = 0;
	LONG	w_nResult = 0;
	BYTE	btCDB[8] = {0};
	DWORD	nLen;
	DWORD	w_dwTimeOut = COMM_TIMEOUT;
	BYTE	w_WaitPacket[10];
	
	memset( w_WaitPacket, 0xAF, 10 );
	do 
	{
		btCDB[0] = 0xEF; btCDB[1] = 0x15;
		nLen = 8;
		if ( !USBSCSIRead(hHandle, btCDB, sizeof(btCDB), g_Packet, &nLen, w_dwTimeOut) )
			return false;
		Sleep(COMM_SLEEP_TIME);
	} while ( !memcmp(g_Packet, w_WaitPacket, 8) );

	nLen = g_pRcmPacket->m_wDataLen + 2;
	if( USB_ReceiveRawData(hHandle, g_Packet+8, nLen) == FALSE )
		return false;
	
	g_dwPacketSize = 8 + nLen;

	//::SendMessage(g_hMainWnd, WM_RCM_PACKET_HOOK, 0, 0);

	if(!CheckReceive(g_Packet, g_dwPacketSize, RCM_DATA_PREFIX_CODE, p_byCMD ))
		return false;

	return true;
}
/***************************************************************************/
/***************************************************************************/
bool USB_SendDataPacket( HANDLE hHandle, BYTE p_byCMD, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID )
{
	DWORD	w_nSendCnt = 0;
	LONG	w_nResult = 0;
	BYTE	btCDB[8] = {0};
	WORD	w_wLen = (WORD)g_dwPacketSize;
	
	//::SendMessage(g_hMainWnd, WM_CMD_PACKET_HOOK, 0, 0);

	btCDB[0] = 0xEF; btCDB[1] = 0x13;
	btCDB[4] = (w_wLen&0xFF); btCDB[5] = (w_wLen>>8);
	
	if( !USBSCSIWrite( hHandle, btCDB, sizeof(btCDB), (PBYTE)g_Packet, g_dwPacketSize, SCSI_TIMEOUT ) )
		return false;
	
	return USB_ReceiveDataAck(hHandle, p_byCMD);
}
/***************************************************************************/
/***************************************************************************/
bool USB_ReceiveDataPacket(HANDLE hHandle, BYTE	p_byCMD, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	return USB_ReceiveDataAck(hHandle, p_byCMD);
}
/***************************************************************************/
/***************************************************************************/
bool USB_ReceiveImage(HANDLE hHandle, PBYTE p_pBuffer, UINT p_dwDataLen )
{
	BYTE	btCDB[8] = {0};	
	BYTE	w_WaitPacket[8];
	DWORD	w_nDataCnt;
	
	memset( w_WaitPacket, 0xAF, 8 );
	
	if (p_dwDataLen == 208*288 || p_dwDataLen == 242*266 || p_dwDataLen == 202*258)
	{
		btCDB[0] = 0xEF; btCDB[1] = 0x16;
		w_nDataCnt = p_dwDataLen;
		if( !USBSCSIRead( hHandle, btCDB, sizeof(btCDB), p_pBuffer, &w_nDataCnt, SCSI_TIMEOUT ) )
			return FALSE;
	}	
	else if (p_dwDataLen == 256*288)
	{
		btCDB[0] = 0xEF; btCDB[1] = 0x16; btCDB[2] = 0x00;
		w_nDataCnt = p_dwDataLen/2;
		if( !USBSCSIRead( hHandle, btCDB, sizeof(btCDB), &p_pBuffer[0], &w_nDataCnt, SCSI_TIMEOUT ) )
			return FALSE;

		btCDB[0] = 0xEF; btCDB[1] = 0x16; btCDB[2] = 0x01;
		w_nDataCnt = p_dwDataLen/2;
		if( !USBSCSIRead( hHandle, btCDB, sizeof(btCDB), &p_pBuffer[w_nDataCnt], &w_nDataCnt, SCSI_TIMEOUT ) )
			return FALSE;
	}

	return TRUE;
}
/***************************************************************************/
/***************************************************************************/
#define ONE_DOWN_IMAGE_UINT		(60000)
#define CMD_EX_DOWN_IMAGE           0xD4
SYI_STATUS SendPackage(HANDLE hHandle,TPCCmd tPCCmd, BYTE* pData)
{
    unsigned long nLen,lWriteLen;

    if(hHandle==NULL)
		return RT_PARAM_ERR;
	
	tPCCmd.cHead[0]=0xEF;
	tPCCmd.cHead[1]=0x01;
	
	nLen=tPCCmd.nLen;
	lWriteLen=sizeof(tPCCmd);
	
	if(!USBSCSIWrite(hHandle,(unsigned char*)&tPCCmd,sizeof(tPCCmd),pData,nLen,SCSI_TIMEOUT))
		return RT_PACKAGE_ERR;
	
	return RT_OK;
	
}
/***************************************************************************/
/***************************************************************************/
bool USB_DownImage(HANDLE hHandle, BYTE* pBuf, DWORD nBufLen)
{
    unsigned long	nLen,lWriteLen;
	TPCCmd			tPCCmd;

	memset(&tPCCmd, 0, sizeof(TPCCmd));

    if(hHandle==NULL)
		return RT_PARAM_ERR;
	
	tPCCmd.cHead[0]=0xEF;
	tPCCmd.cHead[1]=0x17;
	tPCCmd.cParam = 0;
	tPCCmd.nLen = nBufLen;
	
	nLen=tPCCmd.nLen;
	lWriteLen=sizeof(tPCCmd);
	
	if(!USBSCSIWrite(hHandle,(unsigned char*)&tPCCmd,sizeof(tPCCmd),pBuf,nBufLen,SCSI_TIMEOUT))
		return RT_PACKAGE_ERR;
	
	return RT_OK;
}
/***************************************************************************/
/***************************************************************************/
bool USB_ReceiveRawData( HANDLE hHandle, PBYTE p_pBuffer, UINT p_dwDataLen )
{
	DWORD	w_nDataCnt = p_dwDataLen;
	BYTE	btCDB[8] = {0};
	
	btCDB[0] = 0xEF; btCDB[1] = 0x14;
	if( !USBSCSIRead( hHandle, btCDB, sizeof(btCDB), (PBYTE)p_pBuffer, &w_nDataCnt, SCSI_TIMEOUT ) )
		return FALSE;

	return TRUE;
}

DWORD GetReadWaitTime(int p_nCmdCode)
{
	DWORD	w_dwTime;

	switch(p_nCmdCode)
	{
	default:
		w_dwTime = 100;
		break;
	}

	return w_dwTime;
}