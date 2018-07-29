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

#include "CCOMProc-x3U4Remocon.h"

#include "WaitWithMsg.h"

#pragma comment(lib, "Strmiids.lib")

CCOMProc::CCOMProc(void)
	: hThread(NULL),
	hThreadInitComp(NULL),
	hTerminateRequest(NULL),
	CallbackFunc(NULL)
{
	hThreadInitComp = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hTerminateRequest = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	::InitializeCriticalSection(&csLock);

	return;
};

CCOMProc::~CCOMProc(void)
{
	if (hThread) {
		::SetEvent(hTerminateRequest);
		::WaitForSingleObject(hThread, 1000);
		::CloseHandle(hThread);
		hThread = NULL;
	}
	if (hThreadInitComp) {
		::CloseHandle(hThreadInitComp);
		hThreadInitComp = NULL;
	}
	if (hTerminateRequest) {
		::CloseHandle(hTerminateRequest);
		hTerminateRequest = NULL;
	}
	::DeleteCriticalSection(&csLock);

	return;
};

void CCOMProc::SetTunerFriendlyName(std::wstring name)
{
	TunerFriendlyName = name;

	return;
};

void CCOMProc::SetPollingInterval(DWORD dwMilliseconds)
{
	PollingInterval = dwMilliseconds;
}

BOOL CCOMProc::CreateThread(void)
{
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

void CCOMProc::TerminateThread(void)
{
	if (hThread) {
		::SetEvent(hTerminateRequest);
		::WaitForSingleObject(hThread, INFINITE);
		CloseThreadHandle();
	}

	return;
}

void CCOMProc::CloseThreadHandle(void)
{
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

void CCOMProc::SetCallback(RemoconCallbackFunc pFunc, LPARAM pParam)
{
	CallbackFunc = pFunc;
	CallbackInstance = pParam;
	return;
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
		std::wstring name;
		std::wstring guid;

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

			terminate = FALSE;
			::SetEvent(pCOMProc->hThreadInitComp);
			OutputDebug(L"COMProcThread: Initialization complete.\n");
			break;
		}
	}
	catch (...) {
	}

	DWORD keycode;
	while (!terminate) {
		DWORD ret = WaitForSingleObjectWithMessageLoop(pCOMProc->hTerminateRequest, pCOMProc->PollingInterval);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			OutputDebug(L"COMProcThread: Terminate was requested.\n");
			terminate = TRUE;
			break;

		case WAIT_TIMEOUT:
			keycode = 0;
			if (SUCCEEDED(hr = it35_ReadRawIR(pIKsPropertySet, &keycode))) {
				if (keycode != 0) {
					if (pCOMProc->CallbackFunc) {
						pCOMProc->CallbackFunc(keycode, pCOMProc->CallbackInstance);
					}
				}
			}

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
