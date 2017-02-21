#pragma once

#ifdef _WIN64
#pragma comment(linker, "/EXPORT:SCardConnectA=SCardConnectA_")
#pragma comment(linker, "/EXPORT:SCardConnectW=SCardConnectW_")
#pragma comment(linker, "/EXPORT:SCardDisconnect=SCardDisconnect_")
#pragma comment(linker, "/EXPORT:SCardEstablishContext=SCardEstablishContext_")
#pragma comment(linker, "/EXPORT:SCardFreeMemory=SCardFreeMemory_")
#pragma comment(linker, "/EXPORT:SCardGetStatusChangeA=SCardGetStatusChangeA_")
#pragma comment(linker, "/EXPORT:SCardGetStatusChangeW=SCardGetStatusChangeW_")
#pragma comment(linker, "/EXPORT:SCardIsValidContext=SCardIsValidContext_")
#pragma comment(linker, "/EXPORT:SCardListReadersA=SCardListReadersA_")
#pragma comment(linker, "/EXPORT:SCardListReadersW=SCardListReadersW_")
#pragma comment(linker, "/EXPORT:SCardReleaseContext=SCardReleaseContext_")
#pragma comment(linker, "/EXPORT:SCardStatusA=SCardStatusA_")
#pragma comment(linker, "/EXPORT:SCardStatusW=SCardStatusW_")
#pragma comment(linker, "/EXPORT:SCardTransmit=SCardTransmit_")
#pragma comment(linker, "/EXPORT:SCardReconnect=SCardReconnect_")
#pragma comment(linker, "/EXPORT:SCardAccessStartedEvent=SCardAccessStartedEvent_")
#pragma comment(linker, "/EXPORT:SCardReleaseStartedEvent=SCardReleaseStartedEvent_")
#pragma comment(linker, "/EXPORT:SCardCancel=SCardCancel_")
#pragma comment(linker, "/EXPORT:g_rgSCardT1Pci=g_rgSCardT1Pci_")
#else
#pragma comment(linker, "/EXPORT:SCardConnectA=_SCardConnectA_@24")
#pragma comment(linker, "/EXPORT:SCardConnectW=_SCardConnectW_@24")
#pragma comment(linker, "/EXPORT:SCardDisconnect=_SCardDisconnect_@8")
#pragma comment(linker, "/EXPORT:SCardEstablishContext=_SCardEstablishContext_@16")
#pragma comment(linker, "/EXPORT:SCardFreeMemory=_SCardFreeMemory_@8")
#pragma comment(linker, "/EXPORT:SCardGetStatusChangeA=_SCardGetStatusChangeA_@16")
#pragma comment(linker, "/EXPORT:SCardGetStatusChangeW=_SCardGetStatusChangeW_@16")
#pragma comment(linker, "/EXPORT:SCardIsValidContext=_SCardIsValidContext_@4")
#pragma comment(linker, "/EXPORT:SCardListReadersA=_SCardListReadersA_@16")
#pragma comment(linker, "/EXPORT:SCardListReadersW=_SCardListReadersW_@16")
#pragma comment(linker, "/EXPORT:SCardReleaseContext=_SCardReleaseContext_@4")
#pragma comment(linker, "/EXPORT:SCardStatusA=_SCardStatusA_@28")
#pragma comment(linker, "/EXPORT:SCardStatusW=_SCardStatusW_@28")
#pragma comment(linker, "/EXPORT:SCardTransmit=_SCardTransmit_@28")
#pragma comment(linker, "/EXPORT:SCardReconnect=_SCardReconnect_@20")
#pragma comment(linker, "/EXPORT:SCardAccessStartedEvent=_SCardAccessStartedEvent_@0")
#pragma comment(linker, "/EXPORT:SCardReleaseStartedEvent=_SCardReleaseStartedEvent_@0")
#pragma comment(linker, "/EXPORT:SCardCancel=_SCardCancel_@4")
#pragma comment(linker, "/EXPORT:g_rgSCardT1Pci=_g_rgSCardT1Pci_")
#endif

extern "C" {
	LONG WINAPI SCardConnectA_(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol);
	LONG WINAPI SCardConnectW_(SCARDCONTEXT hContext, LPWSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol);
	LONG WINAPI SCardDisconnect_(SCARDHANDLE hCard, DWORD dwDisposition);
	LONG WINAPI SCardEstablishContext_(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext);
	LONG WINAPI SCardFreeMemory_(SCARDCONTEXT hContext, LPCVOID pvMem);
	LONG WINAPI SCardGetStatusChangeA_(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATEA rgReaderStates, DWORD cReaders);
	LONG WINAPI SCardGetStatusChangeW_(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATEW rgReaderStates, DWORD cReaders);
	LONG WINAPI SCardIsValidContext_(SCARDCONTEXT hContext);
	LONG WINAPI SCardListReadersA_(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders);
	LONG WINAPI SCardListReadersW_(SCARDCONTEXT hContext, LPCWSTR mszGroups, LPWSTR mszReaders, LPDWORD pcchReaders);
	LONG WINAPI SCardReleaseContext_(SCARDCONTEXT hContext);
	LONG WINAPI SCardStatusA_(SCARDHANDLE hCard, LPSTR szReaderNames, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen);
	LONG WINAPI SCardStatusW_(SCARDHANDLE hCard, LPWSTR mszReaderNames, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen);
	LONG WINAPI SCardTransmit_(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
	LONG WINAPI SCardReconnect_(SCARDHANDLE hCard, DWORD dwShareMode, DWORD dwPreferredProtocols, DWORD dwInitialization, LPDWORD pdwActiveProtocol);
	HANDLE WINAPI SCardAccessStartedEvent_(void);
	void WINAPI SCardReleaseStartedEvent_(void);
	LONG WINAPI SCardCancel_(SCARDCONTEXT hContext);
	SCARD_IO_REQUEST g_rgSCardT1Pci_;
}

#pragma pack(1)
struct SharedMemory {
	BOOL CardReady;
	BOOL DoneReset;
	BOOL SequenceNumber;
	DWORD reserved12;
	DWORD reserved16;
	DWORD ATRLength;
	BYTE ATR[33];
	BYTE reserved57[3];
	DWORD reserved60;
	DWORD reserved64[240];
};
#pragma pack()


class CComProtocolT1x3U4 : public CComProtocolT1 {
private:
	DWORD LastTickCount;
	DWORD GuardInterval;

public:
	CComProtocolT1x3U4(void);
	virtual COM_PROTOCOL_T1_ERROR_CODE TxBlock(void);
	virtual COM_PROTOCOL_T1_ERROR_CODE RxBlock(void);
	void SetGuardInterval(DWORD dwMilliSec);

private:
	void SetLastTickCount(void);
	void WaitGuardInterval(void);
};

class LockProc {
private:
	DWORD result;

public:
	LockProc(DWORD dwMilliSeconds);
	LockProc(void);
	~LockProc(void);
	BOOL IsSuccess(void);
};

