
#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include "helper.h"
#include "resource.h"
#include "imm.h"

#define CS_INPUTSTAR			(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS)

//��������
#define CLASSNAME_HWPEN  TEXT("HWX_HWPEN")
#define HWX_WINDOWNAME		TEXT("HWX_WND_WAITINGFORHWPEN")

#define CLSNAME_UI			_T("ISUI")		//UI
#define CLSNAME_STATUS	_T("ISSTATUS")	//״̬��
#define CLSNAME_INPUT		_T("ISINPUT")	//���봰��
#define CLSNAME_COMP		_T("ISCOMP")	//���봰��
#define CLSNAME_CAND		_T("ISCAND")	//��ѡ����
#define CLSNAME_SPCHAR	_T("ISSPCHAR")	//�����ַ�����


#define WM_MSG_HWPENINPUT			TEXT("WM_MSG_HWPENSENDWORD")

#define MAX_PRIVATEDATA			100
#define MAX_CAND            256
#define	CANDPERPAGE					5


typedef struct tagUIPRIV 
{      // IME private UI data
	HWND    hStatusWnd;         // status window
	HWND    hInputWnd;          // ���봰�ڣ��������봰�ںͺ�ѡ���������Ӵ���
	HWND		m_hHWPenWnd;
}UIPRIV;

typedef UIPRIV      *PUIPRIV;
typedef UIPRIV NEAR *NPUIPRIV;
typedef UIPRIV FAR  *LPUIPRIV;

typedef struct tagTRANSMSG 
{
	UINT   message;
	WPARAM wParam;
	LPARAM lParam;
} TRANSMSG, *PTRANSMSG, NEAR *NPTRANSMSG, FAR *LPTRANSMSG;

extern HINSTANCE	g_hInst;
extern DWORD			g_dwInputMessage;



//���ڹ��̻ص�
LRESULT WINAPI UIWndProc(HWND hWnd,	UINT message,WPARAM wParam,	LPARAM lParam);
LRESULT WINAPI InputWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT WINAPI CandWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT WINAPI CompWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT WINAPI StatusWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT WINAPI HWWindowWaitForInput(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);