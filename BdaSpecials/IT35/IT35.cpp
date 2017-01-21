#include <Windows.h>
#include <stdio.h>

#include <string>

#include "IT35.h"

#include <iostream>
#include <dshow.h>

#include "common.h"

#pragma comment(lib, "Strmiids.lib" )

using namespace std;

static const GUID KSPROPSETID_BdaFrequencyFilter = { 0x71985f47, 0x1ca1, 0x11d3,{ 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0 } };

enum KSPROPERTY_BDA_FREQUENCY_FILTER {
	KSPROPERTY_BDA_RF_TUNER_FREQUENCY = 0,				// get/put
	KSPROPERTY_BDA_RF_TUNER_POLARITY,					// get/put
	KSPROPERTY_BDA_RF_TUNER_RANGE,						// get/put
	KSPROPERTY_BDA_RF_TUNER_TRANSPONDER,				// not supported
	KSPROPERTY_BDA_RF_TUNER_BANDWIDTH,					// get/put
	KSPROPERTY_BDA_RF_TUNER_FREQUENCY_MULTIPLIER,		// get/put
	KSPROPERTY_BDA_RF_TUNER_CAPS,						// not supported
	KSPROPERTY_BDA_RF_TUNER_SCAN_STATUS,				// not supported
	KSPROPERTY_BDA_RF_TUNER_STANDARD,					// not supported
	KSPROPERTY_BDA_RF_TUNER_STANDARD_MODE,				// not supported
};

static const GUID KSPROPSETID_BdaSignalStats = { 0x1347d106, 0xcf3a, 0x428a,{ 0xa5, 0xcb, 0xac, 0xd, 0x9a, 0x2a, 0x43, 0x38 } };

enum KSPROPERTY_BDA_SIGNAL_STATS {
	KSPROPERTY_BDA_SIGNAL_STRENGTH = 0,					// get only
	KSPROPERTY_BDA_SIGNAL_QUALITY,						// get only
	KSPROPERTY_BDA_SIGNAL_PRESENT,						// get only
	KSPROPERTY_BDA_SIGNAL_LOCKED,						// get only
	KSPROPERTY_BDA_SAMPLE_TIME,							// get only
};

static const GUID KSPROPSETID_BdaDigitalDemodulator = { 0xef30f379, 0x985b, 0x4d10,{ 0xb6, 0x40, 0xa7, 0x9d, 0x5e, 0x4, 0xe1, 0xe0 } };

enum KSPROPERTY_BDA_DIGITAL_DEMODULATOR {
	KSPROPERTY_BDA_MODULATION_TYPE = 0,					// get/put
	KSPROPERTY_BDA_INNER_FEC_TYPE,						// get/put
	KSPROPERTY_BDA_INNER_FEC_RATE,						// get/put
	KSPROPERTY_BDA_OUTER_FEC_TYPE,						// get/put
	KSPROPERTY_BDA_OUTER_FEC_RATE,						// get/put
	KSPROPERTY_BDA_SYMBOL_RATE,							// get/put
	KSPROPERTY_BDA_SPECTRAL_INVERSION,					// get/put
	KSPROPERTY_BDA_GUARD_INTERVAL,						// not supported
	KSPROPERTY_BDA_TRANSMISSION_MODE,					// not supported
	KSPROPERTY_BDA_ROLL_OFF,							// not supported
	KSPROPERTY_BDA_PILOT,								// not supported
	KSPROPERTY_BDA_SIGNALTIMEOUTS,						// not supported
};

static const GUID KSPROPSETID_BdaLNBInfo = { 0x992cf102, 0x49f9, 0x4719,{ 0xa6, 0x64, 0xc4, 0xf2, 0x3e, 0x24, 0x8, 0xf4 } };

enum KSPROPERTY_BDA_LNB_INFO {
	KSPROPERTY_BDA_LNB_LOF_LOW_BAND = 0,				// get/put
	KSPROPERTY_BDA_LNB_LOF_HIGH_BAND,					// get/put
	KSPROPERTY_BDA_LNB_SWITCH_FREQUENCY,				// get/put
};

static const GUID KSPROPSETID_BdaPIDFilter = { 0xd0a67d65, 0x8df, 0x4fec, {0x85, 0x33, 0xe5, 0xb5, 0x50, 0x41, 0xb, 0x85} };

enum KSPROPERTY_BDA_PIDFILTER {
	KSPROPERTY_BDA_PIDFILTER_MAP_PIDS = 0,				// put only
	KSPROPERTY_BDA_PIDFILTER_UNMAP_PIDS,				// put only
	KSPROPERTY_BDA_PIDFILTER_LIST_PIDS,					// put pnly
};

static const GUID KSMETHODSETID_BdaChangeSync = { 0xfd0a5af3, 0xb41d, 0x11d2, {0x9c, 0x95, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0} };

enum KSMETHOD_BDA_CHANGE_SYNC {
	KSMETHOD_BDA_START_CHANGES = 0,
	KSMETHOD_BDA_CHECK_CHANGES,
	KSMETHOD_BDA_COMMIT_CHANGES,
	KSMETHOD_BDA_GET_CHANGE_STATE,
};

static const GUID KSMETHODSETID_BdaDeviceConfiguration = { 0x71985f45, 0x1ca1, 0x11d3,{ 0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0 } };

enum KSMETHOD_BDA_DEVICE_CONFIGURATION {
	KSMETHOD_BDA_CREATE_PIN_FACTORY = 0,
	KSMETHOD_BDA_DELETE_PIN_FACTORY,
	KSMETHOD_BDA_CREATE_TOPOLOGY,
};

static const GUID KSPROPSETID_IteExtension = { 0xc6efe5eb, 0x855a, 0x4f1b, {0xb7, 0xaa, 0x87, 0xb5, 0xe1, 0xdc, 0x41, 0x13} };

enum KSPROPERTY_ITE_EXTENSION {
	KSPROPERTY_ITE_EX_BULK_DATA = 0,					// get/put
	KSPROPERTY_ITE_EX_BULK_DATA_NB,						// get/put
	KSPROPERTY_ITE_EX_PID_FILTER_ON_OFF,				// put only
	KSPROPERTY_ITE_EX_BAND_WIDTH,						// get/put
	KSPROPERTY_ITE_EX_MERCURY_DRIVER_INFO,				// get only
	KSPROPERTY_ITE_EX_MERCURY_DEVICE_INFO,				// get only
	KSPROPERTY_ITE_EX_TS_DATA,							// get/put
	KSPROPERTY_ITE_EX_OVL_CNT,							// get only
	KSPROPERTY_ITE_EX_FREQ,								// get/put
	KSPROPERTY_ITE_EX_RESET_USB,						// put only
	KSPROPERTY_ITE_EX_MERCURY_REG,						// get/put
	KSPROPERTY_ITE_EX_MERCURY_PVBER,					// get/put
	KSPROPERTY_ITE_EX_MERCURY_REC_LEN,					// get/put
	KSPROPERTY_ITE_EX_MERCURY_EEPROM,					// get/put
	KSPROPERTY_ITE_EX_MERCURY_IR,						// get only
	KSPROPERTY_ITE_EX_MERCURY_SIGNAL_STRENGTH,			// get only
	KSPROPERTY_ITE_EX_CHANNEL_MODULATION = 99,			// get only
};

static const GUID KSPROPSETID_DvbsIoCtl = { 0xf23fac2d, 0xe1af, 0x48e0,{ 0x8b, 0xbe, 0xa1, 0x40, 0x29, 0xc9, 0x2f, 0x21 } };

enum KSPROPERTY_DVBS_IO_CTL {
	KSPROPERTY_DVBS_IO_LNB_POWER = 0,					// get/put
	KSPROPERTY_DVBS_IO_DiseqcLoad,						// put only
};

static const GUID KSPROPSETID_ExtIoCtl = { 0xf23fac2d, 0xe1af, 0x48e0,{ 0x8b, 0xbe, 0xa1, 0x40, 0x29, 0xc9, 0x2f, 0x11 } };

enum KSPROPERTY_EXT_IO_CTL {
	KSPROPERTY_EXT_IO_DRV_DATA = 0,						// get/put
	KSPROPERTY_EXT_IO_CARD_READER,						// get/put
	KSPROPERTY_EXT_IO_UNUSED50 = 50,					// not used
	KSPROPERTY_EXT_IO_UNUSED51 = 51,					// not used
	KSPROPERTY_EXT_IO_ISDBT_IO_CTL = 200,				// put only
};

static const GUID KSPROPSETID_PrivateIoCtl = { 0xede22531, 0x92e8, 0x4957, {0x9d, 0x5, 0x6f, 0x30, 0x33, 0x73, 0xe8, 0x37} };

enum KSPROPERTY_PRIVATE_IO_CTL {
	KSPROPERTY_PRIVATE_IO_DIGIBEST_TUNER = 0,			// put only
};

HMODULE hMySelf;

static inline HRESULT it35_PutISDBTioCtlproperty(IKsPropertySet *pIKsPropertySet, DWORD dwData)
{
	return pIKsPropertySet->Set(KSPROPSETID_ExtIoCtl, KSPROPERTY_EXT_IO_ISDBT_IO_CTL, NULL, 0, &dwData, sizeof(dwData));
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		// モジュールハンドル保存
		hMySelf = hModule;
		break;

	case DLL_PROCESS_DETACH:
		// デバッグログファイルのクローズ
		CloseDebugLog();
		break;
	}
    return TRUE;
}

__declspec(dllexport) IBdaSpecials * CreateBdaSpecials(CComPtr<IBaseFilter> pTunerDevice)
{
	return new CIT35Specials(hMySelf, pTunerDevice);
}

__declspec(dllexport) HRESULT CheckAndInitTuner(IBaseFilter *pTunerDevice, const WCHAR *szDisplayName, const WCHAR *szFriendlyName, const WCHAR *szIniFilePath)
{
	// DebugLogを記録するかどうか
	if (::GetPrivateProfileIntW(L"PLEXPX", L"DebugLog", 0, szIniFilePath)) {
		// INIファイルのファイル名取得
		WCHAR szDebugLogPath[_MAX_PATH + 1];
		::GetModuleFileNameW(hMySelf, szDebugLogPath, _MAX_PATH + 1);
		::wcscpy_s(szDebugLogPath + ::wcslen(szDebugLogPath) - 3, 4, L"log");
		SetDebugLog(szDebugLogPath);
	}

	return S_OK;
}

__declspec(dllexport) HRESULT CheckCapture(const WCHAR *szTunerDisplayName, const WCHAR *szTunerFriendlyName,
	const WCHAR *szCaptureDisplayName, const WCHAR *szCaptureFriendlyName, const WCHAR *szIniFilePath)
{
	OutputDebug(L"CheckCapture called.\n");

	return E_FAIL;
}

CIT35Specials::CIT35Specials(HMODULE hMySelf, CComPtr<IBaseFilter> pTunerDevice)
	: m_hMySelf(hMySelf),
	  m_pTunerDevice(pTunerDevice),
	  m_pIKsPropertySet(NULL)
{
	::InitializeCriticalSection(&m_CriticalSection);

	HRESULT hr;

	hr = m_pTunerDevice->QueryInterface(IID_IKsPropertySet, (LPVOID*)&m_pIKsPropertySet);

	return;
}

CIT35Specials::~CIT35Specials()
{
	m_hMySelf = NULL;

	SAFE_RELEASE(m_pIKsPropertySet);

	if (m_pTunerDevice) {
		m_pTunerDevice.Release();
		m_pTunerDevice = NULL; 
	}

	::DeleteCriticalSection(&m_CriticalSection);

	return;
}

const HRESULT CIT35Specials::InitializeHook(void)
{
	if (m_pTunerDevice == NULL) {
		return E_POINTER;
	}

	return S_OK;
}

const HRESULT CIT35Specials::Set22KHz(bool bActive)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::Set22KHz(long nTone)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::FinalizeHook(void)
{
	return S_OK;
}

const HRESULT CIT35Specials::GetSignalState(int *pnStrength, int *pnQuality, int *pnLock)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::LockChannel(BYTE bySatellite, BOOL bHorizontal, unsigned long ulFrequency, BOOL bDvbS2)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::LockChannel(const TuningParam *pTuningParm)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::SetLNBPower(bool bActive)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::ReadIniFile(WCHAR *szIniFilePath)
{
	return S_OK;
}

const HRESULT CIT35Specials::IsDecodingNeeded(BOOL *pbAns)
{
	if (pbAns)
		*pbAns = FALSE;

	return S_OK;
}

const HRESULT CIT35Specials::Decode(BYTE *pBuf, DWORD dwSize)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::GetSignalStrength(float *fVal)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::PreTuneRequest(const TuningParam *pTuningParm, ITuneRequest *pITuneRequest)
{
	return S_OK;
}

const HRESULT CIT35Specials::PostLockChannel(const TuningParam *pTuningParm)
{
	HRESULT hr = S_OK;

	if (pTuningParm->TSID != 0 && pTuningParm->TSID != -1) {
		::EnterCriticalSection(&m_CriticalSection);
		hr = it35_PutISDBTioCtlproperty(m_pIKsPropertySet, (DWORD)pTuningParm->TSID);
		::LeaveCriticalSection(&m_CriticalSection);
	}

	return hr;
}

void CIT35Specials::Release(void)
{
	delete this;
}
