#pragma once

#include <string>

class CCOMProc {
private:
	enum enumCOMRequest {
		eCOMReqDetectCard = 1,
		eCOMReqIsUARTReady,
		eCOMReqSendUART,
		eCOMReqGetUARTData,
		eCOMReqSetUARTBaudRate,
		eCOMReqResetCard,
		eCOMReqGetATRData,
		eCOMReqPrivateIOControl,
	};

	union COMReqParam {
		struct COMReqParamDetectCard {
			BOOL *pbPresent;
		} DetectCard;
		struct COMReqParamIsUARTReady {
			BOOL *pbReady;
		} IsUARTReady;
		struct COMReqParamSendUART {
			const BYTE *pcbySendBuffer;
			DWORD dwSendLength;
		} SendUART;
		struct COMReqParamGetUARTData {
			BYTE *pbyRecvBuffer;
			DWORD *pdwRecvLength;
		} GetUARTData;
		struct COMReqParamSetUARTBaudRate {
			WORD wBaudRate;
		} SetUARTBaudRate;
		struct COMReqParamResetCard {
		} ResetCard;
		struct COMReqParamGetATRData {
			BYTE *pbyBuffer;
		} GetATRData;
		struct COMReqParamPrivateIOControl {
			DWORD dwCode;
		} PrivateIOControl;
	};

private:
	HANDLE hThread;					// スレッドハンドル
	HANDLE hThreadInitComp;			// スレッド初期化完了通知
	HANDLE hReqEvent;				// COMProcスレッドへのコマンド実行要求
	HANDLE hEndEvent;				// COMProcスレッドからのコマンド完了通知
	HANDLE hTerminateRequest;		// スレッド終了要求
	enumCOMRequest Request;			// リクエスト
	COMReqParam Param;				// パラメータ
	HRESULT RetCode;				// 終了コード
	CRITICAL_SECTION csLock;		// 排他用
	std::wstring TunerFriendlyName;
	std::wstring TunerDisplayName;

public:
	CCOMProc(void);
	~CCOMProc(void);
	void SetTunerFriendlyName(std::wstring name);
	std::wstring GetTunerDisplayName(void);
	BOOL CreateThread(void);
	void TerminateThread(void);
	HRESULT DetectCard(BOOL *Present);
	HRESULT IsUARTReady(BOOL *Ready);
	HRESULT SendUART(const BYTE *SendBuffer, DWORD SendLength);
	HRESULT GetUARTData(BYTE *RecvBuffer, DWORD *RecvLength);
	HRESULT SetUARTBaudRate(WORD BaudRate);
	HRESULT ResetCard(void);
	HRESULT GetATRData(BYTE *Buffer);
	HRESULT PrivateIOControl(DWORD Code);

private:
	void CloseThreadHandle(void);
	static DWORD WINAPI COMProcThread(LPVOID lpParameter);
};
