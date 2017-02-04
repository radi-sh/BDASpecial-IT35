#pragma once

class CParseATR {
public:
	CParseATR(void);
	int Parse(BYTE *pBuf, BYTE Len);

public:
	enum enum_Convention {
		CONVENTION_UNKNOWN = 0,
		CONVENTION_DIRECT = 0x3b,
		CONVENTION_INVERSE = 0x3f,
	} Convention;

	enum enum_DI {
		DI_RFU_0 = 0,
		DI_1,
		DI_2,
		DI_4,
		DI_8,
		DI_16,
		DI_32,
		DI_64,
		DI_12,
		DI_20,
		DI_RFU_10,
		DI_RFU_11,
		DI_RFU_12,
		DI_RFU_13,
		DI_RFU_14,
		DI_RFU_15,
	};

	enum enum_FI {
		FI_372_4 = 0,
		FI_372_5,
		FI_558_6,
		FI_744_8,
		FI_1116_12,
		FI_1488_16,
		FI_1860_20,
		FI_RFU_7,
		FI_RFU_8,
		FI_512_5,
		FI_768_7dot5,
		FI_1024_10,
		FI_1536_15,
		FI_2048_20,
		FI_RFU_14,
		FI_RFU_15,
	};

	enum enum_II {
		II_25mA = 0,
		II_50mA,
		II_RFU_2,
		II_RFU_3,
	};

	enum enum_EtuDuration {
		ETU_DURATION_DEFINED_BY_TA1,
		ETU_DURATION_IMPLICITLY_KNOWN,
	};

	enum enum_ModeSpecific {
		MODE_NEGOTIABLE,
		MODE_SPECIFIC_ONLY,
	};

	enum enum_ErrorDetection {
		ERROR_DETECTION_LRC,
		ERROR_DETECTION_CRC,
	};
	struct ParsedInfo_t {
		enum_Convention Convention;			// TS
		enum_DI DI;							// TA1
		enum_FI FI;							// TA1
		BYTE PI1;							// TB1
		enum_II II;							// TB1
		BYTE N;								// TC1
		BYTE T;								// TA2
		enum_EtuDuration EtuDuration;		// TA2
		enum_ModeSpecific ModeSpecific;		// TA2
		BYTE PI2;							// TB2
		BYTE WI;							// TC2
		BYTE IFSC;							// TAi T=1
		BYTE X;								// TAi T=15
		BYTE Y;								// TAi T=15
		BYTE CWI;							// TBi T=1
		BYTE BWI;							// TBi T=1
		BYTE SPU;							// TBi T=15 (???)
		enum_ErrorDetection ErrorDetection;	// TCi T=1
		ParsedInfo_t(void);
	} ParsedInfo;

private:
	BYTE TS;
	BYTE T0;
	BYTE TA1;
	BYTE TB1;
	BYTE TC1;
	BYTE TA2;
	BYTE TB2;
	BYTE TC2;
	BYTE TAi_T1;
	BYTE TAi_T15;
	BYTE TBi_T1;
	BYTE TBi_T15;
	BYTE TCi_T1;
	BYTE Ti[15];
	BYTE TCK;
	BYTE TDi[15];
	BOOL PresenseTA1;
	BOOL PresenseTB1;
	BOOL PresenseTC1;
	BOOL PresenseTA2;
	BOOL PresenseTB2;
	BOOL PresenseTC2;
	BOOL PresenseTAi_T1;
	BOOL PresenseTAi_T15;
	BOOL PresenseTBi_T1;
	BOOL PresenseTBi_T15;
	BOOL PresenseTCi_T1;
	BYTE LenTi;
	BOOL PresenseTCK;
	BOOL PresenseTDi[15];
};

