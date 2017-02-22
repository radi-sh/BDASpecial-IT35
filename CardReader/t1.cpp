#include <Windows.h>
#include <iostream>

#include "t1.h"

#include "common.h"

CComProtocolT1::CComProtocolT1(void)
	: CardIFSC(32),
	SendNAD(0),
	RecvNAD(0),
	EDCType(0),
	SendFrame(),
	SendFrameLen(0),
	RecvFrame(),
	RecvFrameLen(0),
	EnableDetailLog(FALSE),
	IgnoreEDCError(FALSE)
{
};

CComProtocolT1::~CComProtocolT1(void)
{
};

WORD CComProtocolT1::GetEDCInitialValue(void)
{
	if (EDCType == EDC_TYPE_CRC)
		return 0xffff;

	return 0;
};

void CComProtocolT1::CalcEDC(BYTE Val, WORD *pEdc)
{
	if (!pEdc) return;

	if (EDCType == EDC_TYPE_CRC) {
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

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::MakeSendFrame(BYTE Pcb, const BYTE *pInf, BYTE Len)
{
	if (Len && !pInf)
		return COM_PROTOCOL_T1_E_POINTER;

	if (Len > 254)
		return COM_PROTOCOL_T1_E_LEN;

	WORD EDC = GetEDCInitialValue();	// Error Detection LRC or CRC

	BYTE *p = SendFrame;

	*p = SendNAD;	// Node Address
	CalcEDC(*p, &EDC);
	if (EnableDetailLog)
		OutputDebug(L"  Tx: NAD=0x%02x\n", *p);
	p++;

	*p = Pcb;	// Protocol Control Byte
	CalcEDC(*p, &EDC);
	if (EnableDetailLog)
		OutputDebug(L"  Tx: PCB=0x%02x\n", *p);
	p++;

	*p = Len;	// Length
	CalcEDC(*p, &EDC);
	if (EnableDetailLog)
		OutputDebug(L"  Tx: LEN=0x%02x\n", *p);
	p++;

	// Information Field
	if (Len)
		if (EnableDetailLog)
			OutputDebug(L"  Tx: INF=");
	for (int i = 0; i < Len; i++) {
		*p = pInf[i];
		CalcEDC(*p, &EDC);
		if (EnableDetailLog)
			OutputDebug(L"0x%02x ", *p);
		p++;
	}
	if (Len)
		if (EnableDetailLog)
			OutputDebug(L"\n");

	if (EDCType == EDC_TYPE_CRC) {
		// CRC
		*p = EDC & 0xff;
		if (EnableDetailLog)
			OutputDebug(L"  Tx: EDC(CRC)=0x%02x ", *p);
		p++;
		*p = EDC >> 8;
		if (EnableDetailLog)
			OutputDebug(L"0x%02x\n", *p);
		p++;
	}
	else {
		// LRC
		*p = EDC & 0xff;
		if (EnableDetailLog)
			OutputDebug(L"  Tx: EDC(LRC)=0x%02x\n", *p);
		p++;
	}

	SendFrameLen = 3 + Len + (EDCType == EDC_TYPE_CRC ? 2 : 1);

	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::ParseRecvdFrame(BYTE *pPcb, BYTE *pInf, BYTE *pLen)
{
	WORD EDC = GetEDCInitialValue();	// Error Detection LRC or CRC

	BYTE *p = RecvFrame;

	if (EnableDetailLog)
		OutputDebug(L"  Rx: NAD=0x%02x\n", *p);
	BYTE NAD = *p;		// Node Address
	if (NAD != RecvNAD) {
		OutputDebug(L"ParseRecvdFrame: Invalid NAD.\n");
		return COM_PROTOCOL_T1_E_NAD;
	}
	CalcEDC(*p, &EDC);
	p++;

	if (EnableDetailLog)
		OutputDebug(L"  Rx: PCB=0x%02x\n", *p);
	if (pPcb)
		*pPcb = *p;		// Protocol Control Byte
	CalcEDC(*p, &EDC);
	p++;

	if (EnableDetailLog)
		OutputDebug(L"  Rx: LEN=0x%02x\n", *p);
	if (pLen)
		*pLen = *p;		// Length
	if (*p > 254) {
		OutputDebug(L"ParseRecvdFrame: Invalid LEN.\n");
		return COM_PROTOCOL_T1_E_LEN;
	}
	CalcEDC(*p, &EDC);
	BYTE Len = *p;
	p++;

	if (RecvFrameLen < (DWORD)Len + 3 + (EDCType == EDC_TYPE_CRC ? 2 : 1)) {
		OutputDebug(L"ParseRecvdFrame: RecvFrameLen small. RecvFrameLen=%d.\n", RecvFrameLen);
		return COM_PROTOCOL_T1_E_LEN;
	}

	// Information Field
	if (Len)
		if (EnableDetailLog)
			OutputDebug(L"  Rx: INF=");
	for (int i = 0; i < Len; i++) {
		if (EnableDetailLog)
			OutputDebug(L"0x%02x ", *p);
		if (pInf)
			pInf[i] = *p;
		CalcEDC(*p, &EDC);
		p++;
	}
	if (Len)
		if (EnableDetailLog)
			OutputDebug(L"\n");

	// LRC & CRC
	if (EDCType == EDC_TYPE_CRC) {
		if (EnableDetailLog)
			OutputDebug(L"  Rx: EDC(CRC)=0x%02x 0x%02x\n", *p , *(p + 1));
	}
	else {
		if (EnableDetailLog)
			OutputDebug(L"  Rx: EDC(LRC)=0x%02x\n", *p);
	}
	if (*p != (EDC & 0xff) && !IgnoreEDCError) {
		OutputDebug(L"ParseRecvdFrame: EDC does not match.\n");
		return COM_PROTOCOL_T1_E_EDC;
	}

	if (EDCType == EDC_TYPE_CRC) {
		// CRC
		p++;
		if (*p != EDC >> 8 && !IgnoreEDCError) {
			OutputDebug(L"ParseRecvdFrame: EDC does not match.\n");
			return COM_PROTOCOL_T1_E_EDC;
		}
	}

	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::SendIBlock(BOOL SeqNum, BOOL Chain, const BYTE *pInf, BYTE Len)
{
	COM_PROTOCOL_T1_ERROR_CODE r;
	if ((r = MakeSendFrame(BLOCK_TYPE_I | (SeqNum ? IBLOCK_SEQUENCE : 0) | (Chain ? IBLOCK_CHAIN : 0) | IBLOCK_FUNCTION_STANDARD, pInf, Len)) != COM_PROTOCOL_T1_S_NO_ERROR) {
		return r;
	}

	if ((r = TxBlock()) != COM_PROTOCOL_T1_S_NO_ERROR) {
		return r;
	}

	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::SendRBlock(BOOL SeqNum, BYTE Stat)
{
	COM_PROTOCOL_T1_ERROR_CODE r;
	if ((r = MakeSendFrame(BLOCK_TYPE_R | (SeqNum ? RBLOCK_SEQUENCE : 0) | Stat, NULL, 0)) != COM_PROTOCOL_T1_S_NO_ERROR) {
		OutputDebug(L"SendRBlock: Error in MakeSendFrame(). code=%d\n", r);
		return r;
	}

	if ((r = TxBlock()) != COM_PROTOCOL_T1_S_NO_ERROR) {
		OutputDebug(L"SendRBlock: Error in TxBlock(). code=%d\n", r);
		return r;
	}

	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::SendSBlock(BOOL IsResponse, BYTE Func, const BYTE *pInf, BYTE Len)
{
	COM_PROTOCOL_T1_ERROR_CODE r;
	if ((r = MakeSendFrame(BLOCK_TYPE_S | (IsResponse ? SBLOCK_RESPONSE : 0) | Func, pInf, Len)) != COM_PROTOCOL_T1_S_NO_ERROR) {
		OutputDebug(L"SendSBlock: Error in MakeSendFrame(). code=%d\n", r);
		return r;
	}

	if ((r = TxBlock()) != COM_PROTOCOL_T1_S_NO_ERROR) {
		OutputDebug(L"SendSBlock: Error in TxBlock(). code=%d\n", r);
		return r;
	}

	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::RecvBlock(BYTE *pPcb, BYTE *pInf, BYTE *pLen)
{
	COM_PROTOCOL_T1_ERROR_CODE r;
	if ((r = RxBlock()) != COM_PROTOCOL_T1_S_NO_ERROR) {
		OutputDebug(L"RecvBlock: Error in RxBlock(). code=%d\n", r);
		return r;
	}

	if ((r = ParseRecvdFrame(pPcb, pInf, pLen)) != COM_PROTOCOL_T1_S_NO_ERROR) {
		OutputDebug(L"RecvBlock: Error in ParseRecvdFrame(). code=%d\n", r);
		return r;
	}

	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::Transmit(const BYTE *pSnd, DWORD LenSnd, BYTE *pRcv, DWORD *pLenRcv, BOOL *pSeqNum)
{
	DWORD MaxInfSize = CardIFSC;
	const BYTE *pSendBufPointer = pSnd;
	DWORD SendBufRemain = LenSnd;
	BYTE *pRecvBufPointer = pRcv;
	DWORD RecvBufLen = 0;
	BYTE RecvPCB;
	BYTE RecvTemp[254];
	BYTE RecvTempLen;
	COM_PROTOCOL_T1_ERROR_CODE r;
	while (SendBufRemain > MaxInfSize) {
		// ブロック分割送信が必要
		// I Block transmission with chaining
		int retry1 = 0;
		BOOL NeedSendI = TRUE;
		BYTE ErrorType1 = RBLOCK_FUNCTION_NO_ERRORS;
		while (1) {
			if (NeedSendI) {
				// Iブロックの送信（または再送）が必要
				NeedSendI = FALSE;
				if ((r = SendIBlock(*pSeqNum, TRUE, pSendBufPointer, (BYTE)MaxInfSize)) != COM_PROTOCOL_T1_S_NO_ERROR) {
					OutputDebug(L"Transmit: Error in SendIBlock() with chain. code=%d\n", r);
					return r;
				}
			}
			else {
				// エラーハンドリングでRブロックを返す
				if ((r = SendRBlock(*pSeqNum, RBLOCK_FUNCTION_OTHER_ERROR)) != COM_PROTOCOL_T1_S_NO_ERROR) {
					OutputDebug(L"Transmit: Error in SendRBlock() with chain. code=%d\n", r);
					return r;
				}
			}
			if ((r = RecvBlock(&RecvPCB, RecvTemp, &RecvTempLen)) != COM_PROTOCOL_T1_S_NO_ERROR) {
				// 受信フレームの異常
				retry1++;
				OutputDebug(L"Transmit: Error in RecvBlock() with chain. code=%d\n", r);
				if (retry1 > 3) {
					OutputDebug(L"Transmit: Retry count over.\n");
					return COM_PROTOCOL_T1_E_RECEIVE;
				}
				if (r == COM_PROTOCOL_T1_E_EDC)
					ErrorType1 = RBLOCK_FUNCTION_EDC_ERROR;
				else
					ErrorType1 = RBLOCK_FUNCTION_OTHER_ERROR;
				continue;
			}
			// 受信フレームは正常
			if (RecvPCB == (RBLOCK_FUNCTION_NO_ERRORS | (!*pSeqNum ? RBLOCK_SEQUENCE : 0))) {
				// 相手が受取ったので次のブロックへ
				break;
			}
			// 再送
			NeedSendI = TRUE;
			retry1++;
			OutputDebug(L"Transmit: Resent with chain.\n");
			if (retry1 > 3) {
				OutputDebug(L"Transmit: Retry count over.\n");
				return COM_PROTOCOL_T1_E_SEND;
			}
			ErrorType1 = RBLOCK_FUNCTION_NO_ERRORS;
			continue;
		}
		// 次のブロックへ
		*pSeqNum = !*pSeqNum;
		pSendBufPointer += MaxInfSize;
		SendBufRemain -= MaxInfSize;
	}
	// Standard I Block transmission
	int retry2 = 0;
	BOOL DoneSend = FALSE;
	BOOL NeedSendI2 = TRUE;
	BOOL SeqNumRecv = *pSeqNum;
	BYTE ErrorType = RBLOCK_FUNCTION_NO_ERRORS;
	while (1) {
		if (!DoneSend) {
			// 送信ブロックを相手が受け取っていない（または未確認）
			if (NeedSendI2) {
				// Iブロックの送信（または再送）が必要
				NeedSendI2 = FALSE;
				if ((r = SendIBlock(*pSeqNum, FALSE, pSendBufPointer, (BYTE)SendBufRemain)) != COM_PROTOCOL_T1_S_NO_ERROR) {
					OutputDebug(L"Transmit: Error in SendIBlock(). code=%d\n", r);
					return r;
				}
			}
			else {
				// エラーハンドリングでRブロックを返す
				if ((r = SendRBlock(*pSeqNum, ErrorType)) != COM_PROTOCOL_T1_S_NO_ERROR) {
					OutputDebug(L"Transmit: Error in SendRBlock() with sent. code=%d\n", r);
					return r;
				}
			}
		}
		else {
			// 送信ブロックを相手が受取済
			if ((r = SendRBlock(SeqNumRecv, ErrorType)) != COM_PROTOCOL_T1_S_NO_ERROR) {
				OutputDebug(L"Transmit: Error in SendRBlock() with receive. code=%d\n", r);
				return r;
			}
		}
		if ((r = RecvBlock(&RecvPCB, RecvTemp, &RecvTempLen)) != COM_PROTOCOL_T1_S_NO_ERROR) {
			// 受信フレームの異常
			retry2++;
			OutputDebug(L"Transmit: Error in RecvBlock(). code=%d\n", r);
			if (retry2 > 3) {
				OutputDebug(L"Transmit: Retry count over.\n");
				return COM_PROTOCOL_T1_E_RECEIVE;
			}
			if (r == COM_PROTOCOL_T1_E_EDC)
				ErrorType = RBLOCK_FUNCTION_EDC_ERROR;
			else
				ErrorType = RBLOCK_FUNCTION_OTHER_ERROR;
			continue;
		}
		// 受信フレームは正常
		if (!DoneSend && (RecvPCB & 0xc0) == BLOCK_TYPE_R && (RecvPCB & RBLOCK_SEQUENCE) == (*pSeqNum ? RBLOCK_SEQUENCE : 0)) {
			// R Block with current sequence number
			// 相手から再送の要求があった
			NeedSendI2 = TRUE;
			retry2++;
			OutputDebug(L"Transmit: Resent.\n");
			if (retry2 > 3) {
				OutputDebug(L"Transmit: Retry count over.\n");
				return COM_PROTOCOL_T1_E_SEND;
			}
			ErrorType = RBLOCK_FUNCTION_NO_ERRORS;
			continue;
		}
		else if ((RecvPCB & 0x80) == 0x00) {
			// I Block
			// 相手からのIブロックが返ってきた
			if (!DoneSend) {
				DoneSend = TRUE;
				*pSeqNum = !*pSeqNum;
			}
			retry2 = 0;
		}
		else {
			// その他、Rブロックに対するエラーハンドリング等
			retry2++;
			OutputDebug(L"Transmit: What's a happen.\n");
			if (retry2 > 3) {
				OutputDebug(L"Transmit: Retry count over.\n");
				return COM_PROTOCOL_T1_E_RECEIVE;
			}
			// ErrorType はそのまま保持しておく
			continue;
		}
		// 受信フレームの処理
		memcpy(pRecvBufPointer, RecvTemp, RecvTempLen);
		pRecvBufPointer += RecvTempLen;
		RecvBufLen += RecvTempLen;
		if ((RecvPCB & 0xa0) == (BLOCK_TYPE_I | IBLOCK_CHAIN)) {
			// I Block with chaining
			// 相手からのデータは次のブロックに継続している
			SeqNumRecv = !(RecvPCB & IBLOCK_SEQUENCE);
			ErrorType = RBLOCK_FUNCTION_NO_ERRORS;
			continue;
		}
		// Standard I Block
		// 継続ブロックがなければこれで受信完了
		break;
	}
	if (pLenRcv)
		*pLenRcv = RecvBufLen;

	return COM_PROTOCOL_T1_S_NO_ERROR;
}

void CComProtocolT1::SetCardIFSC(BYTE IFSC)
{
	CardIFSC = IFSC;

	return;
};

void CComProtocolT1::SetNodeAddress(BYTE IFD, BYTE ICC)
{
	SendNAD = (IFD & 0x07) | ((ICC & 0x07) << 4);
	RecvNAD = (ICC & 0x07) | ((IFD & 0x07) << 4);

	return;
};

void CComProtocolT1::SetEDCType(BYTE Type)
{
	EDCType = Type;

	return;
};

void CComProtocolT1::SetDetailLog(BOOL Flag)
{
	EnableDetailLog = Flag;

	return;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::TxBlock(void)
{
	return COM_PROTOCOL_T1_S_NO_ERROR;
};

CComProtocolT1::COM_PROTOCOL_T1_ERROR_CODE CComProtocolT1::RxBlock(void)
{
	return COM_PROTOCOL_T1_S_NO_ERROR;
};

