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
	PresenseTDi()
{
	return;
};

int CParseATR::Parse(BYTE *pBuf, BYTE Len)
{
	if (!pBuf)
		return -1;

	if (Len = 0)
		return -1;

	this->TS = *pBuf;
	pBuf++;
	Len--;

	int i = 0;
	int T = 0;
	BOOL PresenseTA = FALSE;
	BOOL PresenseTB = FALSE;
	BOOL PresenseTC = FALSE;
	BOOL PresenseTD = TRUE;

	while (PresenseTA || PresenseTB || PresenseTC || PresenseTD) {
		if (Len = 0)
			return -1;

		if (PresenseTA) {
			if (i == 1) {
				this->PresenseTA1 = TRUE;
				this->TA1 = *pBuf;
			}
			else if (i == 2) {
				this->PresenseTA2 = TRUE;
				this->TA2 = *pBuf;
			}
			else if (i >= 3) {
				if (T == 1) {
					this->PresenseTAi_T1 = TRUE;
					this->TAi_T1 = *pBuf;
					this->ParsedInfo.IFSC = *pBuf;
				}
				else if (T == 15) {
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
				this->PresenseTB1 = TRUE;
				this->TB1 = *pBuf;
			}
			else if (i == 2) {
				this->PresenseTB2 = TRUE;
				this->TB2 = *pBuf;
			}
			else if (i >= 3) {
				if (T == 1) {
					this->PresenseTBi_T1 = TRUE;
					this->TBi_T1 = *pBuf;
					this->ParsedInfo.CWI = *pBuf & 0xf;
					this->ParsedInfo.BWI = (*pBuf & 0xf0) >> 4;
				}
				else if (T == 15) {
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
				this->PresenseTC1 = TRUE;
				this->TC1 = *pBuf;
			}
			else if (i == 2) {
				this->PresenseTC2 = TRUE;
				this->TC2 = *pBuf;
			}
			else if (i >= 3) {
				if (T == 1) {
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
				this->LenTi = *pBuf & 0xf;
			}
			else {
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

	for (int ti = 0; ti < this->LenTi; ti++) {
		if (Len == 0)
			return -1;
		this->Ti[ti] = *pBuf;
		pBuf++;
		i++;
	}

	if (this->PresenseTCK) {
		if (Len == 0)
			return -1;
		this->TCK = *pBuf;
	}

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