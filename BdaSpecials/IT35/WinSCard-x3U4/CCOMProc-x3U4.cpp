#include <Windows.h>

#include <iostream>
#include <DShow.h>

// KSCATEGORY_...
#include <ks.h>
#pragma warning (push)
#pragma warning (disable: 4091)
#include <ksmedia.h>
#pragma warning (pop)
#include <bdatypes.h>
#include <bdamedia.h>

#include "common.h"
#include "IT35propset.h"
#include "DSFilterEnum.h"

#include "CCOMProc-x3U4.h"

#include "WaitWithMsg.h"

#pragma comment(lib, "Strmiids.lib")

using namespace std;

CCOMProc::CCOMProc(void)
	: hThread(NULL),
	hThreadInitComp(NULL),
	hReqEvent(NULL),
	hEndEvent(NULL),
	hTerminateRequest(NULL)
{
	hThreadInitComp = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hReqEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEndEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hTerminateRequest = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	::InitializeCriticalSection(&csLock);

	return;
};

CCOMProc::~CCOMProc(void)
{
	TerminateThread();
	if (hThreadInitComp) {
		::CloseHandle(hThreadInitComp);
		hThreadInitComp = NULL;
	}
	if (hReqEvent) {
		::CloseHandle(hReqEvent);
		hReqEvent = NULL;
	}
	if (hEndEvent) {
		::CloseHandle(hEndEvent);
		hEndEvent = NULL;
	}
	if (hTerminateRequest) {
		::CloseHandle(hTerminateRequest);
		hTerminateRequest = NULL;
	}
	::DeleteCriticalSection(&csLock);

	return;
};

void CCOMProc::SetTunerFriendlyName(wstring name) {
	TunerFriendlyName = name;

	return;
};

wstring CCOMProc::GetTunerDisplayName(void) {
	return TunerDisplayName;
}

BOOL CCOMProc::CreateThread(void) {
	HANDLE hThreadTemp = NULL;
	hThreadTemp = ::CreateThread(NULL, 0, COMProcThread, this, 0, NULL);
	if (!hThreadTemp)
		return FALSE;

	HANDLE h[2] = {
		hThreadTemp,
		hThreadInitComp,
	};

	DWORD ret = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	switch (ret)
	{
	case WAIT_OBJECT_0 + 1:
		break;

	case WAIT_OBJECT_0:
	default:
		try {
			::CloseHandle(hThreadTemp);
		}
		catch (...) {
		}
		return FALSE;
	}

	hThread = hThreadTemp;
	return true;
}

void CCOMProc::TerminateThread(void) {
	if (hThread) {
		::SetEvent(hTerminateRequest);
		::WaitForSingleObject(hThread, INFINITE);
		CloseThreadHandle();
	}

	return;
}

void CCOMProc::CloseThreadHandle(void) {
	if (hThread) {
		try {
			::CloseHandle(hThread);
		}
		catch (...) {
		}
		hThread = NULL;
	}

	return;
}

HRESULT CCOMProc::DetectCard(BOOL *Present)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqDetectCard;
	Param.DetectCard.pbPresent = Present;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

HRESULT CCOMProc::IsUARTReady(BOOL *Ready)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqIsUARTReady;
	Param.IsUARTReady.pbReady = Ready;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

HRESULT CCOMProc::SendUART(const BYTE *SendBuffer, DWORD SendLength)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqSendUART;
	Param.SendUART.pcbySendBuffer = SendBuffer;
	Param.SendUART.dwSendLength = SendLength;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

HRESULT CCOMProc::GetUARTData(BYTE *RecvBuffer, DWORD *RecvLength)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqGetUARTData;
	Param.GetUARTData.pbyRecvBuffer = RecvBuffer;
	Param.GetUARTData.pdwRecvLength = RecvLength;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

HRESULT CCOMProc::SetUARTBaudRate(WORD BaudRate)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqSetUARTBaudRate;
	Param.SetUARTBaudRate.wBaudRate = BaudRate;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

HRESULT CCOMProc::ResetCard(void)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqResetCard;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

HRESULT CCOMProc::GetATRData(BYTE *Buffer)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqGetATRData;
	Param.GetATRData.pbyBuffer = Buffer;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

HRESULT CCOMProc::PrivateIOControl(DWORD Code)
{
	if (hThread == NULL)
		return E_POINTER;

	DWORD dw;
	HRESULT ret = E_HANDLE;

	::EnterCriticalSection(&csLock);

	Request = eCOMReqPrivateIOControl;
	Param.PrivateIOControl.dwCode = Code;
	::SetEvent(hReqEvent);
	HANDLE h[2] = {
		hEndEvent,
		hThread
	};
	dw = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (dw == WAIT_OBJECT_0) {
		ret = RetCode;
	}
	else {
		CloseThreadHandle();
	}

	::LeaveCriticalSection(&csLock);
	return ret;
}

DWORD WINAPI CCOMProc::COMProcThread(LPVOID lpParameter)
{
	IBaseFilter *pTunerDevice = NULL;
	IKsPropertySet *pIKsPropertySet = NULL;
	BOOL terminate = TRUE;
	CCOMProc* pCOMProc = (CCOMProc*)lpParameter;

	HRESULT hr;

	OutputDebug(L"COMProcThread: Thread created.\n");

	// COM初期化
	OutputDebug(L"COMProcThread: Doing CoInitializeEx().\n");
	hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
	OutputDebug(L"COMProcThread: CoInitializeEx() returned 0x%x.\n", hr);

	try {
		wstring name;
		wstring guid;

		CDSFilterEnum dsfEnum(KSCATEGORY_BDA_NETWORK_TUNER, CDEF_DEVMON_PNP_DEVICE);

		// 名前の一致するチューナーデバイスを探す
		while (SUCCEEDED(hr = dsfEnum.next()) && hr == S_OK) {
			dsfEnum.getFriendlyName(&name);
			dsfEnum.getDisplayName(&guid);
			if (size_t pos = name.find(pCOMProc->TunerFriendlyName) != 0)
				continue;

			// チューナーデバイス取得
			if (FAILED(hr = dsfEnum.getFilter(&pTunerDevice))) {
				OutputDebug(L"COMProcThread: Error in get filter.\n");
				break;
			}

			// IKsPropertySet を取得
			if (FAILED(hr = pTunerDevice->QueryInterface(IID_IKsPropertySet, (LPVOID*)&pIKsPropertySet))) {
				OutputDebug(L"COMProcThread: Error in get IKsPropertySet.\n");
				break;
			}

			// これをやっておかないと、Bondriverがチューナーをオープンしている時しか CARD にアクセスできない
			hr = it35_DigibestPrivateIoControl(pIKsPropertySet, PRIVATE_IO_CTL_FUNC_UNPROTECT_TUNER_POWER);
			hr = it35_DigibestPrivateIoControl(pIKsPropertySet, PRIVATE_IO_CTL_FUNC_SET_TUNER_POWER_ON);
			hr = it35_DigibestPrivateIoControl(pIKsPropertySet, PRIVATE_IO_CTL_FUNC_PROTECT_TUNER_POWER);

			// チューナーのGUIDを保存
			pCOMProc->TunerDisplayName = guid;
			terminate = FALSE;
			::SetEvent(pCOMProc->hThreadInitComp);
			OutputDebug(L"COMProcThread: Initialization complete.\n");
			break;
		}
	}
	catch (...) {
	}

	HANDLE h[2] = {
		pCOMProc->hTerminateRequest,
		pCOMProc->hReqEvent
	};

	while (!terminate) {
		DWORD ret = WaitForMultipleObjectsWithMessageLoop(2, h, FALSE, INFINITE);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			OutputDebug(L"COMProcThread: Terminate was requested.\n");
			terminate = TRUE;
			break;

		case WAIT_OBJECT_0 + 1:
			switch (pCOMProc->Request)
			{

			case eCOMReqDetectCard:
				pCOMProc->RetCode = it35_CardDetect(pIKsPropertySet, pCOMProc->Param.DetectCard.pbPresent);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			case eCOMReqIsUARTReady:
				pCOMProc->RetCode = it35_IsUartReady(pIKsPropertySet, pCOMProc->Param.IsUARTReady.pbReady);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			case eCOMReqSendUART:
				pCOMProc->RetCode = it35_SentUart(pIKsPropertySet, (BYTE*)(pCOMProc->Param.SendUART.pcbySendBuffer), pCOMProc->Param.SendUART.dwSendLength);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			case eCOMReqGetUARTData:
				pCOMProc->RetCode = it35_GetUartData(pIKsPropertySet, pCOMProc->Param.GetUARTData.pbyRecvBuffer, pCOMProc->Param.GetUARTData.pdwRecvLength);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			case eCOMReqSetUARTBaudRate:
				pCOMProc->RetCode = it35_SetUartBaudRate(pIKsPropertySet, pCOMProc->Param.SetUARTBaudRate.wBaudRate);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			case eCOMReqResetCard:
				pCOMProc->RetCode = it35_ResetSmartCard(pIKsPropertySet);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			case eCOMReqGetATRData:
				pCOMProc->RetCode = it35_GetATR(pIKsPropertySet, pCOMProc->Param.GetATRData.pbyBuffer);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			case eCOMReqPrivateIOControl:
				pCOMProc->RetCode = it35_DigibestPrivateIoControl(pIKsPropertySet, pCOMProc->Param.PrivateIOControl.dwCode);
				::SetEvent(pCOMProc->hEndEvent);
				break;

			default:
				break;
			} // switch (pCOMProc->Request)
			break;

		case WAIT_TIMEOUT:
			break;

		case WAIT_FAILED:
		default:
			DWORD err = ::GetLastError();
			OutputDebug(L"COMProcThread: Unknown error (ret=%d, LastError=0x%08x).\n", ret, err);
			terminate = TRUE;
			break;
		} // switch (ret)
	} // while (!terminate)

	SAFE_RELEASE(pIKsPropertySet);
	SAFE_RELEASE(pTunerDevice);

	OutputDebug(L"COMProcThread: Doing CoUninitialize().\n");
	::CoUninitialize();
	OutputDebug(L"COMProcThread: Thread terminated.\n");

	return 0;
}
