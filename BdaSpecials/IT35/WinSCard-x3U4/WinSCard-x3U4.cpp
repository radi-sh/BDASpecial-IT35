#include <Windows.h>

#include <iostream>
#include <atlbase.h>
#include <DShow.h>

// KSCATEGORY_...
#include <ks.h>
#pragma warning (push)
#pragma warning (disable: 4091)
#include <ksmedia.h>
#pragma warning (pop)
#include <bdatypes.h>
#include <bdamedia.h>

#include "WinSCard-x3U4.h"

#include "common.h"
#include "IT35propset.h"
#include "atr.h"
#include "t1.h"
#include "DSFilterEnum.h"

#pragma comment(lib, "Strmiids.lib")

using namespace std;

FILE *g_fpLog = NULL;

static const SCARDHANDLE DUMMY_SCARDHANDLE = 0x5ca2d4a1;
static const SCARDCONTEXT DUMMY_SCARDCONTEXT = 0xc013e103;
static const CHAR READER_NAME_A[] = "Plex PX-x3U4 Card Reader 0";
static const WCHAR READER_NAME_W[] = L"Plex PX-x3U4 Card Reader 0";
static const CHAR LIST_READERS_A[] = "Plex PX-x3U4 Card Reader 0\0";
static const WCHAR LIST_READERS_W[] = L"Plex PX-x3U4 Card Reader 0\0";

static HANDLE startedEvent = NULL;
static IBaseFilter *l_pTunerDevice = NULL;
static IKsPropertySet *l_pIKsPropertySet = NULL;
static HANDLE l_hSemaphore = NULL;
static HMODULE l_hModule = NULL;

static CParseATR ParseATR;

static BOOL cardReady = FALSE;

static class CComProtocolT1x3U4 : public CComProtocolT1 {
public:
	CComProtocolT1x3U4(void)
	{
		IgnoreEDCError = FALSE;
	};
	virtual COM_PROTOCOL_T1_ERROR_CODE TxBlock(void)
	{
		HRESULT hr;
		if (!l_pIKsPropertySet)
			return COM_PROTOCOL_T1_E_POINTER;
		BOOL present = FALSE;
		if (FAILED(hr = it35_CardDetect(l_pIKsPropertySet, &present))) {
			OutputDebug(L"TxBlock: Error in it35_CardDetect(). code=0x%x\n", hr);
			return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
		}
		if (!present) {
			OutputDebug(L"TxBlock: Card not present.\n", hr);
			return COM_PROTOCOL_T1_E_NO_CARD;
		}
		::Sleep(50);
		if (FAILED(hr = it35_SentUart(l_pIKsPropertySet, SendFrame, SendFrameLen))) {
			OutputDebug(L"TxBlock: Error in it35_SentUart(). code=0x%x\n", hr);
			return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
		}
		return COM_PROTOCOL_T1_S_NO_ERROR;
	};
	virtual COM_PROTOCOL_T1_ERROR_CODE RxBlock(void)
	{
		HRESULT hr;
		if (!l_pIKsPropertySet)
			return COM_PROTOCOL_T1_E_POINTER;
		BOOL ready = FALSE;
		int retry = 0;
		while (1) {
			if (FAILED(hr = it35_IsUartReady(l_pIKsPropertySet, &ready))) {
				OutputDebug(L"RxBlock: Error in it35_IsUartReady(). code=0x%x\n", hr);
				return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
			}
			if (ready)
				break;
			retry++;
			if (retry > 20) {
				OutputDebug(L"RxBlock: Retry time out in it35_IsUartReady().\n");
				return COM_PROTOCOL_T1_E_NOT_READY;
			}
			Sleep(50);
		}
		DWORD len = sizeof(RecvFrame);
		if (FAILED(hr = it35_GetUartData(l_pIKsPropertySet, RecvFrame, &len))) {
			OutputDebug(L"RxBlock: Error in it35_GetUartData(). code=0x%x\n", hr);
			return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
		}
		RecvFrameLen = len;
		return COM_PROTOCOL_T1_S_NO_ERROR;
	};
} Protocol;

class LockProc {
private:
	DWORD result;
public:
	LockProc(DWORD dwMilliSeconds)
	{
		result = ::WaitForSingleObject(l_hSemaphore, dwMilliSeconds);
		return;
	};
	LockProc(void)
	{
		result = ::WaitForSingleObject(l_hSemaphore, 10000);
		return;
	};
	~LockProc(void)
	{
		::ReleaseSemaphore(l_hSemaphore, 1, NULL);
	};
	BOOL IsSuccess(void)
	{
		return (result == WAIT_OBJECT_0);
	};
};

static HRESULT GetTunerDevice(void) {
	// iniファイルのpath取得
	WCHAR szIniFilePath[_MAX_PATH + 1];
	::GetModuleFileNameW(l_hModule, szIniFilePath, sizeof(szIniFilePath) / sizeof(szIniFilePath[0]));
	::wcscpy_s(szIniFilePath + ::wcslen(szIniFilePath) - 3, 4, L"ini");

	// tunerのFriendlyName取得
	WCHAR buf[256];
	wstring friendlyName;
	::GetPrivateProfileStringW(L"SCard", L"TunerFriendlyName", L"PXW3U4 Multi Tuner ISDB-T BDA Filter #0", buf, sizeof(buf) / sizeof(buf[0]), szIniFilePath);
	::GetPrivateProfileStringW(L"SCard", L"FriendlyName", buf, buf, sizeof(buf) / sizeof(buf[0]), szIniFilePath);
	friendlyName = buf;

	// DebugLogを記録するかどうか
	if (::GetPrivateProfileIntW(L"SCard", L"DebugLog", 0, szIniFilePath)) {
		// INIファイルのファイル名取得
		WCHAR szDebugLogPath[_MAX_PATH + 1];
		::wcscpy_s(szDebugLogPath, ::wcslen(szIniFilePath) + 1, szIniFilePath);
		::wcscpy_s(szDebugLogPath + ::wcslen(szIniFilePath) - 3, 4, L"log");
		SetDebugLog(szDebugLogPath);
	}
	if (::GetPrivateProfileIntW(L"SCard", L"DetailLog", 0, szIniFilePath))
		Protocol.SetDetailLog(TRUE);

	try {
		HRESULT hr;
		wstring name;
		wstring guid;
		wstring semname;
		size_t pos;
		CDSFilterEnum dsfEnum(KSCATEGORY_BDA_NETWORK_TUNER, CDEF_DEVMON_PNP_DEVICE);

		while (SUCCEEDED(hr = dsfEnum.next()) && hr == S_OK) {
			dsfEnum.getFriendlyName(&name);
			dsfEnum.getDisplayName(&guid);
			if (pos = name.find(friendlyName) != 0)
				continue;

			if (FAILED(hr = dsfEnum.getFilter(&l_pTunerDevice))) {
				return hr;
			}

			if (FAILED(hr = l_pTunerDevice->QueryInterface(IID_IKsPropertySet, (LPVOID*)&l_pIKsPropertySet))) {
				return hr;
			}

			// プロセス間排他用のセマフォ作成
			wstring::size_type n, last;
			n = last = 0;
			while ((n = guid.find(L'#', n)) != wstring::npos) {
				last = n;
				n++;
			}
			if (last != 0)
				semname = guid.substr(0, last);
			else
				semname = guid;
			n = 0;
			while ((n = semname.find(L'\\', n)) != wstring::npos) {
				semname.replace(n, 1, 1, L'/');
			}
			semname = L"Global\\WinSCard-x3U4" + semname;
			l_hSemaphore = ::CreateSemaphoreW(NULL, 1, 1, semname.c_str());
			if (!l_hSemaphore) {
				return E_FAIL;
			}

			// これをやっておかないと、Bondriverがチューナーをオープンしている時しか CARD にアクセスできない
			hr = it35_DigibestPrivateIoControl(l_pIKsPropertySet, PRIVATE_IO_CTL_FUNC_UNPROTECT_TUNER_POWER);
			hr = it35_DigibestPrivateIoControl(l_pIKsPropertySet, PRIVATE_IO_CTL_FUNC_SET_TUNER_POWER_ON);
			hr = it35_DigibestPrivateIoControl(l_pIKsPropertySet, PRIVATE_IO_CTL_FUNC_PROTECT_TUNER_POWER);

			return S_OK;
		}
		// 見つからなかった
		return E_FAIL;
	}
	catch (...) {
		return E_FAIL;
	}
}

static HRESULT ResetCard(void)
{
	if (!l_pIKsPropertySet) {
		return E_POINTER;
	}

	HRESULT hr;
	BYTE atr[33];
	BYTE buf[3 + 254 + 2] = {};
	BOOL b;
	BYTE pcb;
	BYTE l;
	DWORD len;
	CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE r;

	// Cardが存在しているか確認
	if (FAILED(hr = it35_CardDetect(l_pIKsPropertySet, &b))) {
		OutputDebug(L"ResetCard: Error in it35_CardDetect(). code=0x%x\n", hr);
		return hr;
	}
	if (!b) {
		return E_FAIL;
	}

	if (ParseATR.ParsedInfo.Convention != CParseATR::CONVENTION_DIRECT ||
			(r = Protocol.SendSBlock(FALSE, CComProtocolT1::SBLOCK_FUNCTION_RESYNC)) != 0 || 
			(r = Protocol.RecvBlock(&pcb, buf, &l)) != 0 || 
			pcb != (CComProtocolT1::SBLOCK_FUNCTION_RESYNC | CComProtocolT1::SBLOCK_RESPONSE)) {
		// Card リセット処理必要
		OutputDebug(L"ResetCard: Need reset.\n");
		LockProc Lock;
		if (!Lock.IsSuccess()) {
			OutputDebug(L"ResetCard: Error in Lock.\n");
			return E_FAIL;
		}

		// Card リセット
		if (FAILED(hr = it35_ResetSmartCard(l_pIKsPropertySet))) {
			OutputDebug(L"ResetCard: Error in it35_ResetSmartCard(). code=0x%x\n", hr);
			return hr;
		}

		::Sleep(50);

		// ATR取得
		if (FAILED(hr = it35_GetATR(l_pIKsPropertySet, atr))) {
			OutputDebug(L"ResetCard: Error in it35_GetATR(). code=0x%x\n", hr);
			return hr;
		}

		len = sizeof(atr);
		if (FAILED(hr = it35_GetUartData(l_pIKsPropertySet, atr, &len))) {
			OutputDebug(L"ResetCard: Error in it35_GetUartData(). code=0x%x\n", hr);
			return hr;
		}

		::Sleep(50);

		// ボーレートセット...9600か19200しか受付けないっぽい...
		if (FAILED(hr = it35_SetUartBaudRate(l_pIKsPropertySet, 19200))) {
			OutputDebug(L"ResetCard: Error in it35_SetUartBaudRate(). code=0x%x\n", hr);
			return hr;
		}

		/*
		// Resync 要求
		if ((r = Protocol.SendSBlock(FALSE, CComProtocolT1::SBLOCK_FUNCTION_RESYNC)) != 0) {
			OutputDebug(L"ResetCard: Error in SendSBlock(). code=%d\n", r);
			return E_FAIL;
		}

		// レスポンス取得
		if ((r = Protocol.RecvBlock(&pcb, buf, &l)) != 0) {
			OutputDebug(L"ResetCard: Error in RecvBlock(). code=%d\n", r);
			return E_FAIL;
		}
		*/
		OutputDebug(L"ResetCard: Reset Complete.\n");
	}
	else {
		OutputDebug(L"ResetCard: No need reset.\n");
	}

	// ATR 解析
	ParseATR.Parse(atr, sizeof(atr));
	Protocol.SetCardIFSC(ParseATR.ParsedInfo.IFSC);
	Protocol.SetEDCType(ParseATR.ParsedInfo.ErrorDetection == CParseATR::ERROR_DETECTION_CRC ? CComProtocolT1::EDC_TYPE_CRC : CComProtocolT1::EDC_TYPE_LRC);
	Protocol.SetNodeAddress(0, 0);

	cardReady = TRUE;
	return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	HRESULT hr;
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		l_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
		startedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
		break;
	case DLL_PROCESS_DETACH:
		SAFE_RELEASE(l_pIKsPropertySet);
		SAFE_RELEASE(l_pTunerDevice);
		if (startedEvent)
			CloseHandle(startedEvent);
		if (l_hSemaphore) {
			try {
				::CloseHandle(l_hSemaphore);
				l_hSemaphore = NULL;
			}
			catch (...) {
			}
		}
		::CoUninitialize();
		// デバッグログファイルのクローズ
		CloseDebugLog();
		break;
	}

	return TRUE;
}

LONG WINAPI SCardConnectA_(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	HRESULT hr;
	
	if (!l_pIKsPropertySet)
		hr = GetTunerDevice();

	if (!cardReady) {
		if (FAILED(hr = ResetCard()))
		{
			return E_FAIL;
		}
	}

	*phCard = DUMMY_SCARDHANDLE;
	*pdwActiveProtocol = SCARD_PROTOCOL_T1;

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardConnectW_(SCARDCONTEXT hContext, LPWSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	HRESULT hr;

	if (!l_pIKsPropertySet)
		hr = GetTunerDevice();

	if (!cardReady) {
		if (FAILED(hr = ResetCard()))
		{
			return E_FAIL;
		}
	}

	*phCard = DUMMY_SCARDHANDLE;
	*pdwActiveProtocol = SCARD_PROTOCOL_T1;

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardDisconnect_(SCARDHANDLE hCard, DWORD dwDisposition)
{
	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardEstablishContext_(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext)
{
	*phContext = DUMMY_SCARDCONTEXT;

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardFreeMemory_(SCARDCONTEXT hContext, LPCVOID pvMem)
{
	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardGetStatusChangeA_(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATEA rgReaderStates, DWORD cReaders)
{
	rgReaderStates->dwEventState = SCARD_STATE_PRESENT;

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardGetStatusChangeW_(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATEW rgReaderStates, DWORD cReaders)
{
	rgReaderStates->dwEventState = SCARD_STATE_PRESENT;

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardIsValidContext_(SCARDCONTEXT hContext)
{
	return hContext ? SCARD_S_SUCCESS : ERROR_INVALID_HANDLE;
}

LONG WINAPI SCardListReadersA_(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders)
{
	if (pcchReaders) {
		if (mszReaders) {
			if (*pcchReaders == SCARD_AUTOALLOCATE)
				*(LPCSTR*)mszReaders = LIST_READERS_A;
			else
				memcpy(mszReaders, LIST_READERS_A, sizeof(LIST_READERS_A));
		}
		*pcchReaders = sizeof(LIST_READERS_A) / sizeof(LIST_READERS_A[0]);
	}

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardListReadersW_(SCARDCONTEXT hContext, LPCWSTR mszGroups, LPWSTR mszReaders, LPDWORD pcchReaders)
{
	if (pcchReaders) {
		if (mszReaders) {
			if (*pcchReaders == SCARD_AUTOALLOCATE)
				*(LPCWSTR*)mszReaders = LIST_READERS_W;
			else
				memcpy(mszReaders, LIST_READERS_W, sizeof(LIST_READERS_W));
		}
		*pcchReaders = sizeof(LIST_READERS_W) / sizeof(LIST_READERS_W[0]);
	}

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardReleaseContext_(SCARDCONTEXT hContext)
{
	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardStatusA_(SCARDHANDLE hCard, LPSTR szReaderNames, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
	if (pcchReaderLen) {
		if (szReaderNames) {
			if (*pcchReaderLen == SCARD_AUTOALLOCATE)
				*(LPCSTR*)szReaderNames = READER_NAME_A;
			else
				memcpy(szReaderNames, READER_NAME_A, sizeof(READER_NAME_A));
		}
		*pcchReaderLen = sizeof(READER_NAME_A) / sizeof(READER_NAME_A[0]);
	}
	if (pdwState)
		*pdwState = SCARD_PRESENT;
	if (pdwProtocol)
		*pdwProtocol = SCARD_PROTOCOL_T1;
	if (pcbAtrLen) {
		if (pbAtr) {
			if (*pcbAtrLen == SCARD_AUTOALLOCATE)
				*(LPBYTE*)pbAtr = ParseATR.RawData;
			else
				memcpy(pbAtr, ParseATR.RawData, ParseATR.RawDataLength);
		}
		*pcbAtrLen = ParseATR.RawDataLength;
	}

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardStatusW_(SCARDHANDLE hCard, LPWSTR szReaderNames, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
	if (pcchReaderLen) {
		if (szReaderNames) {
			if (*pcchReaderLen == SCARD_AUTOALLOCATE)
				*(LPCWSTR*)szReaderNames = READER_NAME_W;
			else
				memcpy(szReaderNames, READER_NAME_W, sizeof(READER_NAME_W));
		}
		*pcchReaderLen = sizeof(READER_NAME_W) / sizeof(READER_NAME_W[0]);
	}
	if (pdwState)
		*pdwState = SCARD_PRESENT;
	if (pdwProtocol)
		*pdwProtocol = SCARD_PROTOCOL_T1;
	if (pcbAtrLen) {
		if (pbAtr) {
			if (*pcbAtrLen == SCARD_AUTOALLOCATE)
				*(LPBYTE*)pbAtr = ParseATR.RawData;
			else
				memcpy(pbAtr, ParseATR.RawData, ParseATR.RawDataLength);
		}
		*pcbAtrLen = ParseATR.RawDataLength;
	}

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardTransmit_(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
{
	HRESULT hr;
	CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE r;

	if (!cardReady) {
		if (FAILED(hr = ResetCard()))
		{
			return E_FAIL;
		}
	}

	{
		LockProc Lock;
		if (!Lock.IsSuccess())
			return SCARD_E_TIMEOUT;

		if ((r = Protocol.Transmit(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength)) != 0) {
			cardReady = FALSE;
			return SCARD_F_COMM_ERROR;
		}
	}

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardReconnect_(SCARDHANDLE hCard, DWORD dwShareMode, DWORD dwPreferredProtocols, DWORD dwInitialization, LPDWORD pdwActiveProtocol)
{
	*pdwActiveProtocol = SCARD_PROTOCOL_T1;

	return SCARD_S_SUCCESS;
}

HANDLE WINAPI SCardAccessStartedEvent_(void)
{
	return startedEvent;
}

void WINAPI SCardReleaseStartedEvent_(void)
{
	return;
}

LONG WINAPI SCardCancel_(SCARDCONTEXT hContext)
{
	return SCARD_S_SUCCESS;
}
