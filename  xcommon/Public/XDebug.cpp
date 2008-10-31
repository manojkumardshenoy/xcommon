/********************************************************************
Copyright (c) 2002-2003 �����Ƽ����޹�˾. ��Ȩ����.
�ļ�����: 	XDebug.cpp					
�ļ�����:	
�汾��ʷ:	1.0
����:		xuejuntao xuejuntao@hanwang.com.cn 2008/10/06
*********************************************************************/
#include "stdafx.h"
#include "XDebug.h"
#include "HWXString.h"

void WINAPI XTraceW(LPCWSTR pwhFormat, ...)
{  	
	static CXStringW strMsg;	
	LPWSTR pwhBuffer = NULL;
	LONG nLen = 0;
	va_list argList;
	va_start(argList, pwhFormat);
	nLen = _vscwprintf(pwhFormat, argList);
	if (nLen	< 0)
	{

	}
	else
	{
		strMsg = L"";
		pwhBuffer = strMsg.GetBuffer(nLen + 1);
		vswprintf_s(pwhBuffer, nLen + 1, pwhFormat, argList); 
		strMsg.ReleaseBuffer();
	}	
	va_end(argList);	
	OutputDebugStringW(strMsg);
}

void WINAPI XTraceA(LPCSTR pchFormat, ...)
{ 	
	static CXStringA strMsg;
	LPSTR pchBuffer = NULL;
	LONG nLen = 0;
	va_list argList;
	va_start(argList, pchFormat);
	nLen = _vscprintf(pchFormat, argList);
	if (nLen	< 0)
	{

	}
	else
	{
		strMsg = "";
		pchBuffer = strMsg.GetBuffer(nLen + 1);
		vsprintf_s(pchBuffer, nLen + 1, pchFormat, argList); 
		strMsg.ReleaseBuffer();
	}	
	va_end(argList);	
	OutputDebugStringA(strMsg);	
}