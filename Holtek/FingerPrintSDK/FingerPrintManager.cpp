#include "stdafx.h"
#include "Communication.h"

using namespace System;
using namespace System::Runtime::InteropServices;

namespace FingerPrintSDK {

	public enum class ConnectionMode : int { SERIAL, USB };

	public enum class BaudRates {
		BAUD9600 = 0, BAUD19200, BAUD38400, BAUD57600, BAUD115200, BAUD230400, BAUD460800, BAUD921600
	};

	public enum class ErrorCode : byte {
		SUCCESS = 0,
		FAIL = 1,
		CONNECTION = 2,
		VERIFY = 0x10,
		IDENTIFY = 0x11,
		TMPL_EMPTY = 0x12,
		TMPL_NOT_EMPTY = 0x13,
		ALL_TMPL_EMPTY = 0x14,
		EMPTY_ID_NOEXIST = 0x15,
		BROKEN_ID_NOEXIST = 0x16,
		INVALID_TMPL_DATA = 0x17,
		DUPLICATION_ID = 0x18,
		BAD_QUALITY = 0x19,
		MERGE_FAIL = 0x1A,
		NOT_AUTHORIZED = 0x1B,
		MEMORY = 0x1C,
		INVALID_TMPL_NO = 0x1D,
		INVALID_PARAM = 0x22,
		GEN_COUNT = 0x25,
		INVALID_BUFFER_ID = 0x26,
		INVALID_OPERATION_MODE = 0x27,
		FP_NOT_DETECTED = 0x28
	};

	public ref class FingerPrintManager
	{
	private:
		CCommunication * _CCommunication;
	public:
		FingerPrintManager()
		{
			_CCommunication = new CCommunication();
		}
		~FingerPrintManager()
		{
			_CCommunication->CloseConnection();
			delete _CCommunication;
		}

		bool TestConnection()
		{
			return _CCommunication->Run_TestConnection() == 0;
		}

		int SetParam(int index, int value)
		{
			return _CCommunication->Run_SetParam(index, value);
		}

		int GetParam(int index, [OutAttribute] int % value)
		{
			int p_pnParamValue;
			int retValue = _CCommunication->Run_GetParam(index, &p_pnParamValue);

			value = p_pnParamValue;

			return retValue;
		}

		int GetDeviceInfo(String ^devInfo)
		{
			return _CCommunication->Run_GetDeviceInfo((char *)Marshal::StringToHGlobalAnsi(devInfo).ToPointer());
		}

		int EnterISPMode()
		{
			return _CCommunication->Run_EnterISPMode();
		}

		int SetIDNote(int tmplN, BYTE* pNote)
		{
			return _CCommunication->Run_SetIDNote(tmplN, pNote);
		}

		int GetIDNote(int tmplN, BYTE* pNote)
		{
			return _CCommunication->Run_GetIDNote(tmplN, pNote);
		}

		int SetModuleSN(BYTE* pModuleSN)
		{
			return _CCommunication->Run_SetModuleSN(pModuleSN);
		}

		int GetModuleSN(BYTE* pModuleSN)
		{
			return _CCommunication->Run_GetModuleSN(pModuleSN);
		}

		int GetImage()
		{
			return _CCommunication->Run_GetImage();
		}

		int FingerDetect(int* p_pnDetectResult)
		{
			return _CCommunication->Run_FingerDetect(p_pnDetectResult);
		}

		int UpImage(int p_nType, BYTE* p_pData, int* p_pnImgWidth, int* p_pnImgHeight)
		{
			return _CCommunication->Run_UpImage(p_nType, p_pData, p_pnImgWidth, p_pnImgHeight);
		}

		int DownImage(BYTE* p_pData, int p_nWidth, int p_nHeight)
		{
			return _CCommunication->Run_DownImage(p_pData, p_nWidth, p_nHeight);
		}

		int SLEDControl(int p_nState)
		{
			return _CCommunication->Run_SLEDControl(p_nState);
		}

		/// <summary>
		/// Memorizza il template presente nel buffer bufferId con ID tmplN e restituisce
		/// eventualmente il numero del template duplicato identificato
		/// </summary>
		int StoreChar(int tmplN, int bufferId, [OutAttribute] int % dupTmplN)
		{
			int p_pnDupTmplNo;
			int retValue = _CCommunication->Run_StoreChar(tmplN, bufferId, &p_pnDupTmplNo);

			dupTmplN = p_pnDupTmplNo;

			return retValue;
		}

		/// <summary>
		/// Carica nel buffer bufferID il template con ID tmplN
		/// </summary>
		int LoadChar(int tmplN, int bufferId)
		{
			return _CCommunication->Run_LoadChar(tmplN, bufferId);
		}

		/// <summary>
		/// Carica nel vettore tmplBytes il buffer bufferId 
		/// </summary>
		int UpChar(int bufferId, [OutAttribute] array<BYTE>^ % tmplBytes)
		{
			tmplBytes = gcnew array<BYTE>(GD_RECORD_SIZE);

			pin_ptr<BYTE> p_pbyTemplate = &tmplBytes[0];

			return _CCommunication->Run_UpChar(bufferId, p_pbyTemplate);
		}

		/// <summary>
		/// Carica nel buffer bufferId il vettore tmplBytes
		/// </summary>
		int DownChar(int bufferId, array<BYTE>^ tmplBytes)
		{
			pin_ptr<BYTE> p_pbyTemplate = &tmplBytes[0];

			return _CCommunication->Run_DownChar(bufferId, p_pbyTemplate);
		}

		int DelChar(int tmplNo)
		{
			return _CCommunication->Run_DelChar(tmplNo, tmplNo);
		}

		int DelAll()
		{
			return _CCommunication->Run_DelChar(1, GD_MAX_RECORD_COUNT);
		}

		bool GetEmptyID([OutAttribute] int % emptyID)
		{
			int p_pnEmptyID;

			int retValue = _CCommunication->Run_GetEmptyID(1, GD_MAX_RECORD_COUNT, &p_pnEmptyID);

			emptyID = p_pnEmptyID;

			return retValue == 0;
		}

		bool GetStatus(int tmplNo, [OutAttribute] bool % isEmpty)
		{
			int status;

			int retValue = _CCommunication->Run_GetStatus(tmplNo, &status);

			isEmpty = status == 0;

			return retValue == 0;
		}

		int GetBrokenID(int p_nSTmplNo, int p_nETmplNo, int* p_pnCount, int* p_pnFirstID)
		{
			return _CCommunication->Run_GetBrokenID(p_nSTmplNo, p_nETmplNo, p_pnCount, p_pnFirstID);
		}

		int GetEnrollCount([OutAttribute] int % count)
		{
			int p_pnEnrollCount;

			int retValue = _CCommunication->Run_GetEnrollCount(1, GD_MAX_RECORD_COUNT, &p_pnEnrollCount);

			count = p_pnEnrollCount;

			return retValue;
		}

		//Genera un template nel buffer bufferID
		int Generate(int bufferID)
		{
			return _CCommunication->Run_Generate(bufferID);
		}

		int Merge(int bufferId, int mergeCount)
		{
			return _CCommunication->Run_Merge(bufferId, mergeCount);
		}

		int Match(int bufferId0, int bufferId1, int* p_pnLearnResult)
		{
			return _CCommunication->Run_Match(bufferId0, bufferId1, p_pnLearnResult);
		}

		int Search([OutAttribute] int % tmplNo, [OutAttribute] int % learnResult)
		{
			int p_pnTmplNo, p_pnLearnResult;

			int retValue = _CCommunication->Run_Search(0, 1, GD_MAX_RECORD_COUNT, &p_pnTmplNo, &p_pnLearnResult);

			tmplNo = p_pnTmplNo;
			learnResult = p_pnLearnResult;

			return retValue;
		}

		int Verify(int tmplN, int bufferId, int* p_pnLearnResult)
		{
			return _CCommunication->Run_Verify(tmplN, bufferId, p_pnLearnResult);
		}

		bool InitConnection(ConnectionMode conMode, String^ comPort, BaudRates baudRate)
		{
			return _CCommunication->InitConnection((int)conMode, comPort, (int)baudRate, 0, 0) == 0;
		}

		BOOL EnableCommunicaton(int p_nDevNum, BOOL p_bVerifyDeviceID, BYTE* p_pDevPwd, BOOL p_bMsgOut)
		{
			return _CCommunication->EnableCommunicaton(p_nDevNum, p_bVerifyDeviceID, p_pDevPwd, p_bMsgOut);
		}

		BOOL OpenSerialPort(CString p_strComPortIndex, int p_nBaudRateIndex)
		{
			return _CCommunication->OpenSerialPort(p_strComPortIndex, p_nBaudRateIndex);
		}

		BOOL Command_NP(BYTE p_byCMD)
		{
			return _CCommunication->Run_Command_NP(p_byCMD);
		}

		void CloseConnection()
		{
			_CCommunication->CloseConnection();
		}

		void SetIPandPort(CString strDestination, DWORD dwPort)
		{
			_CCommunication->SetIPandPort(strDestination, dwPort);
		}
	};

}