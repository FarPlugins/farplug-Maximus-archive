
#ifdef _UNICODE
	#if FAR_UNICODE>=2184
		#include "pluginW3.hpp"

		#undef FAR_UNICODE
		#define FAR_UNICODE FARMANAGERVERSION_BUILD
		
		GUID guid_EditWrap = { /* 580f7f4f-7e64-4367-84c1-5a6eb66dab1f */
		    0x580f7f4f,
		    0x7e64,
		    0x4367,
		    {0x84, 0xc1, 0x5a, 0x6e, 0xb6, 0x6d, 0xab, 0x1f}
		};
		GUID guid_EditWrapMenuItem = { /* f5a6fa34-13a9-40f6-9103-3ffed1f2a9c8 */
			0xf5a6fa34,
			0x13a9,
			0x40f6,
			{0x91, 0x03, 0x3f, 0xfe, 0xd1, 0xf2, 0xa9, 0xc8}
		};
		GUID guid_EditWrapMenuWork = { /* 2b302398-bbe9-4aff-b90e-7114a33432f5 */
			0x2b302398,
			0xbbe9,
			0x4aff,
			{0xb9, 0x0e, 0x71, 0x14, 0xa3, 0x34, 0x32, 0xf5}
		};

		#define PluginNumber &guid_EditWrap
		#define ADV_CTRL_NIL 0, 
	#else
		#include "pluginW.hpp"
		#define PluginNumber psi.ModuleNumber
		#define ADV_CTRL_NIL
	#endif
#else
	#include "pluginA.hpp"
	#define PluginNumber psi.ModuleNumber
	#define ADV_CTRL_NIL
#endif
#include <TCHAR.H>
#include <crtdbg.h>
#include "version.h"

struct PluginStartupInfo psi;
struct FarStandardFunctions FSF;

BOOL    gbLastWrap = FALSE;
int    *gpnWrappedEditors = NULL;
size_t  gnWrappedEditors = 0;

TCHAR gsWordDiv[256] = _T(" \t\r\n~!%^&*()+|{}:\"<>?`-=\\[];',./");
TCHAR gsPuctuators[256] = _T(" \t\r\n!?;,.");
inline bool IsSpaceOrNull(TCHAR x) { return x==_T(' ') || x==_T('\t') || x==0;  }

#define szMsgEditWrapPlugin _T("EditWrap")
#define szMsgItemToggleWrap _T("&1. Toggle wrap")
#define szMsgItemToggleWordWrap _T("&2. Toggle word wrap")
#define szMsgItemWrap _T("&3. Wrap")
#define szMsgItemWordWrap _T("&4. Word wrap")
#define szMsgItemUnWrap _T("&5. Unwrap")

HMODULE ghInstance=NULL;

#ifdef CRTSTARTUP
	extern "C"{
		BOOL WINAPI _DllMainCRTStartup(HANDLE hModule,DWORD dwReason,LPVOID lpReserved)
		{
		    if (ghInstance==NULL)
		        ghInstance = (HMODULE)hModule;
		    return TRUE;
		};
	};
#else
	BOOL APIENTRY DllMain(HANDLE hModule,DWORD dwReason,LPVOID lpReserved)
	{
	    if (ghInstance==NULL)
	        ghInstance = (HMODULE)hModule;
	    return TRUE;
	}
#endif

int WINAPI GetMinFarVersionW(void)
{
	#if FAR_UNICODE>=2184
	#define MAKEFARVERSION2(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))
	return MAKEFARVERSION2(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD);
	#else
	return FARMANAGERVERSION;
	#endif
}

#if FAR_UNICODE>=1906
	void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
	{
		//static wchar_t szTitle[16]; _wcscpy_c(szTitle, L"ConEmu");
		//static wchar_t szDescr[64]; _wcscpy_c(szTitle, L"ConEmu support for Far Manager");
		//static wchar_t szAuthr[64]; _wcscpy_c(szTitle, L"ConEmu.Maximus5@gmail.com");
		
		Info->StructSize = sizeof(GlobalInfo);
		Info->MinFarVersion = FARMANAGERVERSION;

		// Build: YYMMDDX (YY - ��� ����� ����, MM - �����, DD - ����, X - 0 � ����-����� ���������)
		//Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10), VS_RELEASE);
		Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,MVV_4, VS_RELEASE);
		
		Info->Guid = guid_EditWrap;
		Info->Title = szMsgEditWrapPlugin;
		Info->Description = szMsgEditWrapPlugin;
		Info->Author = L"ConEmu.Maximus5@gmail.com";
	}
#endif

void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
	static TCHAR szMenu[MAX_PATH];
	lstrcpy(szMenu, szMsgEditWrapPlugin);
    static TCHAR *pszMenu[1];
    pszMenu[0] = szMenu;

	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR|PF_DISABLEPANELS;

	#if FAR_UNICODE>=1906
		pi->PluginMenu.Guids = &guid_EditWrapMenuItem;
		pi->PluginMenu.Strings = pszMenu;
		pi->PluginMenu.Count = 1;
	#else
		pi->PluginMenuStrings = pszMenu;
		pi->PluginMenuStringsNumber = 1;
		pi->Reserved = 0x45644664; // EdFd
	#endif
}

void WINAPI SetStartupInfoW(const PluginStartupInfo *aInfo)
{
	::psi = *aInfo;
	::FSF = *aInfo->FSF;
	::psi.FSF = &::FSF;
}

void   WINAPI ExitFARW(
	#if FAR_UNICODE>=2000
		void*
	#else
		void
	#endif
)
{
}

INT_PTR EditCtrl(int Cmd, void* Parm)
{
	INT_PTR iRc;
	#if FAR_UNICODE>=1906
	iRc = psi.EditorControl(-1, (EDITOR_CONTROL_COMMANDS)Cmd, 0, Parm);
	#else
	iRc = psi.EditorControl(Cmd, Parm);
	#endif
	return iRc;
}

enum FoldWorkMode
{
	fwm_None = -1,
	fwm_ToggleWrap = 1,
	fwm_ToggleWordWrap = 2,
	fwm_Wrap = 3,
	fwm_WordWrap = 4,
	fwm_Unwrap = 5,
};

//void SetMenuItem(FarMenuItem* pItems, int nIdx, LPCTSTR pszBegin, LPCTSTR pszEnd)
//{
//	#ifdef _UNICODE
//	wchar_t* psz = (wchar_t*)calloc(64,sizeof(wchar_t));
//	pItems[nIdx].Text = psz;
//	#else
//	char* psz = pItems[nIdx].Text;
//	#endif
//	
//	#define MENU_PART 4
//	
//	if (nIdx < 9)
//		wsprintf(psz, _T("&%i. "), nIdx+1);
//	else if (nIdx == 9)
//		lstrcpy(psz, _T("&0. "));
//	else
//		lstrcpy(psz, _T("   "));
//	
//	int iStart = lstrlen(psz);
//	int i = iStart;
//	lstrcpyn(psz+i, pszBegin, MENU_PART+1 /*+1 �.�. ������� \0*/);
//	i = lstrlen(psz);
//	int iFin = iStart+MENU_PART+1;
//	while (i < iFin)
//		psz[i++] = _T(' ');
//	psz[i] = 0;
//	if (pszEnd)
//		lstrcpyn(psz+i, pszEnd, MENU_PART+1 /*+1 �.�. ������� \0*/);
//	i = lstrlen(psz);
//	iFin = iStart+MENU_PART*2+2;
//	while (i < iFin)
//		psz[i++] = _T(' ');
//	psz[i] = 0;
//	lstrcat(psz, pszEnd ? _T("Stream") : _T("Block"));
//}

FoldWorkMode ChooseWorkMode()
{
	#if FAR_UNICODE>=1900
	FarMenuItem
	#else
	FarMenuItemEx
	#endif
		Items[] =
	{
		{0, szMsgItemToggleWrap},
		{0, szMsgItemToggleWordWrap},
		{MIF_SEPARATOR},
		{0, szMsgItemWrap},
		{0, szMsgItemWordWrap},
		{0, szMsgItemUnWrap},
	};

	int nSel = -1;
	#if FAR_UNICODE>=1900
	nSel = psi.Menu(&guid_EditWrap, &guid_EditWrapMenuWork,
		-1,-1, 0, /*FMENU_CHANGECONSOLETITLE|*/FMENU_WRAPMODE,
		szMsgEditWrapPlugin, NULL, NULL,
		NULL, NULL, Items, ARRAYSIZE(Items));
	#else
	nSel = psi.Menu(psi.ModuleNumber,
		-1, -1, 0, FMENU_WRAPMODE|FMENU_USEEXT,
		szMsgEditWrapPlugin,NULL,NULL,
		0, NULL, (FarMenuItem*)Items, ARRAYSIZE(Items));
	#endif

	switch (nSel)
	{
	case 0:
		return fwm_ToggleWrap;
	case 1:
		return fwm_ToggleWordWrap;
	case 3:
		return fwm_Wrap;
	case 4:
		return fwm_WordWrap;
	case 5:
		return fwm_Unwrap;
	}

	return fwm_None;
}

INT_PTR FindExceed(wchar_t* pszCopy, INT_PTR iLine, INT_PTR iFrom, INT_PTR iFind, int iMaxWidth, int TabSize)
{
	INT_PTR nExceed = 0;

	// � ������ ����� ���� '\0', ��� ��� �������� ���� wcschr ������� ������ � ������������� ��������
	wchar_t* pszTab = wcschr(pszCopy+iFrom, L'\t');
	if (pszTab && (pszTab > (pszCopy + iFrom + iMaxWidth)))
	{
		nExceed = iFind;
	}
	else
	{
		INT_PTR TabPos = 1;

		// �������� �� ���� �������� �� ������� ������, ���� ��� ��� � �������� ������,
		// ���� �� ����� ������, ���� ������� ������ �� ��������� ������
		for (nExceed = iFrom; (nExceed < iFind) && (TabPos <= iMaxWidth); nExceed++)
		{
			// ������������ ����
			if (pszCopy[nExceed] == L'\t')
			{
				// ����������� ����� ���� � ������ �������� � ������� ������� � ������
				TabPos += TabSize-(TabPos%TabSize);
			}
			else
				TabPos++;
		}

		if ((TabPos > iMaxWidth) && (nExceed > iFrom) && (pszCopy[nExceed-1] == L'\t'))
		{
			nExceed--; // ����� "�����" ���� �� ����� �� �������
		}
		//	bExceed = true;
	}

	//return bExceed;
	return nExceed;
}

void DoWrap(BOOL abWordWrap, EditorInfo &ei, int iMaxWidth)
{
	INT_PTR iRc = 0;
	INT_PTR cchMax = 0;
	TCHAR* pszCopy = NULL;
	TCHAR szEOL[4];
	INT_PTR iFrom, iTo, iEnd, iFind;
	bool bWasModifed = (ei.CurState & ECSTATE_MODIFIED) && !(ei.CurState & ECSTATE_SAVED);

	gbLastWrap = TRUE;
	
	for (INT_PTR i = 0; i < ei.TotalLines; i++)
	{
		//bool lbCurLine = (i == ei.CurLine);
		
		EditorGetString egs = {};
		egs.StringNumber = i;
		iRc = EditCtrl(ECTL_GETSTRING, &egs);
		if (!iRc)
		{
			_ASSERTE(iRc!=0);
			goto wrap;
		}
		_ASSERTE(egs.StringText!=NULL);
		
		if (egs.StringLength <= iMaxWidth)
		{
			// ��� ������ ������ �� �����
			continue;
		}
		
		lstrcpyn(szEOL, egs.StringEOL?egs.StringEOL:_T(""), ARRAYSIZE(szEOL));
		
		if (egs.StringLength >= cchMax || !pszCopy)
		{
			if (pszCopy)
				free(pszCopy);
			cchMax = egs.StringLength + 255;
			pszCopy = (TCHAR*)malloc(cchMax*sizeof(*pszCopy));
			if (!pszCopy)
			{
				_ASSERTE(pszCopy!=NULL);
				goto wrap;
			}
		}
		// ������ �����, ��� ������� ����� ����������
		memmove(pszCopy, egs.StringText, egs.StringLength*sizeof(*pszCopy));
		pszCopy[egs.StringLength] = 0; // �� ������ ������, ���� ����� ������ ���� ASCIIZ
		
		bool lbFirst = 0;
		iFrom = 0; iEnd = egs.StringLength;
		while (iFrom < iEnd)
		{
			//iTo = min(iEnd,(iFrom+iMaxWidth));
			iTo = FindExceed(pszCopy, i, iFrom, min(iEnd/*+1*/,(iFrom+iMaxWidth)), iMaxWidth, ei.TabSize);
			iFind = iTo;
			if (iFind >= iEnd)
			{
				iFind = iTo = iEnd;
			}
			else if (abWordWrap
				/*&& (((egs.StringLength - iFrom) > iMaxWidth) || IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize))*/
				)
			{
				while (iFind > iFrom)
				{
					if (IsSpaceOrNull(pszCopy[iFind-1]))
						break;
					//{
					//	// ���� ���� ���� - ����� ��������� �� ������
					//	//TODO: Optimize, �� ��������, ���� ���� ����, ����� �������������� ������ �������� �������
					//	bool bExceed = IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize);

					//	if (!bExceed)
					//		break;
					//}
					iFind--;
				}
				// ���� �� �������� �������� �� �������, ��������� �� ������ ������?
				if (iFind == iFrom)
				{
					iFind = iTo;
					while (iFind > iFrom)
					{
						if (_tcschr(gsPuctuators, pszCopy[iFind]) && !_tcschr(gsWordDiv, pszCopy[iFind-1]))
							break;
						//{
						//	// ���� ���� ���� - ����� ��������� �� ������
						//	//TODO: Optimize, �� ��������, ���� ���� ����, ����� �������������� ������ �������� �������
						//	bool bExceed = IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize);

						//	if (!bExceed)
						//		break;
						//}
						iFind--;
					}
					if (iFind == iFrom)
					{
						iFind = iTo;
						while (iFind > iFrom)
						{
							if (_tcschr(gsWordDiv, pszCopy[iFind]) && !_tcschr(gsWordDiv, pszCopy[iFind-1]))
								break;
							//{
							//	// ���� ���� ���� - ����� ��������� �� ������
							//	//TODO: Optimize, �� ��������, ���� ���� ����, ����� �������������� ������ �������� �������
							//	bool bExceed = IsExceed(pszCopy, i, iFrom, iFind, iMaxWidth, ei.TabSize);

							//	if (!bExceed)
							//		break;
							//}
							iFind--;
						}
					}
				}
			}
			if (iFind == iFrom)
				iFind = iTo;

			if (iFind < iEnd)
			{
				// ��� ECTL_INSERTSTRING ����� ���������� ������
				EditorSetPosition eset = {};
				eset.CurLine = i;
				eset.TopScreenLine = -1;
				EditCtrl(ECTL_SETPOSITION, &eset);
				// ������ ����� ��������� ������
				EditCtrl(ECTL_INSERTSTRING, NULL);
				// ����� ������� ������ �� �������� ������
				if (i < ei.CurLine)
					ei.CurLine++;
			}
			// � ������ �� ������
			EditorSetString esset = {};
			esset.StringNumber = i;
			esset.StringText = pszCopy+iFrom;
			esset.StringEOL = (iFind == iEnd) ? szEOL : _T("");
			esset.StringLength = (iFind - iFrom);
			EditCtrl(ECTL_SETSTRING, &esset);

			// ��������� ��������
			if (iFind < iEnd)
			{
				i++; ei.TotalLines++; // �.�. �������� ������
			}
			
			// ��������� ����� ������
			iFrom = iFind;
		}
	}

	// �������� ������� �������
	{
		EditorSetPosition eset = {};
		eset.CurLine = ei.CurLine;
		eset.TopScreenLine = -1;
		EditCtrl(ECTL_SETPOSITION, &eset);
	}
	
wrap:
	if (pszCopy)
		free(pszCopy);

#ifdef _UNICODE
	// ����� ����� "������������"
	//TODO: bis-������?
	if (!bWasModifed)
		EditCtrl(ECTL_DROPMODIFEDFLAG, NULL);
#endif
}

void DoUnwrap(EditorInfo &ei)
{
	INT_PTR iRc = 0;
	INT_PTR cchMax = 0, cchPos = 0;
	TCHAR* pszCopy = NULL;
	TCHAR szEOL[4];
	bool bWasModifed = (ei.CurState & ECSTATE_MODIFIED) && !(ei.CurState & ECSTATE_SAVED);

	gbLastWrap = FALSE;
	
	for (INT_PTR i = 0; i < ei.TotalLines; i++)
	{
		EditorGetString egs = {};
		egs.StringNumber = i;
		iRc = EditCtrl(ECTL_GETSTRING, &egs);
		if (!iRc)
		{
			_ASSERTE(iRc!=0);
			goto wrap;
		}
		_ASSERTE(egs.StringText!=NULL);
		
		if (egs.StringEOL && *egs.StringEOL)
		{
			// � ���� ������ ���� EOL, �� �� �����������
			continue;
		}
		
		cchPos = 0;
		szEOL[0] = 0;
		INT_PTR j = i;
		while (j < ei.TotalLines)
		{
			if (!pszCopy || ((egs.StringLength + cchPos + 65536) > cchMax))
			{
				cchMax = egs.StringLength + cchPos + 65536;
				TCHAR* pszNew = (TCHAR*)malloc(cchMax*sizeof(*pszCopy));
				if (!pszNew)
				{
					_ASSERTE(pszNew!=NULL);
					goto wrap;
				}
				if (pszCopy)
				{
					if (cchPos > 0)
						memmove(pszNew, pszCopy, cchPos*sizeof(*pszCopy));
					free(pszCopy);
				}
				pszCopy = pszNew;
			}
			
			if (egs.StringLength > 0)
			{
				memmove(pszCopy+cchPos, egs.StringText, egs.StringLength*sizeof(*pszCopy));
				cchPos += egs.StringLength;
			}

			bool lbApplyAndBreak = false;

			if (*szEOL)
			{
				lbApplyAndBreak = true;
			}

			// �������� ��������� ������
			if ((j+1) >= ei.TotalLines)
			{
				lbApplyAndBreak = true;
			}
			else if (!lbApplyAndBreak)
			{
				egs.StringNumber = ++j;
				iRc = EditCtrl(ECTL_GETSTRING, &egs);
				if (!iRc)
				{
					_ASSERTE(iRc!=0);
					goto wrap;
				}
				_ASSERTE(egs.StringText!=NULL);
				if (egs.StringEOL && *egs.StringEOL)
				{
					// � ���� ������ ���� EOL, �� �� �����������
					lstrcpyn(szEOL, egs.StringEOL?egs.StringEOL:_T(""), ARRAYSIZE(szEOL));
				}
			}
			
			if (lbApplyAndBreak)
			{
				EditorSetString esset = {};
				esset.StringNumber = i;
				esset.StringText = pszCopy;
				esset.StringEOL = szEOL;
				esset.StringLength = cchPos;
				EditCtrl(ECTL_SETSTRING, &esset);
				
				for (INT_PTR k = i+1; k <= j; k++)
				{
					// ��� ECTL_DELETESTRING ����� ���������� ������
					EditorSetPosition eset = {};
					eset.CurLine = i+1;
					eset.TopScreenLine = -1;
					iRc = EditCtrl(ECTL_SETPOSITION, &eset);
					_ASSERTE(iRc);
					// ������� "���������"
					iRc = EditCtrl(ECTL_DELETESTRING, NULL);
					_ASSERTE(iRc);
					ei.TotalLines--;
					if (ei.CurLine > i)
						ei.CurLine--;
				}
				
				// ����� �� while
				break;
			}
		}
	}
	
	// �������� ������� �������
	{
		EditorSetPosition eset = {};
		eset.CurLine = ei.CurLine;
		eset.TopScreenLine = -1;
		EditCtrl(ECTL_SETPOSITION, &eset);
	}
	
wrap:
	if (pszCopy)
		free(pszCopy);

#ifdef _UNICODE
	// ����� ����� "������������"
	//TODO: bis-������?
	if (!bWasModifed)
		EditCtrl(ECTL_DROPMODIFEDFLAG, NULL);
#endif
}

HANDLE WINAPI OpenPluginW(
#if FAR_UNICODE>=2000
	OpenInfo *Info
#else
	int OpenFrom,INT_PTR Item
#endif
)
{
#if FAR_UNICODE>=1900
	int OpenFrom = Info->OpenFrom;
	INT_PTR Item = Info->Data;
#endif
	WindowInfo wi = {};
	EditorInfo ei = {};
	INT_PTR iRc = 0;
	INT_PTR cchMax = 0;
	SMALL_RECT rcFar = {};
	int iMaxWidth = 79;
	EditorUndoRedo ur = {};
	FoldWorkMode WorkMode = fwm_None;
	int *pWrappedEditor = NULL;
	int *pnFreeEditorId = NULL;

	#if FAR_UNICODE>=2000
	wi.StructSize = sizeof(wi);
	#endif
	wi.Pos = -1;

	#ifdef _UNICODE
	if (psi.AdvControl(PluginNumber, ACTL_GETFARRECT, ADV_CTRL_NIL &rcFar))
	{
		iMaxWidth = rcFar.Right - rcFar.Left;
	}
	#endif

	/*
#ifdef _DEBUG
	iMaxWidth = 20;
#endif
	*/
	
	psi.AdvControl(PluginNumber, ACTL_GETWINDOWINFO, ADV_CTRL_NIL &wi);
	if (wi.Type != WTYPE_EDITOR)
	{
		_ASSERTE(wi.Type != WTYPE_EDITOR);
		goto wrap;
	}
    
	iRc = EditCtrl(ECTL_GETINFO, &ei);
	if (!iRc)
	{
		_ASSERTE(iRc!=0);
		goto wrap;
	}
	
	if (gpnWrappedEditors)
	{
		
		for (size_t i = 0; i < gnWrappedEditors; i++)
		{
			if (gpnWrappedEditors[i] == ei.EditorID)
			{
				pWrappedEditor = gpnWrappedEditors+i;
				break;
			}
			if (!pnFreeEditorId && gpnWrappedEditors[i] == -1)
			{
				pnFreeEditorId = gpnWrappedEditors+i;
			}
		}
	}
	gbLastWrap = (pWrappedEditor != NULL);
	if (!pWrappedEditor)
	{
		if (pnFreeEditorId)
		{
			pWrappedEditor = pnFreeEditorId;
		}
		else
		{
			size_t nNewCount = gnWrappedEditors + 128;
			int *pnNew = (int*)malloc(nNewCount*sizeof(*pnNew));
			if (pnNew)
			{
				if (gpnWrappedEditors && gnWrappedEditors)
					memmove(pnNew, gpnWrappedEditors, gnWrappedEditors*sizeof(*pnNew));
				if (gpnWrappedEditors)
					free(gpnWrappedEditors);
				for (size_t k = gnWrappedEditors; k < nNewCount; k++)
					pnNew[k] = -1;
				gpnWrappedEditors = pnNew;
				pWrappedEditor = gpnWrappedEditors + gnWrappedEditors;
				gnWrappedEditors = nNewCount;
			}
			else
			{
				_ASSERTE(pnNew!=NULL);
			}
		}
	}
	
	
	// "callplugin/Plugin.Call"?
	#ifdef _UNICODE
	if ((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO)
	{
		#if FAR_UNICODE>=2184
		FoldWorkMode fw = fwm_None;
		OpenMacroInfo* p = (OpenMacroInfo*)Item;
		if (p->StructSize >= sizeof(*p) && p->Count)
		{
			if (p->Values[0].Type == FMVT_INTEGER)
				fw = (FoldWorkMode)(int)p->Values[0].Integer;
		}
		Item = fw;
		#endif

		if (Item >= fwm_ToggleWrap && Item <= fwm_Unwrap)
		{
			WorkMode = (FoldWorkMode)Item;
		}
	}
	#endif

	// �������� ����: ��� ������
	if (WorkMode == fwm_None)
		WorkMode = ChooseWorkMode();
	if (WorkMode == fwm_None)
	{
		goto wrap;
	}

	ur.Command = EUR_BEGIN;
	EditCtrl(ECTL_UNDOREDO, &ur);

	if (WorkMode == fwm_ToggleWrap)
		WorkMode = gbLastWrap ? fwm_Unwrap : fwm_Wrap;
	else if (WorkMode == fwm_ToggleWordWrap)
		WorkMode = gbLastWrap ? fwm_Unwrap : fwm_WordWrap;
	
	if (WorkMode == fwm_Wrap || WorkMode == fwm_WordWrap)
	{
		DoWrap((WorkMode == fwm_WordWrap), ei, iMaxWidth);
		if (pWrappedEditor)
			*pWrappedEditor = ei.EditorID;
	}
	else if (WorkMode == fwm_Unwrap)
	{
		DoUnwrap(ei);
		if (pWrappedEditor)
			*pWrappedEditor = -1;
	}
	else
	{
		_ASSERTE(WorkMode == fwm_Unwrap || WorkMode == fwm_Wrap || WorkMode == fwm_WordWrap);
	}
	
wrap:
	if (ur.Command == EUR_BEGIN)
	{
		ur.Command = EUR_END;
		EditCtrl(ECTL_UNDOREDO, &ur);
	}

	return INVALID_HANDLE_VALUE;
}
