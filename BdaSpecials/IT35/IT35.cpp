#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "common.h"

#include "IT35.h"

#include <Windows.h>
#include <string>
#include <algorithm>

#include <dshow.h>

// KSPROPSETID_BdaPIDFilter
#include <ks.h>
#pragma warning (push)
#pragma warning (disable: 4091)
#include <ksmedia.h>
#pragma warning (pop)
#include <bdatypes.h>
#include <bdamedia.h>

#include "IT35propset.h"
#include "CIniFileAccess.h"
#include "DSFilterEnum.h"
#include "WaitWithMsg.h"

FILE *g_fpLog = NULL;

HMODULE CIT35Specials::m_hMySelf = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
		::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
		// ���W���[���n���h���ۑ�
		CIT35Specials::m_hMySelf = hModule;
		break;

	case DLL_PROCESS_DETACH:
		// �f�o�b�O���O�t�@�C���̃N���[�Y
		CloseDebugLog();
		break;
	}
    return TRUE;
}

__declspec(dllexport) IBdaSpecials* CreateBdaSpecials(CComPtr<IBaseFilter> pTunerDevice)
{
	return NULL;
}

__declspec(dllexport) IBdaSpecials* CreateBdaSpecials2(CComPtr<IBaseFilter> pTunerDevice, CComPtr<IBaseFilter> pCaptureDevice, const WCHAR* szTunerDisplayName, const WCHAR* /*szTunerFriendlyName*/, const WCHAR* /*szCaptureDisplayName*/, const WCHAR* /*szCaptureFriendlyName*/)
{
	return new CIT35Specials(pTunerDevice, szTunerDisplayName);
}


__declspec(dllexport) HRESULT CheckAndInitTuner(IBaseFilter* /*pTunerDevice*/, const WCHAR* /*szDisplayName*/, const WCHAR* /*szFriendlyName*/, const WCHAR* szIniFilePath)
{
	CIniFileAccess IniFileAccess(szIniFilePath);

	// DebugLog���L�^���邩�ǂ���
	if (IniFileAccess.ReadKeyB(L"IT35", L"DebugLog", FALSE)) {
		// INI�t�@�C���̃t�@�C�����擾
		// DebugLog�̃t�@�C�����擾
		SetDebugLog(common::GetModuleName(CIT35Specials::m_hMySelf) + L"log");
	}

	return S_OK;
}

__declspec(dllexport) HRESULT CheckCapture(const WCHAR* /*szTunerDisplayName*/, const WCHAR* /*szTunerFriendlyName*/,
	const WCHAR* /*szCaptureDisplayName*/, const WCHAR* /*szCaptureFriendlyName*/, const WCHAR* /*szIniFilePath*/)
{
	// ���ꂪ�Ă΂ꂽ�Ƃ������Ƃ�Bondriver_BDA.ini�̐ݒ肪��������
	OutputDebug(L"CheckCapture called.\n");

	// connect()�����݂Ă����ʂȂ̂� E_FAIL ��Ԃ��Ă���
	return E_FAIL;
}

CIT35Specials::CIT35Specials(CComPtr<IBaseFilter> pTunerDevice, const WCHAR* szTunerDisplayName)
	: m_pTunerDevice(pTunerDevice),
	  m_pIKsPropertySet(m_pTunerDevice),
	  m_hSemaphore(NULL),
	  m_sTunerGUID(szTunerDisplayName),
	  m_bRewriteIFFreq(TRUE),
	  m_nPrivateSetTSID(enumPrivateSetTSID::ePrivateSetTSIDNone),
	  m_ProductID(PID_UNKNOWN),
	  m_nTunerID(0xffffffffU),
	  m_bLNBPowerON(FALSE),
	  m_bDualModeISDB(FALSE),
	  m_nSpecialLockConfirmTime(2000),
	  m_nSpecialLockSetTSIDInterval(100),
	  m_bRewriteNominalRate(FALSE),
	  m_byNominalRate_List(),
	  m_nLastIsdbMode(0),
	  m_nLastModulationType(BDA_MOD_NOT_DEFINED),
	  m_nLastMultiplier(0),
	  m_nLastBandWidth(0)
{
	::InitializeCriticalSection(&m_CriticalSection);

	return;
}

CIT35Specials::~CIT35Specials()
{
	SAFE_CLOSE_HANDLE(m_hSemaphore);
	::DeleteCriticalSection(&m_CriticalSection);

	return;
}

const HRESULT CIT35Specials::InitializeHook(void)
{
	if (!m_pTunerDevice) {
		return E_POINTER;
	}

	if (!m_pIKsPropertySet)
		return E_FAIL;

	HRESULT hr;

	// IBDA_DeviceControl
	{
		CComQIPtr<IBDA_DeviceControl> pDeviceControl(m_pTunerDevice);
		if (!pDeviceControl) {
			OutputDebug(L"Can not get IBDA_DeviceControl.\n");
			return E_NOINTERFACE;
		}
		m_pIBDA_DeviceControl = pDeviceControl;
	}

	// Control Node �擾
	{
		CDSEnumNodes DSEnumNodes(m_pTunerDevice);

		// IBDA_FrequencyFilter / IBDA_SignalStatistics / IBDA_LNBInfo / IBDA_DiseqCommand
		{
			ULONG NodeTypeTuner = DSEnumNodes.findControlNode(__uuidof(IBDA_FrequencyFilter));
			if (NodeTypeTuner != -1) {
				OutputDebug(L"Found RF Tuner Node. NodeType=%ld.\n", NodeTypeTuner);
				CComPtr<IUnknown> pControlNodeTuner;
				if (FAILED(hr = DSEnumNodes.getControlNode(NodeTypeTuner, &pControlNodeTuner))) {
					OutputDebug(L"Fail to get control node.\n");
				}
				else {
					CComQIPtr<IBDA_FrequencyFilter> pIBDA_FrequencyFilter(pControlNodeTuner);
					if (pIBDA_FrequencyFilter) {
						m_pIBDA_FrequencyFilter = pIBDA_FrequencyFilter;
						OutputDebug(L"  Found IBDA_FrequencyFilter.\n");
					}
					CComQIPtr<IBDA_SignalStatistics> pIBDA_SignalStatistics(pControlNodeTuner);
					if (pIBDA_SignalStatistics) {
						m_pIBDA_SignalStatistics = pIBDA_SignalStatistics;
						OutputDebug(L"  Found IBDA_SignalStatistics.\n");
					}
					CComQIPtr<IBDA_LNBInfo> pIBDA_LNBInfo(pControlNodeTuner);
					if (pIBDA_LNBInfo) {
						m_pIBDA_LNBInfo = pIBDA_LNBInfo;
						OutputDebug(L"  Found IBDA_LNBInfo.\n");
					}
				}
			}
		}

		// IBDA_DigitalDemodulator
		{
			ULONG NodeTypeDemod = DSEnumNodes.findControlNode(__uuidof(IBDA_DigitalDemodulator));
			if (NodeTypeDemod != -1) {
				OutputDebug(L"Found Demodulator Node. NodeType=%ld.\n", NodeTypeDemod);
				CComPtr<IUnknown> pControlNodeDemod;
				if (FAILED(hr = DSEnumNodes.getControlNode(NodeTypeDemod, &pControlNodeDemod))) {
					OutputDebug(L"Fail to get control node.\n");
				}
				else {
					CComQIPtr<IBDA_DigitalDemodulator> pIBDA_DigitalDemodulator(pControlNodeDemod);
					if (pIBDA_DigitalDemodulator) {
						m_pIBDA_DigitalDemodulator = pIBDA_DigitalDemodulator;
						OutputDebug(L"  Found IBDA_DigitalDemodulator.\n");
					}
				}
			}
		}
	}

	DeviceInfo deviceInfo = {};
	DriverInfo driverInfo = {};
	DrvDataDataSet drvData = {};

	if (FAILED(hr = it35_GetDeviceInfo(m_pIKsPropertySet, &deviceInfo))) {
		OutputDebug(L"Fail to get device info.\n");
	}
	else {
		OutputDebug(L"  USB Ver=0x%04x, Vender ID=0x%04x, Product ID=0x%04x.\n", deviceInfo.USBVer, deviceInfo.VenderID, deviceInfo.ProductID);
		m_ProductID = (enumProductID)((deviceInfo.VenderID << 16) | deviceInfo.ProductID);
	}

	if (FAILED(hr = it35_GetDriverInfo(m_pIKsPropertySet, &driverInfo))) {
		OutputDebug(L"Fail to get driver info.\n");
	}
	else {
		OutputDebug(L"  Driver Ver=0x%08x, Driver Ver2=0x%08x.\n", driverInfo.DriverVer, driverInfo.DriverVer2);
	}

	if (FAILED(hr = it35_GetDriverData(m_pIKsPropertySet, &drvData))) {
		OutputDebug(L"Fail to get driver data.\n");
	}
	else {
		OutputDebug(L"  Driver PID=0x%08x, Driver Version=0x%08x, Tuner ID=0x%08x.\n",
			drvData.DriverInfo.DriverPID, drvData.DriverInfo.DriverVersion, drvData.DriverInfo.TunerID);
		m_nTunerID = drvData.DriverInfo.TunerID;
	}

	// �v���Z�X�Ԕr���p�̃Z�}�t�H�쐬
	std::wstring::size_type len = m_sTunerGUID.find_last_of(L"#");
	std::wstring str = m_sTunerGUID.substr(0, len);
	std::replace(str.begin(), str.end(), L'\\', L'/');
	std::wstring semname1 = L"Global\\IT35-BulkMsg_Lock" + str;
	std::wstring semname2 = L"Local\\IT35-BulkMsg_Lock" + str;
	m_hSemaphore = ::CreateSemaphoreW(NULL, 1, 1, semname1.c_str());
	if (!m_hSemaphore) {
		OutputDebug(L"Warning! Failed to creating Semaphore Object for Global Namespace. Trying OpenSemaphore().\n");
		m_hSemaphore = ::OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, semname1.c_str());
		if (!m_hSemaphore) {
			OutputDebug(L"Warning! Failed to opening Semaphore Object for Global Namespace. Trying Session Namespace.\n");
			m_hSemaphore = ::CreateSemaphoreW(NULL, 1, 1, semname2.c_str());
			if (!m_hSemaphore) {
				OutputDebug(L"Error! Failed to creating Semaphore Object for Session Namespace.\n");
			}
		}
	}

	if (m_bLNBPowerON) {
		// ini�t�@�C���Ŏw�肳��Ă���� ������LNB Power ��ON����
		// LNB Power ��OFF��BDA driver������ɂ���Ă����݂���
		::EnterCriticalSection(&m_CriticalSection);
		hr = it35_PutLNBPower(m_pIKsPropertySet, 1);
		::LeaveCriticalSection(&m_CriticalSection);

		if FAILED(hr)
			OutputDebug(L"SetLNBPower failed.\n");
		else
			OutputDebug(L"SetLNBPower Success.\n");
	}
	return S_OK;
}

const HRESULT CIT35Specials::LockChannel(const TuningParam* pTuningParam)
{
	if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDSpecial) {
		if (m_pTunerDevice == NULL) {
			return E_POINTER;
		}

		if (!m_pIBDA_SignalStatistics) {
			return E_FAIL;
		}

		HRESULT hr;

		ModulationType eModulationType = BDA_MOD_NOT_SET;

		OutputDebug(L"LockChannel: Start.\n");

		BOOL success = FALSE;
		BOOL failure = FALSE;
		// Dual Mode ISDB Tuner�̏ꍇ�̓f���W�����[�^�[�̕���Mode��ݒ�
		if (m_bDualModeISDB) {
			DWORD isdbMode = 0;
			switch (pTuningParam->Modulation.Modulation) {
			case BDA_MOD_ISDB_T_TMCC:
				isdbMode = PRIVATE_IO_CTL_FUNC_DEMOD_OFDM;
				break;
			case BDA_MOD_ISDB_S_TMCC:
				isdbMode = PRIVATE_IO_CTL_FUNC_DEMOD_PSK;
				break;
			}
			if (isdbMode != m_nLastIsdbMode) {
				::EnterCriticalSection(&m_CriticalSection);
				hr = it35_DigibestPrivateIoControl(m_pIKsPropertySet, isdbMode);
				::LeaveCriticalSection(&m_CriticalSection);
			}
			if (m_nLastIsdbMode == 0) {
				SleepWithMessageLoop(500);
			}
			m_nLastIsdbMode = isdbMode;
		}

		::EnterCriticalSection(&m_CriticalSection);
		do {
			ULONG State = 0;
			if (FAILED(hr = m_pIBDA_DeviceControl->GetChangeState(&State))) {
				OutputDebug(L"  Fail to IBDA_DeviceControl::GetChangeState() function. ret=0x%08lx\n", hr);
				failure = TRUE;
				break;
			}
			// �y���f�B���O��Ԃ̃g�����U�N�V���������邩�m�F
			if (State == BDA_CHANGES_PENDING) {
				OutputDebug(L"  Some changes are pending. Trying CommitChanges.\n");
				// �g�����U�N�V�����̃R�~�b�g
				if (FAILED(hr = m_pIBDA_DeviceControl->CommitChanges())) {
					OutputDebug(L"    Fail to CommitChanges. ret=0x%08lx\n", hr);
				}
			}

			// �g�����U�N�V�����J�n�ʒm
			if (FAILED(hr = m_pIBDA_DeviceControl->StartChanges())) {
				OutputDebug(L"  Fail to IBDA_DeviceControl::StartChanges() function. ret=0x%08lx\n", hr);
				failure = TRUE;
				break;
			}

			// IBDA_DigitalDemodulator
			if (m_pIBDA_DigitalDemodulator) {
				// �ϒ��^�C�v��ݒ�
				eModulationType = pTuningParam->Modulation.Modulation;
				if (m_nLastModulationType != eModulationType) {
					m_pIBDA_DigitalDemodulator->put_ModulationType(&eModulationType);
					m_nLastModulationType = eModulationType;
				}
			}

			// IBDA_FrequencyFilter
			if (m_pIBDA_FrequencyFilter) {
				// ���g���̒P��(Hz)��ݒ�
				ULONG Multiplier = 1000UL;
				if (m_nLastMultiplier != Multiplier) {
					m_pIBDA_FrequencyFilter->put_FrequencyMultiplier(Multiplier);
					m_nLastMultiplier = Multiplier;
				}

				// ���g���̑ш敝 (MHz)��ݒ�
				ULONG bw = pTuningParam->Modulation.BandWidth;
				if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_S_TMCC && bw == -1L) {
					bw = 9L;
				}
				if (m_nLastBandWidth != bw) {
					m_pIBDA_FrequencyFilter->put_Bandwidth(bw);
					m_nLastBandWidth = bw;
				}

				// RF �M���̎��g����ݒ�
				long freq = pTuningParam->Frequency;
				// IF���g���ɕϊ�
				if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_S_TMCC && m_bRewriteIFFreq) {
					if (pTuningParam->Antenna.LNBSwitch != -1) {
						if (pTuningParam->Frequency < pTuningParam->Antenna.LNBSwitch) {
							if (pTuningParam->Frequency > pTuningParam->Antenna.LowOscillator && pTuningParam->Antenna.HighOscillator != -1)
								freq -= pTuningParam->Antenna.LowOscillator;
						}
						else {
							if (pTuningParam->Frequency > pTuningParam->Antenna.HighOscillator && pTuningParam->Antenna.HighOscillator != -1)
								freq -= pTuningParam->Antenna.HighOscillator;
						}
					}
					else {
						if (pTuningParam->Antenna.Tone == 0) {
							if (pTuningParam->Frequency > pTuningParam->Antenna.LowOscillator && pTuningParam->Antenna.HighOscillator != -1)
								freq -= pTuningParam->Antenna.LowOscillator;
						}
						else {
							if (pTuningParam->Frequency > pTuningParam->Antenna.HighOscillator && pTuningParam->Antenna.HighOscillator != -1)
								freq -= pTuningParam->Antenna.HighOscillator;
						}
					}
				}
				m_pIBDA_FrequencyFilter->put_Frequency((ULONG)freq);
			}

			// �g�����U�N�V�����̃R�~�b�g
			if (FAILED(hr = m_pIBDA_DeviceControl->CommitChanges())) {
				OutputDebug(L"  Fail to IBDA_DeviceControl::CommitChanges() function. ret=0x%08lx\n", hr);
				// ���s������S�Ă̕ύX��������
				hr = m_pIBDA_DeviceControl->StartChanges();
				hr = m_pIBDA_DeviceControl->CommitChanges();
				failure = TRUE;
				break;
			}
		} while (0);
		::LeaveCriticalSection(&m_CriticalSection);

		if (failure) {
			return E_FAIL;
		}

		::EnterCriticalSection(&m_CriticalSection);
		if (m_bRewriteNominalRate && pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_T_TMCC && m_ProductID == PID_MLT5PE) {
			i2c_info I2C_SLVT = m_aI2c_slaves_mlt5pe[m_nTunerID].slvt;
			i2c_info I2C_SLVX = m_aI2c_slaves_mlt5pe[m_nTunerID].slvx;
			i2c_info I2C_HELENE = m_aI2c_slaves_mlt5pe[m_nTunerID].gate;

			LockProc Lock(&m_hSemaphore);

			// Set SLV-T Bank : 0x10
			it35_i2c_wr_reg(I2C_SLVT, 0x00, 0x10);

			// TRCG Nominal Rate
			it35_i2c_wr_regs(I2C_SLVT, 0x9f, m_byNominalRate_List, 5);

			// Set SLV-T Bank : 0x00
			it35_i2c_wr_reg(I2C_SLVT, 0x00, 0x00);

			// SW Reset
			it35_i2c_wr_reg(I2C_SLVT, 0xfe, 0x01);
			// Enable TS Output
			it35_i2c_wr_reg(I2C_SLVT, 0xc3, 0x00);
		}

		// TSID��Set����
		if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_S_TMCC) {
			hr = it35_PutISDBIoCtl(m_pIKsPropertySet, pTuningParam->TSID == 0 ? (WORD)-1 : (WORD)pTuningParam->TSID);
		}

		// OneSeg���[�h��Set����
		if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_T_TMCC) {
			hr = it35_PutOneSeg(m_pIKsPropertySet, FALSE);
		}

		// PID off
		hr = it35_PutPidFilterOnOff(m_pIKsPropertySet, FALSE);

		// PID Map
		BDA_PID_MAP pidMap = { MEDIA_SAMPLE_CONTENT::MEDIA_TRANSPORT_PACKET, 1, { 0x1fff } };
		hr = m_pIKsPropertySet->Set(KSPROPSETID_BdaPIDFilter, KSPROPERTY_BDA_PIDFILTER_MAP_PIDS, &pidMap, sizeof(pidMap), &pidMap, sizeof(pidMap));
		::LeaveCriticalSection(&m_CriticalSection);

		static constexpr int CONFIRM_RETRY_TIME = 100;
		unsigned int confirmRemain = m_nSpecialLockConfirmTime;
		unsigned int tsidInterval = max(m_nSpecialLockSetTSIDInterval, CONFIRM_RETRY_TIME);
		while (1) {
			unsigned int sleepTime = min(tsidInterval, min(confirmRemain, CONFIRM_RETRY_TIME));
			if (!sleepTime) {
				OutputDebug(L"  Timed out.\n");
				break;
			}
			OutputDebug(L"    Waiting lock status remaining %d msec.\n", confirmRemain);
			SleepWithMessageLoop((DWORD)sleepTime);

			BOOLEAN sl = 0;

			if (m_bRewriteNominalRate && pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_T_TMCC && m_ProductID == PID_MLT5PE) {
				i2c_info I2C_SLVT = m_aI2c_slaves_mlt5pe[m_nTunerID].slvt;

				BYTE rdata = 0;

				::EnterCriticalSection(&m_CriticalSection);
				{
					LockProc Lock(&m_hSemaphore);
					// Set SLV-T Bank : 0x60
					it35_i2c_wr_reg(I2C_SLVT, 0x00, 0x60);
					// Read reg:0x10
					it35_i2c_rd_reg(I2C_SLVT, 0x10, &rdata);
				}
				::LeaveCriticalSection(&m_CriticalSection);
				if (rdata & 0x10) {
					OutputDebug(L"  Unlock.\n");
					break;
				}
				if (rdata & 0x01) {
					OutputDebug(L"  Lock.\n");
					sl = TRUE;
				}
				if (rdata & 0x02) {
					OutputDebug(L"  Sync.\n");
					sl = TRUE;
				}
			}
			else {
				::EnterCriticalSection(&m_CriticalSection);
				hr = m_pIBDA_SignalStatistics->get_SignalLocked(&sl);
				::LeaveCriticalSection(&m_CriticalSection);
			}
			if (sl) {
				OutputDebug(L"  Lock success.\n");
				success = TRUE;
				break;
			}
			confirmRemain -= sleepTime;
			tsidInterval -= sleepTime;

			if (!tsidInterval) {
				tsidInterval = max(m_nSpecialLockSetTSIDInterval, CONFIRM_RETRY_TIME);

				// TSID��Set����
				if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_S_TMCC) {
					::EnterCriticalSection(&m_CriticalSection);
					hr = it35_PutISDBIoCtl(m_pIKsPropertySet, pTuningParam->TSID == 0 ? (WORD)-1 : (WORD)pTuningParam->TSID);
					::LeaveCriticalSection(&m_CriticalSection);
				}
			}
		}

		OutputDebug(L"LockChannel: Complete.\n");

		return success ? S_OK : S_FALSE;
	}

	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::ReadIniFile(const WCHAR* szIniFilePath)
{
	const std::map<const std::wstring, const int, std::less<>> mapPrivateSetTSID = {
		{ L"NO",      enumPrivateSetTSID::ePrivateSetTSIDNone },
		{ L"YES",     enumPrivateSetTSID::ePrivateSetTSIDPreTR },
		{ L"PRETR",   enumPrivateSetTSID::ePrivateSetTSIDPreTR },
		{ L"POSTTR",  enumPrivateSetTSID::ePrivateSetTSIDPostTR },
		{ L"SPECIAL", enumPrivateSetTSID::ePrivateSetTSIDSpecial },
	};

	CIniFileAccess IniFileAccess(szIniFilePath);
	IniFileAccess.SetSectionName(L"IT35");

	// IF���g���� put_CarrierFrequency() ���s��
	m_bRewriteIFFreq = IniFileAccess.ReadKeyB(L"RewriteIFFreq", m_bRewriteIFFreq);

	// �ŗL�� Property set ���g�p����TSID�̏����݂��s�����[�h
	m_nPrivateSetTSID = (enumPrivateSetTSID)IniFileAccess.ReadKeyIValueMap(L"PrivateSetTSID", enumPrivateSetTSID::ePrivateSetTSIDNone, mapPrivateSetTSID);

	// LNB�d���̋�����ON����
	m_bLNBPowerON = IniFileAccess.ReadKeyB(L"LNBPowerON", FALSE);

	// Dual Mode ISDB Tuner
	m_bDualModeISDB = IniFileAccess.ReadKeyB(L"DualModeISDB", FALSE);

	// BDASpecial�ŗL��LockChannel���g�p����ꍇ��ISDB-S Lock�����m�F����
	m_nSpecialLockConfirmTime = IniFileAccess.ReadKeyI(L"SpecialLockConfirmTime", 4000);

	// BDASpecial�ŗL��LockChannel���g�p����ꍇ��ISDB-S Lock�����҂�����TSID�̍ăZ�b�g���s���C���^�[�o������
	m_nSpecialLockSetTSIDInterval = IniFileAccess.ReadKeyI(L"SpecialLockSetTSIDInterval", 400);

	// ISDB-T���ACXD2856��Nominal Rate���Đݒ肷��
	m_bRewriteNominalRate = IniFileAccess.ReadKeyB(L"RewriteNominalRate", FALSE);

	if (m_bRewriteNominalRate) {
		// ISDB-T���ACXD2856�ɐݒ肷��Nominal Rate
		std::wstring data;
		data = IniFileAccess.ReadKeyS(L"NominalRate_List", L"0x17,0xA0,0x00,0x00,0x00");
		// �J���}��؂��5�ɕ���
		std::wstring tmp;
		for (int n = 0; n < 5; n++) {
			size_t p = common::WStringSplit(&data, L',', &tmp);
			m_byNominalRate_List[n] = (BYTE)common::WStringToLong(tmp);
			if (std::wstring::npos == p)
				break;
		}
	}
	return S_OK;
}

const HRESULT CIT35Specials::PreLockChannel(TuningParam* pTuningParam)
{
	if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDSpecial)
		return S_OK;

	if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_S_TMCC) {
		// ���g���̑ш敝��9�ɐݒ�
		if (pTuningParam->Modulation.BandWidth == -1L) {
			pTuningParam->Modulation.BandWidth = 9L;
		}

		// IF���g���ɕϊ�
		if (m_bRewriteIFFreq) {
			if (pTuningParam->Antenna.LNBSwitch != -1) {
				if (pTuningParam->Frequency < pTuningParam->Antenna.LNBSwitch) {
					if (pTuningParam->Frequency > pTuningParam->Antenna.LowOscillator && pTuningParam->Antenna.HighOscillator != -1)
						pTuningParam->Frequency -= pTuningParam->Antenna.LowOscillator;
				}
				else {
					if (pTuningParam->Frequency > pTuningParam->Antenna.HighOscillator && pTuningParam->Antenna.HighOscillator != -1)
						pTuningParam->Frequency -= pTuningParam->Antenna.HighOscillator;
				}
			}
			else {
				if (pTuningParam->Antenna.Tone == 0) {
					if (pTuningParam->Frequency > pTuningParam->Antenna.LowOscillator && pTuningParam->Antenna.HighOscillator != -1)
						pTuningParam->Frequency -= pTuningParam->Antenna.LowOscillator;
				}
				else {
					if (pTuningParam->Frequency > pTuningParam->Antenna.HighOscillator && pTuningParam->Antenna.HighOscillator != -1)
						pTuningParam->Frequency -= pTuningParam->Antenna.HighOscillator;
				}
			}
		}
	}

	return S_OK;
}

const HRESULT CIT35Specials::PreTuneRequest(const TuningParam* pTuningParam, ITuneRequest* /*pITuneRequest*/)
{
	if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDSpecial)
		return S_OK;

	if (!m_pIKsPropertySet)
		return E_FAIL;

	HRESULT hr;

	// Dual Mode ISDB Tuner�̏ꍇ�̓f���W�����[�^�[�̕���Mode��ݒ�
	if (m_bDualModeISDB) {
		DWORD isdbMode = 0;
		switch (pTuningParam->Modulation.Modulation) {
		case BDA_MOD_ISDB_T_TMCC:
			isdbMode = PRIVATE_IO_CTL_FUNC_DEMOD_OFDM;
			break;
		case BDA_MOD_ISDB_S_TMCC:
			isdbMode = PRIVATE_IO_CTL_FUNC_DEMOD_PSK;
			break;
		}
		if (isdbMode != m_nLastIsdbMode) {
			::EnterCriticalSection(&m_CriticalSection);
			hr = it35_DigibestPrivateIoControl(m_pIKsPropertySet, isdbMode);
			::LeaveCriticalSection(&m_CriticalSection);
		}
		if (m_nLastIsdbMode == 0) {
			SleepWithMessageLoop(500);
		}
		m_nLastIsdbMode = isdbMode;
	}

	// TSID��Set����
	if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDPreTR && pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_S_TMCC) {
		::EnterCriticalSection(&m_CriticalSection);
		hr = it35_PutISDBIoCtl(m_pIKsPropertySet, pTuningParam->TSID == 0 ? (WORD)-1 : (WORD)pTuningParam->TSID);
		::LeaveCriticalSection(&m_CriticalSection);
	}
	return S_OK;
}

const HRESULT CIT35Specials::PostTuneRequest(const TuningParam* pTuningParam)
{
	if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDSpecial)
		return S_OK;

	if (!m_pIKsPropertySet)
		return E_FAIL;

	HRESULT hr;

	if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDPostTR) {
		::EnterCriticalSection(&m_CriticalSection);
		if (m_bRewriteNominalRate && pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_T_TMCC && m_ProductID == PID_MLT5PE) {
			i2c_info I2C_SLVT = m_aI2c_slaves_mlt5pe[m_nTunerID].slvt;
			i2c_info I2C_SLVX = m_aI2c_slaves_mlt5pe[m_nTunerID].slvx;
			i2c_info I2C_HELENE = m_aI2c_slaves_mlt5pe[m_nTunerID].gate;

			LockProc Lock(&m_hSemaphore);

			// Set SLV-T Bank : 0x10
			it35_i2c_wr_reg(I2C_SLVT, 0x00, 0x10);

			// TRCG Nominal Rate
			it35_i2c_wr_regs(I2C_SLVT, 0x9f, m_byNominalRate_List, 5);

			// Set SLV-T Bank : 0x00
			it35_i2c_wr_reg(I2C_SLVT, 0x00, 0x00);

			// SW Reset
			it35_i2c_wr_reg(I2C_SLVT, 0xfe, 0x01);
			// Enable TS Output
			it35_i2c_wr_reg(I2C_SLVT, 0xc3, 0x00);
		}

		// TSID��Set����
		if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_S_TMCC) {
			hr = it35_PutISDBIoCtl(m_pIKsPropertySet, pTuningParam->TSID == 0 ? (WORD)-1 : (WORD)pTuningParam->TSID);
		}

		// OneSeg���[�h��Set����
		if (pTuningParam->Modulation.Modulation == BDA_MOD_ISDB_T_TMCC) {
			hr = it35_PutOneSeg(m_pIKsPropertySet, FALSE);
		}

		// PID off
		hr = it35_PutPidFilterOnOff(m_pIKsPropertySet, FALSE);

		// PID Map
		BDA_PID_MAP pidMap = { MEDIA_SAMPLE_CONTENT::MEDIA_TRANSPORT_PACKET, 1, { 0x1fff } };
		hr = m_pIKsPropertySet->Set(KSPROPSETID_BdaPIDFilter, KSPROPERTY_BDA_PIDFILTER_MAP_PIDS, &pidMap, sizeof(pidMap), &pidMap, sizeof(pidMap));
		::LeaveCriticalSection(&m_CriticalSection);

	}

	return S_OK;
}

void CIT35Specials::Release(void)
{
	delete this;
}

WORD CIT35Specials::it35_checksum(const BYTE* buf, size_t len)
{
	size_t i;
	WORD checksum = 0;

	for (i = 1; i < len; i++) {
		if (i % 2)
			checksum += buf[i] << 8;
		else
			checksum += buf[i];
	}
	checksum = ~checksum;

	return checksum;
}

void CIT35Specials::it35_create_msg(WORD cmd, const BYTE* wbuf, DWORD wlen, BYTE* msg, DWORD* mlen)
{
	static BYTE seq = 0;
	WORD checksum;
	DWORD len;

	len = wlen + 4 + 2;

	msg[0] = (len - 1) & 0xff;
	msg[1] = cmd >> 8;
	msg[2] = cmd & 0xff;
	msg[3] = seq++;
	memcpy(msg + 4, wbuf, wlen);

	*mlen = len;

	checksum = it35_checksum(msg, len - 2);
	msg[len - 2] = (checksum >> 8);
	msg[len - 1] = (checksum & 0xff);

	return;
}

int CIT35Specials::it35_tx_bulk_msg(WORD cmd, const BYTE* wbuf, DWORD wlen, BYTE* rbuf, DWORD* rlen)
{
	BYTE msg[256] = {};
	DWORD mlen;
	HRESULT hr;

	if (wlen > 250)
		return -1;

	it35_create_msg(cmd, wbuf, wlen, msg, &mlen);
	BYTE seq = msg[3];

	if (FAILED(hr = it35_SendBulkData(m_pIKsPropertySet, msg, mlen))) {
		OutputDebug(L"it35_tx_bulk_msg:  Failed to it35_SendBulkData. cmd=0x%04x, hr=0x%08lx\n", cmd, hr);
		return (int)hr;
	}

	if (cmd == CMD_FW_DL || cmd == CMD_FW_BOOT || cmd == CMD_1023) {
		return 0;
	}

	mlen = 256;
	if (FAILED(hr = it35_RcvBulkData(m_pIKsPropertySet, msg, &mlen))) {
		OutputDebug(L"it35_tx_bulk_msg:  Failed to it35_RcvBulkData. cmd=0x%04x, hr=0x%08lx\n", cmd, hr);
		return (int)hr;
	}

	WORD checksum = it35_checksum(msg, mlen - 2);
	WORD tmp_checksum = (msg[mlen - 2] << 8) | msg[mlen - 1];
	if (tmp_checksum != checksum) {
		OutputDebug(L"it35_tx_bulk_msg:  Checksum mismatch. cmd=0x%04x, checksum=0x%04x:0x%04x\n", cmd, tmp_checksum, checksum);
		return -1;
	}

	if (msg[1] != seq) {
		OutputDebug(L"it35_tx_bulk_msg:  Sequence number mismatch. cmd=0x%04x, seq=0x%02x:0x%02x\n", cmd, msg[1], seq);
		return -1;
	}

	BYTE status = msg[2];
	if (status) {
		if (cmd == CMD_IR_GET && status == 1)
			return (int)status;

		OutputDebug(L"it35_tx_bulk_msg:  Error returned. cmd=0x%04x, status=0x%02x\n", cmd, status);
		return (int)status;
	}

	if (msg[1] == 4) {
		if (rlen)
			* rlen = 0;
	}
	else if (msg[1] > 4) {
		DWORD l = 0;
		if (rlen) {
			l = *rlen;
			*rlen = msg[1] - 4;
		}
		if (rbuf)
			memcpy(rbuf, msg + 3, min((DWORD)msg[1] - 4, l));
	}

	return (int)status;
}

int CIT35Specials::it35_i2c_wr(BYTE i2c_bus, BYTE i2c_addr, BYTE reg, BYTE* data, DWORD len)
{
	if (len > 246)
		return -1;

	BYTE buf[256] = {
		(len + 1) & 0xff,
		i2c_bus,
		I2C_ADDR_DATA(i2c_addr, I2C_MASTER_WRITE),
		reg,
	};
	if (len)
		memcpy(buf + 4, data, len);

	return it35_tx_bulk_msg(CMD_GENERIC_I2C_WR, buf, len + 4, NULL, NULL);
}

int CIT35Specials::it35_i2c_rd(BYTE i2c_bus, BYTE i2c_addr, BYTE* data, DWORD* len)
{
	if (!data || !len || *len < 1)
		return -1;

	BYTE buf[256] = {
		(*len) & 0xff,
		i2c_bus,
		I2C_ADDR_DATA(i2c_addr, I2C_MASTER_READ),
	};

	return it35_tx_bulk_msg(CMD_GENERIC_I2C_RD, buf, 3, data, len);
}

int CIT35Specials::it35_mem_wr_regs(DWORD reg, BYTE* data, DWORD len)
{
	if (len > 244)
		return -1;

	BYTE buf[256] = {
		len & 0xff,
		(reg & 0xff00) ? 0x02U : 0x01U,
		0x00,
		0x00,
		(reg & 0xff00) >> 8,
		reg & 0xff,
	};
	memcpy(buf + 6, data, len);

	return it35_tx_bulk_msg(CMD_MEM_WR, buf, len + 6, NULL, NULL);
}

int CIT35Specials::it35_mem_wr_reg(DWORD reg, BYTE data)
{
	BYTE temp = data;

	return it35_mem_wr_regs(reg, &temp, 1);
}

int CIT35Specials::it35_mem_rd_regs(DWORD reg, BYTE* data, DWORD* len)
{
	if (!data || !len || *len < 1)
		return -1;

	BYTE buf[256] = {
		(*len) & 0xff,
		(reg & 0xff00) ? 0x02U : 0x01U,
		0x00,
		0x00,
		(reg & 0xff00) >> 8,
		reg & 0xff,
	};

	return it35_tx_bulk_msg(CMD_MEM_RD, buf, 6, data, len);
}

int CIT35Specials::it35_mem_rd_reg(DWORD reg, BYTE* data)
{
	return it35_mem_wr_regs(reg, data, 1);
}

int CIT35Specials::it35_mem_set_reg_bits(DWORD reg, BYTE data, BYTE mask)
{
	int res;
	BYTE rdata;

	if (mask != 0xff) {
		res = it35_mem_rd_reg(reg, &rdata);
		if (res)
			return res;
		data = ((data & mask) | (rdata & (mask ^ 0xff)));
	}

	return it35_mem_wr_reg(reg, data);
}

int CIT35Specials::it35_i2c_wr_regs(i2c_info slave, BYTE reg, BYTE* data, DWORD len)
{
	return it35_i2c_wr(slave.bus, slave.addr, reg, data, len);
}

int CIT35Specials::it35_i2c_wr_reg(i2c_info slave, BYTE reg, BYTE data)
{
	BYTE temp = data;

	return it35_i2c_wr_regs(slave, reg, &temp, 1);
}

int CIT35Specials::it35_i2c_rd_regs(i2c_info slave, BYTE reg, BYTE* data, DWORD len)
{
	it35_mem_wr_reg(0xf424, 1);

	it35_i2c_wr(slave.bus, slave.addr, reg, NULL, 0);

	it35_mem_wr_reg(0xf424, 0);

	return it35_i2c_rd(slave.bus, slave.addr, data, &len);
}

int CIT35Specials::it35_i2c_rd_reg(i2c_info slave, BYTE reg, BYTE* data)
{
	return it35_i2c_rd_regs(slave, reg, data, 1);
}

int CIT35Specials::it35_i2c_set_reg_bits(i2c_info slave, BYTE reg, BYTE data, BYTE mask)
{
	int res;
	BYTE rdata;

	if (mask != 0xff) {
		res = it35_i2c_rd_reg(slave, reg, &rdata);
		if (res)
			return res;
		data = ((data & mask) | (rdata & (mask ^ 0xff)));
	}

	return it35_i2c_wr_reg(slave, reg, data);
}

int CIT35Specials::it35_i2c_thru_wr_regs(i2c_thru_info slaves, BYTE reg, BYTE* data, DWORD len)
{
	if (len > 248)
		return -1;

	BYTE buf[256] = {
		I2C_ADDR_DATA(slaves.child, I2C_MASTER_WRITE),
		reg,
	};
	if (len)
		memcpy(buf + 2, data, len);

	return it35_i2c_wr_regs(slaves.Parent, 0xfe, buf, len + 2);
}

int CIT35Specials::it35_i2c_thru_wr_reg(i2c_thru_info slaves, BYTE reg, BYTE data)
{
	BYTE temp = data;

	return it35_i2c_thru_wr_regs(slaves, reg, &temp, 1);
}

int CIT35Specials::it35_i2c_thru_rd_regs(i2c_thru_info slaves, BYTE reg, BYTE* data, DWORD len)
{	
	{
		BYTE buf[256] = {
			I2C_ADDR_DATA(slaves.child, I2C_MASTER_WRITE),
			reg,
		};
		it35_i2c_wr_regs(slaves.Parent, 0xfe, buf, 2);
	}

	it35_mem_wr_reg(0xf424, 1);

	{
		BYTE buf[256] = {
			I2C_ADDR_DATA(slaves.child, I2C_MASTER_READ),
		};
		it35_i2c_wr_regs(slaves.Parent, 0xfe, buf, 1);
	}

	it35_mem_wr_reg(0xf424, 0);

	return it35_i2c_rd(slaves.Parent.bus, slaves.Parent.addr, data, &len);
}

int CIT35Specials::it35_i2c_thru_rd_reg(i2c_thru_info slaves, BYTE reg, BYTE* data)
{
	return it35_i2c_thru_rd_regs(slaves, reg, data, 1);
}

int CIT35Specials::it35_i2c_thru_set_reg_bits(i2c_thru_info slaves, BYTE reg, BYTE data, BYTE mask)
{
	int res;
	BYTE rdata;

	if (mask != 0xff) {
		res = it35_i2c_thru_rd_reg(slaves, reg, &rdata);
		if (res)
			return res;
		data = ((data & mask) | (rdata & (mask ^ 0xff)));
	}

	return it35_i2c_thru_wr_reg(slaves, reg, data);
}

LockProc::LockProc(HANDLE* pHandle, DWORD dwMilliSeconds)
	: result(WAIT_FAILED),
	  pSemaphore(pHandle)
{
	if (pSemaphore) {
		result = ::WaitForSingleObject(*pSemaphore, dwMilliSeconds);
	}
	return;
};

LockProc::LockProc(HANDLE* pHandle)
	: LockProc(pHandle, 10000)
{
};

LockProc::~LockProc(void)
{
	if (pSemaphore) {
		::ReleaseSemaphore(*pSemaphore, 1, NULL);
	}
	return;
};

BOOL LockProc::IsSuccess(void)
{
	return (result == WAIT_OBJECT_0);
};

