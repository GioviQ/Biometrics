#include "stdafx.h"
#include <afxmt.h>
#include "stdio.h"

#include "device.h"
#include<devguid.h>
#include <dbt.h>
extern "C"{
#include <setupapi.h>   // from MS Platform SDK
}

#pragma comment(lib, "Setupapi.lib" )
#include "direct.h"
#include <time.h>
#include <winioctl.h>
#include <windows.h>
#include <Basetsd.h>
#include <usbioctl.h>
#include <devioctl.h>
//#include <ntdddisk.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "spti.h" 
//#include "Protocol.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


BOOL USBSCSIRead(HANDLE hHandle,BYTE* pCDB,DWORD nCDBLen,BYTE*pData,DWORD *nLength,DWORD nTimeOut)
{
 
	BOOL status = 0;  
	
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    DWORD length = 0,           
		returned = 0,
		sectorSize = 512;
	DWORD TransLen;
	
	length=*nLength;
	
	if (hHandle == INVALID_HANDLE_VALUE) 
		return 0;
	
    ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));      
	
	TransLen=*nLength;  
	
    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB6GENERIC_LENGTH;
    sptdwb.sptd.SenseInfoLength = 0;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    sptdwb.sptd.DataTransferLength =TransLen;
 
    sptdwb.sptd.TimeOutValue = nTimeOut;
    sptdwb.sptd.DataBuffer = pData;
    sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
	memcpy(sptdwb.sptd.Cdb,pCDB,nCDBLen);

	SetLastError( 0 );

    length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    status = DeviceIoControl(hHandle,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		nLength,
		FALSE);
	*nLength=sptdwb.sptd.DataTransferLength;

	if ( status == FALSE  || sptdwb.sptd.ScsiStatus)
	{
		length = GetLastError();
		return FALSE;
	}

	return status;
}

BOOL USBSCSIWrite(HANDLE hHandle,BYTE* pCDB,DWORD nCDBLen,BYTE* pData,DWORD nLength,DWORD nTimeOut)
{
 
	BOOL status = 0;  
	
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    DWORD length = 0,           
		returned = 0,
		sectorSize = 512;
	if (hHandle == INVALID_HANDLE_VALUE) 
		return 0;
	DWORD TransLen=nLength;
   
    ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER)); 	
	
    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB6GENERIC_LENGTH;
    sptdwb.sptd.SenseInfoLength = 0;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
    sptdwb.sptd.DataTransferLength = TransLen;
 
    sptdwb.sptd.TimeOutValue = nTimeOut;
    sptdwb.sptd.DataBuffer = pData;
    sptdwb.sptd.SenseInfoOffset =
		offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    memcpy(sptdwb.sptd.Cdb,pCDB,nCDBLen);
    length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	SetLastError( 0 );
    status = DeviceIoControl(hHandle,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb,
		length,
		&sptdwb,
		length,
		&returned,
		FALSE);	 

	if ( status == FALSE || sptdwb.sptd.ScsiStatus )
	{
		length = GetLastError();
		return FALSE;
	}

	return status;

}
 
