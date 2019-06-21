#pragma once

#include "IBdaSpecials2.h"

class CIT35Specials : public IBdaSpecials2b3
{
public:
	CIT35Specials(HMODULE hMySelf, CComPtr<IBaseFilter> pTunerDevice);
	virtual ~CIT35Specials(void);

	const HRESULT InitializeHook(void);
	const HRESULT Set22KHz(bool bActive);
	const HRESULT FinalizeHook(void);

	const HRESULT GetSignalState(int *pnStrength, int *pnQuality, int *pnLock);
	const HRESULT LockChannel(BYTE bySatellite, BOOL bHorizontal, unsigned long ulFrequency, BOOL bDvbS2);

	const HRESULT SetLNBPower(bool bActive);

	const HRESULT Set22KHz(long nTone);
	const HRESULT LockChannel(const TuningParam *pTuningParam);

	const HRESULT ReadIniFile(const WCHAR *szIniFilePath);
	const HRESULT IsDecodingNeeded(BOOL *pbAns);
	const HRESULT Decode(BYTE *pBuf, DWORD dwSize);
	const HRESULT GetSignalStrength(float *fVal);
	const HRESULT PreLockChannel(TuningParam *pTuningParam);
	const HRESULT PreTuneRequest(const TuningParam *pTuningParam, ITuneRequest *pITuneRequest);
	const HRESULT PostTuneRequest(const TuningParam *pTuningParam);
	const HRESULT PostLockChannel(const TuningParam *pTuningParam);

	virtual void Release(void);

private:
	HMODULE m_hMySelf;
	CComPtr<IBaseFilter> m_pTunerDevice;
	CComQIPtr<IKsPropertySet> m_pIKsPropertySet;
	CRITICAL_SECTION m_CriticalSection;

	// 固有の Property set を使用してTSIDの書込みを行うモード
	enum enumPrivateSetTSID {
		ePrivateSetTSIDNone = 0,			// 行わない
		ePrivateSetTSIDPreTR,				// PreTuneRequestで行う
		ePrivateSetTSIDPostTR,				// PostTuneRequestで行う
	};

	BOOL m_bRewriteIFFreq;					// IF周波数で put_CarrierFrequency() を行う
	enumPrivateSetTSID m_nPrivateSetTSID;	// 固有の Property set を使用してTSIDの書込みを行うモード
	BOOL m_bLNBPowerON;						// LNB電源の供給をONする
	BOOL m_bDualModeISDB;					// Dual Mode ISDB Tuner
};
