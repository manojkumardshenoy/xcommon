#include "stdafx.h"
#include "HWPenIme.h"

//�������뷨Ҫ����ļ�
BYTE g_bPrecess[] = 
{
	0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,        // 00-0F VK_BACK
	0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,        // 10-1F VK_CAPTIAL
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // 20-2F VK_SPACE
	1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,        // 30-3F VK_0~VK_9
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,        // 40-4F
	1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,        // 50-5F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // 60-6F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // 70-7F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // 80-8F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // 90-9F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // A0-AF
	0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,        // B0-BF
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // C0-CF
	0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,        // D0-DF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        // E0-EF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0			// F0-FF
};

//*****************************************************************
//	��HIMC������Ϣ
//	��Ϣ���ݱ�����lpIMC->dwNumMsgBuf��
//	ÿ�η���Ϣǰ��Ҫ������Ϣ�ռ�
//*****************************************************************
BOOL GenerateMessage(HIMC hIMC, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet=FALSE;
	LPINPUTCONTEXT lpIMC= ImmLockIMC(hIMC);
	if(!lpIMC)
	{
		return FALSE;    
	}
	Helper_Trace(TEXT("IsWindow : %d\n"), LONG(lpIMC->hWnd));
	if (IsWindow(lpIMC->hWnd))
	{
		LPTRANSMSG lpTransMsg;
		if (!(lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf,(lpIMC->dwNumMsgBuf + 1) * sizeof(TRANSMSG))))
		{
			Helper_Trace(TEXT("Error : %d\n"), 0);
			goto error;
		}

		if (!(lpTransMsg = (LPTRANSMSG)ImmLockIMCC(lpIMC->hMsgBuf)))
		{
			Helper_Trace(TEXT("Error : %d\n"), 1);
			goto error;
		}

		lpTransMsg += (lpIMC->dwNumMsgBuf);
		lpTransMsg->message = msg;
		lpTransMsg->wParam = wParam;
		lpTransMsg->lParam = lParam;
		lpIMC->dwNumMsgBuf ++;

		ImmUnlockIMCC(lpIMC->hMsgBuf);
		ImmGenerateMessage(hIMC);//����Ϣ���͵�IME��IME�پ������Լ������Ǽ�������Ӧ�ó���
		bRet=TRUE;
	}
error:
	ImmUnlockIMC(hIMC); 
	return bRet;
}

//************************************************************
//	����Ϣ�����������Ϣ
//	LPDWORD lpdwTransKey:��Ϣ������,��ϵͳ�ṩ,��һ��˫��Ϊ������Ϣ�ռ���
//	UINT *uNumTranMsgs:��ǰ��Ϣ��
//	UINT msg,WPARAM wParam,LPARAM lParam����Ϣ����
//	remark:�ú���ֻ��ImeToAsciiEx�е���
//************************************************************
BOOL GenerateMessageToTransKey(LPDWORD lpdwTransKey, UINT *uNumTranMsgs, UINT msg, 
																 WPARAM wParam, LPARAM lParam) 
{
	LPTRANSMSG lpTransMsg;
	if (((*uNumTranMsgs) + 1) >= (UINT)*lpdwTransKey)
	{
		return FALSE;
	}
	lpTransMsg = (LPTRANSMSG)(lpdwTransKey + 1 + ( *uNumTranMsgs) * 3);
	lpTransMsg->message=msg;
	lpTransMsg->wParam=wParam;
	lpTransMsg->lParam=lParam;
	(*uNumTranMsgs)++;
	return TRUE;
}


BOOL WINAPI ImeInquire(LPIMEINFO lpIMEInfo,LPTSTR lpszUIClass,LPCTSTR lpszOption)
{
	lpIMEInfo->dwPrivateDataSize = 0;//ϵͳ������ΪINPUTCONTEXT.hPrivate����ռ�

	lpIMEInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST |
#ifdef _UNICODE
		IME_PROP_UNICODE |
#endif
		IME_PROP_IGNORE_UPKEYS |
		IME_PROP_END_UNLOAD ;

	lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE |
		IME_CMODE_NATIVE;

	lpIMEInfo->fdwSentenceCaps = IME_SMODE_NONE;
	lpIMEInfo->fdwUICaps = UI_CAP_2700;

	lpIMEInfo->fdwSCSCaps = 0;

	lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;

	_tcscpy(lpszUIClass,CLSNAME_UI);

	return TRUE;
}

BOOL WINAPI AboutDialogProc(HWND hWnd ,	UINT message ,WPARAM wParam ,LPARAM lParam );

BOOL WINAPI ImeConfigure(HKL hKL,HWND hWnd, DWORD dwMode, LPVOID lpData)
{
	//DebugLog(1,(DebugLogFile,"ImeConfigure\n"));
	switch (dwMode) {
		case IME_CONFIG_GENERAL:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUT), (HWND)hWnd, (DLGPROC)AboutDialogProc);
			break;
		default:
			return (FALSE);
			break;
	}
	return (TRUE);
}

/**********************************************************************/
/* ClearCompStr()                                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearCompStr(
												 LPINPUTCONTEXT lpIMC)
{
	HIMCC               hMem;
	LPCOMPOSITIONSTRING lpCompStr;
	DWORD               dwSize;

	if(!lpIMC) 
	{
		return (FALSE);
	}

	dwSize = sizeof(COMPOSITIONSTRING) + (MAX_PRIVATEDATA);

	if (!lpIMC->hCompStr)
	{		
		lpIMC->hCompStr = ImmCreateIMCC(dwSize);
	} 
	else if (hMem = ImmReSizeIMCC(lpIMC->hCompStr, dwSize)) 
	{
		lpIMC->hCompStr = hMem;
	} 
	else 
	{
		ImmDestroyIMCC(lpIMC->hCompStr);
		lpIMC->hCompStr = ImmCreateIMCC(dwSize);
		return FALSE;
	}

	lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
	if (!lpCompStr) 
	{
		ImmDestroyIMCC(lpIMC->hCompStr);
		lpIMC->hCompStr = ImmCreateIMCC(dwSize);
		return FALSE;
	}
	memset(lpCompStr,0, dwSize);
	lpCompStr->dwSize = dwSize;
	lpCompStr->dwCompStrOffset = sizeof(COMPOSITIONSTRING);
	lpCompStr->dwCompStrLen = 0;
	lpCompStr->dwResultStrOffset = sizeof(COMPOSITIONSTRING);
	lpCompStr->dwResultStrLen = 0;

	ImmUnlockIMCC(lpIMC->hCompStr);
	return (TRUE);
}


/**********************************************************************/
/* ImeSelect()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
	LPINPUTCONTEXT lpIMC;
	BOOL bRet=FALSE;
	if (!hIMC) 
	{
		return FALSE;
	}

	lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
	if (!lpIMC) 
	{
		return FALSE;
	}
	lpIMC->fOpen = fSelect;
	if(fSelect) 
	{
		bRet = ClearCompStr(lpIMC);
	}
	ImmUnlockIMC(hIMC);
	return bRet;
}

/*
ʹһ�����������ļ������ʧ���֪ͨ���뷨���µ����������ģ������ڴ���һЩ��ʼ������
HIMC hIMC :����������
BOOL fFlag : TRUE if activated, FALSE if deactivated. 
Returns TRUE if successful, FALSE otherwise. 
*/
BOOL WINAPI ImeSetActiveContext(HIMC hIMC,BOOL fFlag)
{
	//ͨ��IME��Ϣ��ʵ�ִ���״̬�仯
	Helper_Trace(TEXT("ImeSetActiveContext: %d\n"), fFlag);
	GenerateMessage(hIMC, WM_IME_NOTIFY, fFlag ? IMN_OPENSTATUSWINDOW : IMN_CLOSESTATUSWINDOW, 0);
	return TRUE;
}

/*
ϵͳ��������ӿ����ж�IME�Ƿ���ǰ��������
HIMC hIMC:����������
UINT uKey:��ֵ
LPARAM lKeyData: unknown
CONST LPBYTE lpbKeyState:����״̬,����256����״̬
return : TRUE-IME����,FALSE-ϵͳ����
ϵͳ�����ImeToAsciiEx������ֱ�ӽ�������Ϣ����Ӧ�ó���
*/
BOOL WINAPI ImeProcessKey(HIMC hIMC,UINT uKey,LPARAM lKeyData,CONST LPBYTE lpbKeyState)
{
	LPINPUTCONTEXT lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
	BOOL bRet=g_bPrecess[uKey];
	//return FALSE;
	if(lpIMC)
	{
		LPCOMPOSITIONSTRING lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
		if(lpCompStr)
		{
			if(uKey == VK_BACK && !lpCompStr->dwCompStrLen) 
			{//û�б������룬����IME�ر����봰��
				GenerateMessage(hIMC, WM_IME_ENDCOMPOSITION, 0, 0);
				bRet = FALSE;
			}
			ImmUnlockIMCC(lpIMC->hCompStr);
		}
		else
		{
			bRet=FALSE;
		}
		ImmUnlockIMC(hIMC);
	}
	else
	{
		bRet=FALSE;
	}
	return bRet;
}

/*
Ӧ�ó����������ӿ����������������ĵ�ת�������뷨����������ӿ���ת���û�������
UINT uVKey:��ֵ,�����ImeInquire�ӿ���ΪfdwProperty����������IME_PROP_KBD_CHAR_FIRST,����ֽ��������ֵ
UINT uScanCode:������ɨ���룬��ʱ��������ͬ���ļ�ֵ����ʱ��Ҫʹ��uScanCode������
CONST LPBYTE lpbKeyState:����״̬,����256����״̬
LPDWORD lpdwTransKey:��Ϣ����������������IMEҪ����Ӧ�ó������Ϣ����һ��˫���ǻ������������ɵ������Ϣ����
UINT fuState:Active menu flag(come from msdn)
HIMC hIMC:����������
return : ���ر�������Ϣ������lpdwTransKey�е���Ϣ����
*/
UINT WINAPI ImeToAsciiEx (UINT uVKey,UINT uScanCode, CONST LPBYTE lpbKeyState, LPDWORD lpdwTransKey, UINT fuState, HIMC hIMC)
{
	Helper_Trace(TEXT("ImeToAsciiEx\n"));
	LPINPUTCONTEXT lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
	UINT uMsgCount = 0;
	if(lpIMC)
	{
		BYTE byInput = HIBYTE(uVKey);
		LPCOMPOSITIONSTRING lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
		if(lpCompStr)
		{
			LPBYTE pComp = (LPBYTE)GETLPCOMPSTR(lpCompStr);
			if(lpCompStr->dwCompStrLen==0)
			{//��ʼ��������
				//���ɿ�ʼ������Ϣ�Ի�ȡ������ʱ���봰�ڵ�����
				GenerateMessageToTransKey(lpdwTransKey, &uMsgCount, WM_IME_STARTCOMPOSITION, 0, 0);
			}
			if(byInput == VK_BACK)
			{
				if(lpCompStr->dwCompStrLen > 0) 
				{
					lpCompStr->dwCompStrLen --;
				}
			}
			else
			{
				pComp[lpCompStr->dwCompStrLen ++ ] = byInput;
			}

			if(lpCompStr->dwCompStrLen == 9)
			{//������������,ͨ������ת������û����룬��������Ϣ����Ӧ�ó���
				static TCHAR szSample[] = TEXT("�������");
				LPTSTR pResult = GETLPRESULTSTR(lpCompStr);
				_tcscpy(pResult, szSample);
				lpCompStr->dwResultStrLen = _tcslen(szSample);
				lpCompStr->dwCompStrLen = 0;
				//֪ͨӦ�ó����������
				GenerateMessageToTransKey(lpdwTransKey, &uMsgCount, WM_IME_COMPOSITION, 0, GCS_RESULTSTR | GCS_COMPSTR);
				//����IME�ر����봰��
				GenerateMessageToTransKey(lpdwTransKey, &uMsgCount, WM_IME_ENDCOMPOSITION, 0, 0);
			}
			else
			{//�����������
				GenerateMessageToTransKey(lpdwTransKey,&uMsgCount,WM_IME_COMPOSITION, 0, GCS_COMPSTR);
			}
			ImmUnlockIMCC(lpIMC->hCompStr);
		}
		ImmUnlockIMC(hIMC);
	}
	return (uMsgCount);
}


BOOL WINAPI ImeRegisterWord(
														LPCTSTR lpszReading,
														DWORD   dwStyle,
														LPCTSTR lpszString)
{
	return (FALSE);
}


BOOL WINAPI ImeUnregisterWord(
															LPCTSTR lpszReading,
															DWORD   dwStyle,
															LPCTSTR lpszString)
{
	return (FALSE);
}


UINT WINAPI ImeGetRegisterWordStyle(
																		UINT       nItem,
																		LPSTYLEBUF lpStyleBuf)
{
	return (FALSE);
}

UINT WINAPI ImeEnumRegisterWord(
																REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
																LPCTSTR              lpszReading,
																DWORD                dwStyle,
																LPCTSTR              lpszString,
																LPVOID               lpData)
{
	return (FALSE);
}



DWORD WINAPI ImeConversionList(HIMC hIMC,LPCTSTR lpSource,LPCANDIDATELIST lpCandList,DWORD dwBufLen,UINT uFlag)
{
	return 0;
}


BOOL WINAPI ImeDestroy(UINT uForce)
{
	if (uForce) 
	{
		return (FALSE);
	}
	return (TRUE);
}


LRESULT WINAPI ImeEscape(HIMC hIMC,UINT uSubFunc,LPVOID lpData)
{
	return FALSE;
}

//��Ӧ�ó��򷢸����뷨����Ϣ�����뷨�����ڴ���Ӧ�ó��������
//return : TRUE-��ȷ��Ӧ������,FALSE-����Ӧ
BOOL WINAPI NotifyIME(HIMC hIMC,DWORD dwAction,DWORD dwIndex,DWORD dwValue)
{
	BOOL bRet = FALSE;
	switch(dwAction)
	{
	case NI_OPENCANDIDATE:
		break;
	case NI_CLOSECANDIDATE:
		break;
	case NI_SELECTCANDIDATESTR:
		break;
	case NI_CHANGECANDIDATELIST:
		break;
	case NI_SETCANDIDATE_PAGESTART:
		break;
	case NI_SETCANDIDATE_PAGESIZE:
		break;
	case NI_CONTEXTUPDATED:
		switch (dwValue)
		{
		case IMC_SETCONVERSIONMODE:
			break;
		case IMC_SETSENTENCEMODE:
			break;
		case IMC_SETCANDIDATEPOS:
			break;
		case IMC_SETCOMPOSITIONFONT:
			break;
		case IMC_SETCOMPOSITIONWINDOW:
			break;
		case IMC_SETOPENSTATUS:
			break;
		default:
			break;
		}
		break;

	case NI_COMPOSITIONSTR:
		switch (dwIndex)
		{
		case CPS_COMPLETE:
			break;
		case CPS_CONVERT:
			break;
		case CPS_REVERT:
			break;
		case CPS_CANCEL:
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
	return bRet;
}

BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwComp, LPCVOID lpRead, DWORD dwRead)
{
	return FALSE;
}
