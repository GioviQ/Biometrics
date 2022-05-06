#ifndef _DEVICE_H_
#define _DEVICE_H_

//typedef unsigned long  ULONG_PTR;

#define DEVICE_CDROM  1
#define DEVICE_DRIVER 2

 
BOOL USBSCSIRead(HANDLE hHandle,BYTE* pCDB,DWORD nCDBLen,BYTE*pData,DWORD *nLength,DWORD nTimeOut);
BOOL USBSCSIWrite(HANDLE hHandle,BYTE* pCDB,DWORD nCDBLen,BYTE* pData,DWORD nLength,DWORD nTimeOut);

#endif