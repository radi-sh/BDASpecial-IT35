#pragma once

/*
//
// Broadcast Driver Architecture で定義されている Property set / Method set
//
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

static const GUID KSPROPSETID_BdaPIDFilter = { 0xd0a67d65, 0x8df, 0x4fec,{ 0x85, 0x33, 0xe5, 0xb5, 0x50, 0x41, 0xb, 0x85 } };

enum KSPROPERTY_BDA_PIDFILTER {
	KSPROPERTY_BDA_PIDFILTER_MAP_PIDS = 0,				// put only
	KSPROPERTY_BDA_PIDFILTER_UNMAP_PIDS,				// put only
	KSPROPERTY_BDA_PIDFILTER_LIST_PIDS,					// put pnly
};

static const GUID KSMETHODSETID_BdaChangeSync = { 0xfd0a5af3, 0xb41d, 0x11d2,{ 0x9c, 0x95, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0 } };

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
*/

//
// IT9135 BDA driver 固有の Property set
//

// ITE 拡張プロパティセット GUID
static const GUID KSPROPSETID_IteExtension = { 0xc6efe5eb, 0x855a, 0x4f1b,{ 0xb7, 0xaa, 0x87, 0xb5, 0xe1, 0xdc, 0x41, 0x13 } };

// ITE 拡張プロパティID
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
	KSPROPERTY_ITE_EX_MERCURY_IR,						// get only	(ERROR_NOT_SUPORTED)
	KSPROPERTY_ITE_EX_MERCURY_SIGNAL_STRENGTH,			// get only
	KSPROPERTY_ITE_EX_CHANNEL_MODULATION = 99,			// get only
};

// DVB-S IO コントロール プロパティセット GUID
static const GUID KSPROPSETID_DvbsIoCtl = { 0xf23fac2d, 0xe1af, 0x48e0,{ 0x8b, 0xbe, 0xa1, 0x40, 0x29, 0xc9, 0x2f, 0x21 } };

// DVB-S IO コントロール プロパティID
enum KSPROPERTY_DVBS_IO_CTL {
	KSPROPERTY_DVBS_IO_LNB_POWER = 0,					// get/put
	KSPROPERTY_DVBS_IO_DiseqcLoad,						// put only
};

// 拡張 IO コントロール プロパティセット GUID
static const GUID KSPROPSETID_ExtIoCtl = { 0xf23fac2d, 0xe1af, 0x48e0,{ 0x8b, 0xbe, 0xa1, 0x40, 0x29, 0xc9, 0x2f, 0x11 } };

// 拡張 IO コントロール プロパティ ID
enum KSPROPERTY_EXT_IO_CTL {
	KSPROPERTY_EXT_IO_DRV_DATA = 0,						// get/put
	KSPROPERTY_EXT_IO_DEV_IO_CTL,						// get/put
	KSPROPERTY_EXT_IO_UNUSED50 = 50,					// not used
	KSPROPERTY_EXT_IO_UNUSED51 = 51,					// not used
	KSPROPERTY_EXT_IO_ISDBT_IO_CTL = 200,				// put only
};

// 拡張 IO コントロール KSPROPERTY_EXT_IO_DRV_DATA 用ファンクションコード
enum DRV_DATA_FUNC_CODE {
	DRV_DATA_FUNC_GET_DRIVER_INFO = 1,					// Read
};

// 拡張 IO コントロール KSPROPERTY_EXT_IO_DRV_DATA 用構造体
#pragma pack(1)
struct DrvDataDataSet {
	struct {
		DWORD DriverPID;
		DWORD DriverVersion;
		DWORD FwVersion_LINK;
		DWORD FwVersion_OFDM;
		DWORD TunerID;
	} DriverInfo;
};
#pragma pack()

// 拡張 IO コントロール KSPROPERTY_EXT_IO_DEV_IO_CTL 用ファンクションコード
enum DEV_IO_CTL_FUNC_CODE {
	DEV_IO_CTL_FUNC_READ_OFDM_REG = 0,					// Read
	DEV_IO_CTL_FUNC_WRITE_OFDM_REG,						// Write
	DEV_IO_CTL_FUNC_READ_LINK_REG,						// Read
	DEV_IO_CTL_FUNC_WRITE_LINK_REG,						// Write
	DEV_IO_CTL_FUNC_AP_CTRL,							// Write
	DEV_IO_CTL_FUNC_READ_RAW_IR = 7,					// Read
	DEV_IO_CTL_FUNC_GET_UART_DATA = 10,					// Read
	DEV_IO_CTL_FUNC_SENT_UART,							// Write
	DEV_IO_CTL_FUNC_SET_UART_BAUDRATE,					// Write
	DEV_IO_CTL_FUNC_CARD_DETECT,						// Read
	DEV_IO_CTL_FUNC_GET_ATR,							// Read
	DEV_IO_CTL_FUNC_AES_KEY,							// Write
	DEV_IO_CTL_FUNC_AES_ENABLE,							// Write
	DEV_IO_CTL_FUNC_RESET_SMART_CARD,					// Write
	DEV_IO_CTL_FUNC_IS_UART_READY,						// Read
	DEV_IO_CTL_FUNC_SET_ONE_SEG,						// Write
	DEV_IO_CTL_FUNC_GET_BOARD_INPUT_POWER = 25,			// Read
	DEV_IO_CTL_FUNC_SET_DECRYP,							// Write
	DEV_IO_CTL_FUNC_UNKNOWN99 = 99,						// Read
	DEV_IO_CTL_FUNC_GET_RX_DEVICE_ID,					// Read
	DEV_IO_CTL_FUNC_UNKNOWN101,							// Write
	DEV_IO_CTL_FUNC_IT930X_EEPROM_READ = 300,			// Read
	DEV_IO_CTL_FUNC_IT930X_EEPROM_WRITE,				// Write
	DEV_IO_CTL_FUNC_READ_GPIO,							// Read
	DEV_IO_CTL_FUNC_WRITE_GPIO,							// Write
};

// 拡張 IO コントロール KSPROPERTY_EXT_IO_DEV_IO_CTL 用構造体
#pragma pack(1)
struct DevIoCtlDataSet {
	union {
		struct {
			DWORD Addr;
			BYTE WriteData;
		} Reg;
		struct {
			DWORD Key;
			BYTE Enable;
		} Decryp;
		struct {
			BYTE Length;
			BYTE Buffer[3 + 254 + 2];
		} UartData;
		DWORD DeviceId;
	};
	union {
		DWORD CardDetected;
		DWORD UartReady;
		DWORD AesEnable;
	};
	BYTE Reserved1[2];
	WORD UartBaudRate;
	BYTE ATR[13];
	BYTE AesKey[16];
	BYTE Reserved2[2];
	DevIoCtlDataSet(void) {
		memset(this, 0, sizeof(*this));
	};
};
#pragma pack()

// プライベート IO コントロール プロパティセット GUID
static const GUID KSPROPSETID_PrivateIoCtl = { 0xede22531, 0x92e8, 0x4957,{ 0x9d, 0x5, 0x6f, 0x30, 0x33, 0x73, 0xe8, 0x37 } };

// プライベート IO コントロール プロパティ ID
enum KSPROPERTY_PRIVATE_IO_CTL {
	KSPROPERTY_PRIVATE_IO_DIGIBEST_TUNER = 0,			// put only
};

//
// ITE 拡張プロパティセット用関数
//
// チューニング帯域幅取得
static inline HRESULT it35_GetBandWidth(IKsPropertySet *pIKsPropertySet, WORD *pwData)
{
	HRESULT hr = S_OK;
	DWORD dwBytes;
	BYTE buf[sizeof(*pwData)];
	if (FAILED(hr = pIKsPropertySet->Get(KSPROPSETID_IteExtension, KSPROPERTY_ITE_EX_BAND_WIDTH, NULL, 0, buf, sizeof(buf), &dwBytes))) {
		return hr;
	}

	if (pwData)
		*pwData = *(WORD*)buf;

	return hr;
}

// チューニング帯域幅設定
static inline HRESULT it35_PutBandWidth(IKsPropertySet *pIKsPropertySet, WORD wData)
{
	return pIKsPropertySet->Set(KSPROPSETID_IteExtension, KSPROPERTY_ITE_EX_BAND_WIDTH, NULL, 0, &wData, sizeof(wData));
}

// チューニング周波数取得
static inline HRESULT it35_GetFreq(IKsPropertySet *pIKsPropertySet, WORD *pwData)
{
	HRESULT hr = S_OK;
	DWORD dwBytes;
	BYTE buf[sizeof(*pwData)];
	if (FAILED(hr = pIKsPropertySet->Get(KSPROPSETID_IteExtension, KSPROPERTY_ITE_EX_FREQ, NULL, 0, buf, sizeof(buf), &dwBytes))) {
		return hr;
	}

	if (pwData)
		*pwData = *(WORD*)buf;

	return hr;
}

// チューニング周波数設定
static inline HRESULT it35_PutFreq(IKsPropertySet *pIKsPropertySet, WORD wData)
{
	return pIKsPropertySet->Set(KSPROPSETID_IteExtension, KSPROPERTY_ITE_EX_FREQ, NULL, 0, &wData, sizeof(wData));
}

//
// DVB-S IO コントロール プロパティセット用関数
//
// LNBPower の設定状態取得
static inline HRESULT it35_GetLNBPower(IKsPropertySet *pIKsPropertySet, BYTE *pbyData)
{
	HRESULT hr = S_OK;
	DWORD dwBytes;
	BYTE buf[sizeof(*pbyData)];
	if (FAILED(hr = pIKsPropertySet->Get(KSPROPSETID_DvbsIoCtl, KSPROPERTY_DVBS_IO_LNB_POWER, NULL, 0, buf, sizeof(buf), &dwBytes))) {
		return hr;
	}

	if (pbyData)
		*pbyData = *(BYTE*)buf;

	return hr;
}

// LNBPower の設定
static inline HRESULT it35_PutLNBPower(IKsPropertySet *pIKsPropertySet, BYTE byData)
{
	return pIKsPropertySet->Set(KSPROPSETID_DvbsIoCtl, KSPROPERTY_DVBS_IO_LNB_POWER, NULL, 0, &byData, sizeof(byData));
}

//
// 拡張 IO コントロール KSPROPERTY_EXT_IO_DRV_DATA 用関数
//
// KSPROPERTY_EXT_IO_DRV_DATA 共通関数
static inline HRESULT it35_GetDrvData(IKsPropertySet *pIKsPropertySet, DWORD dwCode, DrvDataDataSet *pData)
{
	HRESULT hr = S_OK;
	DWORD dwBytes;
	BYTE buf[sizeof(*pData)];

	if FAILED(hr = pIKsPropertySet->Set(KSPROPSETID_ExtIoCtl, KSPROPERTY_EXT_IO_DRV_DATA, NULL, 0, &dwCode, sizeof(dwCode))) {
		return hr;
	}

	if (FAILED(hr = pIKsPropertySet->Get(KSPROPSETID_ExtIoCtl, KSPROPERTY_EXT_IO_DRV_DATA, NULL, 0, buf, sizeof(buf), &dwBytes))) {
		return hr;
	}

	if (pData)
		*pData = *(DrvDataDataSet*)buf;

	return hr;
}

// ドライバーバージョン情報取得
static inline HRESULT it35_GetDriverInfo(IKsPropertySet *pIKsPropertySet, DrvDataDataSet *pData)
{
	return it35_GetDrvData(pIKsPropertySet, DRV_DATA_FUNC_GET_DRIVER_INFO, pData);
}

//
// 拡張 IO コントロール KSPROPERTY_EXT_IO_DEV_IO_CTL 用関数
//
// KSPROPERTY_EXT_IO_DEV_IO_CTL 共通関数
static inline HRESULT it35_GetDevIoCtl(IKsPropertySet *pIKsPropertySet, BOOL bNeedGet, DWORD dwCode, DWORD *dwRetVal, DevIoCtlDataSet *pDataSet)
{
	HRESULT hr = S_OK;
	DWORD dwBytes;
#pragma pack(1)
	struct {
		DWORD dwData;
		DevIoCtlDataSet DataSet;
	} putGetData;
#pragma pack()

	putGetData.dwData = dwCode;
	if (pDataSet)
		putGetData.DataSet = *pDataSet;

	if FAILED(hr = pIKsPropertySet->Set(KSPROPSETID_ExtIoCtl, KSPROPERTY_EXT_IO_DEV_IO_CTL, NULL, 0, &putGetData, sizeof(putGetData))) {
		return hr;
	}

	if (!bNeedGet)
		return hr;

	if (FAILED(hr = pIKsPropertySet->Get(KSPROPSETID_ExtIoCtl, KSPROPERTY_EXT_IO_DEV_IO_CTL, NULL, 0, &putGetData, sizeof(putGetData), &dwBytes))) {
		return hr;
	}

	if (pDataSet)
		*pDataSet = putGetData.DataSet;

	if (dwRetVal)
		*dwRetVal = putGetData.dwData;

	return hr;
}

// OFDM Register 値取得
static inline HRESULT it35_ReadOfdmReg(IKsPropertySet *pIKsPropertySet, DWORD dwAddr, BYTE *pbyData)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	dataset.Reg.Addr = dwAddr;
	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_READ_OFDM_REG, &dwResult, &dataset))) {
		return hr;
	}

	if (pbyData)
		*pbyData = (BYTE)dwResult;

	return hr;
}

// OFDM Register 値書込
static inline HRESULT it35_WriteOfdmReg(IKsPropertySet *pIKsPropertySet, DWORD dwAddr, BYTE byData)
{
	DevIoCtlDataSet dataset;

	dataset.Reg.Addr = dwAddr;
	dataset.Reg.WriteData = byData;
	return it35_GetDevIoCtl(pIKsPropertySet, FALSE, DEV_IO_CTL_FUNC_WRITE_OFDM_REG, NULL, &dataset);
}

// Link Register 値取得
static inline HRESULT it35_ReadLinkReg(IKsPropertySet *pIKsPropertySet, DWORD dwAddr, BYTE *pbyData)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	dataset.Reg.Addr = dwAddr;
	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_READ_LINK_REG, &dwResult, &dataset))) {
		return hr;
	}

	if (pbyData)
		*pbyData = (BYTE)dwResult;

	return hr;
}

// Link Register 値書込
static inline HRESULT it35_WriteLinkReg(IKsPropertySet *pIKsPropertySet, DWORD dwAddr, BYTE byData)
{
	DevIoCtlDataSet dataset;

	dataset.Reg.Addr = dwAddr;
	dataset.Reg.WriteData = byData;
	return it35_GetDevIoCtl(pIKsPropertySet, FALSE, DEV_IO_CTL_FUNC_WRITE_LINK_REG, NULL, &dataset);
}

//
static inline HRESULT it35_ApCtrl(IKsPropertySet *pIKsPropertySet)
{
	return it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_AP_CTRL, NULL, NULL);
}

// リモコン受信データ取得
static inline HRESULT it35_ReadRawIR(IKsPropertySet *pIKsPropertySet, DWORD *pdwData)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_READ_RAW_IR, &dwResult, &dataset))) {
		return hr;
	}

	if (pdwData)
		*pdwData = dwResult;

	return hr;
}

// UART 送信＆受信データ取得
static inline HRESULT it35_GetUartData(IKsPropertySet *pIKsPropertySet, BYTE *pRcvBuff, DWORD *pdwLength)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;
	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_GET_UART_DATA, &dwResult, &dataset))) {
		return hr;
	}

	if (pRcvBuff)
		memcpy(pRcvBuff, dataset.UartData.Buffer, min((DWORD)min(dataset.UartData.Length, 255), *pdwLength));

	if (pdwLength)
		*pdwLength = dataset.UartData.Length;

	return hr;
}

// UART 送信データ設定
static inline HRESULT it35_SentUart(IKsPropertySet *pIKsPropertySet, BYTE *pSendBuff, DWORD dwLength)
{
	DevIoCtlDataSet dataset;

	if (pSendBuff) {
		dataset.UartData.Length = (BYTE)min(255, dwLength);
		memcpy(dataset.UartData.Buffer, pSendBuff, dataset.UartData.Length);
	}

	return it35_GetDevIoCtl(pIKsPropertySet, FALSE, DEV_IO_CTL_FUNC_SENT_UART, NULL, &dataset);
}

// UART ボーレート設定
static inline HRESULT it35_SetUartBaudRate(IKsPropertySet *pIKsPropertySet, WORD wBaudRate)
{
	DevIoCtlDataSet dataset;

	dataset.UartBaudRate = wBaudRate;
	return it35_GetDevIoCtl(pIKsPropertySet, FALSE, DEV_IO_CTL_FUNC_SET_UART_BAUDRATE, NULL, &dataset);
}

// 
static inline HRESULT it35_CardDetect(IKsPropertySet *pIKsPropertySet, BOOL *pbDetect)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_CARD_DETECT, &dwResult, &dataset))) {
		return hr;
	}

	if (pbDetect)
		*pbDetect = (BOOL)dataset.CardDetected;

	return hr;
}

// CARD ATR データ取得
static inline HRESULT it35_GetATR(IKsPropertySet *pIKsPropertySet, BYTE *pATR)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;
	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_GET_ATR, &dwResult, &dataset))) {
		return hr;
	}

	if (pATR)
		memcpy(pATR, dataset.ATR, sizeof(dataset.ATR));

	return hr;
}

// CARD リセット
static inline HRESULT it35_ResetSmartCard(IKsPropertySet *pIKsPropertySet)
{
	return it35_GetDevIoCtl(pIKsPropertySet, FALSE, DEV_IO_CTL_FUNC_RESET_SMART_CARD, NULL, NULL);
}

// UART Ready 状態取得
static inline HRESULT it35_IsUartReady(IKsPropertySet *pIKsPropertySet, BOOL *pbReady)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_IS_UART_READY, &dwResult, &dataset))) {
		return hr;
	}

	if (pbReady)
		*pbReady = (BOOL)dataset.UartReady;

	return hr;
}

//
static inline HRESULT it35_GetBoardInputPower(IKsPropertySet *pIKsPropertySet, BOOL *pbPower)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_GET_BOARD_INPUT_POWER, &dwResult, &dataset))) {
		return hr;
	}

	if (pbPower)
		*pbPower = (BOOL)dwResult;

	return hr;
}

// 
static inline HRESULT it35_Unk99(IKsPropertySet *pIKsPropertySet, BOOL *pbDetect)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_UNKNOWN99, &dwResult, &dataset))) {
		return hr;
	}

	if (pbDetect)
		*pbDetect = (BOOL)dataset.CardDetected;

	return hr;
}

// 
static inline HRESULT it35_GetRxDeviceId(IKsPropertySet *pIKsPropertySet, DWORD *pdwDevId)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_GET_RX_DEVICE_ID, &dwResult, &dataset))) {
		return hr;
	}

	if (pdwDevId)
		*pdwDevId = dataset.DeviceId;

	return hr;
}

// 
static inline HRESULT it35_Unk101(IKsPropertySet *pIKsPropertySet, BOOL *pbDetect)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	DevIoCtlDataSet dataset;

	if (FAILED(hr = it35_GetDevIoCtl(pIKsPropertySet, TRUE, DEV_IO_CTL_FUNC_UNKNOWN101, &dwResult, &dataset))) {
		return hr;
	}

	if (pbDetect)
		*pbDetect = (BOOL)dataset.CardDetected;

	return hr;
}

//
// 拡張 IO コントロール KSPROPERTY_EXT_IO_ISDBT_IO_CTL 用関数
//
// TSID をセット
static inline HRESULT it35_PutISDBIoCtl(IKsPropertySet *pIKsPropertySet, WORD dwData)
{
	return pIKsPropertySet->Set(KSPROPSETID_ExtIoCtl, KSPROPERTY_EXT_IO_ISDBT_IO_CTL, NULL, 0, &dwData, sizeof(dwData));
}

