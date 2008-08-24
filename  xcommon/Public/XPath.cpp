/********************************************************************
Copyright (c) 2002-2003 �����Ƽ����޹�˾. ��Ȩ����.
�ļ�����: 	XPath.cpp					
�ļ�����:	
�汾��ʷ:	1.0
����:		xuejuntao xuejuntao@hanwang.com.cn 2008/08/22
*********************************************************************/

#include "stdafx.h"
#include "XPath.h"
#include <shlwapi.h>
#pragma  comment (lib, "shlwapi.lib")

CXPathA::CXPathA( LPCSTR lpszPath) : CXStringA(lpszPath)
{

}
CXPathA::CXPathA() : CXStringA()
{
	
}
CXPathA::~CXPathA()
{

}
void CXPathA::Append( LPCSTR lpszPath)
{
	assert(lpszPath);
	assert(lpszPath);
	LONG nLen = lstrlenA(lpszPath) + StrLen();
	nLen = _max(nLen, MAX_PATH + 1);
	LPSTR pchBuffer = GetBuffer(nLen);
	PathAppendA(pchBuffer, lpszPath);
	ReleaseBuffer();
}


CXPathW::CXPathW( LPCWSTR lpszPath) : CXStringW(lpszPath)
{

}
CXPathW::CXPathW() : CXStringW()
{

}
CXPathW::~CXPathW()
{

}
void CXPathW::Append( LPCWSTR lpszPath)
{
	assert(lpszPath);
	LONG nLen = lstrlenW(lpszPath) + StrLen();
	nLen = _max(nLen, MAX_PATH + 1);
	LPWSTR pwhBuffer = GetBuffer(nLen);
	PathAppendW(pwhBuffer, lpszPath);
	ReleaseBuffer();
}

