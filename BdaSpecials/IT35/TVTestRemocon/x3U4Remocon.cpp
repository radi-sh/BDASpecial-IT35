/*
// PX-W3U4/PXQ3U4リモコン用 TVTest プラグイン
// TVTest プラグインサンプル HDUSRemocon.cppのソースコードをベースにしています
*/
#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT

#pragma warning (push)
#pragma warning (disable: 4100)
#include "TVTestPlugin.h"
#pragma warning (pop)

#include "common.h"
#include "CCOMProc-x3U4Remocon.h"
#include "CIniFileAccess.h"

// コントローラ名
#define X3U4_REMOCON_NAME L"PX-x3U4 Remocon"

FILE *g_fpLog = NULL;

// ボタンのリスト
static const struct {
	TVTest::ControllerButtonInfo Info;
	DWORD KeyCode;
} g_ButtonList[] = {
	{ { L"電源",			L"Close",			{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201020df },
	{ { L"ミュート",		L"Mute",			{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20102ad5 },
	{ { L"REC",				L"SaveImage",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201004fb },
	{ { L"EPG",				L"ProgramGuide",	{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201025da },
	{ { L"TV",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201006f9 },
	{ { L"DVD",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201007f8 },
	{ { L"←",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201008f7 },
	{ { L"CC（字幕）",		NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201009f6 },
	{ { L"プレーヤー ON",	NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201002fd },
	{ { L"プレーヤー OFF",	NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201003fc },
	{ { L"音声切替",		L"SwitchAudio",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20100cf3 },
	{ { L"全画面",			L"Fullscreen",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20100ef1 },
	{ { L"カーソル 上",		NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20100df2 },
	{ { L"カーソル 下",		NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201013ec },
	{ { L"カーソル 左",		NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20100ff0 },
	{ { L"カーソル 右",		NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201011ee },
	{ { L"OK",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201030cf },
	{ { L"i",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20100bf4 },
	{ { L"録画",			L"RecordStart",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201018e7 },
	{ { L"一時停止",		L"RecordPause",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201016e9 },
	{ { L"再生",			NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201035ca },
	{ { L"停止",			L"RecordStop",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201017e8 },
	{ { L"<<",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201019e6 },
	{ { L">>",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20103ac5 },
	{ { L"|<",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20101be4 },
	{ { L">|",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20101ce3 },
	{ { L"1",				L"Channel1",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20101de2 },
	{ { L"2",				L"Channel2",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20101ee1 },
	{ { L"3",				L"Channel3",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20101fe0 },
	{ { L"4",				L"Channel4",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201021de },
	{ { L"5",				L"Channel5",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201042bd },
	{ { L"6",				L"Channel6",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201043bc },
	{ { L"7",				L"Channel7",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201045ba },
	{ { L"8",				L"Channel8",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201026d9 },
	{ { L"9",				L"Channel9",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201047b8 },
	{ { L"0",				L"Channel10",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20104ab5 },
	{ { L"*/.",				L"Channel11",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201049b6 },
	{ { L"#",				L"Channel12",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20104bb4 },
	{ { L"チャンネル +",	L"ChannelUp",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201040bf },
	{ { L"チャンネル -",	L"ChannelDown",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201044bb },
	{ { L"音量 +",			L"VolumeUp",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201048b7 },
	{ { L"音量 -",			L"VolumeDown",		{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20104cb3 },
	{ { L"放送切替",		NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20104db2 },
	{ { L"クリア",			NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20104eb1 },
	{ { L"入力",			NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20102fd0 },
	{ { L"データ放送",		NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x20104fb0 },
	{ { L"青",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201050af },
	{ { L"赤",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201051ae },
	{ { L"緑",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201052ad },
	{ { L"黄",				NULL,				{ 0, 0, 0, 0 },			{ 0, 0 } },			0x201022dd },
};

#define NUM_BUTTONS (sizeof(g_ButtonList)/sizeof(g_ButtonList[0]))

static CCOMProc COMProc;

// プラグインクラス
class CRemocon : public TVTest::CTVTestPlugin
{
	bool m_fInitialized;	// 初期化済みか?

	bool InitializePlugin();
	void OnError(LPCWSTR pszMessage);

	static void CALLBACK OnRemoconDown(DWORD Data, LPARAM pParam);

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);

public:
	CRemocon();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CRemocon::CRemocon()
	: m_fInitialized(false)
{
}


bool CRemocon::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"PX-W3U4/PX-Q3U4リモコン";
	pInfo->pszCopyright   = L"radi_sh";
	pInfo->pszDescription = L"PX-W3U4/PX-Q3U4のリモコンに対応します。";
	return true;
}


bool CRemocon::Initialize()
{
	// 初期化処理

	// ボタンのリストを作成
	TVTest::ControllerButtonInfo ButtonList[NUM_BUTTONS];
	for (int i=0;i<NUM_BUTTONS;i++)
		ButtonList[i]=g_ButtonList[i].Info;

	// コントローラの登録
	TVTest::ControllerInfo Info;
	Info.Flags             = 0;
	Info.pszName           = X3U4_REMOCON_NAME;
	Info.pszText           = L"PX-W3U4/PX-Q3U4リモコン";
	Info.NumButtons        = NUM_BUTTONS;
	Info.pButtonList       = ButtonList;
	Info.pszIniFileName    = NULL;
	Info.pszSectionName    = L"x3U4Controller";
	Info.ControllerImageID = NULL;
	Info.SelButtonsImageID = NULL;
	Info.pTranslateMessage = NULL;
	Info.pClientData       = this;
	if (!m_pApp->RegisterController(&Info)) {
		m_pApp->AddLog(L"コントローラを登録できません。",TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


// プラグインが有効にされた時の初期化処理
bool CRemocon::InitializePlugin()
{
	if (!m_fInitialized) {
		// iniファイルのpath取得
		std::wstring tempPath = common::GetModuleName(g_hinstDLL);
		CIniFileAccess IniFileAccess(tempPath + L"ini");
		IniFileAccess.SetSectionName(L"Generic");

		// tunerのFriendlyName取得
		std::wstring name, dip;
		name = IniFileAccess.ReadKeyS(L"TunerFriendlyName", L"PXW3U4 Multi Tuner ISDB-T BDA Filter #0");
		name = IniFileAccess.ReadKeyS(L"FriendlyName", name);
		// tunerのデバイスインスタンスパス取得
		dip = IniFileAccess.ReadKeyS(L"TunerInstancePath", L"");
		COMProc.SetTunerFriendlyName(name, dip);

		// Debug Logを記録するかどうか
		if (IniFileAccess.ReadKeyB(L"DebugLog", FALSE)) {
			SetDebugLog(tempPath + L"log");
		}

		// リモコンスキャン間隔
		DWORD interval = IniFileAccess.ReadKeyI(L"PollingInterval", 100);
		COMProc.SetPollingInterval(interval);

		if (COMProc.CreateThread() != TRUE)
			return false;

		// コールバック関数登録
		COMProc.SetCallback(OnRemoconDown, (LPARAM)this);

		m_fInitialized = TRUE;
	}

	return true;
}


bool CRemocon::Finalize()
{
	// 終了処理
	COMProc.TerminateThread();
	m_fInitialized = FALSE;

	return true;
}


void CRemocon::OnError(LPCWSTR pszMessage)
{
	// エラー発生時のメッセージ表示
	m_pApp->AddLog(pszMessage,TVTest::LOG_TYPE_ERROR);
	if (!m_pApp->GetSilentMode()) {
		::MessageBoxW(m_pApp->GetAppWindow(),pszMessage,L"PX-W3U4/PX-Q3U4リモコン",
					  MB_OK | MB_ICONEXCLAMATION);
	}
}


void CRemocon::OnRemoconDown(DWORD Data, LPARAM pParam) {
	// 押されたボタンを探す
	for (int i = 0; i<NUM_BUTTONS; i++) {
		if (g_ButtonList[i].KeyCode == Data) {
			// ボタンが押されたことを通知する
			CRemocon* pThis = (CRemocon*)pParam;
			pThis->m_pApp->OnControllerButtonDown(X3U4_REMOCON_NAME, i);
			break;
		}
	}
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CRemocon::EventCallback(UINT Event, LPARAM lParam1, LPARAM /*lParam2*/, void *pClientData)
{
	CRemocon *pThis=static_cast<CRemocon*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		if (lParam1!=0) {
			if (!pThis->InitializePlugin()) {
				pThis->OnError(L"初期化でエラーが発生しました。");
				return FALSE;
			}
		} else {
		}
		return TRUE;

	case TVTest::EVENT_CONTROLLERFOCUS:
		// このプロセスが操作の対象になった
		return TRUE;
	}

	return 0;
}


// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CRemocon;
}
