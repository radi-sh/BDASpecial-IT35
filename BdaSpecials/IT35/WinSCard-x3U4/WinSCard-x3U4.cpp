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

#define READER_NAME "Plex PX-x3U4 Card Reader 0"
static const SCARDHANDLE DUMMY_SCARDHANDLE = 0x5ca2d4a1;
static const SCARDCONTEXT DUMMY_SCARDCONTEXT = 0xc013e103;
static const CHAR READER_NAME_A[] = "Plex PX-x3U4 Card Reader 0";
static const WCHAR READER_NAME_W[] = L"Plex PX-x3U4 Card Reader 0";
static const CHAR LIST_READERS_A[] = "Plex PX-x3U4 Card Reader 0\0";
static const WCHAR LIST_READERS_W[] = L"Plex PX-x3U4 Card Reader 0\0";

static BYTE cardATR[256] = {};
static DWORD cardATRLen = 0;

static HANDLE startedEvent = NULL;
static IBaseFilter *l_pTunerDevice = NULL;
static IKsPropertySet *l_pIKsPropertySet = NULL;
static HMODULE l_hModule = NULL;

static CParseATR ParseATR;

static BOOL cardReady = FALSE;
static BYTE cardIFSC = 0;
static BOOL cardEdcTypeCRC = FALSE;

static inline HRESULT GetTunerDevice(wstring friendlyName) {
	try {
		HRESULT hr;
		wstring name;
		size_t pos;
		CDSFilterEnum dsfEnum(KSCATEGORY_BDA_NETWORK_TUNER, CDEF_DEVMON_PNP_DEVICE);
		while (SUCCEEDED(hr = dsfEnum.next()) && hr == S_OK) {
			dsfEnum.getFriendlyName(&name);
			if (pos = name.find(friendlyName) != 0)
				continue;

			if (FAILED(hr = dsfEnum.getFilter(&l_pTunerDevice))) {
				return hr;
			}

			if (FAILED(hr = l_pTunerDevice->QueryInterface(IID_IKsPropertySet, (LPVOID*)&l_pIKsPropertySet))) {
				return hr;
			}

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
	BYTE buf[3 + 254 + 2] = {};
	DWORD len;
	BOOL cardDetect;
	BOOL cardReady;

	// Cardが存在しているか確認
	if (FAILED(hr = it35_CardDetect(l_pIKsPropertySet, &cardDetect))) {
		return hr;
	}
	if (!cardDetect) {
		return E_FAIL;
	}

	// Card リセット
	if (FAILED(hr = it35_ResetSmartCard(l_pIKsPropertySet))) {
		return hr;
	}

	// リセットに成功していればTRUEが返る
	if (FAILED(hr = it35_IsUartReady(l_pIKsPropertySet, &cardReady))) {
		return hr;
	}
	if (!cardReady) {
		return E_FAIL;
	}

	// ATR取得
	if (FAILED(hr = it35_GetATR(l_pIKsPropertySet, buf))) {
		return hr;
	}
	// ここで受たATRは使用しない

	if (FAILED(hr = it35_GetUartData(l_pIKsPropertySet, buf, &len))) {
		return hr;
	}
	// ATRのLengthを含めて取得できるので保存
	memcpy(cardATR, buf, len);
	cardATRLen = len;
	// 解析
	ParseATR.Parse(buf, (BYTE)len);
	cardIFSC = ParseATR.ParsedInfo.IFSC;
	cardEdcTypeCRC = ParseATR.ParsedInfo.ErrorDetection == CParseATR::ERROR_DETECTION_CRC ? TRUE : FALSE;

	// ボーレートセット...9600か19200しか受付けないっぽい...
	if (FAILED(hr = it35_SetUartBaudRate(l_pIKsPropertySet, 19200))) {
		return hr;
	}

	cardReady = TRUE;
	return S_OK;
}

static HRESULT T1_SendR(BYTE NodeAddr, BYTE SeqNum, BYTE Stat) {
	HRESULT hr;

	BYTE Sendframe[3 + 254 + 2];
	DWORD SendFrameLen;

	if (MakeSendFrame(NodeAddr, (0x80 | SeqNum << 4) | Stat, 0, NULL, cardEdcTypeCRC, Sendframe, &SendFrameLen) != 0) {
		return E_FAIL;
	}

	if (FAILED(hr = hr = it35_SentUart(l_pIKsPropertySet, Sendframe, SendFrameLen))) {
		cardReady = FALSE;
		return E_FAIL;
	}

	return S_OK;
}


static HRESULT T1_SendAPDU(BYTE *pSnd, DWORD LenSnd, BYTE *pRcv, DWORD *pLenRcv)
{
	static const BYTE NodeAddr = 0x0;
	static BYTE SeqNum = 0x0;
	HRESULT hr;

	BYTE *SendInf = pSnd;
	DWORD SendInfLen = LenSnd;
	BYTE RecvInf[2048];
	BYTE *pRecvInf = RecvInf;
	DWORD RecvInfLen = 0;
	BYTE Sendframe[3 + 254 + 2];
	DWORD SendFrameLen;
	BYTE RecvFrame[3 + 254 + 2];
	DWORD RecvFrameLen;
	BYTE RecvNAD;
	BYTE RecvPCB;
	BYTE RecvLEN;
	BYTE RecvTemp[254];
	while (SendInfLen > cardIFSC) {
		if (MakeSendFrame(NodeAddr, (SeqNum << 6) | 0x20, cardIFSC, SendInf, cardEdcTypeCRC, Sendframe, &SendFrameLen) != 0) {
			return E_FAIL;
		}
		int retry1 = 0;
		while (1) {
			if (FAILED(hr = hr = it35_SentUart(l_pIKsPropertySet, Sendframe, SendFrameLen))) {
				cardReady = FALSE;
				return E_FAIL;
			}
			if (FAILED(hr = hr = it35_GetUartData(l_pIKsPropertySet, RecvFrame, &RecvFrameLen))) {
				cardReady = FALSE;
				return E_FAIL;
			}
			if (ParseRecvdFrame(&RecvNAD, &RecvPCB, &RecvLEN, RecvTemp, cardEdcTypeCRC, RecvFrame, RecvFrameLen) != 0) {
				return E_FAIL;
			}
			if ((RecvPCB & 0xef) != 0x80 || (RecvPCB & 0x10 ^ 0x10) >> 4 != SeqNum) {
				retry1++;
				if (retry1 > 3) {
					return E_FAIL;
				}
				continue;
			}
			break;
		}
		SeqNum ^= 0x01;
		SendInf += cardIFSC;
		SendInfLen -= cardIFSC;
	}
	if (MakeSendFrame(NodeAddr, (SeqNum << 6), (BYTE)SendInfLen, SendInf, cardEdcTypeCRC, Sendframe, &SendFrameLen) != 0) {
		return E_FAIL;
	}
	int retry2 = 0;
	BOOL DoneSend = FALSE;
	while (1) {
		if (!DoneSend) {
			if (FAILED(hr = hr = it35_SentUart(l_pIKsPropertySet, Sendframe, SendFrameLen))) {
				cardReady = FALSE;
				return E_FAIL;
			}
		}
		if (FAILED(hr = hr = it35_GetUartData(l_pIKsPropertySet, RecvFrame, &RecvFrameLen))) {
			cardReady = FALSE;
			return E_FAIL;
		}
		if (ParseRecvdFrame(&RecvNAD, &RecvPCB, &RecvLEN, RecvTemp, cardEdcTypeCRC, RecvFrame, RecvFrameLen) == 0) {
			if ((RecvPCB & 0x80) == 0x80 && (RecvPCB & 0x10) >> 4 == SeqNum) {
				retry2++;
				if (retry2 > 3) {
					return E_FAIL;
				}
				continue;
			}
			else {
				DoneSend = TRUE;
			}
		}
		else {
			if ((RecvPCB & 0x80) == 0x00) {
				DoneSend = TRUE;
				retry2++;
				if (retry2 > 3) {
					return E_FAIL;
				}
				if (FAILED(T1_SendR(NodeAddr, (RecvPCB & 0x40) >> 6, 0x01))) {
					return E_FAIL;
				}
				continue;
			}
			else
			{
				return E_FAIL;
			}
		}
		retry2 = 0;
		memcpy(pRecvInf, RecvTemp, RecvLEN);
		pRecvInf += RecvLEN;
		RecvInfLen += RecvLEN;
		if ((RecvPCB & 0xa0) == 0x20) {
			if (FAILED(T1_SendR(NodeAddr, (RecvPCB & 0x40 ^ 0x40) >> 6, 0x00))) {
				return E_FAIL;
			}
			continue;
		}
		break;
	}
	if (pRcv)
		memcpy(pRcv, RecvInf, RecvInfLen);
	if (pLenRcv)
		*pLenRcv = RecvInfLen;

	return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		l_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
		startedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

		{
			// iniファイルのpath取得
			WCHAR szIniFilePath[_MAX_PATH + 1];
			::GetModuleFileNameW(l_hModule, szIniFilePath, sizeof(szIniFilePath) / sizeof(szIniFilePath[0]));
			::wcscpy_s(szIniFilePath + ::wcslen(szIniFilePath) - 3, 4, L"ini");

			// tunerのFriendlyName取得
			WCHAR buf[256];
			wstring friendlyName;
			::GetPrivateProfileStringW(L"Settings", L"TunerFriendlyName", L"PXW3U4 Multi Tuner ISDB-S BDA Filter #0", buf, sizeof(buf) / sizeof(buf[0]), szIniFilePath);
			::GetPrivateProfileStringW(L"Settings", L"FriendlyName", buf, buf, sizeof(buf) / sizeof(buf[0]), szIniFilePath);
			friendlyName = buf;

			HRESULT hr;

			hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

			if (SUCCEEDED(hr = GetTunerDevice(friendlyName))) {
				hr = l_pTunerDevice->QueryInterface(IID_IKsPropertySet, (LPVOID*)&l_pIKsPropertySet);
			}
		}
		break;
	case DLL_PROCESS_DETACH:
		SAFE_RELEASE(l_pIKsPropertySet);
		SAFE_RELEASE(l_pTunerDevice);
		if (startedEvent)
			CloseHandle(startedEvent);
		break;
	}

	return TRUE;
}

LONG WINAPI SCardConnectA_(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
	*phCard = DUMMY_SCARDHANDLE;
	*pdwActiveProtocol = SCARD_PROTOCOL_T1;

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardConnectW_(SCARDCONTEXT hContext, LPWSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
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
				*(LPBYTE*)pbAtr = cardATR;
			else
				memcpy(pbAtr, cardATR, cardATRLen);
		}
		*pcbAtrLen = cardATRLen;
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
				*(LPBYTE*)pbAtr = cardATR;
			else
				memcpy(pbAtr, cardATR, cardATRLen);
		}
		*pcbAtrLen = cardATRLen;
	}

	return SCARD_S_SUCCESS;
}

LONG WINAPI SCardTransmit_(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
{
	HRESULT hr;

	if (!cardReady) {
		if (FAILED(hr = ResetCard()))
		{
			return E_FAIL;
		}
	}

	if (FAILED(hr = T1_SendAPDU((LPBYTE)pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength))) {
		return SCARD_F_COMM_ERROR;
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
