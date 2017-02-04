#pragma once

void CalcEdc(BYTE Val, WORD *pEdc, BOOL EdcTypeCrc);
int MakeSendFrame(BYTE Nad, BYTE Pcb, BYTE Len, BYTE *pInf, BOOL EdcTypeCrc, BYTE *pFrame, DWORD *pFrameLen);
int ParseRecvdFrame(BYTE *pByte, BYTE *pPcb, BYTE *pLen, BYTE *pInf, BOOL EdcTypeCrc, BYTE *pFrame, DWORD FrameLen);
