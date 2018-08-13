#pragma once

#include "IBdaSpecials2.h"

class CIT35Specials : public IBdaSpecials2a2
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
	const HRESULT LockChannel(const TuningParam *pTuningParm);

	const HRESULT ReadIniFile(const WCHAR *szIniFilePath);
	const HRESULT IsDecodingNeeded(BOOL *pbAns);
	const HRESULT Decode(BYTE *pBuf, DWORD dwSize);
	const HRESULT GetSignalStrength(float *fVal);
	const HRESULT PreTuneRequest(const TuningParam *pTuningParm, ITuneRequest *pITuneRequest);
	const HRESULT PostLockChannel(const TuningParam *pTuningParm);

	virtual void Release(void);

private:
	HMODULE m_hMySelf;
	CComPtr<IBaseFilter> m_pTunerDevice;
	IKsPropertySet *m_pIKsPropertySet;
	CRITICAL_SECTION m_CriticalSection;
	ModulationType m_CurrentModulationType;

	BOOL m_bRewriteIFFreq;			// IF周波数で put_CarrierFrequency() を行う
	BOOL m_bPrivateSetTSID;			// 固有の Property set を使用してTSIDの書込みが必要
	BOOL m_bLNBPowerON;				// LNB電源の供給をONする
	BOOL m_bDualModeISDB;			// Dual Mode ISDB Tuner
};
