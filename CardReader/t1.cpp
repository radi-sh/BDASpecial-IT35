#include <Windows.h>

#include "t1.h"

void CalcEdc(BYTE Val, WORD *pEdc, BOOL EdcTypeCrc)
{
	if (!pEdc) return;

	if (EdcTypeCrc) {
		WORD crc = *pEdc;
		WORD tmp = Val << 8;
		for (int i = 0; i < 8; i++) {
			if ((crc ^ tmp) & 0x8000)
				crc ^= 0x1021;
			crc <<= 1;
			tmp <<= 1;
		}
		*pEdc = crc;
	}
	else {
		*pEdc = (WORD)(*(LPBYTE)pEdc ^ Val);
	}
};

int MakeSendFrame(BYTE Nad, BYTE Pcb, BYTE Len, BYTE *pInf, BOOL EdcTypeCrc, BYTE *pFrame, DWORD *pFrameLen)
{
	if (!pFrame)
		return -1;

	if (Len && !pInf)
		return -1;

	WORD Edc = 0;	// Error Detection LRC or CRC
	if (EdcTypeCrc)
		Edc = 0xffff;

	BYTE *p = pFrame;

	*p = Nad;	// Node Address
	CalcEdc(*p, &Edc, EdcTypeCrc);
	p++;

	*p = Pcb;	// Protocol Control Byte
	CalcEdc(*p, &Edc, EdcTypeCrc);
	p++;

	*p = Len;	// Length
	CalcEdc(*p, &Edc, EdcTypeCrc);
	p++;

	// Information Field
	for (int i = 0; i < Len; i++) {
		*p = *pInf;
		CalcEdc(*p, &Edc, EdcTypeCrc);
		p++;
		pInf++;
	}

	if (EdcTypeCrc) {
		// CRC
		*(LPWORD)p = Edc;
		p += 2;
	}
	else {
		// LRC
		*p = Edc & 0xff;
		p++;
	}

	if (pFrameLen)
		*pFrameLen = (DWORD)(p - pFrame);

	return 0;
};

int ParseRecvdFrame(BYTE *pNad, BYTE *pPcb, BYTE *pLen, BYTE *pInf, BOOL EdcTypeCrc, BYTE *pFrame, DWORD FrameLen)
{
	if (!pFrame)
		return -1;

	WORD Edc = 0;	// Error Detection LRC or CRC
	if (EdcTypeCrc)
		Edc = 0xffff;

	BYTE *p = pFrame;

	if (pNad)
		*pNad = *p;		// Node Address
	CalcEdc(*p, &Edc, EdcTypeCrc);
	p++;

	if (pPcb)
		*pPcb = *p;		// Protocol Control Byte
	CalcEdc(*p, &Edc, EdcTypeCrc);
	p++;

	if (pLen)
		*pLen = *p;		// Length
	CalcEdc(*p, &Edc, EdcTypeCrc);
	BYTE Len = *p;
	p++;

	if (FrameLen < (DWORD)Len + 3 + EdcTypeCrc ? 2 : 1)
		return -2;

	// Information Field
	for (int i = 0; i < Len; i++) {
		if (pInf)
			*pInf = *p;
		CalcEdc(*p, &Edc, EdcTypeCrc);
		p++;
		pInf++;
	}

	if (EdcTypeCrc) {
		// CRC
		if (*(LPWORD)p != Edc)
			return -1;
	}
	else {
		// LRC
		if (*p != (Edc & 0xff))
			return -1;
	}
	return 0;
};
