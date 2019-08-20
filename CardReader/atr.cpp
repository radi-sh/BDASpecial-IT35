#include <Windows.h>

#include "atr.h"

CParseATR::CParseATR(void)
	: TS(0),
	T0(0),
	TA1(0),
	TB1(0),
	TC1(0),
	TA2(0),
	TB2(0),
	TC2(0),
	TAi_T1(0),
	TAi_T15(0),
	TBi_T1(0),
	TBi_T15(0),
	TCi_T1(0),
	Ti(),
	TCK(0),
	TDi(),
	PresenseTA1(FALSE),
	PresenseTB1(FALSE),
	PresenseTC1(FALSE),
	PresenseTA2(FALSE),
	PresenseTB2(FALSE),
	PresenseTC2(FALSE),
	PresenseTAi_T1(FALSE),
	PresenseTAi_T15(FALSE),
	PresenseTBi_T1(FALSE),
	PresenseTBi_T15(FALSE),
	PresenseTCi_T1(FALSE),
	LenTi(0),
	PresenseTCK(FALSE),
	PresenseTDi(),
	RawData(),
	RawDataLength(0)
{
	return;
};

int CParseATR::Parse(BYTE *pBuf, BYTE Len)
{
	this->RawDataLength = 0;

	if (!pBuf)
		return -1;

	if (Len == 0)
		return -1;

	if (Len > 33)
		Len = 33;

	BYTE *pBufStart = pBuf;

	// TS
	this->TS = *pBuf;
	this->ParsedInfo.Convention = (enum_Convention)*pBuf;
	pBuf++;
	Len--;

	int i = 0;
	int T = 0;
	BOOL PresenseTA = FALSE;
	BOOL PresenseTB = FALSE;
	BOOL PresenseTC = FALSE;
	BOOL PresenseTD = TRUE;

	while (PresenseTA || PresenseTB || PresenseTC || PresenseTD) {
		if (Len == 0)
			return -1;

		if (PresenseTA) {
			if (i == 1) {
				// TA1
				this->PresenseTA1 = TRUE;
				this->TA1 = *pBuf;
				this->ParsedInfo.DI = (enum_DI)(*pBuf & 0x0f);
				this->ParsedInfo.FI = (enum_FI)((*pBuf & 0xf0) >>4);
			}
			else if (i == 2) {
				// TA2
				this->PresenseTA2 = TRUE;
				this->TA2 = *pBuf;
				this->ParsedInfo.T = (*pBuf & 0x0f);
				this->ParsedInfo.EtuDuration = (enum_EtuDuration)((*pBuf & 0x10) >> 4);
				this->ParsedInfo.ModeSpecific = (enum_ModeSpecific)((*pBuf & 0xc0) >> 6);
			}
			else if (i >= 3) {
				// TAi
				if (T == 1) {
					// T=1
					this->PresenseTAi_T1 = TRUE;
					this->TAi_T1 = *pBuf;
					this->ParsedInfo.IFSC = *pBuf;
				}
				else if (T == 15) {
					// T=15
					this->PresenseTAi_T15 = TRUE;
					this->TAi_T15 = *pBuf;
				}
			}
			PresenseTA = FALSE;
			pBuf++;
			Len--;
			continue;
		}
		if (PresenseTB) {
			if (i == 1) {
				// TB1
				this->PresenseTB1 = TRUE;
				this->TB1 = *pBuf;
				this->ParsedInfo.PI1 = (*pBuf & 0x1f);
				this->ParsedInfo.II = (enum_II)((*pBuf & 0x60) >> 5);
			}
			else if (i == 2) {
				// TB2
				this->PresenseTB2 = TRUE;
				this->TB2 = *pBuf;
				this->ParsedInfo.PI2 = *pBuf;
			}
			else if (i >= 3) {
				// TBi
				if (T == 1) {
					// T=1
					this->PresenseTBi_T1 = TRUE;
					this->TBi_T1 = *pBuf;
					this->ParsedInfo.CWI = *pBuf & 0xf;
					this->ParsedInfo.BWI = (*pBuf & 0xf0) >> 4;
				}
				else if (T == 15) {
					// T=15
					this->PresenseTBi_T15 = TRUE;
					this->TBi_T15 = *pBuf;

				}
			}
			PresenseTB = FALSE;
			pBuf++;
			Len--;
			continue;
		}
		if (PresenseTC) {
			if (i == 1) {
				// TC1
				this->PresenseTC1 = TRUE;
				this->TC1 = *pBuf;
				this->ParsedInfo.N = *pBuf;
			}
			else if (i == 2) {
				// TC2
				this->PresenseTC2 = TRUE;
				this->TC2 = *pBuf;
				this->ParsedInfo.WI = *pBuf;
			}
			else if (i >= 3) {
				// TCi
				if (T == 1) {
					// T=1
					this->PresenseTCi_T1 = TRUE;
					this->TCi_T1 = *pBuf;
					if ((*pBuf & 0x01) != 0)
						this->ParsedInfo.ErrorDetection = ERROR_DETECTION_CRC;
					else
						this->ParsedInfo.ErrorDetection = ERROR_DETECTION_LRC;
				}
			}
			PresenseTC = FALSE;
			pBuf++;
			Len--;
			continue;
		}
		if (PresenseTD) {
			if (i == 0) {
				// T0
				this->T0 = *pBuf;
				this->LenTi = *pBuf & 0xf;
			}
			else {
				// TD1, TD2 or TDi
				this->PresenseTDi[i] = TRUE;
				this->TDi[i] = *pBuf;
				T = *pBuf & 0xf;
				if (T != 0)
					this->PresenseTCK = TRUE;
			}
			PresenseTA = (*pBuf & 0x10) != 0;
			PresenseTB = (*pBuf & 0x20) != 0;
			PresenseTC = (*pBuf & 0x40) != 0;
			PresenseTD = (*pBuf & 0x80) != 0;
			pBuf++;
			Len--;
			i++;
			continue;
		}
	}

	// Ti
	for (int ti = 0; ti < this->LenTi; ti++) {
		if (Len == 0)
			return -1;
		this->Ti[ti] = *pBuf;
		pBuf++;
		Len--;
	}

	// TCK
	if (this->PresenseTCK) {
		if (Len == 0)
			return -1;
		this->TCK = *pBuf;
		pBuf++;
		Len--;
	}

	this->RawDataLength = (BYTE)(pBuf - pBufStart);
	memcpy(this->RawData, pBufStart, this->RawDataLength);

	return 0;
};

CParseATR::ParsedInfo_t::ParsedInfo_t(void)
	: Convention(CONVENTION_UNKNOWN),
	DI(DI_1),
	FI(FI_372_5),
	PI1(0),
	II(II_25mA),
	N(255),
	T(0),
	EtuDuration(ETU_DURATION_DEFINED_BY_TA1),
	ModeSpecific(MODE_NEGOTIABLE),
	PI2(0),
	WI(10),
	IFSC(32),
	X(0),
	Y(0),
	CWI(13),
	BWI(4),
	SPU(0),
	ErrorDetection(ERROR_DETECTION_LRC)
{
	return;
};