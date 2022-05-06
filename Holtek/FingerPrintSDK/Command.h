#if !defined(AFX_COMMAND_H__140A1824_D14D_42C0_A498_48CE7FF70937__INCLUDED_)
#define AFX_COMMAND_H__140A1824_D14D_42C0_A498_48CE7FF70937__INCLUDED_

#include "Serial.h"
#include "Define.h"
//////////////////////////////	STRUCT	////////////////////////////////////////////
#pragma pack(1)

#define		COMM_TIMEOUT					5000

typedef struct _ST_CMD_PACKET_
{
	WORD	m_wPrefix;
	BYTE	m_bySrcDeviceID;
	BYTE	m_byDstDeviceID;
	WORD	m_wCMDCode;
	WORD	m_wDataLen;
	BYTE	m_abyData[16];
	WORD	m_wCheckSum;
} ST_CMD_PACKET, *PST_CMD_PACKET;

typedef struct _ST_RCM_PACKET_
{
	WORD	m_wPrefix;
	BYTE	m_bySrcDeviceID;
	BYTE	m_byDstDeviceID;
	WORD	m_wCMDCode;
	WORD	m_wDataLen;
	WORD	m_wRetCode;
	BYTE	m_abyData[14];
	WORD	m_wCheckSum;
} ST_RCM_PACKET, *PST_RCM_PACKET;

typedef struct _ST_COMMAND_
{
	char	szCommandName[64];
	WORD	wCode;
} ST_COMMAND, *PST_COMMAND;

#pragma pack()

#define	RESPONSE_RET				(g_pRcmPacket->m_wRetCode)
#define	CMD_PACKET_LEN				(sizeof(ST_CMD_PACKET))
#define	DATA_PACKET_LEN				(sizeof(ST_RCM_PACKET))

#define	MAX_DATA_LEN				512
/////////////////////////////	Value	/////////////////////////////////////////////
extern CSerial			g_Serial;
extern BYTE				g_Packet[1024*64];
extern DWORD			g_dwPacketSize;
extern PST_CMD_PACKET	g_pCmdPacket;
extern PST_RCM_PACKET	g_pRcmPacket;
extern ST_COMMAND		g_Commands[];

/////////////////////////////	Function	/////////////////////////////////////////////
#define SEND_COMMAND(cmd, ret, nSrcDeviceID, nDstDeviceID)									\
	if( m_nConnectionMode == SERIAL_CON_MODE)		\
		ret = SendCommand(cmd, nSrcDeviceID, nDstDeviceID);									\
	else if (m_nConnectionMode == USB_CON_MODE)												\
		ret = USB_SendPacket(m_hUsbHandle, cmd, nSrcDeviceID, nDstDeviceID);

#define SEND_DATAPACKET(cmd, ret, nSrcDeviceID, nDstDeviceID)								\
	if( m_nConnectionMode == SERIAL_CON_MODE)		\
		ret = SendDataPacket(cmd, nSrcDeviceID, nDstDeviceID);								\
	else if (m_nConnectionMode == USB_CON_MODE)												\
		ret = USB_SendDataPacket(m_hUsbHandle, cmd, nSrcDeviceID, nDstDeviceID);

#define RECEIVE_DATAPACKET(cmd, ret, nSrcDeviceID, nDstDeviceID)						\
	if (m_nConnectionMode == SERIAL_CON_MODE)		\
		ret = ReceiveDataPacket(cmd, 0, nSrcDeviceID, nDstDeviceID);						\
	else if (m_nConnectionMode == USB_CON_MODE)												\
		ret = USB_ReceiveDataPacket(m_hUsbHandle, cmd, nSrcDeviceID, nDstDeviceID);

WORD	GetCheckSum(BOOL bCmdData);
BOOL	CheckReceive(BYTE* p_pbyPacket, DWORD p_dwPacketLen, WORD p_wPrefix, WORD p_wCMDCode);
void	InitCmdPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen);
void	InitCmdDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen);
BOOL	SendCommand(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);
BOOL	ReceiveAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);

BOOL	SendDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);
BOOL	ReceiveDataAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);
BOOL	ReceiveDataPacket(WORD p_wCMDCode, BYTE p_byDataLen, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);

CString GetErrorMsg(DWORD p_dwErrorCode);

#endif // !defined(AFX_COMMAND_H__140A1824_D14D_42C0_A498_48CE7FF70937__INCLUDED_)
