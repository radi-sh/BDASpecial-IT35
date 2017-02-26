#pragma once

#include <string>

class CCOMProc {
public:
	typedef void(CALLBACK *RemoconCallbackFunc)(DWORD Data, LPARAM pParam);

private:
	HANDLE hThread;					// スレッドハンドル
	HANDLE hThreadInitComp;			// スレッド初期化完了通知
	HANDLE hTerminateRequest;		// スレッド終了要求
	CRITICAL_SECTION csLock;		// 排他用
	std::wstring TunerFriendlyName;
	DWORD PollingInterval;
	RemoconCallbackFunc CallbackFunc;
	LPARAM CallbackInstance;

public:
	CCOMProc(void);
	~CCOMProc(void);
	void SetTunerFriendlyName(std::wstring name);
	void SetPollingInterval(DWORD dwMilliseconds);
	BOOL CreateThread(void);
	void TerminateThread(void);
	void SetCallback(RemoconCallbackFunc pFunc, LPARAM pParam);

private:
	void CloseThreadHandle(void);
	static DWORD WINAPI COMProcThread(LPVOID lpParameter);
};
