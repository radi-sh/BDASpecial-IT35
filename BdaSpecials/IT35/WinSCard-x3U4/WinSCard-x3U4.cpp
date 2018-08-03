#include "common.h"

#include "WinSCard-x3U4.h"

#include <Windows.h>
#include <algorithm>

#include "t1.h"
#include "atr.h"
#include "CCOMProc-x3U4.h"
#include "CIniFileAccess.h"

FILE *g_fpLog = NULL;

static const SCARDHANDLE DUMMY_SCARDHANDLE = 0x5ca2d4a1;
static const SCARDCONTEXT DUMMY_SCARDCONTEXT = 0xc013e103;
static const CHAR READER_NAME_A[] = "Plex PX-x3U4 Card Reader 0";
static const WCHAR READER_NAME_W[] = L"Plex PX-x3U4 Card Reader 0";
static const CHAR LIST_READERS_A[] = "Plex PX-x3U4 Card Reader 0\0";
static const WCHAR LIST_READERS_W[] = L"Plex PX-x3U4 Card Reader 0\0";

static BYTE IFSD = 254;						// IFD側の最大受信可能ブロックサイズ

static BOOL l_bInitialized = FALSE;
static DWORD l_dwEstablishedContext = 0;
static HANDLE l_hStartedEvent = NULL;
static HMODULE l_hModule = NULL;
static HANDLE l_hSemaphore = NULL;
static HANDLE l_hMapFile = NULL;

static SharedMemory *l_pShMem = NULL;

static CParseATR ParseATR;
static CCOMProc COMProc;
static CComProtocolT1x3U4 Protocol;

static void CloseAllHandle(void) {
	if (l_pShMem) {
		::UnmapViewOfFile(l_pShMem);
		l_pShMem = NULL;
	}
	if (l_hMapFile) {
		try {
			SAFE_CLOSE_HANDLE(l_hMapFile);
		}
		catch (...) {
		}
	}
	if (l_hSemaphore) {
		try {
			SAFE_CLOSE_HANDLE(l_hSemaphore);
		}
		catch (...) {
		}
	}
	return;
}

static void ProcATR(BYTE *atr) {
	ParseATR.Parse(atr, sizeof(atr));
	Protocol.SetCardIFSC(ParseATR.ParsedInfo.IFSC);
	Protocol.SetEDCType(ParseATR.ParsedInfo.ErrorDetection == CParseATR::ERROR_DETECTION_CRC ? CComProtocolT1::EDC_TYPE_CRC : CComProtocolT1::EDC_TYPE_LRC);
	Protocol.SetNodeAddress(0, 0);

	return;
}

static BOOL InitDevice(void) {
	if (l_bInitialized)
		return TRUE;

	OutputDebug(L"InitDevice: Started.\n");

	if (!COMProc.CreateThread()) {
		OutputDebug(L"InitDevice: Error in creating CCOMProc thread.\n");
		return FALSE;
	}

	do {
		// プロセス間排他用のセマフォ作成
		std::wstring guid = COMProc.GetTunerDisplayName();
		std::wstring::size_type len = guid.find_last_of(L"#");
		std::wstring str = guid.substr(0, len);
		std::replace(str.begin(), str.end(), L'\\', L'/');
		std::wstring semname1 = L"Global\\WinSCard-x3U4_Lock" + str;
		std::wstring semname2 = L"Local\\WinSCard-x3U4_Lock" + str;
		std::wstring mapname1 = L"Global\\WinSCard-x3U4_MapFile" + str;
		std::wstring mapname2 = L"Local\\WinSCard-x3U4_MapFile" + str;
		l_hSemaphore = ::CreateSemaphoreW(NULL, 1, 1, semname1.c_str());
		if (!l_hSemaphore) {
			OutputDebug(L"InitDevice: Error creating Semaphore Object for Global Namespace. Trying OpenSemaphore().\n");
			l_hSemaphore = ::OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, semname1.c_str());
			if (!l_hSemaphore) {
				OutputDebug(L"InitDevice: Error opening Semaphore Object for Global Namespace. Trying Session Namespace.\n");
				l_hSemaphore = ::CreateSemaphoreW(NULL, 1, 1, semname2.c_str());
				if (!l_hSemaphore) {
					OutputDebug(L"InitDevice: Error creating Semaphore Object for Session Namespace. Canceled.\n");
					break;
				}
			}
		}
		l_hMapFile = ::CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, sizeof(SharedMemory) / 0x10000, sizeof(SharedMemory) % 0x10000, mapname1.c_str());
		if (!l_hMapFile) {
			OutputDebug(L"InitDevice: Error creating File Mapping Object for Global Namespace. Trying OpenFileMapping().\n");
			l_hMapFile = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mapname1.c_str());
			if (!l_hMapFile) {
				OutputDebug(L"InitDevice: Error opening File Mapping Object for Global Namespace. Trying Session Namespace.\n");
				l_hMapFile = ::CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, sizeof(SharedMemory) / 0x10000, sizeof(SharedMemory) % 0x10000, mapname2.c_str());
				if (!l_hMapFile) {
					OutputDebug(L"InitDevice: Error creating File Mapping Object for Session Namespace. Canceled.\n");
					break;
				}
			}
		}
		l_pShMem = (SharedMemory*)::MapViewOfFile(l_hMapFile, FILE_MAP_WRITE, 0, 0, 0);
		if (!l_pShMem) {
			OutputDebug(L"InitDevice: Error in MapViewOfFile().\n");
			break;
		}

		if (l_pShMem->ATRLength > 2 && l_pShMem->ATR[0] == CParseATR::CONVENTION_DIRECT) {
			ProcATR(l_pShMem->ATR);
			OutputDebug(L"InitDevice: ATR was read from shared memory.\n");
		}

		l_bInitialized = TRUE;

		OutputDebug(L"InitDevice: Completed.\n");
		return TRUE;
	} while (0);

	COMProc.TerminateThread();
	CloseAllHandle();

	OutputDebug(L"InitDevice: Exit with error.\n");
	return FALSE;
}

static BOOL ResetCard(void)
{
	OutputDebug(L"ResetCard: Started.\n");

	if (!l_pShMem) {
		OutputDebug(L"ResetCard: Device has not been initialized.\n");
		return FALSE;
	}

	HRESULT hr;
	CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE r;
	BOOL present;
	BYTE rxpcb;
	BYTE rxbuf[3 + 254 + 2] = {};
	BYTE rxlen;
	BYTE atrbuf[33];
	DWORD atrlen;

	// Cardが存在しているか確認
	if (FAILED(hr = COMProc.DetectCard(&present))) {
		OutputDebug(L"ResetCard: Error in it35_CardDetect(). code=0x%x\n", hr);
		return FALSE;
	}
	if (!present) {
		OutputDebug(L"ResetCard: Card is not present.\n");
		return FALSE;
	}

	int retry = 0;
	while (!l_pShMem->CardReady) {
		if (!l_pShMem->DoneReset) {
			// Card リセット処理必要
			OutputDebug(L"ResetCard: Need reset.\n");

			// Card リセット
			if (FAILED(hr = COMProc.ResetCard())) {
				OutputDebug(L"ResetCard: Error in it35_ResetSmartCard(). code=0x%x\n", hr);
				return FALSE;
			}

			::Sleep(50);

			// ATR取得
			if (FAILED(hr = COMProc.GetATRData(atrbuf))) {
				OutputDebug(L"ResetCard: Error in it35_GetATR(). code=0x%x\n", hr);
				return FALSE;
			}

			atrlen = sizeof(atrbuf);
			if (FAILED(hr = COMProc.GetUARTData(atrbuf, &atrlen))) {
				OutputDebug(L"ResetCard: Error in it35_GetUartData() for getting ATR. code=0x%x\n", hr);
				return FALSE;
			}

			// ATR 解析
			ProcATR(atrbuf);
			memcpy(l_pShMem->ATR, ParseATR.RawData, ParseATR.RawDataLength);
			l_pShMem->ATRLength = ParseATR.RawDataLength;

			// ボーレートセット...9600か19200しか受付けないっぽい...
			if (FAILED(hr = COMProc.SetUARTBaudRate(19200))) {
				OutputDebug(L"ResetCard: Error in it35_SetUartBaudRate(). code=0x%x\n", hr);
				return FALSE;
			}
			l_pShMem->DoneReset = TRUE;
		}

		// RESYNCH 要求
		if ((r = Protocol.SendSBlock(FALSE, CComProtocolT1::SBLOCK_FUNCTION_RESYNCH, NULL, 0)) != CComProtocolT1::COM_PROTOCOL_T1_S_NO_ERROR) {
			OutputDebug(L"ResetCard: Error sending RESYNCH. code=%d\n", r);
			l_pShMem->DoneReset = FALSE;
			retry++;
			if (retry > 2)
				return FALSE;
			break;
		}

		// レスポンス取得
		if ((r = Protocol.RecvBlock(&rxpcb, rxbuf, &rxlen)) != CComProtocolT1::COM_PROTOCOL_T1_S_NO_ERROR) {
			OutputDebug(L"ResetCard: Error Receiving RESYNCH response. code=%d\n", r);
			l_pShMem->DoneReset = FALSE;
			retry++;
			if (retry > 2)
				return FALSE;
			break;
		}

		// レスポンス確認
		if (rxpcb != (CComProtocolT1::SBLOCK_FUNCTION_RESYNCH | CComProtocolT1::SBLOCK_RESPONSE)) {
			OutputDebug(L"ResetCard: PCB does not match RESYNCH response.\n");
			l_pShMem->DoneReset = FALSE;
			retry++;
			if (retry > 2)
				return FALSE;
			break;
		}

		// IFSD 値通知
		// デフォルト値の32ではM系のカードの不具合（少なくともISO7816-4の仕様どおりには動作しない）に引っかかる...
		// っていうか、ARIB STD-B25には必ず最大値の254に変更しろと明記されているし...
		if ((r = Protocol.SendSBlock(FALSE, CComProtocolT1::SBLOCK_FUNCTION_IFS, &IFSD, sizeof(IFSD))) != CComProtocolT1::COM_PROTOCOL_T1_S_NO_ERROR) {
			OutputDebug(L"ResetCard: Error sending IFS. code=%d\n", r);
			l_pShMem->DoneReset = FALSE;
			retry++;
			if (retry > 2)
				return FALSE;
			break;
		}

		// レスポンス取得
		if ((r = Protocol.RecvBlock(&rxpcb, rxbuf, &rxlen)) != 0) {
			OutputDebug(L"ResetCard: Error Receiving IFS response. code=%d\n", r);
			l_pShMem->DoneReset = FALSE;
			retry++;
			if (retry > 2)
				return FALSE;
			break;
		}

		// レスポンス確認
		if (rxpcb != (CComProtocolT1::SBLOCK_FUNCTION_IFS | CComProtocolT1::SBLOCK_RESPONSE)) {
			OutputDebug(L"ResetCard: PCB does not match IFS response.\n");
			l_pShMem->DoneReset = FALSE;
			retry++;
			if (retry > 2)
				return FALSE;
			break;
		}
		l_pShMem->SequenceNumber = FALSE;
		l_pShMem->CardReady = TRUE;
		OutputDebug(L"ResetCard: Reset Complete.\n");
	}

	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
		l_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
		l_hStartedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		// iniファイルのpath取得
		std::wstring tempPath = common::GetModuleName(l_hModule);
		CIniFileAccess IniFileAccess(tempPath + L"ini");
		IniFileAccess.SetSectionName(L"SCard");

		// tunerのFriendlyName取得
		std::wstring name, dip;
		name = IniFileAccess.ReadKeyS(L"TunerFriendlyName", L"PXW3U4 Multi Tuner ISDB-T BDA Filter #0");
		name = IniFileAccess.ReadKeyS(L"FriendlyName", name);
		// tunerのデバイスインスタンスパス取得
		dip = IniFileAccess.ReadKeyS(L"TunerInstancePath", L"");
		COMProc.SetTunerFriendlyName(name, dip);

		// Debug Logを記録するかどうか
		if (IniFileAccess.ReadKeyI(L"DebugLog", 0)) {
			SetDebugLog(tempPath + L"log");
		}

		// 詳細Logを記録するかどうか
		if (IniFileAccess.ReadKeyI(L"DetailLog", 0))
			Protocol.SetDetailLog(TRUE);

		// 送受信 Guard Interval 時間
		// カード側は2～3msecもあれば十分なはずだけど何故かUARTReadyが落ちてしまうことがあるみたい
		Protocol.SetGuardInterval(IniFileAccess.ReadKeyI(L"GuardInterval", 50));

		// IFD側の最大受信可能ブロックサイズ
		// 本来設定する必要は無いけどM系カードの不具合検証用として用意しておく
		IFSD = IniFileAccess.ReadKeyI(L"IFSD", 254);
	}
		break;

	case DLL_PROCESS_DETACH:
	{
		CloseAllHandle();
		if (l_hStartedEvent) {
			try {
				SAFE_CLOSE_HANDLE(l_hStartedEvent);
			}
			catch (...) {
			}
		}
		// デバッグログファイルのクローズ
		CloseDebugLog();
	}
		break;
	}

	return TRUE;
}

LONG WINAPI SCardConnectA_(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	if (!l_pShMem) {
		if (!InitDevice()) {
			OutputDebug(L"SCardConnectA: Error in InitDevice()\n");
			return SCARD_E_NO_ACCESS;
		}
	}

	if (!l_pShMem->CardReady) {
		LockProc Lock;
		if (!Lock.IsSuccess()) {
			OutputDebug(L"SCardConnectA: Error in Lock.\n");
			return SCARD_E_TIMEOUT;
		}
		if (!ResetCard())
		{
			OutputDebug(L"SCardConnectA: Error in ResetCard()\n");
			return SCARD_E_NOT_READY;
		}
	}

	*phCard = DUMMY_SCARDHANDLE;
	*pdwActiveProtocol = SCARD_PROTOCOL_T1;

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardConnectW_(SCARDCONTEXT hContext, LPWSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	if (!l_pShMem) {
		if (!InitDevice()) {
			OutputDebug(L"SCardConnectW: Error in InitDevice()\n");
			return SCARD_E_NO_ACCESS;
		}
	}

	if (!l_pShMem->CardReady) {
		LockProc Lock;
		if (!Lock.IsSuccess()) {
			OutputDebug(L"SCardConnectW: Error in Lock.\n");
			return SCARD_E_TIMEOUT;
		}
		if (!ResetCard())
		{
			OutputDebug(L"SCardConnectW: Error in ResetCard()\n");
			return SCARD_E_NOT_READY;
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
	l_dwEstablishedContext++;
	OutputDebug(L"SCardEstablishContext: Count=%d.\n", l_dwEstablishedContext);

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
	if (l_dwEstablishedContext)
		l_dwEstablishedContext--;
	OutputDebug(L"SCardReleaseContext: Count=%d.\n", l_dwEstablishedContext);

	if (!l_dwEstablishedContext) {
		COMProc.TerminateThread();
		CloseAllHandle();
	}

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
	CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE r;
	int retry = 0;
	BOOL success = FALSE;

	LockProc Lock;
	if (!Lock.IsSuccess()) {
		OutputDebug(L"SCardTransmit: Error in Lock.\n");
		return SCARD_E_TIMEOUT;
	}

	while (!success) {
		if (!l_pShMem->CardReady) {
			if (!ResetCard())
			{
				OutputDebug(L"SCardTransmit: Error in ResetCard()\n");
				return SCARD_E_NOT_READY;
			}
		}

		switch (r = Protocol.Transmit(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength, &(l_pShMem->SequenceNumber))) {
		case CComProtocolT1::COM_PROTOCOL_T1_S_NO_ERROR:
			success = TRUE;
			break;

		case CComProtocolT1::COM_PROTOCOL_T1_E_POINTER:
		case CComProtocolT1::COM_PROTOCOL_T1_E_NO_CARD:
			OutputDebug(L"SCardTransmit: Fatal error in Transmit()\n");
			return SCARD_E_NOT_READY;

		default:
			OutputDebug(L"SCardTransmit: Error in Transmit()\n");
			l_pShMem->CardReady = FALSE;
			retry++;
			if (retry > 2) {
				OutputDebug(L"SCardTransmit: Aborted.\n");
				return SCARD_F_COMM_ERROR;
			}
			else if (retry > 1) {
				OutputDebug(L"SCardTransmit: Trying reset...\n");
				l_pShMem->DoneReset = FALSE;
				break;
			}
			else {
				OutputDebug(L"SCardTransmit: Trying RESYNCH...\n");
				break;
			}
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
	return l_hStartedEvent;
}

void WINAPI SCardReleaseStartedEvent_(void)
{
	return;
}

LONG WINAPI SCardCancel_(SCARDCONTEXT hContext)
{
	return SCARD_S_SUCCESS;
}

CComProtocolT1x3U4::CComProtocolT1x3U4(void)
	: LastTickCount(0),
	GuardInterval(0)
{
	IgnoreEDCError = FALSE;
}

CComProtocolT1x3U4::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1x3U4::TxBlock(void)
{
	HRESULT hr;
	BOOL present = FALSE;
	if (FAILED(hr = COMProc.DetectCard(&present))) {
		OutputDebug(L"TxBlock: Error in it35_CardDetect(). code=0x%x\n", hr);
		return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
	}
	if (!present) {
		OutputDebug(L"TxBlock: Card not present.\n", hr);
		return COM_PROTOCOL_T1_E_NO_CARD;
	}
	WaitGuardInterval();
	if (FAILED(hr = COMProc.SendUART(SendFrame, SendFrameLen))) {
		OutputDebug(L"TxBlock: Error in it35_SentUart(). code=0x%x\n", hr);
		return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
	}
	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1x3U4::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1x3U4::RxBlock(void)
{
	HRESULT hr;
	BOOL ready = FALSE;
	int retry = 0;
	while (1) {
		if (FAILED(hr = COMProc.IsUARTReady(&ready))) {
			OutputDebug(L"RxBlock: Error in it35_IsUartReady(). code=0x%x\n", hr);
			return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
		}
		if (ready)
			break;
		retry++;
		if (retry > 20) {
			OutputDebug(L"RxBlock: Retry time out for it35_IsUartReady().\n");
			return COM_PROTOCOL_T1_E_NOT_READY;
		}
		::Sleep(10);
	}
	DWORD len = sizeof(RecvFrame);
	if (FAILED(hr = COMProc.GetUARTData(RecvFrame, &len))) {
		OutputDebug(L"RxBlock: Error in it35_GetUartData(). code=0x%x\n", hr);
		return COM_PROTOCOL_T1_E_NOT_FUNCTIONING;
	}
	RecvFrameLen = len;
	SetLastTickCount();
	return COM_PROTOCOL_T1_S_NO_ERROR;
}

void CComProtocolT1x3U4::SetGuardInterval(DWORD dwMilliSec) {
	GuardInterval = dwMilliSec;
	return;
}

void CComProtocolT1x3U4::SetLastTickCount(void) {
	LastTickCount = ::GetTickCount();
	return;
}

void CComProtocolT1x3U4::WaitGuardInterval(void) {
	DWORD t = ::GetTickCount() - LastTickCount;
	if (t < GuardInterval)
		::Sleep(GuardInterval - t);
}

LockProc::LockProc(DWORD dwMilliSeconds)
	: result(WAIT_FAILED)
{
	result = ::WaitForSingleObject(l_hSemaphore, dwMilliSeconds);
	return;
};

LockProc::LockProc(void)
	: LockProc(10000)
{
};

LockProc::~LockProc(void)
{
	::ReleaseSemaphore(l_hSemaphore, 1, NULL);
	return;
};

BOOL LockProc::IsSuccess(void)
{
	return (result == WAIT_OBJECT_0);
};
