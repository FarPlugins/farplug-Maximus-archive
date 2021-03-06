
/*
Copyright (c) 2011 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <windows.h>
#include <rpc.h>
#include <TCHAR.h>
#include <malloc.h>
#include <map>
#include <vector>
#include <crtdbg.h>
#include <TlHelp32.h>
#include "version.h"
#include "Memory.h"
#include "SetHook.h"

#ifdef _DEBUG
	//#define USE_MINIDUMP
	#undef USE_MINIDUMP
	//#define FORCE_TEXT_COMMIT
	#undef FORCE_TEXT_COMMIT
	#define SHOWLOADLIBRARY
	#define DebugStr(x) OutputDebugString(x)
#else
	#undef USE_MINIDUMP
	#undef FORCE_TEXT_COMMIT
	#undef SHOWLOADLIBRARY
	#define DebugStr(x) //OutputDebugString(x)
#endif

#ifdef USE_MINIDUMP
	#include "MiniDump.h"
#else
	#define DebugDump(asText,asTitle)
#endif

#define ZeroStruct(s) memset(&s, 0, sizeof(s))
//TODO
#define InvalidOp()


//#define ASSERTSTRUCT(s) { if (sizeof(Far2::s)!=sizeof(s)) AssertStructSize(sizeof(s), sizeof(Far2::s), L#s, __FILE__, __LINE__); }
//#define ASSERTSTRUCTGT(s) { if (sizeof(Far2::s)>sizeof(s)) AssertStructSize(sizeof(s), sizeof(Far2::s), L#s, __FILE__, __LINE__); }
#include "Assert3.h"


// class LogCmd, LOG_CMD, LOG_CMD0
#define LOG_COMMANDS
#include "LogCmd.h"
#ifdef LOG_COMMANDS
int LogCmd::nNestLevel = 0;
#endif


// Far2 & Far3 plugin API's
#define _FAR_NO_NAMELESS_UNIONS

namespace Far2
{
#undef __PLUGIN_HPP__
#include "pluginW.hpp"
#undef __FARKEYS_HPP__
#include "farkeys.hpp"
#undef __COLORS_HPP__
#include "farcolor.hpp"
};

//namespace Far3
//{
#undef __PLUGIN_HPP__
#if MVV_3<=2102
	#include "pluginW3#2098.hpp"
	#define MCTLARG(g) INVALID_HANDLE_VALUE
#elif MVV_3<=2124
	#include "pluginW3#2124.hpp"
	#define MCTLARG(g) INVALID_HANDLE_VALUE
#elif MVV_3<=2172
	#include "pluginW3#2163.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2176
	#include "pluginW3#2174.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2183
	#include "pluginW3#2180.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2188
	#include "pluginW3#2184.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2203
	#include "pluginW3#2194.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2342
	#include "pluginW3#2342.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2375
	#include "pluginW3#2375.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2400 // �������, ��� ���������� - ������ ����
	#include "pluginW3#2400.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2426
	#include "pluginW3#2426.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2457
	#include "pluginW3#2457.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2461
	#include "pluginW3#2461.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2540
	#include "pluginW3#2540.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2566
	#include "pluginW3#2566.hpp"
	#define MCTLARG(g) &g
#elif MVV_3<=2798
	#include "pluginW3#2798.hpp"
	#define MCTLARG(g) &g
#else
	#include "pluginW3.hpp"
	#define MCTLARG(g) &g
	#define OPEN_FROM_MASK 0xFF
#endif
#undef __FARKEYS_HPP__
#if MVV_3<=2102
	#include "farkeys3#2016.hpp"
#endif
#undef __COLORS_HPP__
#include "farcolor3.hpp"
#ifndef MAKE_OPAQUE
	#define MAKE_OPAQUE(x)
#endif
#include "ConEmuColors3.h"
//};

#include "Far3Wrap.h"

HMODULE ghFar3Wrap = NULL;
DWORD gnMainThreadId = 0;

#if MVV_3>=2103
	FARSTDINPUTRECORDTOKEYNAME FarInputRecordToName = NULL;
	FARSTDKEYNAMETOINPUTRECORD FarNameToInputRecord = NULL;
#endif

#define DEFAULT_DESCRIPTION L"Far 2.x plugin"

#if MVV_3>=2103
	#define WrapGuids(g) &mguid_Plugin, &g
#else
	#define WrapGuids(g) &mguid_Plugin
#endif

#if MVV_3<=2102
	#define FarKeyToName3 FSF3.FarKeyToName
#else
	#define FarKeyToName3 FarKeyToName
#endif

struct WrapPluginInfo;

struct WRAP_CHAR_INFO
{
	size_t nCount;
	CHAR_INFO* p2;
	bool isExt;
};

struct WRAP_FAR_CHAR_INFO
{
	size_t nCount;
	FAR_CHAR_INFO* p;
};

struct Far2Dialog
{
	// TAG
	DWORD_PTR WrapMagic; // F2WR
	// Far3
	HANDLE hDlg3;
	WrapPluginInfo* wpi;
	
	// Far2
    int m_X1, m_Y1, m_X2, m_Y2;
    wchar_t *m_HelpTopic;
    Far2::FarDialogItem *m_Items2; // ������(!) �� ��, ��� �������� Far2 ������
    DWORD *m_Colors2; // ��� ��������, (m_Items2->Flags & (DIF_COLORMASK|DIF_SETCOLOR))
    FarDialogItem *m_Items3;
    FarList *mp_ListInit3;
	//CHAR_INFO * const *mpp_FarCharInfo2;
	WRAP_CHAR_INFO *mpp_FarCharInfo2; // [m_ItemsNumber]
	WRAP_CHAR_INFO mp_FarCharInfoDummy2;
    WRAP_FAR_CHAR_INFO *mpp_FarCharInfo3;
	WRAP_FAR_CHAR_INFO mp_FarCharInfoDummy3;
    UINT m_ItemsNumber;
    DWORD m_Flags;
    Far2::FARWINDOWPROC m_DlgProc;
    LONG_PTR m_Param;
    BOOL m_GuidChecked;
    GUID m_PluginGuid, m_Guid, m_DefGuid;

	WRAP_CHAR_INFO& GetVBufPtr2(UINT DlgItem);
	WRAP_FAR_CHAR_INFO& GetVBufPtr3(UINT DlgItem);
    
    void FreeDlg();
	static INT_PTR WINAPI Far3DlgProc(HANDLE hDlg, int Msg, int Param1, void* Param2);
    int RunDlg();
	HANDLE InitDlg();
    
	Far2Dialog(WrapPluginInfo* pwpi,
		int X1, int Y1, int X2, int Y2,
	    const wchar_t *HelpTopic, Far2::FarDialogItem *Items, UINT ItemsNumber,
	    DWORD Flags, Far2::FARWINDOWPROC DlgProc, LONG_PTR Param,
	    GUID PluginGuid, GUID DefGuid);
	~Far2Dialog();
};

std::map<Far2Dialog*,HANDLE> *gpMapDlg_2_3 = NULL;
std::map<HANDLE,Far2Dialog*> *gpMapDlg_3_2 = NULL;

typedef unsigned int LoaderFunctions3;
static const LoaderFunctions3
	LF3_None               = 0,
	LF3_Analyse            = 0x0001,
	LF3_CustomData         = 0x0002,
	LF3_Dialog             = 0x0004,
	LF3_Editor             = 0x0008,
	LF3_FindList           = 0x0010,
	LF3_Viewer             = 0x0020,
	LF3_GetFiles           = 0x0040,
	LF3_PutFiles           = 0x0080,
	LF3_DeleteFiles        = 0x0100
	//LF3_Window             = (LF3_Dialog|LF3_Editor|LF3_Viewer),
	//LF3_Full               = (LF3_Analyse|LF3_Dialog|LF3_Editor|LF3_Viewer|LF3_Window)
	//LF3_FullCustom       = (LF3_Full|LF3_CustomData)
	;

//struct WrapUpdateFunctions
//{
//	LoaderFunctions3 mn_Functions; // set of LoaderFunctions3
//	int nResourceID;
//	wchar_t ms_Loader[MAX_PATH+1], ms_IniFile[MAX_PATH+1];
//};

//std::vector<WrapUpdateFunctions> UpdateFunc;

//#define IsFarColorValid(c) ( ((c).Flags & (FCF_FG_4BIT|FCF_BG_4BIT)) == (FCF_FG_4BIT|FCF_BG_4BIT) && ((c).Flags & ~0xFF) == 0 )

#define WRAPLISTDATAMAGIC 0x93FB2E5C
struct WrapListItemData
{
	size_t nMagic;
	size_t nSize;
	DWORD  Data;
};

struct WrapPluginInfo
{
	// Instance variables
	HMODULE mh_Loader; // Loader.dll / Loader64.dll
	HMODULE mh_Dll;    // HMODULE ���������� �������
	wchar_t ms_DllFailFunc[64], ms_DllFailTitle[MAX_PATH*2]; DWORD mn_DllFailCode;
	DWORD m_MinFarVersion;
	LoaderFunctions3 mn_LoaderFunctions, mn_PluginFunctions; // set of LoaderFunctions3
	wchar_t* m_ErrorInfo; // In-pointer, Out-error text (used for Loader)
	int m_ErrorInfoMax; // max size of ErrorInfo in wchar_t (used for Loader)
	wchar_t ms_PluginDll[MAX_PATH+1]; // ���������� ������ (Far2)
	wchar_t ms_IniFile[MAX_PATH+1]; // ������ ���� � �������� ����������
	wchar_t ms_File[64]; // ��� ��������, ������ ��� ����� ������� (Far2)
	wchar_t ms_Title[MAX_PATH+1], ms_Desc[256], ms_Author[256]; // ���������� ��� GetGlobalInfoW
	wchar_t ms_RegRoot[1024]; // ���� � ������� ��� �������� Far2, ��� ��� ����� ������� ���������
	wchar_t ms_ForcePrefix[64];
	VersionInfo m_Version;
	DWORD mn_FakeFarVersion;
	int mn_DoNotFreeLibrary;
	int mn_LoadOnStartup;

	GUID mguid_Plugin, mguid_Dialogs;
	#if MVV_3>=2103
	GUID mguid_ApiMessage, mguid_ApiMenu, mguid_ApiInput;
	#endif
	int mn_PluginMenu; GUID* mguids_PluginMenu;
	int mn_PluginDisks; GUID* mguids_PluginDisks;
	int mn_PluginConfig; GUID* mguids_PluginConfig;
	Far2::PluginInfo m_Info;
	InfoPanelLine *m_InfoLines; size_t m_InfoLinesNumber; // Far3
	PanelMode     *m_PanelModesArray; size_t m_PanelModesNumber; // Far3
	KeyBarTitles   m_KeyBar; // Far3
	KeyBarLabel    m_KeyBarLabels[7*12];
	Far2::FarList m_ListItems2;
	FarList m_ListItems3;
	WRAP_CHAR_INFO m_FarCharInfo2;
	WRAP_FAR_CHAR_INFO m_FarCharInfo3;
	BYTE m_BufColors[255]; // ����� ��� ����������� DN_CTLCOLORDLGLIST

	unsigned mn_EditorColorPriority;
	bool mb_EditorChanged;

	AnalyseInfo* mp_Analyze;
	int mn_AnalyzeReturn; // -2, ���� OpenFilePluginW ������ (HANDLE)-2
	static wchar_t* gpsz_LastAnalyzeFile; // ���� !=NULL � ��������� � ������ � AnalyseW - ������ �������� ������ ������� FALSE
	wchar_t* mpsz_LastAnalyzeFile;
	//;; 1 - (AnalyzeW [ & OpenW]) -> OpenFilePluginW
	//;; 2 - (AnalyzeW [ & OpenW]) -> OpenFilePluginW & ClosePluginW [& OpenFilePluginW]
	//;; 3 -- ����� PicView. �������� ��� �������� � AnalyseW � ���� �������� ����� ������
	int m_AnalyzeMode;

	wchar_t* mpsz_ViewerFileName;
	size_t mcch_ViewerFileName;

	int m_OldPutFilesParams;

	Far2Dialog* m_LastFar2Dlg;

	std::map<PluginPanelItem*,Far2::PluginPanelItem*> m_MapPanelItems;
	std::map<Far2::FAR_FIND_DATA*,PluginPanelItem*> m_MapDirList;
	std::map<Far2::PluginPanelItem*,PluginPanelItem*> m_MapPlugDirList;

	int gnMsg_2 /*= 0*/, gnParam1_2 /*= 0*/, gnParam1_3 /*= 0*/;
	FARMESSAGE gnMsg_3 /*= DM_FIRST*/;
	FARMESSAGE gnMsgKey_3 /*= DM_FIRST*/, gnMsgClose_3 /*= DM_FIRST*/;
	LONG_PTR gnParam2_2 /*= 0*/;
	void* gpParam2_3 /*= NULL*/;
	FarListItem* gpListItems3 /*= NULL*/; INT_PTR gnListItemsMax3 /*= 0*/;
	Far2::FarListItem* gpListItems2 /*= NULL*/; UINT_PTR gnListItemsMax2 /*= 0*/;
	FarGetDialogItem m_GetDlgItem;

	BOOL lbPsi2 /*= FALSE*/;
	BOOL lbPsi3;
	PluginStartupInfo psi3;
	FarStandardFunctions FSF3;
	Far2::PluginStartupInfo psi2;
	Far2::FarStandardFunctions FSF2;


	Far2::FARAPIADVCONTROL FarApiAdvControlExp;
	Far2::FARAPICMPNAME FarApiCmpNameExp;
	Far2::FARAPICONTROL FarApiControlExp;
	Far2::FARAPIDEFDLGPROC FarApiDefDlgProcExp;
	Far2::FARAPIDIALOGFREE FarApiDialogFreeExp;
	Far2::FARAPIDIALOGINIT FarApiDialogInitExp;
	Far2::FARAPIDIALOGRUN FarApiDialogRunExp;
	Far2::FARAPIEDITOR FarApiEditorExp;
	Far2::FARAPIEDITORCONTROL FarApiEditorControlExp;
	Far2::FARAPIFILEFILTERCONTROL FarApiFileFilterControlExp;
	Far2::FARAPIFREEDIRLIST FarApiFreeDirListExp;
	Far2::FARAPIFREEPLUGINDIRLIST FarApiFreePluginDirListExp;
	Far2::FARAPIGETDIRLIST FarApiGetDirListExp;
	Far2::FARAPIGETMSG FarApiGetMsgExp;
	Far2::FARAPIGETPLUGINDIRLIST FarApiGetPluginDirListExp;
	Far2::FARAPIINPUTBOX FarApiInputBoxExp;
	Far2::FARAPIMENU FarApiMenuExp;
	Far2::FARAPIMESSAGE FarApiMessageExp;
	Far2::FARAPIPLUGINSCONTROL FarApiPluginsControlExp;
	Far2::FARAPIREGEXPCONTROL FarApiRegExpControlExp;
	Far2::FARAPIRESTORESCREEN FarApiRestoreScreenExp;
	Far2::FARAPISAVESCREEN FarApiSaveScreenExp;
	Far2::FARAPISENDDLGMESSAGE FarApiSendDlgMessageExp;
	Far2::FARAPISHOWHELP FarApiShowHelpExp;
	Far2::FARAPITEXT FarApiTextExp;
	Far2::FARAPIVIEWER FarApiViewerExp;
	Far2::FARAPIVIEWERCONTROL FarApiViewerControlExp;
	Far2::FARCONVERTPATH FarConvertPathExp;
	Far2::FARGETCURRENTDIRECTORY FarGetCurrentDirectoryExp;
	Far2::FARGETREPARSEPOINTINFO FarGetReparsePointInfoExp;
	Far2::FARSTDGETFILEOWNER FarStdGetFileOwnerExp;
	Far2::FARSTDGETPATHROOT FarStdGetPathRootExp;
	Far2::FARSTDCOPYTOCLIPBOARD FarStdCopyToClipboardExp;
	Far2::FARSTDPASTEFROMCLIPBOARD FarStdPasteFromClipboardExp;
	Far2::FARSTDMKLINK FarStdMkLinkExp;
	Far2::FARSTDMKTEMP FarStdMkTempExp;
	Far2::FARSTDPROCESSNAME FarStdProcessNameExp;
	Far2::FARSTDGETNUMBEROFLINKS GetNumberOfLinksExp;
	Far2::FARSTDRECURSIVESEARCH FarStdRecursiveSearchExp;
	Far2::FARSTDXLAT FarStdXlatExp;
	Far2::FARSTDQSORT FarStdQSortExp;
	Far2::FARSTDQSORTEX FarStdQSortExExp;
	Far2::FARSTDBSEARCH FarStdBSearchExp;


/* ******************************** */

	// Ctors
	WrapPluginInfo(Far3WrapFunctions *pInfo2);
	~WrapPluginInfo();

	BOOL LoadPlugin(BOOL abSilent);
	void LoadPluginInfo();
	void GetPluginInfoInternal();
	void CheckPluginExports(LoaderFunctions3 eFunc);

	void UnloadPlugin();
	void ClearProcAddr();

	KeyBarTitles* KeyBarTitles_2_3(const Far2::KeyBarTitles* KeyBar);

	static int OpMode_3_2(OPERATION_MODES OpMode3);
	int OpenFrom_3_2(OPENFROM OpenFrom3, INT_PTR Data, INT_PTR& Item);
	static OPENPANELINFO_FLAGS OpenPanelInfoFlags_2_3(DWORD Flags2);
	static Far2::OPENPLUGININFO_SORTMODES SortMode_3_2(OPENPANELINFO_SORTMODES Mode3);
	static OPENPANELINFO_SORTMODES SortMode_2_3(/*Far2::OPENPLUGININFO_SORTMODES*/int Mode2);
	static PLUGINPANELITEMFLAGS PluginPanelItemFlags_2_3(DWORD Flags2);
	static DWORD PluginPanelItemFlags_3_2(PLUGINPANELITEMFLAGS Flags3);
	static void PluginPanelItem_2_3(const Far2::PluginPanelItem* p2, PluginPanelItem* p3);
	PluginPanelItem* PluginPanelItems_2_3(const Far2::PluginPanelItem* pItems, int ItemsNumber);
	static void PluginPanelItem_3_2(const PluginPanelItem* p3, Far2::PluginPanelItem* p2);
	static void PluginPanelItem_3_2(const PluginPanelItem *p3, Far2::FAR_FIND_DATA* p2);
	static void PluginPanelItem_2_3(const Far2::FAR_FIND_DATA* p2, PluginPanelItem *p3);
	Far2::PluginPanelItem* PluginPanelItems_3_2(const PluginPanelItem* pItems, int ItemsNumber);
	static void FarKey_2_3(int Key2, INPUT_RECORD *r);
	static int WINAPI FarKey_3_2(const INPUT_RECORD *Rec);
	static int FarKeyEx_3_2(const INPUT_RECORD *Rec, bool LeftOnly = false);
	#if MVV_3>=2103
	static size_t WINAPI FarKeyToName3(int Key2,wchar_t *KeyText,size_t Size);
	static int WINAPI FarNameToKey3(const wchar_t *Name);
	//static int WINAPI FarInputRecordToKey3(const INPUT_RECORD *rec);
	#endif
	static int FarColorIndex_2_3(int ColorIndex2);
	static int FarColorIndex_3_2(int ColorIndex3);
	static FARDIALOGITEMTYPES DialogItemTypes_2_3(int ItemType2);
	static int DialogItemTypes_3_2(FARDIALOGITEMTYPES ItemType3);
	static void FarColor_2_3(BYTE Color2, FarColor& Color3);
	//static BYTE FarColor_3_2(const FarColor& Color3);
	//static BYTE RefColorToIndex(COLORREF Color);
	static DWORD FarDialogItemFlags_3_2(FARDIALOGITEMFLAGS Flags3);
	static FARDIALOGITEMFLAGS FarDialogItemFlags_2_3(DWORD Flags2);
	void FarListItem_2_3(const Far2::FarListItem* p2, FarListItem* p3);
	void FarListItem_3_2(const FarListItem* p3, Far2::FarListItem* p2);
	void FarDialogItem_2_3(const Far2::FarDialogItem *p2, FarDialogItem *p3, FarList *pList3, WRAP_FAR_CHAR_INFO& pChars3);
	void FarDialogItem_3_2(const FarDialogItem *p3, /*size_t nAllocated3,*/ Far2::FarDialogItem *p2, Far2::FarList *pList2, WRAP_CHAR_INFO& pVBuf2, BOOL bListPos = FALSE);
	LONG_PTR CallDlgProc_2_3(FARAPIDEFDLGPROC DlgProc3, HANDLE hDlg2, const int Msg2, const int Param1, LONG_PTR Param2);
	Far2::FarMessagesProc FarMessage_3_2(const int Msg3, const int Param1, void*& Param2, Far2Dialog* pDlg);
	void FarMessageParam_2_3(const int Msg2, const int Param1, const void* Param2, void* OrgParam2, LONG_PTR lRc);
	InfoPanelLine* InfoLines_2_3(const Far2::InfoPanelLine *InfoLines, int InfoLinesNumber);
	PanelMode* PanelModes_2_3(const Far2::PanelMode *PanelModesArray, int PanelModesNumber);
	static LPCWSTR FormatGuid(const GUID* guid, wchar_t* tmp, BOOL abQuote = FALSE);
	static LPWSTR MacroFromMultiSZ(LPCWSTR aszMultiSZ);
	MacroParseResult* mp_MacroResult; size_t mn_MaxMacroResult;


/* ******************************** */

	static FARPROC WINAPI GetProcAddressWrap(struct WrapPluginInfo* wpi, HMODULE hModule, LPCSTR lpProcName);
	       FARPROC        GetProcAddressW3  (HMODULE hModule, LPCSTR lpProcName);
	static FARPROC WINAPI GetOldProcAddressWrap(struct WrapPluginInfo* wpi, HMODULE hModule, LPCSTR lpProcName);
	       FARPROC        GetOldProcAddressW3  (HMODULE hModule, LPCSTR lpProcName);
	       
/* ******************************** */

	static void   WINAPI SetStartupInfoWrap(struct WrapPluginInfo* wpi, PluginStartupInfo *Info);
	       void          SetStartupInfoW3  (PluginStartupInfo *Info);
	static void   WINAPI GetGlobalInfoWrap(struct WrapPluginInfo* wpi, GlobalInfo *Info);
	       void          GetGlobalInfoW3  (GlobalInfo *Info);
	static void   WINAPI GetPluginInfoWrap(struct WrapPluginInfo* wpi, PluginInfo *Info);
	       void          GetPluginInfoW3  (PluginInfo *Info);
	static HANDLE WINAPI OpenWrap(struct WrapPluginInfo* wpi, const OpenInfo *Info);
	       HANDLE        OpenW3  (const OpenInfo *Info);
		   HANDLE        OpenFilePluginHelper(LPCWSTR asFile);
	static HANDLE WINAPI AnalyseWrap(struct WrapPluginInfo* wpi, const AnalyseInfo *Info);
	       HANDLE        AnalyseW3  (const AnalyseInfo *Info);
	static void   WINAPI CloseAnalyseWrap(struct WrapPluginInfo* wpi, const CloseAnalyseInfo *Info);
	       void          CloseAnalyseW3  (const CloseAnalyseInfo *Info);
	static void   WINAPI ClosePanelWrap(struct WrapPluginInfo* wpi, const struct ClosePanelInfo *Info);
	       void          ClosePanelW3  (const struct ClosePanelInfo *Info);
	static int    WINAPI CompareWrap(struct WrapPluginInfo* wpi, const CompareInfo *Info);
	       int           CompareW3  (const CompareInfo *Info);
	static int    WINAPI ConfigureWrap(struct WrapPluginInfo* wpi, const struct ConfigureInfo *Info);
	       int           ConfigureW3  (const struct ConfigureInfo *Info);
	static int    WINAPI DeleteFilesWrap(struct WrapPluginInfo* wpi, const DeleteFilesInfo *Info);
	       int           DeleteFilesW3  (const DeleteFilesInfo *Info);
	static void   WINAPI ExitFARWrap(struct WrapPluginInfo* wpi, const struct ExitInfo *Info);
	       void          ExitFARW3  (const struct ExitInfo *Info);
	static void   WINAPI FreeVirtualFindDataWrap(struct WrapPluginInfo* wpi, const FreeFindDataInfo *Info);
	       void          FreeVirtualFindDataW3  (const FreeFindDataInfo *Info);
	static int    WINAPI GetFilesWrap(struct WrapPluginInfo* wpi, GetFilesInfo *Info);
	       int           GetFilesW3  (GetFilesInfo *Info);
	static int    WINAPI GetFindDataWrap(struct WrapPluginInfo* wpi, GetFindDataInfo *Info);
	       int           GetFindDataW3  (GetFindDataInfo *Info);
	static void   WINAPI FreeFindDataWrap(struct WrapPluginInfo* wpi, const FreeFindDataInfo *Info);
	       void          FreeFindDataW3  (const FreeFindDataInfo *Info);
	static void   WINAPI GetOpenPanelInfoWrap(struct WrapPluginInfo* wpi, OpenPanelInfo *Info);
	       void          GetOpenPanelInfoW3  (OpenPanelInfo *Info);
    #if MVV_3<=2798
	static int    WINAPI GetVirtualFindDataWrap(struct WrapPluginInfo* wpi, GetVirtualFindDataInfo *Info);
	       int           GetVirtualFindDataW3  (GetVirtualFindDataInfo *Info);
    #endif
	static int    WINAPI MakeDirectoryWrap(struct WrapPluginInfo* wpi, MakeDirectoryInfo *Info);
	       int           MakeDirectoryW3  (MakeDirectoryInfo *Info);
	static int    WINAPI ProcessDialogEventWrap(struct WrapPluginInfo* wpi, const struct ProcessDialogEventInfo *Info);
	       int           ProcessDialogEventW3  (const struct ProcessDialogEventInfo *Info);
	static int    WINAPI ProcessEditorEventWrap(struct WrapPluginInfo* wpi, const struct ProcessEditorEventInfo *Info);
	       int           ProcessEditorEventW3  (const struct ProcessEditorEventInfo *Info);
	static int    WINAPI ProcessEditorInputWrap(struct WrapPluginInfo* wpi, const ProcessEditorInputInfo *Info);
	       int           ProcessEditorInputW3  (const ProcessEditorInputInfo *Info);
	static int    WINAPI ProcessPanelEventWrap(struct WrapPluginInfo* wpi, const struct ProcessPanelEventInfo *Info);
	       int           ProcessPanelEventW3  (const struct ProcessPanelEventInfo *Info);
	static int    WINAPI ProcessHostFileWrap(struct WrapPluginInfo* wpi, const ProcessHostFileInfo *Info);
	       int           ProcessHostFileW3  (const ProcessHostFileInfo *Info);
	static int    WINAPI ProcessPanelInputWrap(struct WrapPluginInfo* wpi, const struct ProcessPanelInputInfo *Info);
	       int           ProcessPanelInputW3  (const struct ProcessPanelInputInfo *Info);
	static int    WINAPI ProcessConsoleInputWrap(struct WrapPluginInfo* wpi, ProcessConsoleInputInfo *Info);
	       int           ProcessConsoleInputW3  (ProcessConsoleInputInfo *Info);
	static int    WINAPI ProcessSynchroEventWrap(struct WrapPluginInfo* wpi, const struct ProcessSynchroEventInfo *Info);
	       int           ProcessSynchroEventW3  (const struct ProcessSynchroEventInfo *Info);
	static int    WINAPI ProcessViewerEventWrap(struct WrapPluginInfo* wpi, const struct ProcessViewerEventInfo *Info);
	       int           ProcessViewerEventW3  (const struct ProcessViewerEventInfo *Info);
	static int    WINAPI PutFilesWrap(struct WrapPluginInfo* wpi, const PutFilesInfo *Info);
	       int           PutFilesW3  (const PutFilesInfo *Info);
	static int    WINAPI SetDirectoryWrap(struct WrapPluginInfo* wpi, const SetDirectoryInfo *Info);
	       int           SetDirectoryW3  (const SetDirectoryInfo *Info);
	static int    WINAPI SetFindListWrap(struct WrapPluginInfo* wpi, const SetFindListInfo *Info);
	       int           SetFindListW3  (const SetFindListInfo *Info);
	static int    WINAPI GetCustomDataWrap(struct WrapPluginInfo* wpi, const wchar_t *FilePath, wchar_t **CustomData);
	       int    WINAPI GetCustomDataW3  (const wchar_t *FilePath, wchar_t **CustomData);
	static void   WINAPI FreeCustomDataWrap(struct WrapPluginInfo* wpi, wchar_t *CustomData);
	       void   WINAPI FreeCustomDataW3  (wchar_t *CustomData);


/* ******************************** */

	// Some internal typedefs
	struct RecSearchUserFnArg
	{
		Far2::FRSUSERFUNC UserFn2;
		void *Param2;
	};
	static int WINAPI RecSearchUserFn(const struct PluginPanelItem *FData, const wchar_t *FullName, void *Param);
	DWORD GetFarSetting(HANDLE h, size_t Root, LPCWSTR Name);
	INT_PTR GetFarSetting(HANDLE h, size_t Root, LPCWSTR Name, wchar_t* Result);
	DWORD GetFarSystemSettings();
	DWORD GetFarPanelSettings();
	DWORD GetFarInterfaceSettings();
	DWORD GetFarConfirmations();
	DWORD GetFarDescSettings();
	DWORD GetFarDialogSettings();

	// Changed functions
	static LONG_PTR WINAPI FarApiDefDlgProcWrap(WrapPluginInfo* wpi, HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
	       LONG_PTR        FarApiDefDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
	static LONG_PTR WINAPI FarApiSendDlgMessageWrap(WrapPluginInfo* wpi, HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
	       LONG_PTR        FarApiSendDlgMessage(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
	static BOOL WINAPI FarApiShowHelpWrap(WrapPluginInfo* wpi, const wchar_t *ModuleName, const wchar_t *Topic, DWORD Flags);
	       BOOL        FarApiShowHelp(const wchar_t *ModuleName, const wchar_t *Topic, DWORD Flags);
	static HANDLE WINAPI FarApiSaveScreenWrap(WrapPluginInfo* wpi, int X1, int Y1, int X2, int Y2);
	       HANDLE        FarApiSaveScreen(int X1, int Y1, int X2, int Y2);
	static void WINAPI FarApiRestoreScreenWrap(WrapPluginInfo* wpi, HANDLE hScreen);
	       void        FarApiRestoreScreen(HANDLE hScreen);
	static void WINAPI FarApiTextWrap(WrapPluginInfo* wpi, int X, int Y, int Color, const wchar_t *Str);
	       void        FarApiText(int X, int Y, int Color, const wchar_t *Str);
	static int WINAPI FarApiEditorWrap(WrapPluginInfo* wpi, const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, int StartLine, int StartChar, UINT CodePage);
	       int        FarApiEditor(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, int StartLine, int StartChar, UINT CodePage);
	static int WINAPI FarApiViewerWrap(WrapPluginInfo* wpi, const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, UINT CodePage);
	       int        FarApiViewer(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, UINT CodePage);
	static int WINAPI FarApiMenuWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags, const wchar_t* Title, const wchar_t* Bottom, const wchar_t* HelpTopic, const int* BreakKeys, int* BreakCode, const struct Far2::FarMenuItem *Item, int ItemsNumber);
	       int        FarApiMenu(INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags, const wchar_t* Title, const wchar_t* Bottom, const wchar_t* HelpTopic, const int* BreakKeys, int* BreakCode, const struct Far2::FarMenuItem *Item, int ItemsNumber);
	static int WINAPI FarApiMessageWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic, const wchar_t * const *Items, int ItemsNumber, int ButtonsNumber);
	       int        FarApiMessage(INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic, const wchar_t * const *Items, int ItemsNumber, int ButtonsNumber);
	static HANDLE WINAPI FarApiDialogInitWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const wchar_t* HelpTopic, struct Far2::FarDialogItem *Item, unsigned int ItemsNumber, DWORD Reserved, DWORD Flags, Far2::FARWINDOWPROC DlgProc, LONG_PTR Param);
	       HANDLE        FarApiDialogInit(INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const wchar_t* HelpTopic, struct Far2::FarDialogItem *Item, unsigned int ItemsNumber, DWORD Reserved, DWORD Flags, Far2::FARWINDOWPROC DlgProc, LONG_PTR Param);
	static int WINAPI FarApiDialogRunWrap(WrapPluginInfo* wpi, HANDLE hDlg);
	       int        FarApiDialogRun(HANDLE hDlg);
	static void WINAPI FarApiDialogFreeWrap(WrapPluginInfo* wpi, HANDLE hDlg);
	       void        FarApiDialogFree(HANDLE hDlg);
	static int WINAPI FarApiControlWrap(WrapPluginInfo* wpi, HANDLE hPlugin, int Command, int Param1, LONG_PTR Param2);
	       int        FarApiControl(HANDLE hPlugin, int Command, int Param1, LONG_PTR Param2);
	static int WINAPI FarApiGetDirListWrap(WrapPluginInfo* wpi, const wchar_t *Dir, struct Far2::FAR_FIND_DATA **pPanelItem, int *pItemsNumber);
	       int        FarApiGetDirList(const wchar_t *Dir, struct Far2::FAR_FIND_DATA **pPanelItem, int *pItemsNumber);
	static void WINAPI FarApiFreeDirListWrap(WrapPluginInfo* wpi, struct Far2::FAR_FIND_DATA *PanelItem, int nItemsNumber);
	       void        FarApiFreeDirList(struct Far2::FAR_FIND_DATA *PanelItem, int nItemsNumber);
	static int WINAPI FarApiGetPluginDirListWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, HANDLE hPlugin, const wchar_t *Dir, struct Far2::PluginPanelItem **pPanelItem, int *pItemsNumber);
	       int        FarApiGetPluginDirList(INT_PTR PluginNumber, HANDLE hPlugin, const wchar_t *Dir, struct Far2::PluginPanelItem **pPanelItem, int *pItemsNumber);
	static void WINAPI FarApiFreePluginDirListWrap(WrapPluginInfo* wpi, struct Far2::PluginPanelItem *PanelItem, int nItemsNumber);
	       void        FarApiFreePluginDirList(struct Far2::PluginPanelItem *PanelItem, int nItemsNumber);
	static int WINAPI FarApiCmpNameWrap(WrapPluginInfo* wpi, const wchar_t *Pattern, const wchar_t *String, int SkipPath);
	       int        FarApiCmpName(const wchar_t *Pattern, const wchar_t *String, int SkipPath);
	static LPCWSTR WINAPI FarApiGetMsgWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, int MsgId);
	       LPCWSTR        FarApiGetMsg(INT_PTR PluginNumber, int MsgId);
	static INT_PTR WINAPI FarApiAdvControlWrap(WrapPluginInfo* wpi, INT_PTR ModuleNumber, int Command, void *Param);
	       INT_PTR        FarApiAdvControl(INT_PTR ModuleNumber, int Command, void *Param);
	static int WINAPI FarApiViewerControlWrap(WrapPluginInfo* wpi, int Command, void *Param);
	       int        FarApiViewerControl(int Command, void *Param);
	static int WINAPI FarApiEditorControlWrap(WrapPluginInfo* wpi, int Command, void *Param);
	       int        FarApiEditorControl(int Command, void *Param);
	static int WINAPI FarApiInputBoxWrap(WrapPluginInfo* wpi, const wchar_t *Title, const wchar_t *SubTitle, const wchar_t *HistoryName, const wchar_t *SrcText, wchar_t *DestText, int   DestLength, const wchar_t *HelpTopic, DWORD Flags);
	       int        FarApiInputBox(const wchar_t *Title, const wchar_t *SubTitle, const wchar_t *HistoryName, const wchar_t *SrcText, wchar_t *DestText, int   DestLength, const wchar_t *HelpTopic, DWORD Flags);
	static int WINAPI FarApiPluginsControlWrap(WrapPluginInfo* wpi, HANDLE hHandle, int Command, int Param1, LONG_PTR Param2);
	       int        FarApiPluginsControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2);
	static int WINAPI FarApiFileFilterControlWrap(WrapPluginInfo* wpi, HANDLE hHandle, int Command, int Param1, LONG_PTR Param2);
	       int        FarApiFileFilterControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2);
	static int WINAPI FarApiRegExpControlWrap(WrapPluginInfo* wpi, HANDLE hHandle, int Command, LONG_PTR Param);
	       int        FarApiRegExpControl(HANDLE hHandle, int Command, LONG_PTR Param);
	static int WINAPI FarStdGetFileOwnerWrap(WrapPluginInfo* wpi, const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,int Size);
	       int        FarStdGetFileOwner(const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,int Size);
	static int WINAPI FarStdGetPathRootWrap(WrapPluginInfo* wpi, const wchar_t *Path,wchar_t *Root, int DestSize);
	       int        FarStdGetPathRoot(const wchar_t *Path,wchar_t *Root, int DestSize);
	static int WINAPI FarStdCopyToClipboardWrap(WrapPluginInfo* wpi, const wchar_t *Data);
	       int        FarStdCopyToClipboard(const wchar_t *Data);
	#if MVV_3>2798
	static void WINAPI DeleteBufferWrap(void *Buffer);
	#endif
	static wchar_t* WINAPI FarStdPasteFromClipboardWrap(WrapPluginInfo* wpi);
	       wchar_t*   FarStdPasteFromClipboard();
	static wchar_t* WINAPI FarStdXlatWrap(WrapPluginInfo* wpi, wchar_t *Line,int StartPos,int EndPos,DWORD Flags);
	       wchar_t*        FarStdXlatW3(wchar_t *Line,int StartPos,int EndPos,DWORD Flags);
	static int WINAPI GetNumberOfLinksWrap(WrapPluginInfo* wpi, const wchar_t *Name);
	       int        GetNumberOfLinks(const wchar_t *Name);
	static void WINAPI FarStdRecursiveSearchWrap(WrapPluginInfo* wpi, const wchar_t *InitDir,const wchar_t *Mask,Far2::FRSUSERFUNC Func,DWORD Flags,void *Param);
	       void        FarStdRecursiveSearch(const wchar_t *InitDir,const wchar_t *Mask,Far2::FRSUSERFUNC Func,DWORD Flags,void *Param);
	static int WINAPI FarStdMkTempWrap(WrapPluginInfo* wpi, wchar_t *Dest, DWORD size, const wchar_t *Prefix);
	       int        FarStdMkTemp(wchar_t *Dest, DWORD size, const wchar_t *Prefix);
	static int WINAPI FarStdProcessNameWrap(WrapPluginInfo* wpi, const wchar_t *param1, wchar_t *param2, DWORD size, DWORD flags);
	       int        FarStdProcessName(const wchar_t *param1, wchar_t *param2, DWORD size, DWORD flags);
	static int WINAPI FarStdMkLinkWrap(WrapPluginInfo* wpi, const wchar_t *Src,const wchar_t *Dest,DWORD Flags);
	       int        FarStdMkLink(const wchar_t *Src,const wchar_t *Dest,DWORD Flags);
	static int WINAPI FarConvertPathWrap(WrapPluginInfo* wpi, enum Far2::CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize);
	       int        FarConvertPath(enum Far2::CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize);
	static int WINAPI FarGetReparsePointInfoWrap(WrapPluginInfo* wpi, const wchar_t *Src, wchar_t *Dest,int DestSize);
	       int        FarGetReparsePointInfo(const wchar_t *Src, wchar_t *Dest,int DestSize);
	static DWORD WINAPI FarGetCurrentDirectoryWrap(WrapPluginInfo* wpi, DWORD Size,wchar_t* Buffer);
	       DWORD        FarGetCurrentDirectory(DWORD Size,wchar_t* Buffer);
	static void WINAPI FarStdQSortWrap(WrapPluginInfo* wpi, void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
	       void        FarStdQSort(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
	static void WINAPI FarStdQSortExWrap(WrapPluginInfo* wpi, void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam);
	       void        FarStdQSortEx(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam);
	static void* WINAPI FarStdBSearchWrap(WrapPluginInfo* wpi, const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
	       void*        FarStdBSearch(const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));


/* ******************************** */

	BOOL mb_ExportsLoaded;
	// Exported Far2(!) Function
	typedef int    (WINAPI* _AnalyseW)(const struct Far2::AnalyseData *Info);
	_AnalyseW AnalyseW;
	typedef void   (WINAPI* _ClosePluginW)(HANDLE hPlugin);
	_ClosePluginW ClosePluginW;
	typedef int    (WINAPI* _CompareW)(HANDLE hPlugin,const Far2::PluginPanelItem *Item1,const Far2::PluginPanelItem *Item2,unsigned int Mode);
	_CompareW CompareW;
	typedef int    (WINAPI* _ConfigureW)(int ItemNumber);
	_ConfigureW ConfigureW;
	typedef int    (WINAPI* _DeleteFilesW)(HANDLE hPlugin,Far2::PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	_DeleteFilesW DeleteFilesW;
	typedef void   (WINAPI* _ExitFARW)(void);
	typedef void   (WINAPI* _ExitFARW3)(void*);
	_ExitFARW ExitFARW;
	typedef void   (WINAPI* _FreeFindDataW)(HANDLE hPlugin,Far2::PluginPanelItem *PanelItem,int ItemsNumber);
	_FreeFindDataW FreeFindDataW;
	typedef void   (WINAPI* _FreeVirtualFindDataW)(HANDLE hPlugin,Far2::PluginPanelItem *PanelItem,int ItemsNumber);
	_FreeVirtualFindDataW FreeVirtualFindDataW;
	typedef int    (WINAPI* _GetFilesW)(HANDLE hPlugin,Far2::PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,int OpMode);
	_GetFilesW GetFilesW;
	typedef int    (WINAPI* _GetFindDataW)(HANDLE hPlugin,Far2::PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
	_GetFindDataW GetFindDataW;
	typedef void   (WINAPI* _GetGlobalInfoW)(struct GlobalInfo *Info);
	_GetGlobalInfoW GetGlobalInfoWPlugin;
	typedef int    (WINAPI* _GetMinFarVersionW)(void);
	_GetMinFarVersionW GetMinFarVersionW;
	typedef void   (WINAPI* _GetOpenPluginInfoW)(HANDLE hPlugin,Far2::OpenPluginInfo *Info);
	_GetOpenPluginInfoW GetOpenPluginInfoW;
	typedef void   (WINAPI* _GetPluginInfoW)(Far2::PluginInfo *Info);
	_GetPluginInfoW GetPluginInfoW;
	typedef int    (WINAPI* _GetVirtualFindDataW)(HANDLE hPlugin,Far2::PluginPanelItem **pPanelItem,int *pItemsNumber,const wchar_t *Path);
	_GetVirtualFindDataW GetVirtualFindDataW;
	typedef int    (WINAPI* _MakeDirectoryW)(HANDLE hPlugin,const wchar_t **Name,int OpMode);
	_MakeDirectoryW MakeDirectoryW;
	typedef HANDLE (WINAPI* _OpenFilePluginW)(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode);
	_OpenFilePluginW OpenFilePluginW;
	typedef HANDLE (WINAPI* _OpenPluginW)(int OpenFrom,INT_PTR Item);
	_OpenPluginW OpenPluginW;
	typedef int    (WINAPI* _ProcessDialogEventW)(int Event,void *Param);
	_ProcessDialogEventW ProcessDialogEventW;
	typedef int    (WINAPI* _ProcessEditorEventW)(int Event,void *Param);
	_ProcessEditorEventW ProcessEditorEventW;
	typedef int    (WINAPI* _ProcessEditorInputW)(const INPUT_RECORD *Rec);
	_ProcessEditorInputW ProcessEditorInputW;
	typedef int    (WINAPI* _ProcessEventW)(HANDLE hPlugin,int Event,void *Param);
	_ProcessEventW ProcessEventW;
	typedef int    (WINAPI* _ProcessHostFileW)(HANDLE hPlugin,Far2::PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	_ProcessHostFileW ProcessHostFileW;
	typedef int    (WINAPI* _ProcessKeyW)(HANDLE hPlugin,int Key,unsigned int ControlState);
	_ProcessKeyW ProcessKeyW;
	typedef int    (WINAPI* _ProcessSynchroEventW)(int Event,void *Param);
	_ProcessSynchroEventW ProcessSynchroEventW;
	typedef int    (WINAPI* _ProcessViewerEventW)(int Event,void *Param);
	_ProcessViewerEventW ProcessViewerEventW;
	typedef int    (WINAPI* _PutFilesW)(HANDLE hPlugin,Far2::PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *SrcPath,int OpMode);
	typedef int    (WINAPI* _PutFilesOldW)(HANDLE hPlugin,Far2::PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode);
	_PutFilesW PutFilesW;
	typedef int    (WINAPI* _SetDirectoryW)(HANDLE hPlugin,const wchar_t *Dir,int OpMode);
	_SetDirectoryW SetDirectoryW;
	typedef int    (WINAPI* _SetFindListW)(HANDLE hPlugin,const Far2::PluginPanelItem *PanelItem,int ItemsNumber);
	_SetFindListW SetFindListW;
	typedef void   (WINAPI* _SetStartupInfoW)(const Far2::PluginStartupInfo *Info);
	_SetStartupInfoW SetStartupInfoW;
	typedef int (WINAPI* _GetCustomDataW)(const wchar_t *FilePath, wchar_t **CustomData);
	_GetCustomDataW GetCustomDataW;
	typedef void (WINAPI* _FreeCustomDataW)(wchar_t *CustomData);
	_FreeCustomDataW FreeCustomDataW;
	// End of Exported Far2(!) Function
};
//WrapPluginInfo* wpi = NULL;

void dbgLoadLibrary(LPCWSTR asFormat, LPCWSTR asModule)
{
#ifdef SHOWLOADLIBRARY
	if (!asFormat)
	{
		_ASSERTE(asFormat!=NULL);
		return;
	}
	int nLen = lstrlen(asFormat) + (asModule ? lstrlen(asModule) : 16) + 4;
	wchar_t* pszOut = (wchar_t*)malloc(nLen*sizeof(wchar_t));
	if (pszOut)
	{
		wsprintf(pszOut, asFormat, asModule ? asModule : L"");
		nLen = lstrlen(pszOut);
		if (nLen > 1 && pszOut[nLen-1] != L'\n')
		{
			pszOut[nLen++] = L'\n'; pszOut[nLen] = 0;
		}
		DebugStr(pszOut);
		free(pszOut);
	}
#endif
}

void dbgFreeLibrary(LPCWSTR asFrom, HMODULE hLib, LPCWSTR asModule)
{
#ifdef SHOWLOADLIBRARY
	LPCWSTR pszFormat = 
		#ifdef _WIN64
			L"Far3Wrap.FreeLibrary.%s(x%08X%08X, %s)\n"
		#else
			L"Far3Wrap.FreeLibrary.%s(x%08X, %s)\n"
		#endif
			;

	int nLen = lstrlen(pszFormat) + (asFrom ? lstrlen(asFrom) : 4) + (asModule ? lstrlen(asModule) : 16) + 32;
	wchar_t* pszOut = (wchar_t*)malloc(nLen*sizeof(wchar_t));
	if (pszOut)
	{
		#ifdef _WIN64
			wsprintf(pszOut, pszFormat, asFrom ? asFrom : L"???", (DWORD)(((DWORD_PTR)hLib) >> 32), (DWORD)(DWORD_PTR)hLib, asModule ? asModule : L"<NULL>");
		#else
			wsprintf(pszOut, pszFormat, asFrom ? asFrom : L"???", (DWORD)hLib, asModule ? asModule : L"<NULL>");
		#endif
		DebugStr(pszOut);
		free(pszOut);
	}
#endif
}

std::map<DWORD,WrapPluginInfo*> *gpMapSysID = NULL;

//static int WrapPluginInfo::mn_AnalyzeReturn = 0; // -2, ���� OpenFilePluginW ������ (HANDLE)-2
wchar_t* WrapPluginInfo::gpsz_LastAnalyzeFile = NULL; // ���� !=NULL � ��������� � ������ � AnalyseW - ������ �������� ������ ������� FALSE

WrapPluginInfo::WrapPluginInfo(Far3WrapFunctions *pInfo2)
{
#ifdef _DEBUG
	LPCWSTR pszMSZ = L"Line 1\0Line Line Line Line 2\0Line --- 3\0";
	wchar_t* pszDeMsz = MacroFromMultiSZ(pszMSZ);
	if (pszDeMsz)
		free(pszDeMsz);
#endif

	mh_Loader = pInfo2->hLoader;
	mh_Dll = NULL; ms_DllFailFunc[0] = ms_DllFailTitle[0] = 0; mn_DllFailCode = 0;
	m_MinFarVersion = 0;
	mn_PluginFunctions = mn_LoaderFunctions = LF3_None;
	ms_PluginDll[0] = ms_IniFile[0] = ms_Title[0] = ms_Desc[0] = ms_Author[0] = 0;
	ms_RegRoot[0] = ms_File[0] = ms_ForcePrefix[0] = 0;
	mn_FakeFarVersion = 0;
	ZeroStruct(m_Version);
	ZeroStruct(mguid_Plugin);
	ZeroStruct(mguid_Dialogs);
	#if MVV_3>=2103
	ZeroStruct(mguid_ApiMessage);
	ZeroStruct(mguid_ApiMenu);
	ZeroStruct(mguid_ApiInput);
	#endif
	mn_PluginMenu = mn_PluginDisks = mn_PluginConfig = 0;
	mguids_PluginMenu = mguids_PluginDisks = mguids_PluginConfig = NULL;
	ZeroStruct(m_Info);
	m_InfoLines = NULL; m_InfoLinesNumber = 0;
	m_PanelModesArray = NULL; m_PanelModesNumber = 0;
	m_KeyBar.CountLabels = 0; m_KeyBar.Labels = m_KeyBarLabels;
	m_LastFar2Dlg = NULL;
	m_OldPutFilesParams = 0;
	m_AnalyzeMode = 2;
	ZeroStruct(m_ListItems2); ZeroStruct(m_ListItems3);
	ZeroStruct(m_GetDlgItem);
	m_GetDlgItem.StructSize = sizeof(m_GetDlgItem);
	ZeroStruct(m_FarCharInfo2);
	ZeroStruct(m_FarCharInfo3);
	mn_EditorColorPriority = 0;
	mb_EditorChanged = false;

	mpsz_ViewerFileName = NULL;
	mcch_ViewerFileName = 0;

	mp_MacroResult = NULL; mn_MaxMacroResult = 0;

	gnMsg_2 = 0; gnParam1_2 = 0; gnParam1_3 = 0;
	gnMsg_3 = DM_FIRST;
	gnMsgKey_3 = DM_FIRST; gnMsgClose_3 = DM_FIRST;
	gnParam2_2 = 0;
	gpParam2_3 = NULL;
	gpListItems3 = NULL; gnListItemsMax3 = 0;
	gpListItems2 = NULL; gnListItemsMax2 = 0;

	lbPsi2 = FALSE;
	lbPsi3 = FALSE;
	ZeroStruct(psi3);
	ZeroStruct(FSF3);
	ZeroStruct(psi2);
	ZeroStruct(FSF2);

	ClearProcAddr();
	mp_Analyze = NULL;
	mn_AnalyzeReturn = 0;
	mpsz_LastAnalyzeFile = NULL;

	m_ErrorInfo = pInfo2->ErrorInfo;
	m_ErrorInfoMax = pInfo2->ErrorInfoMax;

	#undef SET_FN
	#define SET_FN(n) n##Exp = pInfo2->n
	SET_FN(FarApiAdvControl);
	SET_FN(FarApiCmpName);
	SET_FN(FarApiControl);
	SET_FN(FarApiDefDlgProc);
	SET_FN(FarApiDialogFree);
	SET_FN(FarApiDialogInit);
	SET_FN(FarApiDialogRun);
	SET_FN(FarApiEditor);
	SET_FN(FarApiEditorControl);
	SET_FN(FarApiFileFilterControl);
	SET_FN(FarApiFreeDirList);
	SET_FN(FarApiFreePluginDirList);
	SET_FN(FarApiGetDirList);
	SET_FN(FarApiGetMsg);
	SET_FN(FarApiGetPluginDirList);
	SET_FN(FarApiInputBox);
	SET_FN(FarApiMenu);
	SET_FN(FarApiMessage);
	SET_FN(FarApiPluginsControl);
	SET_FN(FarApiRegExpControl);
	SET_FN(FarApiRestoreScreen);
	SET_FN(FarApiSaveScreen);
	SET_FN(FarApiSendDlgMessage);
	SET_FN(FarApiShowHelp);
	SET_FN(FarApiText);
	SET_FN(FarApiViewer);
	SET_FN(FarApiViewerControl);
	SET_FN(FarConvertPath);
	SET_FN(FarGetCurrentDirectory);
	SET_FN(FarGetReparsePointInfo);
	SET_FN(FarStdGetFileOwner);
	SET_FN(FarStdGetPathRoot);
	SET_FN(FarStdCopyToClipboard);
	SET_FN(FarStdPasteFromClipboard);
	SET_FN(FarStdMkLink);
	SET_FN(FarStdMkTemp);
	SET_FN(FarStdProcessName);
	SET_FN(GetNumberOfLinks);
	SET_FN(FarStdRecursiveSearch);
	SET_FN(FarStdXlat);
	SET_FN(FarStdQSort);
	SET_FN(FarStdQSortEx);
	SET_FN(FarStdBSearch);
	#undef SET_FN
};

WrapPluginInfo::~WrapPluginInfo()
{
	if (m_Info.Reserved)
		(*gpMapSysID).erase(m_Info.Reserved);
	if (m_ListItems2.Items)
	{
		free(m_ListItems2.Items);
		m_ListItems2.Items = NULL;
	}
	if (m_ListItems3.Items)
	{
		free(m_ListItems3.Items);
		m_ListItems3.Items = NULL;
	}
	if (m_FarCharInfo2.p2)
	{
		if (!m_FarCharInfo2.isExt)
			free(m_FarCharInfo2.p2);
		m_FarCharInfo2.p2 = NULL;
	}
	if (m_FarCharInfo3.p)
	{
		free(m_FarCharInfo3.p);
		m_FarCharInfo3.p = NULL;
	}
	if (mguids_PluginMenu)
		free(mguids_PluginMenu);
	if (mguids_PluginDisks)
		free(mguids_PluginDisks);
	if (mguids_PluginConfig)
		free(mguids_PluginConfig);
	if (mp_Analyze)
		free(mp_Analyze);
	if (m_InfoLines)
		free(m_InfoLines);
	if (m_PanelModesArray)
		free(m_PanelModesArray);
	_ASSERTE(m_KeyBar.Labels==m_KeyBarLabels);
	if (m_GetDlgItem.Item)
		free(m_GetDlgItem.Item);
	if (mpsz_LastAnalyzeFile && mpsz_LastAnalyzeFile == gpsz_LastAnalyzeFile)
	{
		free(gpsz_LastAnalyzeFile);
		gpsz_LastAnalyzeFile = NULL;
	}
	if (mp_MacroResult)
	{
		free(mp_MacroResult);
		mp_MacroResult = NULL;
	}
}

void WrapPluginInfo::LoadPluginInfo()
{
	BOOL lbRc = FALSE;
	wchar_t szSelf[MAX_PATH+1]; szSelf[0] = 0;
	wchar_t szIni[MAX_PATH+1], szTemp[2048];
	
	if (GetModuleFileName(mh_Loader, szSelf, ARRAYSIZE(szSelf)-4))
	{
		GUID tmpGuid = {0}; wchar_t szTmp[64];
		HANDLE hIniFile = NULL;
		DWORD nIniFileError = 0;
		BOOL lbNewIniFile = FALSE;
		WIN32_FIND_DATA fnd;
		HMODULE hTestDll = NULL;
		DWORD nTestDllError = 0;
		FARPROC lpSetStartupInfoW = NULL, lpGetGlobalInfoW = NULL; // ������ ��� ����������, ����� �� ������
		HANDLE hFind = NULL;
		
		
		lstrcpy(szIni, szSelf);
		wchar_t* pszSelfName = wcsrchr(szSelf, L'\\');
		if (pszSelfName) pszSelfName++; else pszSelfName = szSelf;
		wchar_t* pszSlash = wcsrchr(szIni, L'\\');
		wchar_t* pszFilePtr = NULL;
		if (!pszSlash) pszSlash = szIni;
		wchar_t* pszDot = wcsrchr(pszSlash, L'.');
		if (pszDot)
			*pszDot = 0;
		lstrcat(szIni, L".ini");
		lstrcpy(ms_IniFile, szIni);

		
		lstrcpy(ms_PluginDll, szSelf);
		pszSlash = wcsrchr(ms_PluginDll, L'\\');
		//if (pszSlash) pszSlash++; else pszSlash = PluginDll;
		pszFilePtr = pszSlash ? (pszSlash+1) : ms_PluginDll;
		hIniFile = CreateFile(szIni, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hIniFile == INVALID_HANDLE_VALUE)
		{
			hIniFile = CreateFile(szIni, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hIniFile != INVALID_HANDLE_VALUE)
			{
				nIniFileError = GetLastError();
				CloseHandle(hIniFile);
				// OK, �������, ������ ����� ������������ ���������� �� ������� �������
				lbNewIniFile = TRUE;
				//if (pszSlash) *pszSlash = 0;
				//FSF3.FarRecursiveSearch(PluginDll, L"*.dll", WrapPluginInfo::SeekForPlugin,
				//	(FRSMODE)0, this);
				//if (pszSlash) *pszSlash = L'\\';
			}
		}
		else
		{
			CloseHandle(hIniFile);
		}

		if (!lbNewIniFile)
		{
			if (!GetPrivateProfileString(L"Plugin", L"PluginFile", L"", pszFilePtr, ARRAYSIZE(ms_PluginDll)-lstrlen(ms_PluginDll), szIni))
			{
				//PluginDll[0] = 0;
				lbNewIniFile = TRUE;
			}
			else
				lstrcpyn(ms_File, pszFilePtr, ARRAYSIZE(ms_File));
		}

		if (lbNewIniFile)
		{
			_ASSERTE(mh_Dll == NULL);
			//*pszFilePtr = 0;
			//SetDllDirectory(ms_PluginDll); // ����/�� ����?
			for (int i = 0; !hTestDll && i <= 1; i++)
			{
				lstrcpy(pszFilePtr, i ? L"*.dl_" : L"*.dll");
				hFind = FindFirstFile(ms_PluginDll, &fnd);
				if (hFind && (hFind != INVALID_HANDLE_VALUE))
				{
					*pszFilePtr = 0;
					int nLen = ARRAYSIZE(ms_PluginDll)-lstrlen(ms_PluginDll);
					do {
						if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
							continue;
						if (lstrcmpi(pszSelfName, fnd.cFileName) == 0)
							continue; // ��� ��
						lstrcpy(pszFilePtr, fnd.cFileName);
						dbgLoadLibrary(L"Far3Wrap.LoadLibraryEx.Find(%s)", ms_PluginDll);
						hTestDll = LoadLibraryEx(ms_PluginDll, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
						if (hTestDll)
						{
							lpSetStartupInfoW = GetProcAddress(hTestDll, "SetStartupInfoW");
							lpGetGlobalInfoW = GetProcAddress(hTestDll, "GetGlobalInfoW");
							if (lpSetStartupInfoW && !lpGetGlobalInfoW)
							{
								mh_Dll = hTestDll; // ����� ������ ��� �������, �������� ��������� �� �����
								break; // ��, ��� Far2 ������!
							}
							if (GetPrivateProfileInt(L"Plugin", L"DoNotFreeLibrary", 0, szIni) == 0)
							{
								dbgFreeLibrary(L"LoadPluginInfo", hTestDll, ms_PluginDll);
								FreeLibrary(hTestDll);
							}
							else
							{
								dbgFreeLibrary(L"Skip.LoadPluginInfo", hTestDll, ms_PluginDll);
							}
							hTestDll = NULL; // ���������� �����
						}
						else
						{
							nTestDllError = GetLastError();
						}
					} while (FindNextFile(hFind, &fnd));
					FindClose(hFind);
				}
			}

			if (hTestDll)
			{
				lstrcpyn(ms_File, pszFilePtr, ARRAYSIZE(ms_File));
				// OK, �������������� .ini ����
				DWORD dwErr = 0;
				BOOL lb = WritePrivateProfileString(L"Plugin", L"PluginFile", pszFilePtr, szIni);
				if (!lb)
					dwErr = GetLastError();
				//lb = WritePrivateProfileString(L"Plugin", L"DisabledFunctions", L"0", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"MinFarVersion", L"0x00000000", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"AnalyzeMode", L"2", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"OldPutFilesParams", L"0", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"EditorColorPriority", L"0", szIni);
				wchar_t* pszDot = wcsrchr(pszFilePtr, L'.'); if (pszDot) *pszDot = 0;
				lb = WritePrivateProfileString(L"Plugin", L"Title", pszFilePtr, szIni);
				if (pszDot) *pszDot = L'.';
				lb = WritePrivateProfileString(L"Plugin", L"Description", DEFAULT_DESCRIPTION, szIni);
				lb = WritePrivateProfileString(L"Plugin", L"Author", L"<Unknown>", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"RegRoot", L"Software\\Far Manager\\Plugins", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"Version", L"1.0.0.0", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"ForcePrefix", L"", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"FakeFarVersion", L"", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"DoNotFreeLibrary", L"0", szIni);
				lb = WritePrivateProfileString(L"Plugin", L"LoadOnStartup", L"0", szIni);
				// ��������� GUID-�
				UuidCreate(&tmpGuid);
				lb = WritePrivateProfileString(L"Plugin", L"GUID", FormatGuid(&tmpGuid, szTmp), szIni);
				UuidCreate(&tmpGuid);
				lb = WritePrivateProfileString(L"Plugin", L"DialogsGUID", FormatGuid(&tmpGuid, szTmp), szIni);
			}
			else
			{
				ms_File[0] = 0;
			}
		}

		if (hIniFile && (hIniFile != INVALID_HANDLE_VALUE))
		{
			//if (!GetPrivateProfileString(L"Plugin", L"PluginFile", L"", pszFilePtr, ARRAYSIZE(PluginDll)-lstrlen(PluginDll), szIni))
			//	PluginDll[0] = 0;
			//else
			//	lstrcpyn(File, pszFilePtr, ARRAYSIZE(File));

			//mn_OldAbsentFunctions = GetPrivateProfileInt(L"Plugin", L"DisabledFunctions", 0, szIni);

			m_AnalyzeMode = GetPrivateProfileInt(L"Plugin", L"AnalyzeMode", 2, szIni);
			if (m_AnalyzeMode != 1 && m_AnalyzeMode != 2 && m_AnalyzeMode != 3)
				m_AnalyzeMode = 2;

			mn_EditorColorPriority = GetPrivateProfileInt(L"Plugin", L"EditorColorPriority", 0, szIni);
			//if (mn_EditorColorPriority < 0)
			//{
			//	if (ms_File[0] >= L'a' && ms_File[0] <= L'z')
			//		mn_EditorColorPriority = ms_File[0] - L'a';
			//	else if (ms_File[0] >= 32)
			//		mn_EditorColorPriority = ms_File[0] - 32;
			//	else
			//		mn_EditorColorPriority = 0;
			//	wsprintf(szTemp, L"%i", mn_EditorColorPriority);
			//	WritePrivateProfileString(L"Plugin", L"EditorColorPriority", szTemp, szIni);
			//}

			m_OldPutFilesParams = GetPrivateProfileInt(L"Plugin", L"OldPutFilesParams", 0, szIni);
			if (m_OldPutFilesParams != 0 && m_OldPutFilesParams != 1)
				m_OldPutFilesParams = 0;

			if (GetPrivateProfileString(L"Plugin", L"MinFarVersion", L"0", szTemp, ARRAYSIZE(szTemp), szIni)
				&& szTemp[0] == L'0' && szTemp[1] == L'x')
			{
				wchar_t* pszEnd = NULL;
				m_MinFarVersion = wcstoul(szTemp+2, &pszEnd, 16);
			}
			
			if (GetPrivateProfileString(L"Plugin", L"Title", L"Sample Far3 plugin", szTemp, ARRAYSIZE(szTemp), szIni))
				lstrcpyn(ms_Title, szTemp, ARRAYSIZE(ms_Title));
			if (GetPrivateProfileString(L"Plugin", L"Description", DEFAULT_DESCRIPTION/*L"Far2->Far3 plugin wrapper"*/, szTemp, ARRAYSIZE(szTemp), szIni))
				lstrcpyn(ms_Desc, szTemp, ARRAYSIZE(ms_Desc));
			if (GetPrivateProfileString(L"Plugin", L"Author", L"Maximus5", szTemp, ARRAYSIZE(szTemp), szIni))
				lstrcpyn(ms_Author, szTemp, ARRAYSIZE(ms_Author));
			if (GetPrivateProfileString(L"Plugin", L"RegRoot", L"Software\\Far Manager\\Plugins", szTemp, ARRAYSIZE(szTemp), szIni))
				lstrcpyn(ms_RegRoot, szTemp, ARRAYSIZE(ms_RegRoot));
			if (GetPrivateProfileString(L"Plugin", L"ForcePrefix", L"", szTemp, ARRAYSIZE(szTemp), szIni))
				lstrcpyn(ms_ForcePrefix, szTemp, ARRAYSIZE(ms_ForcePrefix));
			if (GetPrivateProfileString(L"Plugin", L"FakeFarVersion", L"", szTemp, ARRAYSIZE(szTemp), szIni))
			{
				mn_FakeFarVersion = 0;
				int nMajor = 0, nMinor = 0, nBuild = 0;
				wchar_t* pszEnd = NULL;
				nMajor = wcstol(szTemp, &pszEnd, 10);
				if ((nMajor > 0) && pszEnd && *pszEnd == L'.')
				{
					nMinor = wcstol(pszEnd+1, &pszEnd, 10);
					if (pszEnd && *pszEnd == L'.')
					{
						nBuild = wcstol(pszEnd+1, &pszEnd, 10);
					}
				}
				if ((nMajor == 1 || nMajor == 2) && (nMinor >= 0) && (nBuild >= 0))
					mn_FakeFarVersion = MAKEFARVERSION2(nMajor, nMinor, nBuild);
			}
			mn_DoNotFreeLibrary = GetPrivateProfileInt(L"Plugin", L"DoNotFreeLibrary", 0, szIni);
			mn_LoadOnStartup = GetPrivateProfileInt(L"Plugin", L"LoadOnStartup", 0, szIni);
			if (GetPrivateProfileString(L"Plugin", L"Version", L"1.0.0.0", szTemp, ARRAYSIZE(szTemp), szIni))
			{
				ZeroStruct(m_Version);
				int nMajor = 0, nMinor = 0, nRevision = 0, nBuild = 0, nStage = 0;
				wchar_t* pszEnd = NULL;
				nMajor = wcstol(szTemp, &pszEnd, 10);
				if (pszEnd && *pszEnd == L'.')
				{
					nMinor = wcstol(pszEnd+1, &pszEnd, 10);
					if (pszEnd && *pszEnd == L'.')
					{
						nRevision = wcstol(pszEnd+1, &pszEnd, 10);
						if (pszEnd && *pszEnd == L'.')
						{
							nBuild = wcstol(pszEnd+1, &pszEnd, 10);
							if (pszEnd && *pszEnd == L'.')
							{
								nStage = wcstol(pszEnd+1, &pszEnd, 10);
							}
						}
					}
					m_Version.Major = nMajor;
					m_Version.Minor = nMinor;
					m_Version.Revision = nRevision;
					m_Version.Build = nBuild;
					if (nStage >= VS_RELEASE && nStage <= VS_RC)
						m_Version.Stage = (VERSION_STAGE)nStage;
				}
			}

			GUID guid;
			struct {
				LPCWSTR sName;
				GUID* pGuid;
			} initGuids[] = {
				{L"GUID", &mguid_Plugin},
				{L"DialogsGUID", &mguid_Dialogs},
				#if MVV_3>=2103
				{L"ApiMessage", &mguid_ApiMessage},
				{L"ApiMenu", &mguid_ApiMenu},
				{L"ApiInput", &mguid_ApiInput},
				#endif
			};
			for (UINT ig = 0; ig < ARRAYSIZE(initGuids); ig++)
			{
				if (GetPrivateProfileString(L"Plugin", initGuids[ig].sName, L"", szTemp, ARRAYSIZE(szTemp), szIni))
				{
					if (UuidFromStringW((RPC_WSTR)szTemp, &guid) == RPC_S_OK)
						*initGuids[ig].pGuid = guid;
					else
					{
						UuidCreate(initGuids[ig].pGuid);
						WritePrivateProfileString(L"Plugin", initGuids[ig].sName, FormatGuid(initGuids[ig].pGuid, szTmp), szIni);
					}
				}
			}
			
			lbRc = TRUE;
		}
	}
	
	if (!lbRc)
	{
		lstrcpyn(ms_Title, szSelf[0] ? szSelf : L"Far3Wrap", ARRAYSIZE(ms_Title));
		lstrcpy(ms_Desc, L"Far2->Far3 plugin wrapper");
		lstrcpy(ms_Author, L"Maximus5");
		lstrcpy(ms_RegRoot, L"Software\\Far Manager\\Plugins");
		ms_ForcePrefix[0] = 0;
		UuidCreate(&mguid_Plugin);
		UuidCreate(&mguid_Dialogs);
		#ifdef _DEBUG
		{
			DebugDump(L"LoadPluginInfo failed!", szSelf[0] ? szSelf : L"Far3Wrap");
		}
		#endif
		//mguid_Plugin = guid_DefPlugin;
		//guid_PluginMenu = ::guid_DefPluginMenu;
		//guid_PluginConfigMenu = ::guid_DefPluginConfigMenu;
		//guid_Dialogs = ::guid_DefDialogs;
	}
}

void WrapPluginInfo::CheckPluginExports(LoaderFunctions3 eFunc)
{
	bool lbPluginExport = false; // ������� � Far2 ������� ���������
	bool lbLoaderExport = false; // ������� �������� � Loader.dll
	
	
	switch (eFunc)
	{
	case LF3_Analyse:
		lbPluginExport = (AnalyseW!=NULL)||(OpenFilePluginW!=NULL);
		lbLoaderExport = GetProcAddress(mh_Loader, "AnalyseW")!=NULL;
		break;
	case LF3_CustomData:
		lbPluginExport = (GetCustomDataW!=NULL);
		lbLoaderExport = GetProcAddress(mh_Loader, "GetCustomDataW")!=NULL;
		break;
	case LF3_Dialog:
		lbPluginExport = (ProcessDialogEventW!=NULL);
		lbLoaderExport = GetProcAddress(mh_Loader, "ProcessDialogEventW")!=NULL;
		break;
	case LF3_Editor:
		lbPluginExport = (ProcessEditorEventW!=NULL)||(ProcessEditorInputW!=NULL);
		lbLoaderExport = (GetProcAddress(mh_Loader, "ProcessEditorEventW")!=NULL)
					  && (GetProcAddress(mh_Loader, "ProcessEditorInputW")!=NULL);
		break;
	case LF3_FindList:
		lbPluginExport = (SetFindListW!=NULL);
		lbLoaderExport = (GetProcAddress(mh_Loader, "SetFindListW")!=NULL)
					  && (GetProcAddress(mh_Loader, "AnalyseW")!=NULL); // TempPanel!!
		break;
	case LF3_Viewer:
		lbPluginExport = (ProcessViewerEventW!=NULL);
		lbLoaderExport = GetProcAddress(mh_Loader, "ProcessViewerEventW")!=NULL;
		break;
	case LF3_GetFiles:
		lbPluginExport = (GetFilesW!=NULL);
		lbLoaderExport = GetProcAddress(mh_Loader, "GetFilesW")!=NULL;
		break;
	case LF3_PutFiles:
		lbPluginExport = (PutFilesW!=NULL);
		lbLoaderExport = GetProcAddress(mh_Loader, "PutFilesW")!=NULL;
		break;
	case LF3_DeleteFiles:
		lbPluginExport = (DeleteFilesW!=NULL);
		lbLoaderExport = GetProcAddress(mh_Loader, "DeleteFilesW")!=NULL;
		break;
	//case LF3_Window:
	//	lbPluginExport = (
	//		(((ProcessEditorEventW!=NULL)||(ProcessEditorInputW!=NULL)) ? 1 : 0) +
	//		((ProcessViewerEventW!=NULL) ? 1 : 0) + 
	//		((ProcessDialogEventW!=NULL) ? 1 : 0)) > 1;
	//	lbLoaderExport = 
	//		   (GetProcAddress(mh_Loader, "ProcessEditorEventW")!=NULL)
	//		&& (GetProcAddress(mh_Loader, "ProcessEditorInputW")!=NULL)
	//		&& (GetProcAddress(mh_Loader, "ProcessViewerEventW")!=NULL)
	//		&& (GetProcAddress(mh_Loader, "ProcessDialogEventW")!=NULL);
	//	break;
	default:
		// ����������� ���
		_ASSERTE(eFunc == LF3_Viewer);
	}
	
	if (lbPluginExport)
		mn_PluginFunctions |= eFunc;
	
	if (lbLoaderExport)
		mn_LoaderFunctions |= eFunc;
}

BOOL WrapPluginInfo::LoadPlugin(BOOL abSilent)
{
	#ifdef _DEBUG
	void *p1 = NULL, *p2 = NULL;
	#endif

	if (!*ms_PluginDll)
	{
		#ifdef _DEBUG
		{
			DebugDump(L"LoadPlugin failed (ms_PluginDll)!", ms_File);
		}
		#endif
		return FALSE;
	}
	
	DWORD dwErr = 0;
	wchar_t szInfo[1024] = {0};
	
	if (mh_Dll == NULL)
	{
		//wchar_t szOldDir[MAX_PATH] = {0};
		//GetCurrentDirectory(ARRAYSIZE(szOldDir), szOldDir);
		//wchar_t* pszSlash = wcsrchr(PluginDll, L'\\');
		//if (pszSlash)
		//{
		//	*pszSlash = 0;
		//	SetCurrentDirectory(PluginDll);
		//	*pszSlash = L'\\';
		//}

		bool lbErrorCreated = false;
		
		if (wcschr(ms_PluginDll, L'*'))
		{
			wsprintf(szInfo, L"Far3Wrap\nPlugin module not found: \n%s", ms_PluginDll);
			wchar_t szSelf[MAX_PATH+1]; szSelf[0] = 0;
			GetModuleFileName(mh_Loader, szSelf, ARRAYSIZE(szSelf));
			wsprintf(ms_DllFailTitle, L"[%s] Plugin not found", szSelf);
			dwErr = ERROR_FILE_NOT_FOUND;
			lbErrorCreated = true;
		}
		else
		{
			//SetDllDirectory(...); // ����/�� ����?
			if (wcschr(ms_PluginDll, L'\\'))
			{
				dbgLoadLibrary(L"Far3Wrap.LoadLibraryEx(%s)", ms_PluginDll);
				mh_Dll = LoadLibraryEx(ms_PluginDll, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
			}
			else
			{
				dbgLoadLibrary(L"Far3Wrap.LoadLibrary(%s)", ms_PluginDll);
				mh_Dll = LoadLibrary(ms_PluginDll);
			}
			dwErr = GetLastError();
		}
		//if (pszSlash)
		//	SetCurrentDirectory(szOldDir);
		if (mh_Dll == NULL)
		{
			//TODO: ��������� ������ ��������
			if (szInfo[0] == 0)
			{
				wsprintf(szInfo, L"Far3Wrap\nPlugin loading failed!\n%s\nErrCode=0x%08X\n", ms_PluginDll, dwErr);
				int nLen = lstrlen(szInfo);
				wchar_t* pszFmt = szInfo + nLen;
				int nLeft = ARRAYSIZE(szInfo) - nLen - 1;
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwErr,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), pszFmt, nLeft, NULL);
				while ((pszFmt = wcschr(pszFmt, L'\r')) != NULL)
					*pszFmt = L' '; // '\r' ������������ � ��������� ���� ��� ������ "����"
			}

			wchar_t szSelf[MAX_PATH+1]; szSelf[0] = 0;
			GetModuleFileName(mh_Loader, szSelf, ARRAYSIZE(szSelf));
			wchar_t* pszPlugin = *ms_File ? ms_File : szSelf;
			if (!lbErrorCreated)
				wsprintf(ms_DllFailTitle, L"[%s] Load plugin failed x%X",
					pszPlugin,
					dwErr);

			if (abSilent)
			{
				if (m_ErrorInfo && m_ErrorInfoMax > 0)
					lstrcpyn(m_ErrorInfo, szInfo, m_ErrorInfoMax);
			}
			else
			{
				MessageBox(NULL, szInfo, L"Far3Wrapper", MB_ICONSTOP|MB_SYSTEMMODAL);
			}
		}
	}

	if ((mh_Dll != NULL) && !mb_ExportsLoaded)
	{
		mb_ExportsLoaded = TRUE;
		AnalyseW = (WrapPluginInfo::_AnalyseW)GetProcAddress(mh_Dll, "AnalyseW");
		ClosePluginW = (WrapPluginInfo::_ClosePluginW)GetProcAddress(mh_Dll, "ClosePluginW");
		CompareW = (WrapPluginInfo::_CompareW)GetProcAddress(mh_Dll, "CompareW");
		ConfigureW = (WrapPluginInfo::_ConfigureW)GetProcAddress(mh_Dll, "ConfigureW");
		DeleteFilesW = (WrapPluginInfo::_DeleteFilesW)GetProcAddress(mh_Dll, "DeleteFilesW");
		ExitFARW = (WrapPluginInfo::_ExitFARW)GetProcAddress(mh_Dll, "ExitFARW");
		FreeFindDataW = (WrapPluginInfo::_FreeFindDataW)GetProcAddress(mh_Dll, "FreeFindDataW");
		FreeVirtualFindDataW = (WrapPluginInfo::_FreeVirtualFindDataW)GetProcAddress(mh_Dll, "FreeVirtualFindDataW");
		GetFilesW = (WrapPluginInfo::_GetFilesW)GetProcAddress(mh_Dll, "GetFilesW");
		GetFindDataW = (WrapPluginInfo::_GetFindDataW)GetProcAddress(mh_Dll, "GetFindDataW");
		GetGlobalInfoWPlugin = (WrapPluginInfo::_GetGlobalInfoW)GetProcAddress(mh_Dll, "GetGlobalInfoW");
		GetMinFarVersionW = (WrapPluginInfo::_GetMinFarVersionW)GetProcAddress(mh_Dll, "GetMinFarVersionW");
		GetOpenPluginInfoW = (WrapPluginInfo::_GetOpenPluginInfoW)GetProcAddress(mh_Dll, "GetOpenPluginInfoW");
		GetPluginInfoW = (WrapPluginInfo::_GetPluginInfoW)GetProcAddress(mh_Dll, "GetPluginInfoW");
		GetVirtualFindDataW = (WrapPluginInfo::_GetVirtualFindDataW)GetProcAddress(mh_Dll, "GetVirtualFindDataW");
		MakeDirectoryW = (WrapPluginInfo::_MakeDirectoryW)GetProcAddress(mh_Dll, "MakeDirectoryW");
		OpenFilePluginW = (WrapPluginInfo::_OpenFilePluginW)GetProcAddress(mh_Dll, "OpenFilePluginW");
		OpenPluginW = (WrapPluginInfo::_OpenPluginW)GetProcAddress(mh_Dll, "OpenPluginW");
		ProcessDialogEventW = (WrapPluginInfo::_ProcessDialogEventW)GetProcAddress(mh_Dll, "ProcessDialogEventW");
		ProcessEditorEventW = (WrapPluginInfo::_ProcessEditorEventW)GetProcAddress(mh_Dll, "ProcessEditorEventW");
		ProcessEditorInputW = (WrapPluginInfo::_ProcessEditorInputW)GetProcAddress(mh_Dll, "ProcessEditorInputW");
		ProcessEventW = (WrapPluginInfo::_ProcessEventW)GetProcAddress(mh_Dll, "ProcessEventW");
		ProcessHostFileW = (WrapPluginInfo::_ProcessHostFileW)GetProcAddress(mh_Dll, "ProcessHostFileW");
		ProcessKeyW = (WrapPluginInfo::_ProcessKeyW)GetProcAddress(mh_Dll, "ProcessKeyW");
		ProcessSynchroEventW = (WrapPluginInfo::_ProcessSynchroEventW)GetProcAddress(mh_Dll, "ProcessSynchroEventW");
		ProcessViewerEventW = (WrapPluginInfo::_ProcessViewerEventW)GetProcAddress(mh_Dll, "ProcessViewerEventW");
		PutFilesW = (WrapPluginInfo::_PutFilesW)GetProcAddress(mh_Dll, "PutFilesW");
		SetDirectoryW = (WrapPluginInfo::_SetDirectoryW)GetProcAddress(mh_Dll, "SetDirectoryW");
		SetFindListW = (WrapPluginInfo::_SetFindListW)GetProcAddress(mh_Dll, "SetFindListW");
		SetStartupInfoW = (WrapPluginInfo::_SetStartupInfoW)GetProcAddress(mh_Dll, "SetStartupInfoW");
		GetCustomDataW = (WrapPluginInfo::_GetCustomDataW)GetProcAddress(mh_Dll, "GetCustomDataW");
		FreeCustomDataW = (WrapPluginInfo::_FreeCustomDataW)GetProcAddress(mh_Dll, "FreeCustomDataW");
		
		if (GetMinFarVersionW)
		{
			DWORD nMinFarVersion = GetMinFarVersionW();
			if (nMinFarVersion != m_MinFarVersion)
			{
				m_MinFarVersion = nMinFarVersion;
				wchar_t szVer[32]; wsprintf(szVer, L"0x%08X", nMinFarVersion);
				WritePrivateProfileString(L"Plugin", L"MinFarVersion", szVer, ms_IniFile);
			}
		}

		//TODO: ���� �������������� ������� �� mh_Loader �� ��������� � mn_OldAbsentFunctions - �������� (�������� mn_OldAbsentFunctions � 0)
		mn_PluginFunctions = LF3_None;
		mn_LoaderFunctions = LF3_None; // ���������� �� ������ �������� ��������� � Loader.dll

		CheckPluginExports(LF3_Analyse);
		CheckPluginExports(LF3_CustomData);
		CheckPluginExports(LF3_Dialog);
		CheckPluginExports(LF3_Editor);
		CheckPluginExports(LF3_FindList);
		CheckPluginExports(LF3_Viewer);
		CheckPluginExports(LF3_GetFiles);
		CheckPluginExports(LF3_PutFiles);
		CheckPluginExports(LF3_DeleteFiles);

		#if 0	
		CheckPluginExports(ALF3_Analyse);
		CheckPluginExports(ALF3_Open);
		CheckPluginExports(ALF3_Configure);
		CheckPluginExports(ALF3_Compare);
		CheckPluginExports(ALF3_GetFiles);
		CheckPluginExports(ALF3_PutFiles);
		CheckPluginExports(ALF3_FindData);
		CheckPluginExports(ALF3_VirtualFindData);
		CheckPluginExports(ALF3_ProcessHostFile);
		CheckPluginExports(ALF3_ProcessDialogEvent);
		CheckPluginExports(ALF3_ProcessEditorEvent);
		CheckPluginExports(ALF3_ProcessEditorInput);
		CheckPluginExports(ALF3_ProcessViewerEvent);
		CheckPluginExports(ALF3_SetFindList);
		CheckPluginExports(ALF3_CustomData);
		#endif

		#if 0
		// ���� ����� � ���������� ������ (��� ������������) �����-�� �������� - ����� ����� ��������� ��� � �������
		int nLoaderResourceId = 0;
		#ifdef _WIN64
		#define MAKE_LOADER_RC(s) IDR_LOADER64_##s
		#else
		#define MAKE_LOADER_RC(s) IDR_LOADER_##s
		#endif
		// !! LF3_FindList ������ ���� ����� LF3_Analyse !!
		if (mn_LoaderFunctions == LF3_FindList)
		{
			if (mn_PluginFunctions != LF3_FindList)
				nLoaderResourceId = MAKE_LOADER_RC(FINDLIST);
		}
		else if (mn_LoaderFunctions == LF3_Analyse)
		{
			if (mn_PluginFunctions != LF3_Analyse)
				nLoaderResourceId = MAKE_LOADER_RC(ARC);
		}
		else if (mn_LoaderFunctions == LF3_CustomData)
		{
			if (mn_PluginFunctions != LF3_CustomData)
				nLoaderResourceId = MAKE_LOADER_RC(CUSTOM);
		}
		else if (mn_LoaderFunctions == LF3_Window)
		{
			if (mn_PluginFunctions != LF3_Window)
				nLoaderResourceId = MAKE_LOADER_RC(WINDOW);
		}
		else if (mn_LoaderFunctions == LF3_Dialog)
		{
			if (mn_PluginFunctions != LF3_Dialog)
				nLoaderResourceId = MAKE_LOADER_RC(DIALOG);
		}
		else if (mn_LoaderFunctions == LF3_Editor)
		{
			if (mn_PluginFunctions != LF3_Editor)
				nLoaderResourceId = MAKE_LOADER_RC(EDITOR);
		}
		else
		{
			_ASSERTE(mn_LoaderFunctions == LF3_Full);
			if ((mn_PluginFunctions & LF3_Full) != LF3_Full)
				nLoaderResourceId = MAKE_LOADER_RC(FULL);
		};
		if (mn_LoaderFunctions != mn_PluginFunctions && nLoaderResourceId == 0)
		{
			_ASSERTE(mn_LoaderFunctions == mn_PluginFunctions);
		}
		if (mn_LoaderFunctions != mn_PluginFunctions && *ms_PluginDll && nLoaderResourceId)
		{
			// ��������, ����� ��� ������� � ������ "�� �����������"
			bool lbExist = false;
			std::vector<WrapUpdateFunctions>::iterator iter;
			for (iter = UpdateFunc.begin(); !lbExist && iter != UpdateFunc.end(); iter++)
			{
				if (lstrcmpi(iter->ms_IniFile, ms_IniFile) == 0)
					lbExist = true;
			}

			if (!lbExist)
			{
				WrapUpdateFunctions upd = {mn_PluginFunctions, nLoaderResourceId};
				if (GetModuleFileName(mh_Loader, upd.ms_Loader, ARRAYSIZE(upd.ms_Loader)))
				{
					lstrcpy(upd.ms_IniFile, ms_IniFile);
					UpdateFunc.push_back(upd);
				}
			}
		}
		#endif

		#if 0
		int nIdx = 0;
		ExportFunc strNull[64] = {{NULL}};
		#undef SET_EXP
		#define SET_EXP_(n,s) if (!n) { strNull[nIdx].Name = s; strNull[nIdx].OldAddress = GetProcAddress(mh_Loader, s); nIdx++; }
		#define SET_EXP(n) SET_EXP_(n,#n)
		SET_EXP(ConfigureW);
		SET_EXP_((AnalyseW||OpenFilePluginW),"AnalyseW");
		SET_EXP_(ProcessKeyW, "ProcessPanelInputW");
		SET_EXP(ProcessDialogEventW);
		SET_EXP(ProcessEditorEventW);
		SET_EXP(ProcessViewerEventW);
		SET_EXP(GetFilesW);
		SET_EXP(PutFilesW);
		SET_EXP(DeleteFilesW);
		SET_EXP(GetFindDataW);
		SET_EXP(FreeFindDataW);
		SET_EXP(GetVirtualFindDataW);
		SET_EXP(FreeVirtualFindDataW);
		SET_EXP(SetDirectoryW);
		SET_EXP(SetFindListW);
		SET_EXP(GetCustomDataW);
		SET_EXP(FreeCustomDataW);
		SET_EXP_((OpenPluginW||OpenFilePluginW), "OpenW");
		#undef SET_EXP
		#ifdef _DEBUG
		p1 = GetProcAddress(mh_Loader, "SetFindListW");
		#endif
		ChangeExports(strNull, mh_Loader);
		#ifdef _DEBUG
		p2 = GetProcAddress(mh_Loader, "SetFindListW");
		#endif
		#endif
	}

	#ifdef _DEBUG
	if (mh_Dll == NULL)
	{
		DebugDump(L"LoadPlugin failed (mh_Dll)!", ms_File);
	}
	#endif

	if (mh_Dll != NULL)
	{
		SetHook(ms_File, mh_Dll, TRUE);
	}
	
	return (mh_Dll != NULL);
}

void WrapPluginInfo::ClearProcAddr()
{
	mb_ExportsLoaded = FALSE;
	AnalyseW = NULL;
	ClosePluginW = NULL;
	CompareW = NULL;
	ConfigureW = NULL;
	DeleteFilesW = NULL;
	ExitFARW = NULL;
	FreeFindDataW = NULL;
	FreeVirtualFindDataW = NULL;
	GetFilesW = NULL;
	GetFindDataW = NULL;
	GetMinFarVersionW = NULL;
	GetOpenPluginInfoW = NULL;
	GetPluginInfoW = NULL;
	GetVirtualFindDataW = NULL;
	MakeDirectoryW = NULL;
	OpenFilePluginW = NULL;
	OpenPluginW = NULL;
	ProcessDialogEventW = NULL;
	ProcessEditorEventW = NULL;
	ProcessEditorInputW = NULL;
	ProcessEventW = NULL;
	ProcessHostFileW = NULL;
	ProcessKeyW = NULL;
	ProcessSynchroEventW = NULL;
	ProcessViewerEventW = NULL;
	PutFilesW = NULL;
	SetDirectoryW = NULL;
	SetFindListW  = NULL;
	SetStartupInfoW = NULL;
	GetCustomDataW = NULL;
	FreeCustomDataW = NULL;
}

void WrapPluginInfo::UnloadPlugin()
{
	if (mh_Dll)
	{
		if (mn_DoNotFreeLibrary == 0)
		{
			dbgFreeLibrary(L"UnloadPlugin", mh_Dll, ms_PluginDll);
			FreeLibrary(mh_Dll);
		}
		else
		{
			dbgFreeLibrary(L"Skip.UnloadPlugin", mh_Dll, ms_PluginDll);
		}
		mh_Dll = NULL;
	}
	ClearProcAddr();
}



LPCWSTR WrapPluginInfo::FormatGuid(const GUID* guid, wchar_t* tmp, BOOL abQuote /*= FALSE*/)
{
	if (!guid) guid = &GUID_NULL;
	wsprintf(tmp, 
		abQuote ?
			L"\"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\"" :
			L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"
			,
		guid->Data1, guid->Data2, guid->Data3,
		guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
		guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
	return tmp;
}

LPWSTR WrapPluginInfo::MacroFromMultiSZ(LPCWSTR aszMultiSZ)
{
	if (!aszMultiSZ || !*aszMultiSZ)
		return NULL;
	
	// �������� SequenceText ����������� � ������� REG_MULTI_SZ.
    // ������ 1\x00 ������ 2\x00 ... ������ N\x00\x00
    
    // ������� ���������� ����� ������
    wchar_t* pszEnd = (wchar_t*)aszMultiSZ;
    while (*pszEnd)
    {
    	pszEnd += lstrlen(pszEnd)+1;
    }
    size_t nLen = (pszEnd - aszMultiSZ + 1);
    
    wchar_t* pszNew = (wchar_t*)malloc(nLen*sizeof(wchar_t));
    memmove(pszNew, aszMultiSZ, nLen*sizeof(wchar_t));

    pszEnd = pszNew;
    while (TRUE)
    {
    	wchar_t* psz = pszEnd + lstrlen(pszEnd);
    	if (psz[1] == 0)
    		break;
		*psz = L'\n';
    	pszEnd = psz+1;
    }
    
    return pszNew;
}

InfoPanelLine* WrapPluginInfo::InfoLines_2_3(const Far2::InfoPanelLine *InfoLines, int InfoLinesNumber)
{
	if (m_InfoLines)
	{
		free(m_InfoLines);
		m_InfoLines = NULL;
	}
	m_InfoLinesNumber = 0;
	if (InfoLines && InfoLinesNumber > 0)
	{	
		m_InfoLines = (InfoPanelLine*)calloc(InfoLinesNumber, sizeof(*m_InfoLines));
		for (int i = 0; i < InfoLinesNumber; i++)
		{
			m_InfoLines[i].Text = InfoLines[i].Text;
			m_InfoLines[i].Data = InfoLines[i].Data;
			#if MVV_3<=2798
			m_InfoLines[i].Separator = InfoLines[i].Separator;
			#else
			if (InfoLines[i].Separator)
				m_InfoLines[i].Flags |= IPLFLAGS_SEPARATOR;
			#endif
		}
		m_InfoLinesNumber = InfoLinesNumber;
	}
	return m_InfoLines;
}

PanelMode* WrapPluginInfo::PanelModes_2_3(const Far2::PanelMode *PanelModesArray, int PanelModesNumber)
{
	if (m_PanelModesArray)
	{
		free(m_PanelModesArray);
		m_PanelModesArray = NULL;
	}
	m_PanelModesNumber = 0;
	if (PanelModesArray && PanelModesNumber > 0)
	{
		m_PanelModesArray = (PanelMode*)calloc(PanelModesNumber, sizeof(*m_PanelModesArray));
		for (int i = 0; i < PanelModesNumber; i++)
		{
			#if MVV_3<=2798
			m_PanelModesArray[i].StructSize = sizeof(*PanelModesArray);
			#endif
			m_PanelModesArray[i].ColumnTypes = PanelModesArray[i].ColumnTypes;
			m_PanelModesArray[i].ColumnWidths = PanelModesArray[i].ColumnWidths;
			m_PanelModesArray[i].ColumnTitles = PanelModesArray[i].ColumnTitles;
			m_PanelModesArray[i].StatusColumnTypes = PanelModesArray[i].StatusColumnTypes;
			m_PanelModesArray[i].StatusColumnWidths = PanelModesArray[i].StatusColumnWidths;
			m_PanelModesArray[i].Flags = 
				(PanelModesArray[i].FullScreen ? PMFLAGS_FULLSCREEN : 0) |
				(PanelModesArray[i].DetailedStatus ? PMFLAGS_DETAILEDSTATUS : 0) |
				(PanelModesArray[i].AlignExtensions ? PMFLAGS_ALIGNEXTENSIONS : 0) |
				(PanelModesArray[i].CaseConversion ? PMFLAGS_CASECONVERSION : 0);
		}
		m_PanelModesNumber = PanelModesNumber;
	}
	return m_PanelModesArray;
}


int WrapPluginInfo::OpMode_3_2(OPERATION_MODES OpMode3)
{
	int OpMode2 = 0;
	if (OpMode3 & OPM_SILENT)
		OpMode2 |= Far2::OPM_SILENT;
	if (OpMode3 & OPM_FIND)
		OpMode2 |= Far2::OPM_FIND;
	if (OpMode3 & OPM_VIEW)
		OpMode2 |= Far2::OPM_VIEW;
	if (OpMode3 & OPM_EDIT)
		OpMode2 |= Far2::OPM_EDIT;
	if (OpMode3 & OPM_TOPLEVEL)
		OpMode2 |= Far2::OPM_TOPLEVEL;
	if (OpMode3 & OPM_DESCR)
		OpMode2 |= Far2::OPM_DESCR;
	if (OpMode3 & OPM_QUICKVIEW)
		OpMode2 |= Far2::OPM_QUICKVIEW;
	//if (OpMode3 & OPM_PGDN) -- � Far2 �����������
	//	OpMode2 |= Far2::OPM_PGDN;
	return OpMode2;
}
//OPERATION_MODES OpMode_2_3(int OpMode2)
//{
//}

int WrapPluginInfo::OpenFrom_3_2(OPENFROM OpenFrom3, INT_PTR Data, INT_PTR& Item)
{
	int OpenFrom2 = 0;
	
	#if MVV_3>=2458
	if (OpenFrom3 == OPEN_FROMMACRO)
	{
		INT_PTR nArea = psi3.MacroControl(MCTLARG(mguid_Plugin), MCTL_GETAREA, 0, 0);
		switch(nArea)
		{
			case MACROAREA_SHELL:
			case MACROAREA_SHELLAUTOCOMPLETION:
			case MACROAREA_INFOPANEL:
			case MACROAREA_QVIEWPANEL:
			case MACROAREA_TREEPANEL:
			case MACROAREA_SEARCH:
			case MACROAREA_FINDFOLDER:
				OpenFrom2 |= Far2::OPEN_FILEPANEL; break;
			case MACROAREA_VIEWER:
				OpenFrom2 |= Far2::OPEN_VIEWER; break;
			case MACROAREA_EDITOR:
				OpenFrom2 |= Far2::OPEN_EDITOR; break;
			case MACROAREA_DIALOG:
			case MACROAREA_DIALOGAUTOCOMPLETION:
				OpenFrom2 = Far2::OPEN_DIALOG; break;
				break;
			case MACROAREA_DISKS:
				OpenFrom2 |= Far2::OPEN_DISKMENU; break;
			case MACROAREA_HELP:
			case MACROAREA_MAINMENU:
			case MACROAREA_MENU:
			case MACROAREA_USERMENU:
			case MACROAREA_OTHER:
				OpenFrom2 |= 0; //TODO: ???
				break;
			default:
				_ASSERTE(FALSE); // ����������� �������?
				OpenFrom2 = 0;
		}
		
		Item = 0;
		OpenFrom2 |= Far2::OPEN_FROMMACRO;
		
		OpenMacroInfo* p = (OpenMacroInfo*)Data;
		if (p->StructSize >= sizeof(*p) && p->Count)
		{
			switch (p->Values[0].Type)
			{
			case FMVT_UNKNOWN:
				// ����� ��� ����������
				break;
			case FMVT_INTEGER:
				Item = (INT_PTR)p->Values[0].Integer;
				break;
			case FMVT_DOUBLE:
				Item = (INT_PTR)p->Values[0].Double; // � ����������� �������� ���������
				break;
			case FMVT_STRING:
				Item = (INT_PTR)p->Values[0].String;
				if (Item)
				{
					OpenFrom2 |= Far2::OPEN_FROMMACROSTRING;
				}
				else
				{
					_ASSERTE(p->Values[0].String != NULL);
				}
				break;
			}
		}
	}
	#else
	switch ((OpenFrom3 & OPEN_FROM_MASK))
	{
		case OPEN_LEFTDISKMENU:
			OpenFrom2 |= Far2::OPEN_DISKMENU; break;
		case OPEN_PLUGINSMENU:
			OpenFrom2 |= Far2::OPEN_PLUGINSMENU; break;
		case OPEN_FINDLIST:
			OpenFrom2 |= Far2::OPEN_FINDLIST; break;
		case OPEN_SHORTCUT:
			OpenFrom2 |= Far2::OPEN_SHORTCUT; break;
		case OPEN_COMMANDLINE:
			OpenFrom2 |= Far2::OPEN_COMMANDLINE; break;
		case OPEN_EDITOR:
			OpenFrom2 |= Far2::OPEN_EDITOR; break;
		case OPEN_VIEWER:
			OpenFrom2 |= Far2::OPEN_VIEWER; break;
		case OPEN_FILEPANEL:
			OpenFrom2 |= Far2::OPEN_FILEPANEL; break;
		case OPEN_DIALOG:
			OpenFrom2 |= Far2::OPEN_DIALOG; break;
		case OPEN_ANALYSE:
			OpenFrom2 |= Far2::OPEN_ANALYSE; break;
		//case OPEN_RIGHTDISKMENU: -- � Far2 ����������
	}
	if (OpenFrom3 & OPEN_FROMMACRO_MASK)
		OpenFrom2 |= Far2::OPEN_FROMMACRO;
	Item = Data;
	#endif
	
	return OpenFrom2;
}

OPENPANELINFO_FLAGS WrapPluginInfo::OpenPanelInfoFlags_2_3(DWORD Flags2)
{
	OPENPANELINFO_FLAGS Flags3 = OPIF_NONE;
	
	if (!(Flags2 & Far2::OPIF_USEFILTER)) Flags3 |= OPIF_DISABLEFILTER;
	if (!(Flags2 & Far2::OPIF_USESORTGROUPS)) Flags3 |= OPIF_DISABLESORTGROUPS;
	if (!(Flags2 & Far2::OPIF_USEHIGHLIGHTING)) Flags3 |= OPIF_DISABLEHIGHLIGHTING;
	if ((Flags2 & Far2::OPIF_ADDDOTS)) Flags3 |= OPIF_ADDDOTS;
	if ((Flags2 & Far2::OPIF_RAWSELECTION)) Flags3 |= OPIF_RAWSELECTION;
	if ((Flags2 & Far2::OPIF_REALNAMES)) Flags3 |= OPIF_REALNAMES;
	if ((Flags2 & Far2::OPIF_SHOWNAMESONLY)) Flags3 |= OPIF_SHOWNAMESONLY;
	if ((Flags2 & Far2::OPIF_SHOWRIGHTALIGNNAMES)) Flags3 |= OPIF_SHOWRIGHTALIGNNAMES;
	if ((Flags2 & Far2::OPIF_SHOWPRESERVECASE)) Flags3 |= OPIF_SHOWPRESERVECASE;
	if ((Flags2 & Far2::OPIF_COMPAREFATTIME)) Flags3 |= OPIF_COMPAREFATTIME;
	if ((Flags2 & Far2::OPIF_EXTERNALGET)) Flags3 |= OPIF_EXTERNALGET;
	if ((Flags2 & Far2::OPIF_EXTERNALPUT)) Flags3 |= OPIF_EXTERNALPUT;
	if ((Flags2 & Far2::OPIF_EXTERNALDELETE)) Flags3 |= OPIF_EXTERNALDELETE;
	if ((Flags2 & Far2::OPIF_EXTERNALMKDIR)) Flags3 |= OPIF_EXTERNALMKDIR;
	if ((Flags2 & Far2::OPIF_USEATTRHIGHLIGHTING)) Flags3 |= OPIF_USEATTRHIGHLIGHTING;

	Flags3 |= OPIF_SHORTCUT;
	
	return Flags3;
}

Far2::OPENPLUGININFO_SORTMODES WrapPluginInfo::SortMode_3_2(OPENPANELINFO_SORTMODES Mode3)
{
	Far2::OPENPLUGININFO_SORTMODES Mode2 = Far2::SM_DEFAULT;
	switch (Mode3)
	{
		case SM_UNSORTED: Mode2 = Far2::SM_UNSORTED; break;
		case SM_NAME: Mode2 = Far2::SM_NAME; break;
		case SM_EXT: Mode2 = Far2::SM_EXT; break;
		case SM_MTIME: Mode2 = Far2::SM_MTIME; break;
		case SM_CTIME: Mode2 = Far2::SM_CTIME; break;
		case SM_ATIME: Mode2 = Far2::SM_ATIME; break;
		case SM_SIZE: Mode2 = Far2::SM_SIZE; break;
		case SM_DESCR: Mode2 = Far2::SM_DESCR; break;
		case SM_OWNER: Mode2 = Far2::SM_OWNER; break;
		case SM_COMPRESSEDSIZE: Mode2 = Far2::SM_COMPRESSEDSIZE; break;
		case SM_NUMLINKS: Mode2 = Far2::SM_NUMLINKS; break;
		case SM_NUMSTREAMS: Mode2 = Far2::SM_NUMSTREAMS; break;
		case SM_STREAMSSIZE: Mode2 = Far2::SM_STREAMSSIZE; break;
		case SM_FULLNAME: Mode2 = Far2::SM_FULLNAME; break;
	}
	return Mode2;
}

OPENPANELINFO_SORTMODES WrapPluginInfo::SortMode_2_3(/*Far2::OPENPLUGININFO_SORTMODES*/int Mode2)
{
	OPENPANELINFO_SORTMODES Mode3 = SM_DEFAULT;
	switch (Mode2)
	{
		case Far2::SM_UNSORTED: Mode3 = SM_UNSORTED; break;
		case Far2::SM_NAME: Mode3 = SM_NAME; break;
		case Far2::SM_EXT: Mode3 = SM_EXT; break;
		case Far2::SM_MTIME: Mode3 = SM_MTIME; break;
		case Far2::SM_CTIME: Mode3 = SM_CTIME; break;
		case Far2::SM_ATIME: Mode3 = SM_ATIME; break;
		case Far2::SM_SIZE: Mode3 = SM_SIZE; break;
		case Far2::SM_DESCR: Mode3 = SM_DESCR; break;
		case Far2::SM_OWNER: Mode3 = SM_OWNER; break;
		case Far2::SM_COMPRESSEDSIZE: Mode3 = SM_COMPRESSEDSIZE; break;
		case Far2::SM_NUMLINKS: Mode3 = SM_NUMLINKS; break;
		case Far2::SM_NUMSTREAMS: Mode3 = SM_NUMSTREAMS; break;
		case Far2::SM_STREAMSSIZE: Mode3 = SM_STREAMSSIZE; break;
		case Far2::SM_FULLNAME: Mode3 = SM_FULLNAME; break;
	}
	return Mode3;
}

PLUGINPANELITEMFLAGS WrapPluginInfo::PluginPanelItemFlags_2_3(DWORD Flags2)
{
	PLUGINPANELITEMFLAGS Flags3 = PPIF_NONE;
	if (Flags2 & Far2::PPIF_PROCESSDESCR)
		Flags3 |= PPIF_PROCESSDESCR;
	if (Flags2 & Far2::PPIF_USERDATA)
	{
		#if MVV_3<=2798
		Flags3 |= PPIF_USERDATA;
		#else
		_ASSERTE((Flags2 & Far2::PPIF_USERDATA)==0);
		#endif
	}
	if (Flags2 & Far2::PPIF_SELECTED)
		Flags3 |= PPIF_SELECTED;
	return Flags3;
}

DWORD WrapPluginInfo::PluginPanelItemFlags_3_2(PLUGINPANELITEMFLAGS Flags3)
{
	DWORD Flags2 = 0;
	if (Flags3 & PPIF_PROCESSDESCR)
		Flags2 |= Far2::PPIF_PROCESSDESCR;
	#if MVV_3<=2798
	if (Flags3 & PPIF_USERDATA)
		Flags2 |= Far2::PPIF_USERDATA;
	#endif
	if (Flags3 & PPIF_SELECTED)
		Flags2 |= Far2::PPIF_SELECTED;
	return Flags2;
}

void WrapPluginInfo::PluginPanelItem_2_3(const Far2::PluginPanelItem* p2, PluginPanelItem* p3)
{
	p3->FileAttributes = p2->FindData.dwFileAttributes;
	p3->CreationTime = p2->FindData.ftCreationTime;
	p3->LastAccessTime = p2->FindData.ftLastAccessTime;
	p3->LastWriteTime = p2->FindData.ftLastWriteTime;
	p3->ChangeTime = p2->FindData.ftLastWriteTime;
	p3->FileSize = p2->FindData.nFileSize;
	p3->AllocationSize = p2->FindData.nPackSize;
	p3->FileName = p2->FindData.lpwszFileName;
	p3->AlternateFileName = p2->FindData.lpwszAlternateFileName;
	p3->Flags = PluginPanelItemFlags_2_3(p2->Flags);
	p3->NumberOfLinks = p2->NumberOfLinks;
	p3->Description = p2->Description;
	p3->Owner = p2->Owner;
	p3->CustomColumnData = p2->CustomColumnData;
	p3->CustomColumnNumber = p2->CustomColumnNumber;
	#if MVV_3<=2798
	p3->UserData = p2->UserData;
	#else
	_ASSERTE(p2->UserData==0);
	#endif
	p3->CRC32 = p2->CRC32;
}

PluginPanelItem* WrapPluginInfo::PluginPanelItems_2_3(const Far2::PluginPanelItem* pItems, int ItemsNumber)
{
	PluginPanelItem* p3 = NULL;
	if (pItems && ItemsNumber > 0)
	{
		p3 = (PluginPanelItem*)calloc(ItemsNumber, sizeof(*p3));
		if (p3)
		{
			for (int i = 0; i < ItemsNumber; i++)
				PluginPanelItem_2_3(pItems+i, p3+i);
		}
	}
	return p3;
}

void WrapPluginInfo::PluginPanelItem_3_2(const PluginPanelItem* p3, Far2::PluginPanelItem* p2)
{
	p2->FindData.dwFileAttributes = p3->FileAttributes;
	p2->FindData.ftCreationTime = p3->CreationTime;
	p2->FindData.ftLastAccessTime = p3->LastAccessTime;
	p2->FindData.ftLastWriteTime = p3->LastWriteTime;
	//p2->FindData.ftLastWriteTime = p3->ChangeTime;
	p2->FindData.nFileSize = p3->FileSize;
	p2->FindData.nPackSize = p3->AllocationSize;
	p2->FindData.lpwszFileName = p3->FileName;
	p2->FindData.lpwszAlternateFileName = p3->AlternateFileName;
	p2->Flags = PluginPanelItemFlags_3_2(p3->Flags);
	p2->NumberOfLinks = p3->NumberOfLinks;
	p2->Description = p3->Description;
	p2->Owner = p3->Owner;
	p2->CustomColumnData = p3->CustomColumnData;
	p2->CustomColumnNumber = p3->CustomColumnNumber;
	#if MVV_3<=2798
	p2->UserData = p3->UserData;
	#else
	_ASSERTE(p3->UserData.Data==NULL);
	#endif
	p2->CRC32 = p3->CRC32;
}

void WrapPluginInfo::PluginPanelItem_3_2(const PluginPanelItem *p3, Far2::FAR_FIND_DATA* p2)
{
	p2->dwFileAttributes = p3->FileAttributes;
	p2->ftCreationTime = p3->CreationTime;
	p2->ftLastAccessTime = p3->LastAccessTime;
	p2->ftLastWriteTime = p3->LastWriteTime;
	p2->nFileSize = p3->FileSize;
	p2->nPackSize = p3->AllocationSize;
	p2->lpwszFileName = p3->FileName;
	p2->lpwszAlternateFileName = p3->AlternateFileName;
}

void WrapPluginInfo::PluginPanelItem_2_3(const Far2::FAR_FIND_DATA* p2, PluginPanelItem *p3)
{
	memset(p3, 0, sizeof(*p3));
	p3->FileAttributes = p2->dwFileAttributes;
	p3->CreationTime = p2->ftCreationTime;
	p3->LastAccessTime = p2->ftLastAccessTime;
	p3->LastWriteTime = p2->ftLastWriteTime;
	p3->ChangeTime = p2->ftLastWriteTime;
	p3->FileSize = p2->nFileSize;
	p3->AllocationSize = p2->nPackSize;
	p3->FileName = p2->lpwszFileName;
	p3->AlternateFileName = p2->lpwszAlternateFileName;
}

Far2::PluginPanelItem* WrapPluginInfo::PluginPanelItems_3_2(const PluginPanelItem* pItems, int ItemsNumber)
{
	Far2::PluginPanelItem* p2 = NULL;
	if (pItems && ItemsNumber > 0)
	{
		p2 = (Far2::PluginPanelItem*)calloc(ItemsNumber, sizeof(*p2));
		if (p2)
		{
			for (int i = 0; i < ItemsNumber; i++)
			{
				PluginPanelItem_3_2(pItems+i, p2+i);
			}
		}
	}
	return p2;
}

#if MVV_3>=2103
//int TranslateKeyToVK(int Key,INPUT_RECORD *Rec);
int TranslateKeyToVK(int Key,int &VirtKey,int &ControlState,INPUT_RECORD *Rec);
#endif

void WrapPluginInfo::FarKey_2_3(int Key2, INPUT_RECORD *r)
{
	memset(r, 0, sizeof(INPUT_RECORD));
#if MVV_3>=2103
	int VirtKey = 0, ControlState = 0;
	TranslateKeyToVK(Key2, VirtKey, ControlState, r);
#else
	FSF3.FarKeyToInputRecord(Key2, r);
#endif
}

#if MVV_3>=2103
DWORD CalcKeyCode(bool LeftOnly, INPUT_RECORD *rec,int RealKey=FALSE,int *NotMacros=NULL,bool ProcessCtrlCode=true);
#endif

int WrapPluginInfo::FarKey_3_2(const INPUT_RECORD *Rec)
{
	// ������� � Far3 build 2103 FarInputRecordToKey �������
	DWORD Key2 = 0;

	Key2 = FarKeyEx_3_2(Rec);

	return Key2;
}

int WrapPluginInfo::FarKeyEx_3_2(const INPUT_RECORD *Rec, bool LeftOnly /*= false*/)
{
	// ������� � Far3 build 2103 FarInputRecordToKey �������
	DWORD Key2 = 0;

#if MVV_3>=2103

	// ������� � Far3 build 2103 FarInputRecordToKey �������
	INPUT_RECORD r = *Rec;
	Key2 = CalcKeyCode(LeftOnly, &r);

#ifdef _DEBUG
	if (Key2 != Far2::KEY_NONE && !LeftOnly && IsDebuggerPresent())
	{
		INPUT_RECORD rDbg = {};
		FarKey_2_3(Key2, &rDbg);
		int nCmp = 0; //memcmp(&rDbg, Rec, sizeof(rDbg)) != 0;
		if (r.EventType == KEY_EVENT)
		{
			#define CTRLMASK (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED|RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED|SHIFT_PRESSED)
			if (r.Event.KeyEvent.bKeyDown != rDbg.Event.KeyEvent.bKeyDown)
				nCmp = 1;
			if (r.Event.KeyEvent.wVirtualKeyCode && rDbg.Event.KeyEvent.wVirtualKeyCode)
			{
				// ����� �� �������� ����������
				if (!((r.Event.KeyEvent.uChar.UnicodeChar == rDbg.Event.KeyEvent.uChar.UnicodeChar)
						&& ((r.Event.KeyEvent.uChar.UnicodeChar >= '0' && r.Event.KeyEvent.uChar.UnicodeChar <='9')
							|| (r.Event.KeyEvent.uChar.UnicodeChar=='.') || (r.Event.KeyEvent.uChar.UnicodeChar==','))))
				{
					if (r.Event.KeyEvent.wVirtualKeyCode != rDbg.Event.KeyEvent.wVirtualKeyCode)
					{
						// ���� ��������� - �� � �����
						if (r.Event.KeyEvent.uChar.UnicodeChar && r.Event.KeyEvent.uChar.UnicodeChar == rDbg.Event.KeyEvent.uChar.UnicodeChar)
							;
						else if (rDbg.Event.KeyEvent.wVirtualKeyCode >= VK_NUMPAD0 && rDbg.Event.KeyEvent.wVirtualKeyCode <= VK_NUMPAD9)
							;
						else
							nCmp = 1;
					}
					else
					{
						if ((r.Event.KeyEvent.wVirtualKeyCode != VK_SHIFT // ������/����� Shift ����� ������ Scan-����
							&& r.Event.KeyEvent.wVirtualKeyCode != VK_PAUSE // Scan ��� "Pause" �� ����������
							)
						&& (r.Event.KeyEvent.wVirtualScanCode != rDbg.Event.KeyEvent.wVirtualScanCode))
							nCmp = 1;
					}
				}
			}
			if (r.Event.KeyEvent.uChar.UnicodeChar != rDbg.Event.KeyEvent.uChar.UnicodeChar
					&& !(r.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
				nCmp = 1;
			if (rDbg.Event.KeyEvent.wVirtualKeyCode &&
				(r.Event.KeyEvent.dwControlKeyState&CTRLMASK) != (rDbg.Event.KeyEvent.dwControlKeyState&CTRLMASK)
					&& !((r.Event.KeyEvent.dwControlKeyState&CTRLMASK)==SHIFT_PRESSED
							&& (rDbg.Event.KeyEvent.dwControlKeyState&CTRLMASK)==0
							&& (r.Event.KeyEvent.wVirtualKeyCode>='A' && r.Event.KeyEvent.wVirtualKeyCode<='Z'
								||(r.Event.KeyEvent.wVirtualKeyCode==0xBA/*�*/)))
					&& !(r.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL
							&& (r.Event.KeyEvent.dwControlKeyState&RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED)
							&& (rDbg.Event.KeyEvent.dwControlKeyState&RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
					&& !((r.Event.KeyEvent.uChar.UnicodeChar=='.') || (r.Event.KeyEvent.uChar.UnicodeChar==','))
				)
				nCmp = 1;
		}
		else if (r.EventType == MOUSE_EVENT)
		{
			nCmp = (r.Event.MouseEvent.dwButtonState != rDbg.Event.MouseEvent.dwButtonState)
				|| ((r.Event.MouseEvent.dwControlKeyState&CTRLMASK) != (rDbg.Event.MouseEvent.dwControlKeyState&CTRLMASK))
				|| (r.Event.MouseEvent.dwEventFlags != rDbg.Event.MouseEvent.dwEventFlags)
				;
		}
		if (nCmp != 0)
		{
			static bool bFirstCall = false;
			if (!bFirstCall)
			{
				bFirstCall = true;
				_ASSERTE(nCmp == 0);
			}
		}
	}
#endif
	

#else
#if 1
	// � Far3 build 2026 ����� ����������
	Key2 = FSF3.FarInputRecordToKey(Rec);

#else

	if (Rec->EventType == KEY_EVENT)
	{
		if (!Rec->Event.KeyEvent.uChar.UnicodeChar
			&& !Rec->Event.KeyEvent.dwControlKeyState
			&& !Rec->Event.KeyEvent.wVirtualScanCode
			&& (Rec->Event.KeyEvent.wVirtualKeyCode <= 8))
		{
			Key2 = INTERNAL_KEY_BASE_2+Rec->Event.KeyEvent.wVirtualKeyCode;
		}
		else if (Rec->Event.KeyEvent.uChar.UnicodeChar == 0
			&& Rec->Event.KeyEvent.wVirtualScanCode && Rec->Event.KeyEvent.wVirtualKeyCode
				/*&& (Rec->Event.KeyEvent.wVirtualKeyCode == VK_SHIFT
				|| Rec->Event.KeyEvent.wVirtualKeyCode == VK_CONTROL || Rec->Event.KeyEvent.wVirtualKeyCode == VK_RCONTROL
				|| Rec->Event.KeyEvent.wVirtualKeyCode == VK_MENU || Rec->Event.KeyEvent.wVirtualKeyCode == VK_RMENU)*/)
		{
			switch (Rec->Event.KeyEvent.wVirtualKeyCode)
			{
			case VK_SHIFT: Key2 = KEY_SHIFT; break;
			case VK_CONTROL: Key2 = KEY_CTRL; break;
			case VK_MENU: Key2 = KEY_ALT; break;
			case VK_RCONTROL: Key2 = KEY_RCTRL; break;
			case VK_RMENU: Key2 = KEY_RALT; break;
			default: Key2 = Rec->Event.KeyEvent.wVirtualKeyCode;
			}
			if (Rec->Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED)
				Key2 |= KEY_CTRL;
			if (Rec->Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
				Key2 |= KEY_RCTRL;
			if (Rec->Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED)
				Key2 |= KEY_ALT;
			if (Rec->Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED)
				Key2 |= KEY_RALT;
			if (Rec->Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
				Key2 |= KEY_SHIFT;
		}
		else
		{
			//TODO: � ���� (2018) ������� �����: http://bugs.farmanager.com/view.php?id=1760
			//_ASSERTE(FALSE);
			Key2 = FSF3.FarInputRecordToKey(Rec);
		}
	}
	else
	{
		// � ���� (2018) ������� �����: http://bugs.farmanager.com/view.php?id=1760
		_ASSERTE(FALSE);
		Key2 = FSF3.FarInputRecordToKey(Rec);
	}

	//TODO: Ctrl/Shift/Alt?
	//DWORD FShift = Key2 & 0x7F000000; // ������� ��� ������������ � ������ �����!
	//ControlState =
	//	(FShift & KEY_SHIFT ? Far2::PKF_SHIFT : 0)|
	//	(FShift & KEY_ALT ? Far2::PKF_ALT : 0)|
	//	(FShift & KEY_CTRL ? Far2::PKF_CONTROL : 0);
#endif
#endif

	return Key2;
}

#if MVV_3>=2103
size_t WrapPluginInfo::FarKeyToName3(int Key2,wchar_t *KeyText,size_t Size)
{
	if (!FarInputRecordToName)
	{
		_ASSERTE(FarInputRecordToName!=NULL);
		if (KeyText && Size)
			*KeyText = 0;
		return 0;
	}
	INPUT_RECORD r = {};
	int VirtKey = 0, ControlState = 0;
	TranslateKeyToVK(Key2, VirtKey, ControlState, &r);
	return FarInputRecordToName(&r, KeyText, Size);
}

int WrapPluginInfo::FarNameToKey3(const wchar_t *Name)
{
#ifdef _DEBUG
	if (lstrcmpi(Name, L"RAltBS")==0)
	{
		int Dbg = 1;
	}
#endif
	if (!FarNameToInputRecord)
	{
		_ASSERTE(FarNameToInputRecord!=NULL);
		return -1;
	}

	INPUT_RECORD r = {};
	if (!FarNameToInputRecord(Name, &r))
	{
		return -1;
	}

	int Key2 = FarKey_3_2(&r);
#ifdef _DEBUG
	wchar_t szBackName[128];
	size_t nBackSize = FarKeyToName3(Key2, szBackName, ARRAYSIZE(szBackName));
	int nCmp = lstrcmpi(Name, szBackName);
	if (nCmp != 0)
	{
		static bool bFirstCall = false;
		if (!bFirstCall)
		{
			bFirstCall = true;
			_ASSERTE(nCmp==0);
		}
	}
#endif
	return Key2;
}
#endif // #if MVV_3>=2103

int WrapPluginInfo::FarColorIndex_2_3(int ColorIndex2)
{
	int ColorIndex3;
	switch (ColorIndex2)
	{
	case Far2::COL_MENUTEXT: ColorIndex3 = COL_MENUTEXT; break;
	case Far2::COL_MENUSELECTEDTEXT: ColorIndex3 = COL_MENUSELECTEDTEXT; break;
	case Far2::COL_MENUHIGHLIGHT: ColorIndex3 = COL_MENUHIGHLIGHT; break;
	case Far2::COL_MENUSELECTEDHIGHLIGHT: ColorIndex3 = COL_MENUSELECTEDHIGHLIGHT; break;
	case Far2::COL_MENUBOX: ColorIndex3 = COL_MENUBOX; break;
	case Far2::COL_MENUTITLE: ColorIndex3 = COL_MENUTITLE; break;

	case Far2::COL_HMENUTEXT: ColorIndex3 = COL_HMENUTEXT; break;
	case Far2::COL_HMENUSELECTEDTEXT: ColorIndex3 = COL_HMENUSELECTEDTEXT; break;
	case Far2::COL_HMENUHIGHLIGHT: ColorIndex3 = COL_HMENUHIGHLIGHT; break;
	case Far2::COL_HMENUSELECTEDHIGHLIGHT: ColorIndex3 = COL_HMENUSELECTEDHIGHLIGHT; break;

	case Far2::COL_PANELTEXT: ColorIndex3 = COL_PANELTEXT; break;
	case Far2::COL_PANELSELECTEDTEXT: ColorIndex3 = COL_PANELSELECTEDTEXT; break;
	case Far2::COL_PANELHIGHLIGHTTEXT: ColorIndex3 = COL_PANELHIGHLIGHTTEXT; break;
	case Far2::COL_PANELINFOTEXT: ColorIndex3 = COL_PANELINFOTEXT; break;
	case Far2::COL_PANELCURSOR: ColorIndex3 = COL_PANELCURSOR; break;
	case Far2::COL_PANELSELECTEDCURSOR: ColorIndex3 = COL_PANELSELECTEDCURSOR; break;
	case Far2::COL_PANELTITLE: ColorIndex3 = COL_PANELTITLE; break;
	case Far2::COL_PANELSELECTEDTITLE: ColorIndex3 = COL_PANELSELECTEDTITLE; break;
	case Far2::COL_PANELCOLUMNTITLE: ColorIndex3 = COL_PANELCOLUMNTITLE; break;
	case Far2::COL_PANELTOTALINFO: ColorIndex3 = COL_PANELTOTALINFO; break;
	case Far2::COL_PANELSELECTEDINFO: ColorIndex3 = COL_PANELSELECTEDINFO; break;

	case Far2::COL_DIALOGTEXT: ColorIndex3 = COL_DIALOGTEXT; break;
	case Far2::COL_DIALOGHIGHLIGHTTEXT: ColorIndex3 = COL_DIALOGHIGHLIGHTTEXT; break;
	case Far2::COL_DIALOGBOX: ColorIndex3 = COL_DIALOGBOX; break;
	case Far2::COL_DIALOGBOXTITLE: ColorIndex3 = COL_DIALOGBOXTITLE; break;
	case Far2::COL_DIALOGHIGHLIGHTBOXTITLE: ColorIndex3 = COL_DIALOGHIGHLIGHTBOXTITLE; break;
	case Far2::COL_DIALOGEDIT: ColorIndex3 = COL_DIALOGEDIT; break;
	case Far2::COL_DIALOGBUTTON: ColorIndex3 = COL_DIALOGBUTTON; break;
	case Far2::COL_DIALOGSELECTEDBUTTON: ColorIndex3 = COL_DIALOGSELECTEDBUTTON; break;
	case Far2::COL_DIALOGHIGHLIGHTBUTTON: ColorIndex3 = COL_DIALOGHIGHLIGHTBUTTON; break;
	case Far2::COL_DIALOGHIGHLIGHTSELECTEDBUTTON: ColorIndex3 = COL_DIALOGHIGHLIGHTSELECTEDBUTTON; break;

	case Far2::COL_DIALOGLISTTEXT: ColorIndex3 = COL_DIALOGLISTTEXT; break;
	case Far2::COL_DIALOGLISTSELECTEDTEXT: ColorIndex3 = COL_DIALOGLISTSELECTEDTEXT; break;
	case Far2::COL_DIALOGLISTHIGHLIGHT: ColorIndex3 = COL_DIALOGLISTHIGHLIGHT; break;
	case Far2::COL_DIALOGLISTSELECTEDHIGHLIGHT: ColorIndex3 = COL_DIALOGLISTSELECTEDHIGHLIGHT; break;

	case Far2::COL_WARNDIALOGTEXT: ColorIndex3 = COL_WARNDIALOGTEXT; break;
	case Far2::COL_WARNDIALOGHIGHLIGHTTEXT: ColorIndex3 = COL_WARNDIALOGHIGHLIGHTTEXT; break;
	case Far2::COL_WARNDIALOGBOX: ColorIndex3 = COL_WARNDIALOGBOX; break;
	case Far2::COL_WARNDIALOGBOXTITLE: ColorIndex3 = COL_WARNDIALOGBOXTITLE; break;
	case Far2::COL_WARNDIALOGHIGHLIGHTBOXTITLE: ColorIndex3 = COL_WARNDIALOGHIGHLIGHTBOXTITLE; break;
	case Far2::COL_WARNDIALOGEDIT: ColorIndex3 = COL_WARNDIALOGEDIT; break;
	case Far2::COL_WARNDIALOGBUTTON: ColorIndex3 = COL_WARNDIALOGBUTTON; break;
	case Far2::COL_WARNDIALOGSELECTEDBUTTON: ColorIndex3 = COL_WARNDIALOGSELECTEDBUTTON; break;
	case Far2::COL_WARNDIALOGHIGHLIGHTBUTTON: ColorIndex3 = COL_WARNDIALOGHIGHLIGHTBUTTON; break;
	case Far2::COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON: ColorIndex3 = COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON; break;

	case Far2::COL_KEYBARNUM: ColorIndex3 = COL_KEYBARNUM; break;
	case Far2::COL_KEYBARTEXT: ColorIndex3 = COL_KEYBARTEXT; break;
	case Far2::COL_KEYBARBACKGROUND: ColorIndex3 = COL_KEYBARBACKGROUND; break;

	case Far2::COL_COMMANDLINE: ColorIndex3 = COL_COMMANDLINE; break;

	case Far2::COL_CLOCK: ColorIndex3 = COL_CLOCK; break;

	case Far2::COL_VIEWERTEXT: ColorIndex3 = COL_VIEWERTEXT; break;
	case Far2::COL_VIEWERSELECTEDTEXT: ColorIndex3 = COL_VIEWERSELECTEDTEXT; break;
	case Far2::COL_VIEWERSTATUS: ColorIndex3 = COL_VIEWERSTATUS; break;

	case Far2::COL_EDITORTEXT: ColorIndex3 = COL_EDITORTEXT; break;
	case Far2::COL_EDITORSELECTEDTEXT: ColorIndex3 = COL_EDITORSELECTEDTEXT; break;
	case Far2::COL_EDITORSTATUS: ColorIndex3 = COL_EDITORSTATUS; break;

	case Far2::COL_HELPTEXT: ColorIndex3 = COL_HELPTEXT; break;
	case Far2::COL_HELPHIGHLIGHTTEXT: ColorIndex3 = COL_HELPHIGHLIGHTTEXT; break;
	case Far2::COL_HELPTOPIC: ColorIndex3 = COL_HELPTOPIC; break;
	case Far2::COL_HELPSELECTEDTOPIC: ColorIndex3 = COL_HELPSELECTEDTOPIC; break;
	case Far2::COL_HELPBOX: ColorIndex3 = COL_HELPBOX; break;
	case Far2::COL_HELPBOXTITLE: ColorIndex3 = COL_HELPBOXTITLE; break;

	case Far2::COL_PANELDRAGTEXT: ColorIndex3 = COL_PANELDRAGTEXT; break;
	case Far2::COL_DIALOGEDITUNCHANGED: ColorIndex3 = COL_DIALOGEDITUNCHANGED; break;
	case Far2::COL_PANELSCROLLBAR: ColorIndex3 = COL_PANELSCROLLBAR; break;
	case Far2::COL_HELPSCROLLBAR: ColorIndex3 = COL_HELPSCROLLBAR; break;
	case Far2::COL_PANELBOX: ColorIndex3 = COL_PANELBOX; break;
	case Far2::COL_PANELSCREENSNUMBER: ColorIndex3 = COL_PANELSCREENSNUMBER; break;
	case Far2::COL_DIALOGEDITSELECTED: ColorIndex3 = COL_DIALOGEDITSELECTED; break;
	case Far2::COL_COMMANDLINESELECTED: ColorIndex3 = COL_COMMANDLINESELECTED; break;
	case Far2::COL_VIEWERARROWS: ColorIndex3 = COL_VIEWERARROWS; break;

	case Far2::COL_DIALOGLISTSCROLLBAR: ColorIndex3 = COL_DIALOGLISTSCROLLBAR; break;
	case Far2::COL_MENUSCROLLBAR: ColorIndex3 = COL_MENUSCROLLBAR; break;
	case Far2::COL_VIEWERSCROLLBAR: ColorIndex3 = COL_VIEWERSCROLLBAR; break;
	case Far2::COL_COMMANDLINEPREFIX: ColorIndex3 = COL_COMMANDLINEPREFIX; break;
	case Far2::COL_DIALOGDISABLED: ColorIndex3 = COL_DIALOGDISABLED; break;
	case Far2::COL_DIALOGEDITDISABLED: ColorIndex3 = COL_DIALOGEDITDISABLED; break;
	case Far2::COL_DIALOGLISTDISABLED: ColorIndex3 = COL_DIALOGLISTDISABLED; break;
	case Far2::COL_WARNDIALOGDISABLED: ColorIndex3 = COL_WARNDIALOGDISABLED; break;
	case Far2::COL_WARNDIALOGEDITDISABLED: ColorIndex3 = COL_WARNDIALOGEDITDISABLED; break;
	case Far2::COL_WARNDIALOGLISTDISABLED: ColorIndex3 = COL_WARNDIALOGLISTDISABLED; break;

	case Far2::COL_MENUDISABLEDTEXT: ColorIndex3 = COL_MENUDISABLEDTEXT; break;

	case Far2::COL_EDITORCLOCK: ColorIndex3 = COL_EDITORCLOCK; break;
	case Far2::COL_VIEWERCLOCK: ColorIndex3 = COL_VIEWERCLOCK; break;

	case Far2::COL_DIALOGLISTTITLE: ColorIndex3 = COL_DIALOGLISTTITLE; break;
	case Far2::COL_DIALOGLISTBOX: ColorIndex3 = COL_DIALOGLISTBOX; break;

	case Far2::COL_WARNDIALOGEDITSELECTED: ColorIndex3 = COL_WARNDIALOGEDITSELECTED; break;
	case Far2::COL_WARNDIALOGEDITUNCHANGED: ColorIndex3 = COL_WARNDIALOGEDITUNCHANGED; break;

	case Far2::COL_DIALOGCOMBOTEXT: ColorIndex3 = COL_DIALOGCOMBOTEXT; break;
	case Far2::COL_DIALOGCOMBOSELECTEDTEXT: ColorIndex3 = COL_DIALOGCOMBOSELECTEDTEXT; break;
	case Far2::COL_DIALOGCOMBOHIGHLIGHT: ColorIndex3 = COL_DIALOGCOMBOHIGHLIGHT; break;
	case Far2::COL_DIALOGCOMBOSELECTEDHIGHLIGHT: ColorIndex3 = COL_DIALOGCOMBOSELECTEDHIGHLIGHT; break;
	case Far2::COL_DIALOGCOMBOBOX: ColorIndex3 = COL_DIALOGCOMBOBOX; break;
	case Far2::COL_DIALOGCOMBOTITLE: ColorIndex3 = COL_DIALOGCOMBOTITLE; break;
	case Far2::COL_DIALOGCOMBODISABLED: ColorIndex3 = COL_DIALOGCOMBODISABLED; break;
	case Far2::COL_DIALOGCOMBOSCROLLBAR: ColorIndex3 = COL_DIALOGCOMBOSCROLLBAR; break;

	case Far2::COL_WARNDIALOGLISTTEXT: ColorIndex3 = COL_WARNDIALOGLISTTEXT; break;
	case Far2::COL_WARNDIALOGLISTSELECTEDTEXT: ColorIndex3 = COL_WARNDIALOGLISTSELECTEDTEXT; break;
	case Far2::COL_WARNDIALOGLISTHIGHLIGHT: ColorIndex3 = COL_WARNDIALOGLISTHIGHLIGHT; break;
	case Far2::COL_WARNDIALOGLISTSELECTEDHIGHLIGHT: ColorIndex3 = COL_WARNDIALOGLISTSELECTEDHIGHLIGHT; break;
	case Far2::COL_WARNDIALOGLISTBOX: ColorIndex3 = COL_WARNDIALOGLISTBOX; break;
	case Far2::COL_WARNDIALOGLISTTITLE: ColorIndex3 = COL_WARNDIALOGLISTTITLE; break;
	case Far2::COL_WARNDIALOGLISTSCROLLBAR: ColorIndex3 = COL_WARNDIALOGLISTSCROLLBAR; break;

	case Far2::COL_WARNDIALOGCOMBOTEXT: ColorIndex3 = COL_WARNDIALOGCOMBOTEXT; break;
	case Far2::COL_WARNDIALOGCOMBOSELECTEDTEXT: ColorIndex3 = COL_WARNDIALOGCOMBOSELECTEDTEXT; break;
	case Far2::COL_WARNDIALOGCOMBOHIGHLIGHT: ColorIndex3 = COL_WARNDIALOGCOMBOHIGHLIGHT; break;
	case Far2::COL_WARNDIALOGCOMBOSELECTEDHIGHLIGHT: ColorIndex3 = COL_WARNDIALOGCOMBOSELECTEDHIGHLIGHT; break;
	case Far2::COL_WARNDIALOGCOMBOBOX: ColorIndex3 = COL_WARNDIALOGCOMBOBOX; break;
	case Far2::COL_WARNDIALOGCOMBOTITLE: ColorIndex3 = COL_WARNDIALOGCOMBOTITLE; break;
	case Far2::COL_WARNDIALOGCOMBODISABLED: ColorIndex3 = COL_WARNDIALOGCOMBODISABLED; break;
	case Far2::COL_WARNDIALOGCOMBOSCROLLBAR: ColorIndex3 = COL_WARNDIALOGCOMBOSCROLLBAR; break;

	case Far2::COL_DIALOGLISTARROWS: ColorIndex3 = COL_DIALOGLISTARROWS; break;
	case Far2::COL_DIALOGLISTARROWSDISABLED: ColorIndex3 = COL_DIALOGLISTARROWSDISABLED; break;
	case Far2::COL_DIALOGLISTARROWSSELECTED: ColorIndex3 = COL_DIALOGLISTARROWSSELECTED; break;
	case Far2::COL_DIALOGCOMBOARROWS: ColorIndex3 = COL_DIALOGCOMBOARROWS; break;
	case Far2::COL_DIALOGCOMBOARROWSDISABLED: ColorIndex3 = COL_DIALOGCOMBOARROWSDISABLED; break;
	case Far2::COL_DIALOGCOMBOARROWSSELECTED: ColorIndex3 = COL_DIALOGCOMBOARROWSSELECTED; break;
	case Far2::COL_WARNDIALOGLISTARROWS: ColorIndex3 = COL_WARNDIALOGLISTARROWS; break;
	case Far2::COL_WARNDIALOGLISTARROWSDISABLED: ColorIndex3 = COL_WARNDIALOGLISTARROWSDISABLED; break;
	case Far2::COL_WARNDIALOGLISTARROWSSELECTED: ColorIndex3 = COL_WARNDIALOGLISTARROWSSELECTED; break;
	case Far2::COL_WARNDIALOGCOMBOARROWS: ColorIndex3 = COL_WARNDIALOGCOMBOARROWS; break;
	case Far2::COL_WARNDIALOGCOMBOARROWSDISABLED: ColorIndex3 = COL_WARNDIALOGCOMBOARROWSDISABLED; break;
	case Far2::COL_WARNDIALOGCOMBOARROWSSELECTED: ColorIndex3 = COL_WARNDIALOGCOMBOARROWSSELECTED; break;
	case Far2::COL_MENUARROWS: ColorIndex3 = COL_MENUARROWS; break;
	case Far2::COL_MENUARROWSDISABLED: ColorIndex3 = COL_MENUARROWSDISABLED; break;
	case Far2::COL_MENUARROWSSELECTED: ColorIndex3 = COL_MENUARROWSSELECTED; break;
	case Far2::COL_COMMANDLINEUSERSCREEN: ColorIndex3 = COL_COMMANDLINEUSERSCREEN; break;
	case Far2::COL_EDITORSCROLLBAR: ColorIndex3 = COL_EDITORSCROLLBAR; break;

	case Far2::COL_MENUGRAYTEXT: ColorIndex3 = COL_MENUGRAYTEXT; break;
	case Far2::COL_MENUSELECTEDGRAYTEXT: ColorIndex3 = COL_MENUSELECTEDGRAYTEXT; break;
	case Far2::COL_DIALOGCOMBOGRAY: ColorIndex3 = COL_DIALOGCOMBOGRAY; break;
	case Far2::COL_DIALOGCOMBOSELECTEDGRAYTEXT: ColorIndex3 = COL_DIALOGCOMBOSELECTEDGRAYTEXT; break;
	case Far2::COL_DIALOGLISTGRAY: ColorIndex3 = COL_DIALOGLISTGRAY; break;
	case Far2::COL_DIALOGLISTSELECTEDGRAYTEXT: ColorIndex3 = COL_DIALOGLISTSELECTEDGRAYTEXT; break;
	case Far2::COL_WARNDIALOGCOMBOGRAY: ColorIndex3 = COL_WARNDIALOGCOMBOGRAY; break;
	case Far2::COL_WARNDIALOGCOMBOSELECTEDGRAYTEXT: ColorIndex3 = COL_WARNDIALOGCOMBOSELECTEDGRAYTEXT; break;
	case Far2::COL_WARNDIALOGLISTGRAY: ColorIndex3 = COL_WARNDIALOGLISTGRAY; break;
	case Far2::COL_WARNDIALOGLISTSELECTEDGRAYTEXT: ColorIndex3 = COL_WARNDIALOGLISTSELECTEDGRAYTEXT; break;

	case Far2::COL_DIALOGDEFAULTBUTTON: ColorIndex3 = COL_DIALOGDEFAULTBUTTON; break;
	case Far2::COL_DIALOGSELECTEDDEFAULTBUTTON: ColorIndex3 = COL_DIALOGSELECTEDDEFAULTBUTTON; break;
	case Far2::COL_DIALOGHIGHLIGHTDEFAULTBUTTON: ColorIndex3 = COL_DIALOGHIGHLIGHTDEFAULTBUTTON; break;
	case Far2::COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON: ColorIndex3 = COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON; break;
	case Far2::COL_WARNDIALOGDEFAULTBUTTON: ColorIndex3 = COL_WARNDIALOGDEFAULTBUTTON; break;
	case Far2::COL_WARNDIALOGSELECTEDDEFAULTBUTTON: ColorIndex3 = COL_WARNDIALOGSELECTEDDEFAULTBUTTON; break;
	case Far2::COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON: ColorIndex3 = COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON; break;
	case Far2::COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON: ColorIndex3 = COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON; break;

	case Far2::COL_LASTPALETTECOLOR: ColorIndex3 = COL_LASTPALETTECOLOR; break;

	default:
		ColorIndex3 = COL_LASTPALETTECOLOR;
	}
	return ColorIndex3;
}

int WrapPluginInfo::FarColorIndex_3_2(int ColorIndex3)
{
	int ColorIndex2;
	switch (ColorIndex3)
	{
	case COL_MENUTEXT: ColorIndex2 = Far2::COL_MENUTEXT; break;
	case COL_MENUSELECTEDTEXT: ColorIndex2 = Far2::COL_MENUSELECTEDTEXT; break;
	case COL_MENUHIGHLIGHT: ColorIndex2 = Far2::COL_MENUHIGHLIGHT; break;
	case COL_MENUSELECTEDHIGHLIGHT: ColorIndex2 = Far2::COL_MENUSELECTEDHIGHLIGHT; break;
	case COL_MENUBOX: ColorIndex2 = Far2::COL_MENUBOX; break;
	case COL_MENUTITLE: ColorIndex2 = Far2::COL_MENUTITLE; break;

	case COL_HMENUTEXT: ColorIndex2 = Far2::COL_HMENUTEXT; break;
	case COL_HMENUSELECTEDTEXT: ColorIndex2 = Far2::COL_HMENUSELECTEDTEXT; break;
	case COL_HMENUHIGHLIGHT: ColorIndex2 = Far2::COL_HMENUHIGHLIGHT; break;
	case COL_HMENUSELECTEDHIGHLIGHT: ColorIndex2 = Far2::COL_HMENUSELECTEDHIGHLIGHT; break;

	case COL_PANELTEXT: ColorIndex2 = Far2::COL_PANELTEXT; break;
	case COL_PANELSELECTEDTEXT: ColorIndex2 = Far2::COL_PANELSELECTEDTEXT; break;
	case COL_PANELHIGHLIGHTTEXT: ColorIndex2 = Far2::COL_PANELHIGHLIGHTTEXT; break;
	case COL_PANELINFOTEXT: ColorIndex2 = Far2::COL_PANELINFOTEXT; break;
	case COL_PANELCURSOR: ColorIndex2 = Far2::COL_PANELCURSOR; break;
	case COL_PANELSELECTEDCURSOR: ColorIndex2 = Far2::COL_PANELSELECTEDCURSOR; break;
	case COL_PANELTITLE: ColorIndex2 = Far2::COL_PANELTITLE; break;
	case COL_PANELSELECTEDTITLE: ColorIndex2 = Far2::COL_PANELSELECTEDTITLE; break;
	case COL_PANELCOLUMNTITLE: ColorIndex2 = Far2::COL_PANELCOLUMNTITLE; break;
	case COL_PANELTOTALINFO: ColorIndex2 = Far2::COL_PANELTOTALINFO; break;
	case COL_PANELSELECTEDINFO: ColorIndex2 = Far2::COL_PANELSELECTEDINFO; break;

	case COL_DIALOGTEXT: ColorIndex2 = Far2::COL_DIALOGTEXT; break;
	case COL_DIALOGHIGHLIGHTTEXT: ColorIndex2 = Far2::COL_DIALOGHIGHLIGHTTEXT; break;
	case COL_DIALOGBOX: ColorIndex2 = Far2::COL_DIALOGBOX; break;
	case COL_DIALOGBOXTITLE: ColorIndex2 = Far2::COL_DIALOGBOXTITLE; break;
	case COL_DIALOGHIGHLIGHTBOXTITLE: ColorIndex2 = Far2::COL_DIALOGHIGHLIGHTBOXTITLE; break;
	case COL_DIALOGEDIT: ColorIndex2 = Far2::COL_DIALOGEDIT; break;
	case COL_DIALOGBUTTON: ColorIndex2 = Far2::COL_DIALOGBUTTON; break;
	case COL_DIALOGSELECTEDBUTTON: ColorIndex2 = Far2::COL_DIALOGSELECTEDBUTTON; break;
	case COL_DIALOGHIGHLIGHTBUTTON: ColorIndex2 = Far2::COL_DIALOGHIGHLIGHTBUTTON; break;
	case COL_DIALOGHIGHLIGHTSELECTEDBUTTON: ColorIndex2 = Far2::COL_DIALOGHIGHLIGHTSELECTEDBUTTON; break;

	case COL_DIALOGLISTTEXT: ColorIndex2 = Far2::COL_DIALOGLISTTEXT; break;
	case COL_DIALOGLISTSELECTEDTEXT: ColorIndex2 = Far2::COL_DIALOGLISTSELECTEDTEXT; break;
	case COL_DIALOGLISTHIGHLIGHT: ColorIndex2 = Far2::COL_DIALOGLISTHIGHLIGHT; break;
	case COL_DIALOGLISTSELECTEDHIGHLIGHT: ColorIndex2 = Far2::COL_DIALOGLISTSELECTEDHIGHLIGHT; break;

	case COL_WARNDIALOGTEXT: ColorIndex2 = Far2::COL_WARNDIALOGTEXT; break;
	case COL_WARNDIALOGHIGHLIGHTTEXT: ColorIndex2 = Far2::COL_WARNDIALOGHIGHLIGHTTEXT; break;
	case COL_WARNDIALOGBOX: ColorIndex2 = Far2::COL_WARNDIALOGBOX; break;
	case COL_WARNDIALOGBOXTITLE: ColorIndex2 = Far2::COL_WARNDIALOGBOXTITLE; break;
	case COL_WARNDIALOGHIGHLIGHTBOXTITLE: ColorIndex2 = Far2::COL_WARNDIALOGHIGHLIGHTBOXTITLE; break;
	case COL_WARNDIALOGEDIT: ColorIndex2 = Far2::COL_WARNDIALOGEDIT; break;
	case COL_WARNDIALOGBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGBUTTON; break;
	case COL_WARNDIALOGSELECTEDBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGSELECTEDBUTTON; break;
	case COL_WARNDIALOGHIGHLIGHTBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGHIGHLIGHTBUTTON; break;
	case COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGHIGHLIGHTSELECTEDBUTTON; break;

	case COL_KEYBARNUM: ColorIndex2 = Far2::COL_KEYBARNUM; break;
	case COL_KEYBARTEXT: ColorIndex2 = Far2::COL_KEYBARTEXT; break;
	case COL_KEYBARBACKGROUND: ColorIndex2 = Far2::COL_KEYBARBACKGROUND; break;

	case COL_COMMANDLINE: ColorIndex2 = Far2::COL_COMMANDLINE; break;

	case COL_CLOCK: ColorIndex2 = Far2::COL_CLOCK; break;

	case COL_VIEWERTEXT: ColorIndex2 = Far2::COL_VIEWERTEXT; break;
	case COL_VIEWERSELECTEDTEXT: ColorIndex2 = Far2::COL_VIEWERSELECTEDTEXT; break;
	case COL_VIEWERSTATUS: ColorIndex2 = Far2::COL_VIEWERSTATUS; break;

	case COL_EDITORTEXT: ColorIndex2 = Far2::COL_EDITORTEXT; break;
	case COL_EDITORSELECTEDTEXT: ColorIndex2 = Far2::COL_EDITORSELECTEDTEXT; break;
	case COL_EDITORSTATUS: ColorIndex2 = Far2::COL_EDITORSTATUS; break;

	case COL_HELPTEXT: ColorIndex2 = Far2::COL_HELPTEXT; break;
	case COL_HELPHIGHLIGHTTEXT: ColorIndex2 = Far2::COL_HELPHIGHLIGHTTEXT; break;
	case COL_HELPTOPIC: ColorIndex2 = Far2::COL_HELPTOPIC; break;
	case COL_HELPSELECTEDTOPIC: ColorIndex2 = Far2::COL_HELPSELECTEDTOPIC; break;
	case COL_HELPBOX: ColorIndex2 = Far2::COL_HELPBOX; break;
	case COL_HELPBOXTITLE: ColorIndex2 = Far2::COL_HELPBOXTITLE; break;

	case COL_PANELDRAGTEXT: ColorIndex2 = Far2::COL_PANELDRAGTEXT; break;
	case COL_DIALOGEDITUNCHANGED: ColorIndex2 = Far2::COL_DIALOGEDITUNCHANGED; break;
	case COL_PANELSCROLLBAR: ColorIndex2 = Far2::COL_PANELSCROLLBAR; break;
	case COL_HELPSCROLLBAR: ColorIndex2 = Far2::COL_HELPSCROLLBAR; break;
	case COL_PANELBOX: ColorIndex2 = Far2::COL_PANELBOX; break;
	case COL_PANELSCREENSNUMBER: ColorIndex2 = Far2::COL_PANELSCREENSNUMBER; break;
	case COL_DIALOGEDITSELECTED: ColorIndex2 = Far2::COL_DIALOGEDITSELECTED; break;
	case COL_COMMANDLINESELECTED: ColorIndex2 = Far2::COL_COMMANDLINESELECTED; break;
	case COL_VIEWERARROWS: ColorIndex2 = Far2::COL_VIEWERARROWS; break;

	case COL_DIALOGLISTSCROLLBAR: ColorIndex2 = Far2::COL_DIALOGLISTSCROLLBAR; break;
	case COL_MENUSCROLLBAR: ColorIndex2 = Far2::COL_MENUSCROLLBAR; break;
	case COL_VIEWERSCROLLBAR: ColorIndex2 = Far2::COL_VIEWERSCROLLBAR; break;
	case COL_COMMANDLINEPREFIX: ColorIndex2 = Far2::COL_COMMANDLINEPREFIX; break;
	case COL_DIALOGDISABLED: ColorIndex2 = Far2::COL_DIALOGDISABLED; break;
	case COL_DIALOGEDITDISABLED: ColorIndex2 = Far2::COL_DIALOGEDITDISABLED; break;
	case COL_DIALOGLISTDISABLED: ColorIndex2 = Far2::COL_DIALOGLISTDISABLED; break;
	case COL_WARNDIALOGDISABLED: ColorIndex2 = Far2::COL_WARNDIALOGDISABLED; break;
	case COL_WARNDIALOGEDITDISABLED: ColorIndex2 = Far2::COL_WARNDIALOGEDITDISABLED; break;
	case COL_WARNDIALOGLISTDISABLED: ColorIndex2 = Far2::COL_WARNDIALOGLISTDISABLED; break;

	case COL_MENUDISABLEDTEXT: ColorIndex2 = Far2::COL_MENUDISABLEDTEXT; break;

	case COL_EDITORCLOCK: ColorIndex2 = Far2::COL_EDITORCLOCK; break;
	case COL_VIEWERCLOCK: ColorIndex2 = Far2::COL_VIEWERCLOCK; break;

	case COL_DIALOGLISTTITLE: ColorIndex2 = Far2::COL_DIALOGLISTTITLE; break;
	case COL_DIALOGLISTBOX: ColorIndex2 = Far2::COL_DIALOGLISTBOX; break;

	case COL_WARNDIALOGEDITSELECTED: ColorIndex2 = Far2::COL_WARNDIALOGEDITSELECTED; break;
	case COL_WARNDIALOGEDITUNCHANGED: ColorIndex2 = Far2::COL_WARNDIALOGEDITUNCHANGED; break;

	case COL_DIALOGCOMBOTEXT: ColorIndex2 = Far2::COL_DIALOGCOMBOTEXT; break;
	case COL_DIALOGCOMBOSELECTEDTEXT: ColorIndex2 = Far2::COL_DIALOGCOMBOSELECTEDTEXT; break;
	case COL_DIALOGCOMBOHIGHLIGHT: ColorIndex2 = Far2::COL_DIALOGCOMBOHIGHLIGHT; break;
	case COL_DIALOGCOMBOSELECTEDHIGHLIGHT: ColorIndex2 = Far2::COL_DIALOGCOMBOSELECTEDHIGHLIGHT; break;
	case COL_DIALOGCOMBOBOX: ColorIndex2 = Far2::COL_DIALOGCOMBOBOX; break;
	case COL_DIALOGCOMBOTITLE: ColorIndex2 = Far2::COL_DIALOGCOMBOTITLE; break;
	case COL_DIALOGCOMBODISABLED: ColorIndex2 = Far2::COL_DIALOGCOMBODISABLED; break;
	case COL_DIALOGCOMBOSCROLLBAR: ColorIndex2 = Far2::COL_DIALOGCOMBOSCROLLBAR; break;

	case COL_WARNDIALOGLISTTEXT: ColorIndex2 = Far2::COL_WARNDIALOGLISTTEXT; break;
	case COL_WARNDIALOGLISTSELECTEDTEXT: ColorIndex2 = Far2::COL_WARNDIALOGLISTSELECTEDTEXT; break;
	case COL_WARNDIALOGLISTHIGHLIGHT: ColorIndex2 = Far2::COL_WARNDIALOGLISTHIGHLIGHT; break;
	case COL_WARNDIALOGLISTSELECTEDHIGHLIGHT: ColorIndex2 = Far2::COL_WARNDIALOGLISTSELECTEDHIGHLIGHT; break;
	case COL_WARNDIALOGLISTBOX: ColorIndex2 = Far2::COL_WARNDIALOGLISTBOX; break;
	case COL_WARNDIALOGLISTTITLE: ColorIndex2 = Far2::COL_WARNDIALOGLISTTITLE; break;
	case COL_WARNDIALOGLISTSCROLLBAR: ColorIndex2 = Far2::COL_WARNDIALOGLISTSCROLLBAR; break;

	case COL_WARNDIALOGCOMBOTEXT: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOTEXT; break;
	case COL_WARNDIALOGCOMBOSELECTEDTEXT: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOSELECTEDTEXT; break;
	case COL_WARNDIALOGCOMBOHIGHLIGHT: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOHIGHLIGHT; break;
	case COL_WARNDIALOGCOMBOSELECTEDHIGHLIGHT: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOSELECTEDHIGHLIGHT; break;
	case COL_WARNDIALOGCOMBOBOX: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOBOX; break;
	case COL_WARNDIALOGCOMBOTITLE: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOTITLE; break;
	case COL_WARNDIALOGCOMBODISABLED: ColorIndex2 = Far2::COL_WARNDIALOGCOMBODISABLED; break;
	case COL_WARNDIALOGCOMBOSCROLLBAR: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOSCROLLBAR; break;

	case COL_DIALOGLISTARROWS: ColorIndex2 = Far2::COL_DIALOGLISTARROWS; break;
	case COL_DIALOGLISTARROWSDISABLED: ColorIndex2 = Far2::COL_DIALOGLISTARROWSDISABLED; break;
	case COL_DIALOGLISTARROWSSELECTED: ColorIndex2 = Far2::COL_DIALOGLISTARROWSSELECTED; break;
	case COL_DIALOGCOMBOARROWS: ColorIndex2 = Far2::COL_DIALOGCOMBOARROWS; break;
	case COL_DIALOGCOMBOARROWSDISABLED: ColorIndex2 = Far2::COL_DIALOGCOMBOARROWSDISABLED; break;
	case COL_DIALOGCOMBOARROWSSELECTED: ColorIndex2 = Far2::COL_DIALOGCOMBOARROWSSELECTED; break;
	case COL_WARNDIALOGLISTARROWS: ColorIndex2 = Far2::COL_WARNDIALOGLISTARROWS; break;
	case COL_WARNDIALOGLISTARROWSDISABLED: ColorIndex2 = Far2::COL_WARNDIALOGLISTARROWSDISABLED; break;
	case COL_WARNDIALOGLISTARROWSSELECTED: ColorIndex2 = Far2::COL_WARNDIALOGLISTARROWSSELECTED; break;
	case COL_WARNDIALOGCOMBOARROWS: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOARROWS; break;
	case COL_WARNDIALOGCOMBOARROWSDISABLED: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOARROWSDISABLED; break;
	case COL_WARNDIALOGCOMBOARROWSSELECTED: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOARROWSSELECTED; break;
	case COL_MENUARROWS: ColorIndex2 = Far2::COL_MENUARROWS; break;
	case COL_MENUARROWSDISABLED: ColorIndex2 = Far2::COL_MENUARROWSDISABLED; break;
	case COL_MENUARROWSSELECTED: ColorIndex2 = Far2::COL_MENUARROWSSELECTED; break;
	case COL_COMMANDLINEUSERSCREEN: ColorIndex2 = Far2::COL_COMMANDLINEUSERSCREEN; break;
	case COL_EDITORSCROLLBAR: ColorIndex2 = Far2::COL_EDITORSCROLLBAR; break;

	case COL_MENUGRAYTEXT: ColorIndex2 = Far2::COL_MENUGRAYTEXT; break;
	case COL_MENUSELECTEDGRAYTEXT: ColorIndex2 = Far2::COL_MENUSELECTEDGRAYTEXT; break;
	case COL_DIALOGCOMBOGRAY: ColorIndex2 = Far2::COL_DIALOGCOMBOGRAY; break;
	case COL_DIALOGCOMBOSELECTEDGRAYTEXT: ColorIndex2 = Far2::COL_DIALOGCOMBOSELECTEDGRAYTEXT; break;
	case COL_DIALOGLISTGRAY: ColorIndex2 = Far2::COL_DIALOGLISTGRAY; break;
	case COL_DIALOGLISTSELECTEDGRAYTEXT: ColorIndex2 = Far2::COL_DIALOGLISTSELECTEDGRAYTEXT; break;
	case COL_WARNDIALOGCOMBOGRAY: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOGRAY; break;
	case COL_WARNDIALOGCOMBOSELECTEDGRAYTEXT: ColorIndex2 = Far2::COL_WARNDIALOGCOMBOSELECTEDGRAYTEXT; break;
	case COL_WARNDIALOGLISTGRAY: ColorIndex2 = Far2::COL_WARNDIALOGLISTGRAY; break;
	case COL_WARNDIALOGLISTSELECTEDGRAYTEXT: ColorIndex2 = Far2::COL_WARNDIALOGLISTSELECTEDGRAYTEXT; break;

	case COL_DIALOGDEFAULTBUTTON: ColorIndex2 = Far2::COL_DIALOGDEFAULTBUTTON; break;
	case COL_DIALOGSELECTEDDEFAULTBUTTON: ColorIndex2 = Far2::COL_DIALOGSELECTEDDEFAULTBUTTON; break;
	case COL_DIALOGHIGHLIGHTDEFAULTBUTTON: ColorIndex2 = Far2::COL_DIALOGHIGHLIGHTDEFAULTBUTTON; break;
	case COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON: ColorIndex2 = Far2::COL_DIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON; break;
	case COL_WARNDIALOGDEFAULTBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGDEFAULTBUTTON; break;
	case COL_WARNDIALOGSELECTEDDEFAULTBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGSELECTEDDEFAULTBUTTON; break;
	case COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGHIGHLIGHTDEFAULTBUTTON; break;
	case COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON: ColorIndex2 = Far2::COL_WARNDIALOGHIGHLIGHTSELECTEDDEFAULTBUTTON; break;

	case COL_LASTPALETTECOLOR: ColorIndex2 = Far2::COL_LASTPALETTECOLOR; break;

	default:
		ColorIndex2 = Far2::COL_LASTPALETTECOLOR;
	}
	return ColorIndex2;
}

FARDIALOGITEMTYPES WrapPluginInfo::DialogItemTypes_2_3(int ItemType2)
{
	FARDIALOGITEMTYPES ItemType3 = DI_TEXT;
	switch (ItemType2)
	{
	case Far2::DI_TEXT: ItemType3 = DI_TEXT; break;
	case Far2::DI_VTEXT: ItemType3 = DI_VTEXT; break;
	case Far2::DI_SINGLEBOX: ItemType3 = DI_SINGLEBOX; break;
	case Far2::DI_DOUBLEBOX: ItemType3 = DI_DOUBLEBOX; break;
	case Far2::DI_EDIT: ItemType3 = DI_EDIT; break;
	case Far2::DI_PSWEDIT: ItemType3 = DI_PSWEDIT; break;
	case Far2::DI_FIXEDIT: ItemType3 = DI_FIXEDIT; break;
	case Far2::DI_BUTTON: ItemType3 = DI_BUTTON; break;
	case Far2::DI_CHECKBOX: ItemType3 = DI_CHECKBOX; break;
	case Far2::DI_RADIOBUTTON: ItemType3 = DI_RADIOBUTTON; break;
	case Far2::DI_COMBOBOX: ItemType3 = DI_COMBOBOX; break;
	case Far2::DI_LISTBOX: ItemType3 = DI_LISTBOX; break;
	case Far2::DI_USERCONTROL: ItemType3 = DI_USERCONTROL; break;
	}
	return ItemType3;
}

int WrapPluginInfo::DialogItemTypes_3_2(FARDIALOGITEMTYPES ItemType3)
{
	int ItemType2 = DI_TEXT;
	switch (ItemType3)
	{
	case DI_TEXT: ItemType2 = Far2::DI_TEXT; break;
	case DI_VTEXT: ItemType2 = Far2::DI_VTEXT; break;
	case DI_SINGLEBOX: ItemType2 = Far2::DI_SINGLEBOX; break;
	case DI_DOUBLEBOX: ItemType2 = Far2::DI_DOUBLEBOX; break;
	case DI_EDIT: ItemType2 = Far2::DI_EDIT; break;
	case DI_PSWEDIT: ItemType2 = Far2::DI_PSWEDIT; break;
	case DI_FIXEDIT: ItemType2 = Far2::DI_FIXEDIT; break;
	case DI_BUTTON: ItemType2 = Far2::DI_BUTTON; break;
	case DI_CHECKBOX: ItemType2 = Far2::DI_CHECKBOX; break;
	case DI_RADIOBUTTON: ItemType2 = Far2::DI_RADIOBUTTON; break;
	case DI_COMBOBOX: ItemType2 = Far2::DI_COMBOBOX; break;
	case DI_LISTBOX: ItemType2 = Far2::DI_LISTBOX; break;
	case DI_USERCONTROL: ItemType2 = Far2::DI_USERCONTROL; break;
	}
	return ItemType2;
}

void WrapPluginInfo::FarColor_2_3(BYTE Color2, FarColor& Color3)
{
	ZeroStruct(Color3);
	Color3.Flags = FCF_FG_4BIT|FCF_BG_4BIT;
	Color3.ForegroundColor = Color2 & 0xF;
	MAKE_OPAQUE(Color3.ForegroundColor);
	Color3.BackgroundColor = (Color2 & 0xF0) >> 4;
	MAKE_OPAQUE(Color3.BackgroundColor);
}

//BYTE WrapPluginInfo::RefColorToIndex(COLORREF Color)
//{
//	COLORREF crDefConPalette[16] = 
//	{
//		0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
//		0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
//	};
//
//	for (int i = 0; i < ARRAYSIZE(crDefConPalette); i++)
//	{
//		if (crDefConPalette[i] == Color)
//		{
//			return i;
//		}
//	}
//
//	// ���� �� ������� �� ����� - �������� ��������
//	int B = (Color & 0xFF0000) >> 16;
//	int G = (Color & 0xFF00) >> 8;
//	int R = (Color & 0xFF);
//	int nMax = max(B,max(R,G));
//	
//	BYTE Pal =
//		(((B+32) > nMax) ? 1 : 0) |
//		(((G+32) > nMax) ? 2 : 0) |
//		(((R+32) > nMax) ? 4 : 0);
//
//	return Pal;
//}

//BYTE WrapPluginInfo::FarColor_3_2(const FarColor& Color3)
//{
//	BYTE Color2 = 0;
//
//	if (Color3.Flags & FCF_FG_4BIT)
//	{
//		Color2 |= (Color3.ForegroundColor & 0xF);
//	}
//	else
//	{
//		// ����� ��������� ������!
//		Color2 |= RefColorToIndex(Color3.ForegroundColor);
//	}
//
//	if (Color3.Flags & FCF_BG_4BIT)
//	{
//		Color2 |= ((Color3.BackgroundColor & 0xF) << 4);
//	}
//	else
//	{
//		// ����� ��������� ������!
//		Color2 |= RefColorToIndex(Color3.BackgroundColor);
//	}
//
//	//if ((Color3.Flags & (FCF_FG_4BIT|FCF_BG_4BIT)) == (FCF_FG_4BIT|FCF_BG_4BIT))
//	//{
//	//	Color2 = (Color3.ForegroundColor & 0xF) | ((Color3.BackgroundColor & 0xF) << 4);
//	//}
//	//else
//	//{
//	//	_ASSERTE((Color3.Flags & (FCF_FG_4BIT|FCF_BG_4BIT)) == (FCF_FG_4BIT|FCF_BG_4BIT) || Color3.Flags == 0);
//	//}
//
//	return Color2;
//}

DWORD WrapPluginInfo::FarDialogItemFlags_3_2(FARDIALOGITEMFLAGS Flags3)
{
	//_ASSERTE(Far2::DIF_COLORMASK == DIF_COLORMASK);
	DWORD Flags2 = 0; //(DWORD)(Flags3 & DIF_COLORMASK);

	//if (Flags3 & DIF_SETCOLOR)
	//	Flags2 |= Far2::DIF_SETCOLOR;
	if (Flags3 & DIF_BOXCOLOR)
		Flags2 |= Far2::DIF_BOXCOLOR;
	if (Flags3 & DIF_GROUP)
		Flags2 |= Far2::DIF_GROUP;
	if (Flags3 & DIF_LEFTTEXT)
		Flags2 |= Far2::DIF_LEFTTEXT;
	if (Flags3 & DIF_MOVESELECT)
		Flags2 |= Far2::DIF_MOVESELECT;
	if (Flags3 & DIF_SHOWAMPERSAND)
		Flags2 |= Far2::DIF_SHOWAMPERSAND;
	if (Flags3 & DIF_CENTERGROUP)
		Flags2 |= Far2::DIF_CENTERGROUP;
	if (Flags3 & DIF_NOBRACKETS/* == DIF_MANUALADDHISTORY*/)
		Flags2 |= Far2::DIF_NOBRACKETS/* == DIF_MANUALADDHISTORY*/;
	if (Flags3 & DIF_SEPARATOR)
		Flags2 |= Far2::DIF_SEPARATOR;
	if (Flags3 & DIF_SEPARATOR2/* == DIF_EDITOR == DIF_LISTNOAMPERSAND*/)
		Flags2 |= Far2::DIF_SEPARATOR2/* == DIF_EDITOR == DIF_LISTNOAMPERSAND*/;
	if (Flags3 & DIF_LISTNOBOX/* == DIF_HISTORY == DIF_BTNNOCLOSE == DIF_CENTERTEXT*/)
		Flags2 |= Far2::DIF_LISTNOBOX/* == DIF_HISTORY == DIF_BTNNOCLOSE == DIF_CENTERTEXT*/;
	if (Flags3 & DIF_SETSHIELD/* == DIF_EDITEXPAND*/)
		Flags2 |= Far2::DIF_SETSHIELD/* == DIF_EDITEXPAND*/;
	if (Flags3 & DIF_DROPDOWNLIST)
		Flags2 |= Far2::DIF_DROPDOWNLIST;
	if (Flags3 & DIF_USELASTHISTORY)
		Flags2 |= Far2::DIF_USELASTHISTORY;
	if (Flags3 & DIF_MASKEDIT)
		Flags2 |= Far2::DIF_MASKEDIT;
	if (Flags3 & DIF_SELECTONENTRY/* == DIF_3STATE*/)
		Flags2 |= Far2::DIF_SELECTONENTRY/* == DIF_3STATE*/;
	if (Flags3 & DIF_SELECTONENTRY/* == DIF_3STATE*/)
		Flags2 |= Far2::DIF_SELECTONENTRY/* == DIF_3STATE*/;
	if (Flags3 & DIF_NOAUTOCOMPLETE/* == DIF_LISTAUTOHIGHLIGHT*/)
		Flags2 |= Far2::DIF_NOAUTOCOMPLETE/* == DIF_LISTAUTOHIGHLIGHT*/;
	if (Flags3 & DIF_LISTNOCLOSE)
		Flags2 |= Far2::DIF_LISTNOCLOSE;
	if (Flags3 & DIF_HIDDEN)
		Flags2 |= Far2::DIF_HIDDEN;
	if (Flags3 & DIF_READONLY)
		Flags2 |= Far2::DIF_READONLY;
	if (Flags3 & DIF_NOFOCUS)
		Flags2 |= Far2::DIF_NOFOCUS;
	if (Flags3 & DIF_DISABLE)
		Flags2 |= Far2::DIF_DISABLE;
	if (Flags3 & DIF_DISABLE)
		Flags2 |= Far2::DIF_DISABLE;

	return Flags2;
}

FARDIALOGITEMFLAGS WrapPluginInfo::FarDialogItemFlags_2_3(DWORD Flags2)
{
	//_ASSERTE(Far2::DIF_COLORMASK == DIF_COLORMASK);
	FARDIALOGITEMFLAGS Flags3 = (Flags2 & ~(Far2::DIF_COLORMASK|Far2::DIF_SETCOLOR));
	
	//if (Flags2 & Far2::DIF_SETCOLOR)
	//	Flags3 |= DIF_SETCOLOR;
	if (Flags2 & Far2::DIF_BOXCOLOR)
		Flags3 |= DIF_BOXCOLOR;
	if (Flags2 & Far2::DIF_GROUP)
		Flags3 |= DIF_GROUP;
	if (Flags2 & Far2::DIF_LEFTTEXT)
		Flags3 |= DIF_LEFTTEXT;
	if (Flags2 & Far2::DIF_MOVESELECT)
		Flags3 |= DIF_MOVESELECT;
	if (Flags2 & Far2::DIF_SHOWAMPERSAND)
		Flags3 |= DIF_SHOWAMPERSAND;
	if (Flags2 & Far2::DIF_CENTERGROUP)
		Flags3 |= DIF_CENTERGROUP;
	if (Flags2 & Far2::DIF_NOBRACKETS/* == DIF_MANUALADDHISTORY*/)
		Flags3 |= DIF_NOBRACKETS/* == DIF_MANUALADDHISTORY*/;
	if (Flags2 & Far2::DIF_SEPARATOR)
		Flags3 |= DIF_SEPARATOR;
	if (Flags2 & Far2::DIF_SEPARATOR2/* == DIF_EDITOR == DIF_LISTNOAMPERSAND*/)
		Flags3 |= DIF_SEPARATOR2/* == DIF_EDITOR == DIF_LISTNOAMPERSAND*/;
	if (Flags2 & Far2::DIF_LISTNOBOX/* == DIF_HISTORY == DIF_BTNNOCLOSE == DIF_CENTERTEXT*/)
		Flags3 |= DIF_LISTNOBOX/* == DIF_HISTORY == DIF_BTNNOCLOSE == DIF_CENTERTEXT*/;
	if (Flags2 & Far2::DIF_SETSHIELD/* == DIF_EDITEXPAND*/)
		Flags3 |= DIF_SETSHIELD/* == DIF_EDITEXPAND*/;
	if (Flags2 & Far2::DIF_DROPDOWNLIST)
		Flags3 |= DIF_DROPDOWNLIST;
	if (Flags2 & Far2::DIF_USELASTHISTORY)
		Flags3 |= DIF_USELASTHISTORY;
	if (Flags2 & Far2::DIF_MASKEDIT)
		Flags3 |= DIF_MASKEDIT;
	if (Flags2 & Far2::DIF_SELECTONENTRY/* == DIF_3STATE*/)
		Flags3 |= DIF_SELECTONENTRY/* == DIF_3STATE*/;
	if (Flags2 & Far2::DIF_SELECTONENTRY/* == DIF_3STATE*/)
		Flags3 |= DIF_SELECTONENTRY/* == DIF_3STATE*/;
	if (Flags2 & Far2::DIF_NOAUTOCOMPLETE/* == DIF_LISTAUTOHIGHLIGHT*/)
		Flags3 |= DIF_NOAUTOCOMPLETE/* == DIF_LISTAUTOHIGHLIGHT*/;
	if (Flags2 & Far2::DIF_LISTNOCLOSE)
		Flags3 |= DIF_LISTNOCLOSE;
	if (Flags2 & Far2::DIF_HIDDEN)
		Flags3 |= DIF_HIDDEN;
	if (Flags2 & Far2::DIF_READONLY)
		Flags3 |= DIF_READONLY;
	if (Flags2 & Far2::DIF_NOFOCUS)
		Flags3 |= DIF_NOFOCUS;
	if (Flags2 & Far2::DIF_DISABLE)
		Flags3 |= DIF_DISABLE;
	if (Flags2 & Far2::DIF_DISABLE)
		Flags3 |= DIF_DISABLE;

	return Flags3;
}

void WrapPluginInfo::FarListItem_2_3(const Far2::FarListItem* p2, FarListItem* p3)
{
	//p3->StructSize = sizeof(*p3);
	//TODO: ����������� ������
	p3->Flags = p2->Flags;
	p3->Text = p2->Text;
	p3->Reserved[0] = p2->Reserved[0];
	p3->Reserved[1] = p2->Reserved[1];
	p3->Reserved[2] = p2->Reserved[2];
}

void WrapPluginInfo::FarListItem_3_2(const FarListItem* p3, Far2::FarListItem* p2)
{
	//_ASSERTE(p3->StructSize==sizeof(*p3));
	//TODO: ����������� ������
	p2->Flags = p3->Flags;
	p2->Text = p3->Text;
	p2->Reserved[0] = p3->Reserved[0];
	p2->Reserved[1] = p3->Reserved[1];
	p2->Reserved[2] = p3->Reserved[2];
}

#define VBufDim(Item) (((Item)->X2-(Item)->X1+1)*((Item)->Y2-(Item)->Y1+1))

// pnColor2 - ���� DIF_SETCOLOR � DIF_COLORMASK (��, ��� ������ ���� � Far2)
void WrapPluginInfo::FarDialogItem_2_3(const Far2::FarDialogItem *p2, FarDialogItem *p3, FarList *pList3, WRAP_FAR_CHAR_INFO& pChars3)
{
	memset(&p3->Reserved, 0, sizeof(p3->Reserved));

	p3->Type = DialogItemTypes_2_3(p2->Type);
	p3->X1 = p2->X1;
	p3->Y1 = p2->Y1;
	p3->X2 = p2->X2;
	p3->Y2 = p2->Y2;

	p3->Flags = FarDialogItemFlags_2_3(p2->Flags);
	if (p2->DefaultButton)
		p3->Flags |= DIF_DEFAULTBUTTON;
	if (p2->Focus)
		p3->Flags |= DIF_FOCUS;

	p3->Data = p2->PtrData;
	p3->MaxLength = p2->MaxLen;

	if (p3->Type == DI_EDIT)
	{
		p3->History = p2->Param.History;
	}
	else if (p3->Type == DI_FIXEDIT)
	{
		p3->Mask = p2->Param.Mask;
	}
	else if (p3->Type == DI_COMBOBOX || p3->Type == DI_LISTBOX)
	{
		//pList3->StructSize = sizeof(*pList3);
		if (pList3->Items)
		{
			free(pList3->Items);
			pList3->Items = NULL;
		}
		if (p2->Param.ListItems != NULL)
		{
			int nItems = p2->Param.ListItems->ItemsNumber;
			pList3->ItemsNumber = nItems;
			if (p2->Param.ListItems > 0)
			{
				const Far2::FarListItem* pi2 = p2->Param.ListItems->Items;
				if (pi2 != NULL)
				{
					pList3->Items = (FarListItem*)calloc(nItems,sizeof(*pList3->Items));
					FarListItem* pi3 = pList3->Items;
					for (int j = 0; j < nItems; j++, pi2++, pi3++)
					{
						FarListItem_2_3(pi2, pi3);
					}
				}
			}
			p3->ListItems = pList3;
		}
	}
	else if (p3->Type == DI_USERCONTROL)
	{
		size_t nCount = VBufDim(p2);
		if (pChars3.p && (pChars3.nCount < nCount))
		{
			free(pChars3.p);
			pChars3.p = NULL;
		}
		if (!p2->Param.VBuf)
			p3->VBuf = NULL;
		else
		{
			if (!pChars3.p)
			{
				pChars3.nCount = nCount;
				pChars3.p = (FAR_CHAR_INFO*)malloc(nCount*sizeof(*pChars3.p));
			}
			for (size_t i = 0; i < nCount; i++)
			{
				pChars3.p[i].Char = p2->Param.VBuf[i].Char.UnicodeChar;
				//TODO: COMMON_LVB_UNDERSCORE/COMMON_LVB_REVERSE_VIDEO/COMMON_LVB_GRID_RVERTICAL/COMMON_LVB_GRID_LVERTICAL/COMMON_LVB_GRID_HORIZONTAL/COMMON_LVB_TRAILING_BYTE/COMMON_LVB_LEADING_BYTE
				FarColor_2_3((BYTE)(p2->Param.VBuf[i].Attributes & 0xFF), pChars3.p[i].Attributes);
			}
			p3->VBuf = pChars3.p;
		}
	}
	else
	{
		#if MVV_3<=2798
		p3->Reserved = p2->Param.Reserved;
		#endif
	}
}

void WrapPluginInfo::FarDialogItem_3_2(const FarDialogItem *p3, /*size_t nAllocated3,*/ Far2::FarDialogItem *p2, Far2::FarList *pList2, WRAP_CHAR_INFO& pVBuf2, BOOL bListPos /*= FALSE*/)
{
	p2->Param.Reserved = 0;

	p2->Type = DialogItemTypes_3_2(p3->Type);
	p2->X1 = p3->X1;
	p2->Y1 = p3->Y1;
	p2->X2 = p3->X2;
	p2->Y2 = p3->Y2;

	p2->Flags = FarDialogItemFlags_3_2(p3->Flags);
	p2->DefaultButton = (p3->Flags & DIF_DEFAULTBUTTON) == DIF_DEFAULTBUTTON;
	p2->Focus = (p3->Flags & DIF_FOCUS) == DIF_FOCUS;

	//#define CpyDlgStr(dst,src) \
	//if (!nAllocated3 || ((LPBYTE)p3->src) < ((LPBYTE)p3) || ((LPBYTE)p3->src) >= (((LPBYTE)p3)+nAllocated3))
	//	p2->dst = p3->src;
	//else
	//{
	//	p2->dst = (wchar_t*)(((LPBYTE)p2) + (((LPBYTE)p3->src) - ((LPBYTE)p3)));
	//	lstrcpy((wchar_t*)p2->dst, p3->src);
	//}

	p2->PtrData = p3->Data;
	//CpyDlgStr(PtrData,Data);
	p2->MaxLen = p3->MaxLength;

	if (p3->Type == DI_EDIT)
	{
		p2->Param.History = p3->History;
		//CpyDlgStr(Param.History, History);
	}
	else if (p3->Type == DI_FIXEDIT)
	{
		p2->Param.Mask = p3->Mask;
		//CpyDlgStr(Param.Mask, Mask);
	}
	else if (p3->Type == DI_COMBOBOX || p3->Type == DI_LISTBOX)
	{
		if (pList2->Items)
		{
			free(pList2->Items);
			pList2->Items = NULL;
		}
		if (!bListPos && (p3->ListItems != NULL))
		{
			int nItems = p3->ListItems->ItemsNumber;
			pList2->ItemsNumber = nItems;
			if (p3->ListItems > 0)
			{
				const FarListItem* pi3 = p3->ListItems->Items;
				if (pi3 != NULL)
				{
					pList2->Items = (Far2::FarListItem*)calloc(nItems,sizeof(*pList2->Items));
					Far2::FarListItem* pi2 = pList2->Items;
					for (int j = 0; j < nItems; j++, pi2++, pi3++)
					{
						FarListItem_3_2(pi3, pi2);
					}
				}
			}
			p2->Param.ListItems = pList2;
		}
	}
	else if (p3->Type == DI_USERCONTROL)
	{
		//p2->Param.VBuf = p3->VBuf;
		if (!p3->VBuf)
			p2->Param.VBuf = NULL;
		else
		{
			if (!pVBuf2.isExt)
			{
				size_t nCount = VBufDim(p3);
				if (pVBuf2.p2 && (pVBuf2.nCount < nCount))
				{
					free(pVBuf2.p2);
					pVBuf2.p2 = NULL;
				}

				if (!pVBuf2.p2)
				{
					pVBuf2.nCount = nCount;
					pVBuf2.p2 = (CHAR_INFO*)malloc(nCount*sizeof(*pVBuf2.p2));
				}

				_ASSERTE((void*)pVBuf2.p2!=(void*)p3->VBuf && pVBuf2.p2);
				for (size_t i = 0; i < nCount; i++)
				{
					pVBuf2.p2[i].Char.UnicodeChar = p3->VBuf[i].Char;
					//TODO: COMMON_LVB_UNDERSCORE/COMMON_LVB_REVERSE_VIDEO/COMMON_LVB_GRID_RVERTICAL/COMMON_LVB_GRID_LVERTICAL/COMMON_LVB_GRID_HORIZONTAL/COMMON_LVB_TRAILING_BYTE/COMMON_LVB_LEADING_BYTE
					pVBuf2.p2[i].Attributes = FarColor_3_2(p3->VBuf[i].Attributes);
				}
			}

			p2->Param.VBuf = pVBuf2.p2;
		}
	}
	else
	{
		#if MVV_3<=2798
		p2->Param.Reserved = p3->Reserved;
		#else
		_ASSERTE(p3->Reserved[0]==0 && p3->Reserved[1]==0);
		#endif
	}
}

// VP 2013.01.20: Many structs differ only by a StructSize field at the beginning.
// This macro copies an otherwise-identical structure.
#define ADD_STRUCTSIZE(Struct, Temporary, Pointer)	\
	if (sizeof(size_t)+sizeof(Far2::Struct)!=sizeof(Struct)) AssertStructSize(sizeof(Struct), sizeof(size_t)+sizeof(Far2::Struct), L#Struct, __FILE__, __LINE__); \
	Temporary.StructSize = sizeof(Temporary); \
	memcpy(((size_t*)(&Temporary)) + 1, (void*)Pointer, sizeof(Far2::Struct)); \
	*(Struct**)&Pointer = &Temporary;

// VP 2013.01.20: As above, but declare a temporary variable locally.
// Warning: "Pointer" will be left dangling once it leaves the current scope.
#define ADD_STRUCTSIZE_HERE(Struct, Pointer)			\
	Struct Struct##_Temporary;							\
	ADD_STRUCTSIZE(Struct, Struct##_Temporary, Pointer)

//int gnMsg_2 = 0, gnParam1_2 = 0, gnParam1_3 = 0;
//FARMESSAGE gnMsg_3 = DM_FIRST;
//FARMESSAGE gnMsgKey_3 = DM_FIRST, gnMsgClose_3 = DM_FIRST;
//LONG_PTR gnParam2_2 = 0;
//void* gpParam2_3 = NULL;
//FarListItem* gpListItems3 = NULL; INT_PTR gnListItemsMax3 = 0;
//Far2::FarListItem* gpListItems2 = NULL; UINT_PTR gnListItemsMax2 = 0;

LONG_PTR WrapPluginInfo::CallDlgProc_2_3(FARAPIDEFDLGPROC DlgProc3, HANDLE hDlg2, const int Msg2, const int Param1, LONG_PTR Param2)
{
	if (!hDlg2)
	{
		_ASSERTE(hDlg2!=NULL);
		return 0;
	}
	LONG_PTR lRc = 0;
	Far2Dialog* pDlg = (Far2Dialog*)hDlg2;
	HANDLE hDlg3 = (*gpMapDlg_2_3)[pDlg];
	if (!hDlg3) // ����� ���� NULL, ���� ��� ������ �� �� ����� �������
	{
		bool lbSkip = false;
		if (!IsBadReadPtr(pDlg, sizeof(pDlg->WrapMagic)))
		{
			if (pDlg->WrapMagic == 'F2WR')
			{
				hDlg3 = pDlg->hDlg3;
				lbSkip = true;
				if (!hDlg3)
				{
					_ASSERTE(pDlg->hDlg3 != NULL);
					hDlg3 = pDlg->InitDlg();
				}
			}
		}
		if (!hDlg3 && !lbSkip)
		{
			hDlg3 = hDlg2;
			pDlg = NULL;
		}
	}

	FARMESSAGE Msg3 = DM_FIRST;
#ifdef _DEBUG
	Far2::FarMessagesProc Msg2_ = (Far2::FarMessagesProc)Msg2;
#endif
	LONG_PTR OrgParam2 = Param2;

	INPUT_RECORD r;
	FarListGetItem flgi3;
	FarListPos flp3;
	FarListDelete fld3;
	FarList fl3;
	FarListUpdate flu3;
	FarListInsert fli3;
	FarListFind flf3;
	FarListInfo flInfo3;
	FarListItemData flItemData3;
	FarListTitles flTitles3;
	//FarListColors flColors3;
	FarGetDialogItem fgdi3;
	FarDialogItem fdi3;
	DialogInfo di3;
	FarDialogItemColors fdic3;
	FarColor colors[32];
	WRAP_FAR_CHAR_INFO pfci = {};
	FarDialogItemData fdid3;
	WrapListItemData* pwlid2 = NULL;
	EditorSelect es;
	EditorSetPosition esp;

	//TODO: ��������� gnMsg_2/gnMsg_3 ������ ���� ��� <DM_USER!
	if (Msg2 == gnMsg_2 && gnMsg_3 != DM_FIRST
		&& gnParam1_2 == Param1 && gnParam2_2 == Param2)
	{
		Msg3 = gnMsg_3;
		Param2 = (LONG_PTR)gpParam2_3;
	}
	else if (Msg2 >= Far2::DM_USER)
	{
		Msg3 = (FARMESSAGE)Msg2;
	}
	else switch (Msg2)
	{
		//TODO: ������ ����� ����������� ����������� ���������� ��� ��������� ���������
		case Far2::DM_FIRST:
			_ASSERTE(Msg2!=Far2::DM_FIRST); // ��� ��������� �� ���� ������������ �� ������
			Msg3 = DM_FIRST; break;
		case Far2::DM_CLOSE:
			Msg3 = (gnMsgClose_3!=DM_FIRST) ? gnMsgClose_3 : DM_CLOSE;
			_ASSERTE(Msg3 == DM_CLOSE || Msg3 == DN_CLOSE);
			break;
		case Far2::DM_ENABLE:
			Msg3 = DM_ENABLE; break;
		case Far2::DM_ENABLEREDRAW:
			Msg3 = DM_ENABLEREDRAW; break;
		case Far2::DM_GETDLGDATA:
			Msg3 = DM_GETDLGDATA; break;
		case Far2::DM_GETDLGRECT:
			Msg3 = DM_GETDLGRECT; break;
		case Far2::DM_GETTEXT:
		case Far2::DM_GETTEXTPTR:
			if (!Param2)
			{
				Msg3 = DM_GETTEXT;
			}
			else if (Msg2 == Far2::DM_GETTEXT)
			{
				const Far2::FarDialogItemData* p2 = (const Far2::FarDialogItemData*)Param2;
				ZeroStruct(fdid3);
				fdid3.StructSize = sizeof(fdid3);
				fdid3.PtrLength = p2->PtrLength;
				fdid3.PtrData = p2->PtrData;
				Param2 = (LONG_PTR)&fdid3;
				Msg3 = DM_GETTEXT;
			}
			else
			{
				// ������ ������ - �� ������� �������, �� ���-�� ����� �������� � Far3
				size_t nPtrLength = DlgProc3(hDlg3, DM_GETTEXT, 0, 0);
				ZeroStruct(fdid3);
				fdid3.StructSize = sizeof(fdid3);
				fdid3.PtrLength = nPtrLength;
				fdid3.PtrData = (wchar_t*)Param2;
				Param2 = (LONG_PTR)&fdid3;
				Msg3 = DM_GETTEXT;
			}
			break;
		case Far2::DM_GETTEXTLENGTH:
			#if MVV_3<=2798
			Msg3 = DM_GETTEXTLENGTH;
			#else
			Msg3 = DM_GETTEXT;
			Param2 = 0;
			#endif
			break;
		case Far2::DM_KEY:
			//TODO: ���������?
			//Msg3 = (gnMsg_3 == DN_KEY) ? DN_KEY : DM_KEY;
			{
				//static INPUT_RECORD r;
				ZeroStruct(r);
				FarKey_2_3((int)Param2, &r);
				Param2 = (LONG_PTR)&r;
				Msg3 = (gnMsgKey_3 != DM_FIRST) ? gnMsgKey_3 : DM_KEY;
				_ASSERTE(Msg3 == DM_KEY || Msg3 == DN_CONTROLINPUT);
			}
			break;
		case Far2::DM_MOVEDIALOG:
			Msg3 = DM_MOVEDIALOG; break;
		case Far2::DM_SETDLGDATA:
			Msg3 = DM_SETDLGDATA; break;
		case Far2::DM_SETFOCUS:
			Msg3 = DM_SETFOCUS; break;
		case Far2::DM_REDRAW:
			Msg3 = DM_REDRAW; break;
		//DM_SETREDRAW=DM_REDRAW,
		case Far2::DM_SETTEXT:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarDialogItemData* p2 = (const Far2::FarDialogItemData*)Param2;
				ZeroStruct(fdid3);
				fdid3.StructSize = sizeof(fdid3);
				fdid3.PtrLength = p2->PtrLength;
				fdid3.PtrData = p2->PtrData;
				Param2 = (LONG_PTR)&fdid3;
				Msg3 = DM_SETTEXT;
			}
			break;
		case Far2::DM_SETMAXTEXTLENGTH:
			Msg3 = DM_SETMAXTEXTLENGTH; break;
		//DM_SETTEXTLENGTH=DM_SETMAXTEXTLENGTH,
		case Far2::DM_SHOWDIALOG:
			Msg3 = DM_SHOWDIALOG; break;
		case Far2::DM_GETFOCUS:
			Msg3 = DM_GETFOCUS; break;
		case Far2::DM_GETCURSORPOS:
			Msg3 = DM_GETCURSORPOS; break;
		case Far2::DM_SETCURSORPOS:
			Msg3 = DM_SETCURSORPOS; break;
		case Far2::DM_SETTEXTPTR:
			Msg3 = DM_SETTEXTPTR; break;
		case Far2::DM_SHOWITEM:
			Msg3 = DM_SHOWITEM; break;
		case Far2::DM_ADDHISTORY:
			Msg3 = DM_ADDHISTORY; break;
		case Far2::DM_GETCHECK:
			Msg3 = DM_GETCHECK; break;
		case Far2::DM_SETCHECK:
			Msg3 = DM_SETCHECK; break;
		case Far2::DM_SET3STATE:
			Msg3 = DM_SET3STATE; break;
		case Far2::DM_LISTSORT:
			Msg3 = DM_LISTSORT; break;
		case Far2::DM_LISTGETITEM:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarListGetItem* p2 = (const Far2::FarListGetItem*)Param2;
				//static FarListGetItem flgi3;
				ZeroStruct(flgi3);
				flgi3.StructSize = sizeof(flgi3);
				flgi3.ItemIndex = p2->ItemIndex;
				FarListItem_2_3(&p2->Item, &flgi3.Item);
				Param2 = (LONG_PTR)&flgi3;
				Msg3 = DM_LISTGETITEM;
			}
			break;
		case Far2::DM_LISTGETCURPOS: //Msg3 = DM_LISTGETCURPOS; break;
		case Far2::DM_LISTSETCURPOS: //Msg3 = DM_LISTSETCURPOS; break;
			if (!Param2)
			{
				if (Msg2 == Far2::DM_LISTGETCURPOS)
				{
					Msg3 = DM_LISTGETCURPOS;
				}
				else
				{
					_ASSERTE(Param2!=NULL);
				}
			}
			else
			{
				const Far2::FarListPos* p2 = (const Far2::FarListPos*)Param2;
				//static FarListPos flp3;
				ZeroStruct(flp3);
				flp3.StructSize = sizeof(flp3);
				flp3.SelectPos = p2->SelectPos;
				flp3.TopPos = p2->TopPos;
				Param2 = (LONG_PTR)&flp3;
				switch (Msg2)
				{
				case Far2::DM_LISTGETCURPOS: Msg3 = DM_LISTGETCURPOS; break;
				case Far2::DM_LISTSETCURPOS: Msg3 = DM_LISTSETCURPOS; break;
				}
			}
			break;
		case Far2::DM_LISTDELETE:
			if (!Param2)
			{
				//_ASSERTE(Param2!=NULL); -- ���������. �������� ���� ���������
				Msg3 = DM_LISTDELETE;
			}
			else
			{
				const Far2::FarListDelete* p2 = (const Far2::FarListDelete*)Param2;
				//static FarListDelete fld3;
				ZeroStruct(fld3);
				fld3.StructSize = sizeof(fld3);
				fld3.StartIndex = p2->StartIndex;
				fld3.Count = p2->Count;
				Param2 = (LONG_PTR)&fld3;
				Msg3 = DM_LISTDELETE;
			}
			break;
		case Far2::DM_LISTADD:
		case Far2::DM_LISTSET:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarList* p2 = (const Far2::FarList*)Param2;
				//static FarList fl3;
				ZeroStruct(fl3);
				//fl3.StructSize = sizeof(fl3);
				fl3.ItemsNumber = p2->ItemsNumber;
				if (!gpListItems3 || (gnListItemsMax3 < p2->ItemsNumber))
				{
					if (gpListItems3)
					{
						free(gpListItems3);
						gpListItems3 = NULL;
					}
					gnListItemsMax3 = p2->ItemsNumber;
					gpListItems3 = (FarListItem*)calloc(p2->ItemsNumber,sizeof(*gpListItems3));
				}
				if (gpListItems3)
				{
					const Far2::FarListItem* pp2 = p2->Items;
					FarListItem* pp3 = gpListItems3;
					for (int i = 0; i < p2->ItemsNumber; i++, pp2++, pp3++)
					{
						FarListItem_2_3(pp2,pp3);
					}
					fl3.Items = gpListItems3;
					Param2 = (LONG_PTR)&fl3;
					switch (Msg2)
					{
					case Far2::DM_LISTADD: Msg3 = DM_LISTADD; break;
					case Far2::DM_LISTSET: Msg3 = DM_LISTSET; break;
					}
				}
			}
			break;
		case Far2::DM_LISTADDSTR:
			Msg3 = DM_LISTADDSTR; break;
		case Far2::DM_LISTUPDATE:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarListUpdate* p2 = (const Far2::FarListUpdate*)Param2;
				//static FarListUpdate flu3;
				ZeroStruct(flu3);
				flu3.StructSize = sizeof(flu3);
				flu3.Index = p2->Index;
				FarListItem_2_3(&p2->Item, &flu3.Item);
				Param2 = (LONG_PTR)&flu3;
				Msg3 = DM_LISTUPDATE;
			}
			break;
		case Far2::DM_LISTINSERT:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarListInsert* p2 = (const Far2::FarListInsert*)Param2;
				//static FarListInsert fli3;
				ZeroStruct(fli3);
				fli3.StructSize = sizeof(fli3);
				fli3.Index = p2->Index;
				FarListItem_2_3(&p2->Item, &fli3.Item);
				Param2 = (LONG_PTR)&fli3;
				Msg3 = DM_LISTINSERT;
			}
			break;
		case Far2::DM_LISTFINDSTRING:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarListFind* p2 = (const Far2::FarListFind*)Param2;
				//static FarListFind flf3;
				ZeroStruct(flf3);
				flf3.StructSize = sizeof(flf3);
				flf3.StartIndex = p2->StartIndex;
				flf3.Pattern = p2->Pattern;
				flf3.Flags = p2->Flags;
				#if MVV_3<=2798
				flf3.Reserved = p2->Reserved;
				#else
				_ASSERTE(p2->Reserved==0);
				#endif
				Param2 = (LONG_PTR)&flf3;
				Msg3 = DM_LISTFINDSTRING;
			}
			break;
		case Far2::DM_LISTINFO:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarListInfo* p2 = (const Far2::FarListInfo*)Param2;
				//static FarListInfo flInfo3;
				ZeroStruct(flInfo3);
				flInfo3.StructSize = sizeof(flInfo3);
				flInfo3.Flags = p2->Flags;
				flInfo3.ItemsNumber = p2->ItemsNumber;
				flInfo3.SelectPos = p2->SelectPos;
				flInfo3.TopPos = p2->TopPos;
				flInfo3.MaxHeight = p2->MaxHeight;
				flInfo3.MaxLength = p2->MaxLength;
				#if MVV_3<=2798
				flInfo3.Reserved[0] = p2->Reserved[0];
				flInfo3.Reserved[1] = p2->Reserved[1];
				flInfo3.Reserved[2] = p2->Reserved[2];
				flInfo3.Reserved[3] = p2->Reserved[3];
				flInfo3.Reserved[4] = p2->Reserved[4];
				flInfo3.Reserved[5] = p2->Reserved[5];
				#else
				DWORD resNil[6] = {};
				_ASSERTE(memcmp(resNil, p2->Reserved, sizeof(resNil))==0);
				#endif
				Param2 = (LONG_PTR)&flInfo3;
				Msg3 = DM_LISTINFO;
			}
			break;
		case Far2::DM_LISTGETDATA:
			Msg3 = DM_LISTGETDATA; break;
		case Far2::DM_LISTSETDATA:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarListItemData* p2 = (const Far2::FarListItemData*)Param2;
				//static FarListItemData flItemData3;
				ZeroStruct(flItemData3);
				flItemData3.StructSize = sizeof(flItemData3);
				flItemData3.Index = p2->Index;
				size_t nOurSize = sizeof(WrapListItemData)+p2->DataSize;
				pwlid2 = (WrapListItemData*)malloc(nOurSize);
				pwlid2->nMagic = WRAPLISTDATAMAGIC;
				pwlid2->nSize = p2->DataSize;
				if (p2->DataSize <= sizeof(DWORD))
				{
					memmove(&pwlid2->Data, &p2->Data, p2->DataSize);
				}
				else
				{
					memmove(&pwlid2->Data, p2->Data, p2->DataSize);
				}
				flItemData3.DataSize = nOurSize;
				flItemData3.Data = pwlid2;
				#if MVV_3<=2798
				flItemData3.Reserved = p2->Reserved;
				#else
				_ASSERTE(p2->Reserved==0);
				#endif
				Param2 = (LONG_PTR)&flItemData3;
				Msg3 = DM_LISTSETDATA;
			}
			break;
		case Far2::DM_LISTSETTITLES: //Msg3 = DM_LISTSETTITLES; break;
		case Far2::DM_LISTGETTITLES: //Msg3 = DM_LISTGETTITLES; break;
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const Far2::FarListTitles* p2 = (const Far2::FarListTitles*)Param2;
				//static FarListTitles flTitles3;
				ZeroStruct(flTitles3);
				flTitles3.StructSize = sizeof(flTitles3);
				flTitles3.TitleSize = p2->TitleLen;
				flTitles3.Title = p2->Title;
				flTitles3.BottomSize = p2->BottomLen;
				flTitles3.Bottom = p2->Bottom;
				Param2 = (LONG_PTR)&flTitles3;
				switch (Msg2)
				{
				case Far2::DM_LISTSETTITLES: Msg3 = DM_LISTSETTITLES; break;
				case Far2::DM_LISTGETTITLES: Msg3 = DM_LISTGETTITLES; break;
				}
			}
			break;
		case Far2::DM_RESIZEDIALOG:
			Msg3 = DM_RESIZEDIALOG; break;
		case Far2::DM_SETITEMPOSITION:
			Msg3 = DM_SETITEMPOSITION; break;
		case Far2::DM_GETDROPDOWNOPENED:
			Msg3 = DM_GETDROPDOWNOPENED; break;
		case Far2::DM_SETDROPDOWNOPENED:
			Msg3 = DM_SETDROPDOWNOPENED; break;
		case Far2::DM_SETHISTORY:
			Msg3 = DM_SETHISTORY; break;
		case Far2::DM_GETITEMPOSITION:
			Msg3 = DM_GETITEMPOSITION; break;
		case Far2::DM_SETMOUSEEVENTNOTIFY:
			Msg3 = DM_SETMOUSEEVENTNOTIFY; break;
		case Far2::DM_EDITUNCHANGEDFLAG:
			Msg3 = DM_EDITUNCHANGEDFLAG; break;
		case Far2::DM_GETITEMDATA:
			Msg3 = DM_GETITEMDATA; break;
		case Far2::DM_SETITEMDATA:
			Msg3 = DM_SETITEMDATA; break;
		case Far2::DM_LISTSETMOUSEREACTION:
			#if MVV_3>=2116
			//TODO: ���������, � �������� ��?
			// build 2116:
			// DM_LISTSETMOUSEREACTION ������ ���. �������� ��������� ������ �������� �������
			// DIF_LISTTRACKMOUSE/DIF_LISTTRACKMOUSEINFOCUS ��� �������� �������.
			// �� ��������� ��� ����� �������� ����� �� ����������� (������ �� ����� �� ������),
			// ��� ansi-�������� ���������� DIF_LISTTRACKMOUSE (������ ������).
			{
				lRc = 0;
				ZeroStruct(fgdi3);
				fgdi3.StructSize = sizeof(fgdi3);
				LONG_PTR lSize = psi3.SendDlgMessage(hDlg3, DM_GETDLGITEM, Param1, &fgdi3);
				if (lSize > 0)
				{
					fgdi3.Size = lSize;
					fgdi3.Item = (FarDialogItem*)calloc(lSize, 1);
					if (fgdi3.Item && (psi3.SendDlgMessage(hDlg3, DM_GETDLGITEM, Param1, &fgdi3) == lSize))
					{
						// ������� ��������� ���
						if (fgdi3.Item->Flags & DIF_LISTTRACKMOUSEINFOCUS)
							lRc = Far2::LMRT_ONLYFOCUS;
						else if (fgdi3.Item->Flags & DIF_LISTTRACKMOUSE)
							lRc = Far2::LMRT_ALWAYS;
						else
							lRc = Far2::LMRT_NEVER;
						// ������
						FARDIALOGITEMFLAGS NewFlags = fgdi3.Item->Flags;
						if (Param2 == Far2::LMRT_NEVER)
						{
							NewFlags &= ~(DIF_LISTTRACKMOUSEINFOCUS|DIF_LISTTRACKMOUSE);
						}
						else if (Param2 == Far2::LMRT_ALWAYS)
						{
							NewFlags &= ~DIF_LISTTRACKMOUSEINFOCUS;
							NewFlags |= DIF_LISTTRACKMOUSE;
						}
						else if (Param2 == Far2::LMRT_ONLYFOCUS)
						{
							NewFlags &= ~DIF_LISTTRACKMOUSE;
							NewFlags |= DIF_LISTTRACKMOUSEINFOCUS;
						}
						// ����������?
						if (NewFlags != fgdi3.Item->Flags)
						{
							fgdi3.Item->Flags = NewFlags;
							psi3.SendDlgMessage(hDlg3, DM_SETDLGITEM, Param1, &fgdi3);
						}
						free(fgdi3.Item);
					}
					ZeroStruct(fgdi3);
                    fgdi3.StructSize = sizeof(fgdi3);
				}
				return lRc;
			}
			#else
			Msg3 = DM_LISTSETMOUSEREACTION; -- 
			#endif
			break;
		case Far2::DM_GETCURSORSIZE:
			Msg3 = DM_GETCURSORSIZE; break;
		case Far2::DM_SETCURSORSIZE:
			Msg3 = DM_SETCURSORSIZE; break;
		case Far2::DM_LISTGETDATASIZE:
			Msg3 = DM_LISTGETDATASIZE; break;
		case Far2::DM_GETSELECTION: // Msg3 = DM_GETSELECTION; break;
		case Far2::DM_SETSELECTION: // Msg3 = DM_SETSELECTION; break;
			#if MVV_3<2801
			ASSERTSTRUCT(EditorSelect);
			#else
			ADD_STRUCTSIZE(EditorSelect, es, Param2);
			#endif
			switch (Msg2)
			{
			case Far2::DM_GETSELECTION: Msg3 = DM_GETSELECTION; break;
			case Far2::DM_SETSELECTION: Msg3 = DM_SETSELECTION; break;
			}
			break;
		case Far2::DM_GETEDITPOSITION: // Msg3 = DM_GETEDITPOSITION; break;
		case Far2::DM_SETEDITPOSITION: // Msg3 = DM_SETEDITPOSITION; break;
			#if MVV_3<2801
			ASSERTSTRUCT(EditorSetPosition);
			#else
			ADD_STRUCTSIZE(EditorSetPosition, esp, Param2);
			#endif
			switch (Msg2)
			{
			case Far2::DM_GETEDITPOSITION: Msg3 = DM_GETEDITPOSITION; break;
			case Far2::DM_SETEDITPOSITION: Msg3 = DM_SETEDITPOSITION; break;
			}
			break;
		case Far2::DM_SETCOMBOBOXEVENT:
			Msg3 = DM_SETCOMBOBOXEVENT; break;
		case Far2::DM_GETCOMBOBOXEVENT:
			Msg3 = DM_GETCOMBOBOXEVENT; break;
		case Far2::DM_GETCONSTTEXTPTR:
			Msg3 = DM_GETCONSTTEXTPTR; break;
		case Far2::DM_GETDIALOGINFO:
			ZeroStruct(di3);
			di3.StructSize = sizeof(di3);
			Param2 = (LONG_PTR)&di3;
			Msg3 = DM_GETDIALOGINFO;
			break;
		case Far2::DN_FIRST:
			Msg3 = DN_FIRST; break;
		case Far2::DN_BTNCLICK:
			Msg3 = DN_BTNCLICK; break;
		case Far2::DN_CTLCOLORDIALOG:
			Msg3 = DN_CTLCOLORDIALOG;
			FarColor_2_3((BYTE)LOBYTE(LOWORD(Param2)), colors[0]);
			FarColor_2_3((BYTE)HIBYTE(LOWORD(Param2)), colors[1]);
			FarColor_2_3((BYTE)LOBYTE(HIWORD(Param2)), colors[2]);
			FarColor_2_3((BYTE)HIBYTE(HIWORD(Param2)), colors[3]);
			Param2 = (LONG_PTR)colors;
			break;
		case Far2::DN_CTLCOLORDLGITEM:
			Msg3 = DN_CTLCOLORDLGITEM;
			//if (!Param2)
			ZeroStruct(fdic3);
			fdic3.StructSize = sizeof(fdic3);
			fdic3.Colors = colors;
			_ASSERTE(ARRAYSIZE(colors)>=4);
			fdic3.ColorsCount = 4;
			FarColor_2_3((BYTE)LOBYTE(LOWORD(Param2)), colors[0]);
			FarColor_2_3((BYTE)HIBYTE(LOWORD(Param2)), colors[1]);
			FarColor_2_3((BYTE)LOBYTE(HIWORD(Param2)), colors[2]);
			FarColor_2_3((BYTE)HIBYTE(HIWORD(Param2)), colors[3]);
			Param2 = (LONG_PTR)&fdic3;
			break;

		case Far2::DN_CTLCOLORDLGLIST:
			if (!Param2)
			{
				_ASSERTE(Param2!=0);
			}
			else
			{
				const Far2::FarListColors* p2 = (const Far2::FarListColors*)Param2;
				//static FarListColors flColors3;
				ZeroStruct(fdic3);
				fdic3.StructSize = sizeof(fdic3);
				fdic3.Colors = colors;
				_ASSERTE(ARRAYSIZE(colors)>=p2->ColorCount);
				//fdic3.Flags = 0/*p2->Flags*/; // �����. ���� ������ ���� ����� 0.
				//fdic3.Reserved = 0/*p2->Reserved*/;
				fdic3.ColorsCount = min(p2->ColorCount,ARRAYSIZE(colors));
				//TODO: ����������� �������� ������ � �������?
				for (UINT i = 0; i < fdic3.ColorsCount; i++)
					FarColor_2_3(p2->Colors[i], colors[i]);
				// done
				Param2 = (LONG_PTR)&fdic3;
				Msg3 = DN_CTLCOLORDLGLIST;
			}
			break;

		case Far2::DN_DRAWDIALOG:
			Msg3 = DN_DRAWDIALOG; break;

		case Far2::DM_GETDLGITEM:
			if (Param2)
			{
				// ��� �� ������ ������� ������, ������� ������ ���������� � FarMessageParam_3_2
				//static FarGetDialogItem fgdi3;
				ZeroStruct(fgdi3);
				fgdi3.StructSize = sizeof(fgdi3);
				Param2 = (LONG_PTR)&fgdi3;
			}
			Msg3 = DM_GETDLGITEM;
			break;
		case Far2::DM_GETDLGITEMSHORT:
			if (Param2)
			{
				//static FarDialogItem fdi3;
				ZeroStruct(fdi3);
				Param2 = (LONG_PTR)&fdi3;
			}
			Msg3 = DM_GETDLGITEMSHORT;
			break;
		case Far2::DM_SETDLGITEM:
		case Far2::DM_SETDLGITEMSHORT:
		case Far2::DN_EDITCHANGE:
		case Far2::DN_DRAWDLGITEM:
			#ifdef _DEBUG
			if (Msg2 == Far2::DN_DRAWDLGITEM)
			{
				int nDbg = 0;
			}
			#endif
			if (!Param2)
			{
				_ASSERTE(Param2!=0);
			}
			else
			{
				const Far2::FarDialogItem* p2 = (const Far2::FarDialogItem*)Param2;
				//static FarDialogItem p3;
				ZeroStruct(fdi3);
				FarDialogItem_2_3(p2, &fdi3, &m_ListItems3, pDlg ? pDlg->GetVBufPtr3(Param1) : pfci);
				Param2 = (LONG_PTR)&fdi3;
				switch (Msg2)
				{
				//case Far2::DM_GETDLGITEM: Msg3 = DM_GETDLGITEM; break;
				//case Far2::DM_GETDLGITEMSHORT: Msg3 = DM_GETDLGITEMSHORT; break;
				case Far2::DM_SETDLGITEM:
					Msg3 = DM_SETDLGITEM; break;
				case Far2::DM_SETDLGITEMSHORT:
					Msg3 = DM_SETDLGITEMSHORT; break;
				case Far2::DN_EDITCHANGE:
					Msg3 = DN_EDITCHANGE; break;
				case Far2::DN_DRAWDLGITEM:
					Msg3 = DN_DRAWDLGITEM; break;
				}
				if (Msg3 == DM_SETDLGITEM || Msg3 == DM_SETDLGITEMSHORT)
				{
					Far2Dialog* p = (*gpMapDlg_3_2)[hDlg3];
					if (p)
					{
						if (p->m_Colors2 && (p->m_ItemsNumber > (UINT)Param1))
						{
							if (p2->Flags & Far2::DIF_SETCOLOR)
								p->m_Colors2[Param1] = p2->Flags & (Far2::DIF_COLORMASK|Far2::DIF_SETCOLOR);
							else
								p->m_Colors2[Param1] = 0;
						}

						if (p2->Type == Far2::DI_USERCONTROL && p2->Param.VBuf)
						{
							WRAP_CHAR_INFO& pVBuf2 = p->GetVBufPtr2(Param1);
							if (!pVBuf2.isExt && pVBuf2.p2)
								free(pVBuf2.p2);
							pVBuf2.nCount = VBufDim(p2);
							pVBuf2.p2 = p2->Param.VBuf;
							pVBuf2.isExt = true;
						}
					}
				}
			}
			break;

		case Far2::DN_ENTERIDLE:
			Msg3 = DN_ENTERIDLE; break;
		case Far2::DN_GOTFOCUS:
			Msg3 = DN_GOTFOCUS; break;
		case Far2::DN_HELP:
			Msg3 = DN_HELP; break;
		case Far2::DN_HOTKEY:
			Msg3 = DN_HOTKEY; break;
		case Far2::DN_INITDIALOG:
			Msg3 = DN_INITDIALOG; break;
		case Far2::DN_KILLFOCUS:
			Msg3 = DN_KILLFOCUS; break;
		case Far2::DN_LISTCHANGE:
			Msg3 = DN_LISTCHANGE; break;
		case Far2::DN_MOUSECLICK:
			// DN_KEY � DN_MOUSECLICK ���������� � DN_CONTROLINPUT. 
			// Param2 ��������� �� INPUT_RECORD. � ������� ����������� ������ � ������ �������.
			ZeroStruct(r);
			r.EventType = MOUSE_EVENT;
			r.Event.MouseEvent = *(MOUSE_EVENT_RECORD*)Param2;
			Msg3 = DN_CONTROLINPUT;
			break;
		case Far2::DN_DRAGGED:
			Msg3 = DN_DRAGGED; break;
		case Far2::DN_RESIZECONSOLE:
			Msg3 = DN_RESIZECONSOLE; break;
		case Far2::DN_MOUSEEVENT:
			// DN_MOUSEEVENT ������������� � DN_INPUT. Param2 ��������� �� INPUT_RECORD. 
			// � ������� ����������� ������ �� ������ ������� �������, ������� ������������ 
			// ������������� ��������� EventType.
			ZeroStruct(r);
			r.EventType = MOUSE_EVENT;
			r.Event.MouseEvent = *(MOUSE_EVENT_RECORD*)Param2;
			_ASSERTE(Msg2 != Far2::DN_MOUSEEVENT);
			//TODO: ��� ��� �����? DN_CONTROLINPUT ��� DN_INPUT?
			Msg3 = DN_CONTROLINPUT;
			break;
		case Far2::DN_DRAWDIALOGDONE:
			Msg3 = DN_DRAWDIALOGDONE; break;
		case Far2::DN_LISTHOTKEY:
			Msg3 = DN_LISTHOTKEY; break;
		//DN_GETDIALOGINFO=DM_GETDIALOGINFO, -- ����������� � Far3
		//DN_CLOSE=DM_CLOSE,
		//DN_KEY=DM_KEY, // 3. DM_KEY ������ �� ����� DN_KEY.
		default:
			// ��������� ���������� ������� ��� �� ������� � Plugin.hpp
			_ASSERTE(Msg2==(DN_FIRST-1) || Msg2==(DN_FIRST-2) || Msg2==(DM_USER-1));
			Msg3 = (FARMESSAGE)Msg2;
	}

	// ���������� �����
	if (Msg3 == DM_FIRST && Msg2 != Far2::DM_FIRST)
	{
		_ASSERTE(Msg3!=DM_FIRST || Msg2==Far2::DM_FIRST);
		lRc = 0;
	}
	else
	{
		lRc = DlgProc3(hDlg3, Msg3, Param1, (void*)Param2);
		if (Msg3 == DM_LISTGETDATA)
		{
			WrapListItemData* pd2 = (WrapListItemData*)lRc;
			if (pd2 && (pd2->nMagic == WRAPLISTDATAMAGIC))
			{
				if (pd2->nSize <= sizeof(DWORD))
					lRc = pd2->Data;
				else
					lRc = (LONG_PTR)&pd2->Data;
			}
		}
		else if (Param2 && OrgParam2 && Param2 != OrgParam2)
		{
			//FarMessageParam_3_2(hDlg3, Msg, Param1, Param2, OrgParam2, lRc);
			switch (Msg3)
			{
			case DM_GETDIALOGINFO:
				if (lRc > 0)
				{
					Far2::DialogInfo* p2 = (Far2::DialogInfo*)OrgParam2;
					if (p2 && p2->StructSize >= sizeof(Far2::DialogInfo))
					{
						p2->Id = di3.Id;
					}
				}
				break;
			case DM_GETDLGITEM:
				if (lRc > 0)
				{
					Far2::FarDialogItem* p2 = (Far2::FarDialogItem*)OrgParam2;
					//_ASSERTE(sizeof(Far2::FarDialogItem)>=sizeof(FarDialogItem));
					//FarGetDialogItem item = {lRc};
					if (!m_GetDlgItem.Item || ((INT_PTR)m_GetDlgItem.Size < lRc))
					{
						if (m_GetDlgItem.Item)
							free(m_GetDlgItem.Item);
						m_GetDlgItem.Size = lRc;
						m_GetDlgItem.Item = (FarDialogItem*)calloc(lRc, 1);
					}
					lRc = psi3.SendDlgMessage(hDlg3, DM_GETDLGITEM, Param1, &m_GetDlgItem);
					if (lRc > 0)
					{
						FarDialogItem_3_2(m_GetDlgItem.Item, /*m_GetDlgItem.Size,*/ p2, &m_ListItems2, m_FarCharInfo2);
						Far2Dialog* p = (*gpMapDlg_3_2)[hDlg3];
						if (p && p->m_Colors2 && (p->m_ItemsNumber > (UINT)Param1) && (p->m_Colors2[Param1] & Far2::DIF_SETCOLOR))
						{
							p2->Flags |= p->m_Colors2[Param1];
						}
					}
					//free(item.Item);
				}
				break;
			case DM_GETDLGITEMSHORT:
				{
					const FarDialogItem* p3 = (const FarDialogItem*)Param2;
					Far2::FarDialogItem* p2 = (Far2::FarDialogItem*)OrgParam2;
					// � Far2 DM_GETDLGITEMSHORT ��������� ListPos!
					FarDialogItem_3_2(p3, /*0,*/ p2, &m_ListItems2, m_FarCharInfo2, TRUE);
					p2->Param.ListPos = (int)psi3.SendDlgMessage(hDlg3, DM_LISTGETCURPOS, Param1, NULL);
					Far2Dialog* ps = (*gpMapDlg_3_2)[hDlg3];
					if (ps && ps->m_Colors2 && (ps->m_ItemsNumber > (UINT)Param1) && (ps->m_Colors2[Param1] & Far2::DIF_SETCOLOR))
					{
						p2->Flags |= ps->m_Colors2[Param1];
					}
				}
				break;
			case DM_LISTGETITEM:
				{
					const FarListGetItem* p3 = (const FarListGetItem*)Param2;
					_ASSERTE(p3->StructSize==sizeof(*p3));
					Far2::FarListGetItem* p2 = (Far2::FarListGetItem*)OrgParam2;
					p2->ItemIndex = p3->ItemIndex;
					FarListItem_3_2(&p3->Item, &p2->Item);
				}
				break;
			case DM_LISTINFO:
				{
					const FarListInfo* p3 = (const FarListInfo*)Param2;
					_ASSERTE(p3->StructSize==sizeof(*p3));
					Far2::FarListInfo* p2 = (Far2::FarListInfo*)OrgParam2;
					//TODO: ����������� ������
					p2->Flags = p3->Flags;
					p2->ItemsNumber = p3->ItemsNumber;
					p2->SelectPos = p3->SelectPos;
					p2->TopPos = p3->TopPos;
					p2->MaxHeight = p3->MaxHeight;
					p2->MaxLength = p3->MaxLength;
					#if MVV_3<=2798
					p2->Reserved[0] = p3->Reserved[0];
					p2->Reserved[1] = p3->Reserved[1];
					p2->Reserved[2] = p3->Reserved[2];
					p2->Reserved[3] = p3->Reserved[3];
					p2->Reserved[4] = p3->Reserved[4];
					p2->Reserved[5] = p3->Reserved[5];
					#endif
				}
				break;
			case DM_LISTGETCURPOS:
				{
					const FarListPos* p3 = (const FarListPos*)Param2;
					_ASSERTE(p3->StructSize==sizeof(*p3));
					Far2::FarListPos* p2 = (Far2::FarListPos*)OrgParam2;
					p2->SelectPos = p3->SelectPos;
					p2->TopPos = p3->TopPos;
				}
				break;
			case DM_LISTGETTITLES:
				{
					const FarListTitles* p3 = (const FarListTitles*)Param2;
					_ASSERTE(p3->StructSize==sizeof(*p3));
					Far2::FarListTitles* p2 = (Far2::FarListTitles*)OrgParam2;
					p2->TitleLen = p3->TitleSize;
					p2->Title = p3->Title;
					p2->BottomLen = p3->BottomSize;
					p2->Bottom = p3->Bottom;
				}
				break;
			}
		}
	}

	if (pfci.p != NULL)
		free(pfci.p);
	if (pwlid2)
		free(pwlid2);
	return lRc;
}

// VP 2013.01.20: Many structs differ only by a StructSize field at the beginning.
// This macro moves the pointer towards an otherwise-identical structure.
#define STRIP_STRUCTSIZE(Struct, Pointer)	\
	if (sizeof(size_t)+sizeof(Far2::Struct)!=sizeof(Struct)) AssertStructSize(sizeof(Struct), sizeof(size_t)+sizeof(Far2::Struct), L#Struct, __FILE__, __LINE__); \
	(*(size_t**)&Pointer)++;

Far2::FarMessagesProc WrapPluginInfo::FarMessage_3_2(const int Msg3, const int Param1, void*& Param2, Far2Dialog* pDlg)
{
	Far2::FarMessagesProc Msg2 = Far2::DM_FIRST;
	//if (Msg3 < DM_USER)
	//{
	//	gnMsg_3 = (FARMESSAGE)Msg3;
	//	gnParam1_2 = gnParam1_3 = Param1;
	//	gpParam2_3 = Param2;
	//}

	if (Msg3 >= DM_USER)
	{
		Msg2 = (Far2::FarMessagesProc)Msg3;
	}
	else switch (Msg3)
	{
		// ����������� ����������� ���������� ��� ��������� ���������
		case DM_KEY:
		case DN_INPUT:
		case DN_CONTROLINPUT:
			if (!Param2)
			{
				_ASSERTE(Param2!=0);
				Msg2 = Far2::DM_FIRST;
			}
			else
			{
				//#error ������ ����� ���� �� ��������
				const INPUT_RECORD* p = (const INPUT_RECORD*)Param2;
				if (p->EventType == MOUSE_EVENT)
				{
					static MOUSE_EVENT_RECORD mer;
					ZeroStruct(mer);
					mer = p->Event.MouseEvent;
					Param2 = &mer;
					if (Msg3 == DN_CONTROLINPUT)
					{
						if (p->Event.MouseEvent.dwEventFlags == 0 || p->Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)
							Msg2 = Far2::DN_MOUSECLICK;
						else if (p->Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
							Msg2 = Far2::DN_MOUSEEVENT;
						else if (p->Event.MouseEvent.dwEventFlags == MOUSE_WHEELED)
						{
							Msg2 = Far2::DN_KEY;
							int Key2 = (((short)HIWORD(p->Event.MouseEvent.dwButtonState)) > 0) ? Far2::KEY_MSWHEEL_UP : Far2::KEY_MSWHEEL_DOWN;
							Param2 = (void*)Key2;
						}
						else if (p->Event.MouseEvent.dwEventFlags == MOUSE_HWHEELED)
						{
							Msg2 = Far2::DN_KEY;
							int Key2 = (((short)HIWORD(p->Event.MouseEvent.dwButtonState)) > 0) ? Far2::KEY_MSWHEEL_RIGHT : Far2::KEY_MSWHEEL_LEFT;
							Param2 = (void*)Key2;
						}
					}
					else
						Msg2 = Far2::DN_MOUSEEVENT;
					_ASSERTE(Msg3!=DM_KEY);
				}
				else if (p->EventType == KEY_EVENT)
				{
					Param2 = (void*)FarKeyEx_3_2(p, true);
					Msg2 = (Msg3 == DM_KEY) ? Far2::DM_KEY : Far2::DN_KEY;
				}
				else
					Msg2 = Far2::DM_FIRST;
			}
			break;

		case DM_FIRST:
			Msg2 = Far2::DM_FIRST; break;
		case DM_CLOSE:
			Msg2 = Far2::DM_CLOSE; break;
		case DM_ENABLE:
			Msg2 = Far2::DM_ENABLE; break;
		case DM_ENABLEREDRAW:
			Msg2 = Far2::DM_ENABLEREDRAW; break;
		case DM_GETDLGDATA:
			Msg2 = Far2::DM_GETDLGDATA; break;
		case DM_GETDLGRECT:
			Msg2 = Far2::DM_GETDLGRECT; break;
		case DM_GETTEXT:
			Msg2 = Far2::DM_GETTEXT; break;
		#if MVV_3<=2798
		case DM_GETTEXTLENGTH:
			Msg2 = Far2::DM_GETTEXTLENGTH; break;
		#endif
		//case DM_KEY:
		//	//TODO: ���������?
		//	_ASSERTE(Msg3!=DM_KEY);
		//	Msg2 = Far2::DM_KEY;
		//	break;
		case DM_MOVEDIALOG:
			Msg2 = Far2::DM_MOVEDIALOG; break;
		case DM_SETDLGDATA:
			Msg2 = Far2::DM_SETDLGDATA; break;
		case DM_SETFOCUS:
			Msg2 = Far2::DM_SETFOCUS; break;
		case DM_REDRAW:
			Msg2 = Far2::DM_REDRAW; break;
		case DM_SETTEXT:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarDialogItemData* p3 = (const FarDialogItemData*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarDialogItemData p2;
				p2.PtrLength = p3->PtrLength;
				p2.PtrData = p3->PtrData;
				Param2 = &p2;
				Msg2 = Far2::DM_SETTEXT;
			}
			break;
		case DM_SETMAXTEXTLENGTH:
			Msg2 = Far2::DM_SETMAXTEXTLENGTH; break;
		case DM_SHOWDIALOG:
			Msg2 = Far2::DM_SHOWDIALOG; break;
		case DM_GETFOCUS:
			Msg2 = Far2::DM_GETFOCUS; break;
		case DM_GETCURSORPOS:
			Msg2 = Far2::DM_GETCURSORPOS; break;
		case DM_SETCURSORPOS:
			Msg2 = Far2::DM_SETCURSORPOS; break;
		#if MVV_3<=2798
		case DM_GETTEXTPTR:
			Msg2 = Far2::DM_GETTEXTPTR; break;
		#endif
		case DM_SETTEXTPTR:
			Msg2 = Far2::DM_SETTEXTPTR; break;
		case DM_SHOWITEM:
			Msg2 = Far2::DM_SHOWITEM; break;
		case DM_ADDHISTORY:
			Msg2 = Far2::DM_ADDHISTORY; break;
		case DM_GETCHECK:
			Msg2 = Far2::DM_GETCHECK; break;
		case DM_SETCHECK:
			Msg2 = Far2::DM_SETCHECK; break;
		case DM_SET3STATE:
			Msg2 = Far2::DM_SET3STATE; break;
		case DM_LISTSORT:
			Msg2 = Far2::DM_LISTSORT; break;
		case DM_LISTGETITEM:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListGetItem* p3 = (const FarListGetItem*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListGetItem p2;
				p2.ItemIndex = p3->ItemIndex;
				FarListItem_3_2(&p3->Item, &p2.Item);
				Param2 = &p2;
				Msg2 = Far2::DM_LISTGETITEM;
			}
			break;
		case DM_LISTGETCURPOS: //Msg2 = Far2::DM_LISTGETCURPOS; break;
		case DM_LISTSETCURPOS: //Msg2 = Far2::DM_LISTSETCURPOS; break;
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListPos* p3 = (const FarListPos*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListPos p2;
				p2.SelectPos = p3->SelectPos;
				p2.TopPos = p3->TopPos;
				Param2 = &p2;
				switch (Msg3)
				{
				case DM_LISTGETCURPOS:
					Msg2 = Far2::DM_LISTGETCURPOS; break;
				case DM_LISTSETCURPOS:
					Msg2 = Far2::DM_LISTSETCURPOS; break;
				}
			}
			break;
		case DM_LISTDELETE:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListDelete* p3 = (const FarListDelete*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListDelete p2;
				p2.StartIndex = p3->StartIndex;
				p2.Count = p3->Count;
				Param2 = &p2;
				Msg2 = Far2::DM_LISTDELETE;
			}
			break;
		case DM_LISTADD:
		case DM_LISTSET:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarList* p3 = (const FarList*)Param2;
				//_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarList p2;
				p2.ItemsNumber = p3->ItemsNumber;
				if (!gpListItems2 || (gnListItemsMax2 < p3->ItemsNumber))
				{
					if (gpListItems2)
					{
						free(gpListItems2);
						gpListItems2 = NULL;
					}
					gnListItemsMax2 = p3->ItemsNumber;
					gpListItems2 = (Far2::FarListItem*)calloc(gnListItemsMax2,sizeof(*gpListItems2));
				}
				if (gpListItems2)
				{
					const FarListItem* pp3 = p3->Items;
					Far2::FarListItem* pp2 = gpListItems2;
					for (size_t i = 0; i < p3->ItemsNumber; i++, pp2++, pp3++)
					{
						FarListItem_3_2(pp3, pp2);
					}
					p2.Items = gpListItems2;
					Param2 = &p2;
					switch (Msg3)
					{
					case DM_LISTADD:
						Msg2 = Far2::DM_LISTADD; break;
					case DM_LISTSET:
						Msg2 = Far2::DM_LISTSET; break;
					}
				}
			}
			break;
		case DM_LISTADDSTR:
			Msg2 = Far2::DM_LISTADDSTR; break;
		case DM_LISTUPDATE:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListUpdate* p3 = (const FarListUpdate*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListUpdate p2;
				p2.Index = p3->Index;
				FarListItem_3_2(&p3->Item, &p2.Item);
				Param2 = &p2;
				Msg2 = Far2::DM_LISTUPDATE;
			}
			break;
		case DM_LISTINSERT:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListInsert* p3 = (const FarListInsert*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListInsert p2;
				p2.Index = p3->Index;
				FarListItem_3_2(&p3->Item, &p2.Item);
				Param2 = &p2;
				Msg2 = Far2::DM_LISTINSERT;
			}
			break;
		case DM_LISTFINDSTRING:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListFind* p3 = (const FarListFind*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListFind p2;
				p2.StartIndex = p3->StartIndex;
				p2.Pattern = p3->Pattern;
				//TODO: ����������� ������
				p2.Flags = p3->Flags;
				#if MVV_3<=2798
				p2.Reserved = p3->Reserved;
				#endif
				Param2 = &p2;
				Msg2 = Far2::DM_LISTFINDSTRING;
			}
			break;
		case DM_LISTINFO:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListInfo* p3 = (const FarListInfo*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListInfo p2;
				//TODO: ����������� ������
				p2.Flags = p3->Flags;
				p2.ItemsNumber = p3->ItemsNumber;
				p2.SelectPos = p3->SelectPos;
				p2.TopPos = p3->TopPos;
				p2.MaxHeight = p3->MaxHeight;
				p2.MaxLength = p3->MaxLength;
				#if MVV_3<=2798
				p2.Reserved[0] = p3->Reserved[0];
				p2.Reserved[1] = p3->Reserved[1];
				p2.Reserved[2] = p3->Reserved[2];
				p2.Reserved[3] = p3->Reserved[3];
				p2.Reserved[4] = p3->Reserved[4];
				p2.Reserved[5] = p3->Reserved[5];
				#endif
				Param2 = &p2;
				Msg2 = Far2::DM_LISTINFO;
			}
			break;
		case DM_LISTGETDATA:
			Msg2 = Far2::DM_LISTGETDATA; break;
		case DM_LISTSETDATA:
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListItemData* p3 = (const FarListItemData*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListItemData p2;
				p2.Index = p3->Index;
				p2.DataSize = p3->DataSize;
				p2.Data = p3->Data;
				#if MVV_3<=2798
				p2.Reserved = p3->Reserved;
				#endif
				Param2 = &p2;
				Msg2 = Far2::DM_LISTSETDATA;
			}
			break;
		case DM_LISTSETTITLES: //Msg2 = Far2::DM_LISTSETTITLES; break;
		case DM_LISTGETTITLES: //Msg2 = Far2::DM_LISTGETTITLES; break;
			if (!Param2)
			{
				_ASSERTE(Param2!=NULL);
			}
			else
			{
				const FarListTitles* p3 = (const FarListTitles*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListTitles p2;
				p2.TitleLen = p3->TitleSize;
				p2.Title = p3->Title;
				p2.BottomLen = p3->BottomSize;
				p2.Bottom = p3->Bottom;
				Param2 = &p2;
				switch (Msg2)
				{
				case DM_LISTSETTITLES:
					Msg2 = Far2::DM_LISTSETTITLES; break;
				case DM_LISTGETTITLES:
					Msg2 = Far2::DM_LISTGETTITLES; break;
				}
			}
			break;
		case DM_RESIZEDIALOG:
			Msg2 = Far2::DM_RESIZEDIALOG; break;
		case DM_SETITEMPOSITION:
			Msg2 = Far2::DM_SETITEMPOSITION; break;
		case DM_GETDROPDOWNOPENED:
			Msg2 = Far2::DM_GETDROPDOWNOPENED; break;
		case DM_SETDROPDOWNOPENED:
			Msg2 = Far2::DM_SETDROPDOWNOPENED; break;
		case DM_SETHISTORY:
			Msg2 = Far2::DM_SETHISTORY; break;
		case DM_GETITEMPOSITION:
			Msg2 = Far2::DM_GETITEMPOSITION; break;
		case DM_SETMOUSEEVENTNOTIFY:
			Msg2 = Far2::DM_SETMOUSEEVENTNOTIFY; break;
		case DM_EDITUNCHANGEDFLAG:
			Msg2 = Far2::DM_EDITUNCHANGEDFLAG; break;
		case DM_GETITEMDATA:
			Msg2 = Far2::DM_GETITEMDATA; break;
		case DM_SETITEMDATA:
			Msg2 = Far2::DM_SETITEMDATA; break;
		#if MVV_3<2116
		// build 2116:
		// DM_LISTSETMOUSEREACTION ������ ���. �������� ��������� ������ �������� �������
		// DIF_LISTTRACKMOUSE/DIF_LISTTRACKMOUSEINFOCUS ��� �������� �������.
		// �� ��������� ��� ����� �������� ����� �� ����������� (������ �� ����� �� ������),
		// ��� ansi-�������� ���������� DIF_LISTTRACKMOUSE (������ ������).
		case DM_LISTSETMOUSEREACTION:
			Msg2 = Far2::DM_LISTSETMOUSEREACTION; break;
		#endif
		case DM_GETCURSORSIZE:
			Msg2 = Far2::DM_GETCURSORSIZE; break;
		case DM_SETCURSORSIZE:
			Msg2 = Far2::DM_SETCURSORSIZE; break;
		case DM_LISTGETDATASIZE:
			Msg2 = Far2::DM_LISTGETDATASIZE; break;
		case DM_GETSELECTION: // Msg2 = Far2::DM_GETSELECTION; break;
		case DM_SETSELECTION: // Msg2 = Far2::DM_SETSELECTION; break;
			#if MVV_3<2801
			ASSERTSTRUCT(EditorSelect);
			#else
			STRIP_STRUCTSIZE(EditorSelect, Param2);
			#endif
			switch (Msg2)
			{
			case DM_GETSELECTION: Msg2 = Far2::DM_GETSELECTION; break;
			case DM_SETSELECTION: Msg2 = Far2::DM_SETSELECTION; break;
			}
			break;
		case DM_GETEDITPOSITION: // Msg2 = Far2::DM_GETEDITPOSITION; break;
		case DM_SETEDITPOSITION: // Msg2 = Far2::DM_SETEDITPOSITION; break;
			#if MVV_3<2801
			ASSERTSTRUCT(EditorSetPosition);
			#else
			STRIP_STRUCTSIZE(EditorSetPosition, Param2);
			#endif
			switch (Msg2)
			{
			case DM_GETEDITPOSITION: Msg2 = Far2::DM_GETEDITPOSITION; break;
			case DM_SETEDITPOSITION: Msg2 = Far2::DM_SETEDITPOSITION; break;
			}
			break;
		case DM_SETCOMBOBOXEVENT:
			Msg2 = Far2::DM_SETCOMBOBOXEVENT; break;
		case DM_GETCOMBOBOXEVENT:
			Msg2 = Far2::DM_GETCOMBOBOXEVENT; break;
		case DM_GETCONSTTEXTPTR:
			Msg2 = Far2::DM_GETCONSTTEXTPTR; break;
		case DM_GETDIALOGINFO:
			Msg2 = Far2::DM_GETDIALOGINFO; break;
		case DN_FIRST:
			Msg2 = Far2::DN_FIRST; break;
		case DN_BTNCLICK:
			Msg2 = Far2::DN_BTNCLICK; break;
		case DN_CTLCOLORDIALOG:
			Msg2 = Far2::DN_CTLCOLORDIALOG;
			if (!Param2)
			{
				_ASSERTE(Param2!=0);
			}
			else
			{
				FarColor* p3 = (FarColor*)Param2;
				LONG_PTR Color2 = FarColor_3_2(*p3);
				Param2 = (void*)Color2;
			}
			break;
		case DN_CTLCOLORDLGITEM:
			Msg2 = Far2::DN_CTLCOLORDLGITEM;
			if (!Param2)
			{
				_ASSERTE(Param2!=0);
			}
			else
			{
				FarDialogItemColors* p3 = (FarDialogItemColors*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				//_ASSERTE((p3->Flags & (FCF_FG_4BIT|FCF_BG_4BIT)) == (FCF_FG_4BIT|FCF_BG_4BIT));
				LONG_PTR Color2 = 0;
				if (p3->ColorsCount >= 1 /*&& IsFarColorValid(p3->Colors[0])*/)
					Color2 |= FarColor_3_2(p3->Colors[0]);
				if (p3->ColorsCount >= 2 /*&& IsFarColorValid(p3->Colors[1])*/)
					Color2 |= (((DWORD)FarColor_3_2(p3->Colors[1])) << 8);
				if (p3->ColorsCount >= 3 /*&& IsFarColorValid(p3->Colors[2])*/)
					Color2 |= (((DWORD)FarColor_3_2(p3->Colors[2])) << 16);
				if (p3->ColorsCount >= 4 /*&& IsFarColorValid(p3->Colors[3])*/)
					Color2 |= (((DWORD)FarColor_3_2(p3->Colors[3])) << 24);
				Param2 = (void*)Color2;
			}
			break;

		case DN_CTLCOLORDLGLIST:
			if (!Param2)
			{
				_ASSERTE(Param2!=0);
			}
			else
			{
				const FarDialogItemColors* p3 = (const FarDialogItemColors*)Param2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				static Far2::FarListColors p2;
				p2.Flags = /*p3->Flags*/ 0; // �����. ���� ������ ���� ����� 0.
				p2.Reserved = 0/*p3->Reserved*/;
				p2.ColorCount = min(p3->ColorsCount,ARRAYSIZE(m_BufColors));
				for (int i = 0; i < p2.ColorCount; i++)
					m_BufColors[i] = FarColor_3_2(p3->Colors[i]);
				p2.Colors = m_BufColors;
				Param2 = &p2;
				Msg2 = Far2::DN_CTLCOLORDLGLIST;
			}
			break;
		case DN_DRAWDIALOG:
			Msg2 = Far2::DN_DRAWDIALOG; break;

		case DM_GETDLGITEM:
			_ASSERTE(Msg3!=DM_GETDLGITEM);
			break;
		case DM_GETDLGITEMSHORT:
			_ASSERTE(Msg3!=DM_GETDLGITEMSHORT);
			break;
		case DM_SETDLGITEM:
		case DM_SETDLGITEMSHORT:
		case DN_EDITCHANGE:
		case DN_DRAWDLGITEM:
			#ifdef _DEBUG
			if (Msg3 == DN_DRAWDLGITEM)
			{
				int nDbg = 0;
			}
			#endif
			if (!Param2)
			{
				_ASSERTE(Param2!=0);
			}
			else
			{
				const FarDialogItem* p3 = (const FarDialogItem*)Param2;
				static Far2::FarDialogItem p2;
				memset(&p2, 0, sizeof(p2));
				FarDialogItem_3_2(p3, /*0,*/ &p2, &m_ListItems2, (pDlg==NULL) ? m_FarCharInfo2 : pDlg->GetVBufPtr2(Param1));
				Param2 = &p2;
				switch (Msg3)
				{
				//case DM_GETDLGITEM:Msg2 = Far2::DM_GETDLGITEM; break;
				//case DM_GETDLGITEMSHORT: Msg2 = Far2::DM_GETDLGITEMSHORT; break;
				case DM_SETDLGITEM:
					Msg2 = Far2::DM_SETDLGITEM; break;
				case DM_SETDLGITEMSHORT:
					Msg2 = Far2::DM_SETDLGITEMSHORT; break;
				case DN_DRAWDLGITEM:
					Msg2 = Far2::DN_DRAWDLGITEM; break;
				case DN_EDITCHANGE:
					Msg2 = Far2::DN_EDITCHANGE; break;
				}
			}
			break;
		
		case DN_ENTERIDLE:
			Msg2 = Far2::DN_ENTERIDLE; break;
		case DN_GOTFOCUS:
			Msg2 = Far2::DN_GOTFOCUS; break;
		case DN_HELP:
			Msg2 = Far2::DN_HELP; break;
		case DN_HOTKEY:
			Msg2 = Far2::DN_HOTKEY; break;
		case DN_INITDIALOG:
			Msg2 = Far2::DN_INITDIALOG; break;
		case DN_KILLFOCUS:
			Msg2 = Far2::DN_KILLFOCUS; break;
		case DN_LISTCHANGE:
			Msg2 = Far2::DN_LISTCHANGE; break;
		case DN_DRAGGED:
			Msg2 = Far2::DN_DRAGGED; break;
		case DN_RESIZECONSOLE:
			Msg2 = Far2::DN_RESIZECONSOLE; break;
		case DN_DRAWDIALOGDONE:
			Msg2 = Far2::DN_DRAWDIALOGDONE; break;
		case DN_LISTHOTKEY:
			Msg2 = Far2::DN_LISTHOTKEY; break;
		case DN_CLOSE:
			Msg2 = Far2::DN_CLOSE; break;
		default:
			// ��������� ���������� ������� ��� �� ������� � Plugin.hpp
			_ASSERTE(Msg3==(DN_FIRST-1) || Msg3==(DN_FIRST-2) || Msg3==(DM_USER-1) || Msg3==4118/*DN_GETVALUE*/);
			Msg2 = (Far2::FarMessagesProc)Msg3;
	}

	//gnMsg_3 = DM_FIRST;
	//gnMsg_2 = Msg2;
	//gnParam2_2 = (LONG_PTR)Param2;

	return Msg2;
}

void WrapPluginInfo::FarMessageParam_2_3(const int Msg2, const int Param1, const void* Param2, void* OrgParam2, LONG_PTR lRc)
{
	if (Param2 == OrgParam2 || !Param2 || !OrgParam2)
	{
		_ASSERTE(Param2 != OrgParam2 && Param2 && OrgParam2);
		return;
	}
	switch (Msg2)
	{
	case Far2::DM_GETDLGITEM:
		{
			_ASSERTE(Msg2!=Far2::DM_GETDLGITEM);
		}
		break;
	case Far2::DM_GETDLGITEMSHORT:
		{
			_ASSERTE(Msg2!=Far2::DM_GETDLGITEMSHORT);
			const Far2::FarDialogItem* p2 = (const Far2::FarDialogItem*)Param2;
			FarDialogItem* p3 = (FarDialogItem*)OrgParam2;
			FarDialogItem_2_3(p2, p3, &m_ListItems3, m_FarCharInfo3);
		}
		break;
	case Far2::DM_LISTGETITEM:
		{
			const Far2::FarListGetItem* p2 = (const Far2::FarListGetItem*)Param2;
			FarListGetItem* p3 = (FarListGetItem*)OrgParam2;
			_ASSERTE(p3->StructSize==sizeof(*p3));
			p3->StructSize = sizeof(*p3);
			p3->ItemIndex = p2->ItemIndex;
			FarListItem_2_3(&p2->Item, &p3->Item);
		}
		break;
	case Far2::DM_LISTINFO:
		{
			const Far2::FarListInfo* p2 = (const Far2::FarListInfo*)Param2;
			FarListInfo* p3 = (FarListInfo*)OrgParam2;
			_ASSERTE(p3->StructSize==sizeof(*p3));
			p3->StructSize = sizeof(*p3);
			p3->Flags = p2->Flags;
			p3->ItemsNumber = p2->ItemsNumber;
			p3->SelectPos = p2->SelectPos;
			p3->TopPos = p2->TopPos;
			p3->MaxHeight = p2->MaxHeight;
			p3->MaxLength = p2->MaxLength;
			#if MVV_3<=2798
			p3->Reserved[0] = p2->Reserved[0];
			p3->Reserved[1] = p2->Reserved[1];
			p3->Reserved[2] = p2->Reserved[2];
			p3->Reserved[3] = p2->Reserved[3];
			p3->Reserved[4] = p2->Reserved[4];
			p3->Reserved[5] = p2->Reserved[5];
			#else
			DWORD resNil[6] = {};
			_ASSERTE(memcmp(resNil, p2->Reserved, sizeof(resNil))==0);
			#endif
		}
		break;
	case Far2::DM_LISTGETCURPOS:
		{
			const Far2::FarListPos* p2 = (const Far2::FarListPos*)Param2;
			FarListPos* p3 = (FarListPos*)OrgParam2;
			_ASSERTE(p3->StructSize==sizeof(*p3));
			p3->StructSize = sizeof(*p3);
			p3->SelectPos = p2->SelectPos;
			p3->TopPos = p2->TopPos;
		}
		break;
	case Far2::DM_LISTGETTITLES:
		{
			const Far2::FarListTitles* p2 = (const Far2::FarListTitles*)Param2;
			FarListTitles* p3 = (FarListTitles*)OrgParam2;
			_ASSERTE(p3->StructSize==sizeof(*p3));
			p3->StructSize = sizeof(*p3);
			p3->TitleSize = p2->TitleLen;
			p3->Title = p2->Title;
			p3->BottomSize = p2->BottomLen;
			p3->Bottom = p2->Bottom;
		}
		break;
	case Far2::DN_DRAWDLGITEM:
		{
			const Far2::FarDialogItem* p2 = (Far2::FarDialogItem*)Param2;
			FarDialogItem* p3 = (FarDialogItem*)OrgParam2;
			if (p2->Type == Far2::DI_USERCONTROL && p2->Param.VBuf)
			{
				size_t nCount = VBufDim(p2);
				_ASSERTE((void*)p3->VBuf!=(void*)p2->Param.VBuf);
				FAR_CHAR_INFO* pChars3 = p3->VBuf; // ��� ��������
				for (size_t i = 0; i < nCount; i++)
				{
					pChars3[i].Char = p2->Param.VBuf[i].Char.UnicodeChar;
					//TODO: COMMON_LVB_UNDERSCORE/COMMON_LVB_REVERSE_VIDEO/COMMON_LVB_GRID_RVERTICAL/COMMON_LVB_GRID_LVERTICAL/COMMON_LVB_GRID_HORIZONTAL/COMMON_LVB_TRAILING_BYTE/COMMON_LVB_LEADING_BYTE
					FarColor_2_3((BYTE)(p2->Param.VBuf[i].Attributes & 0xFF), pChars3[i].Attributes);
				}
			}
		}
		break;
	case Far2::DN_CTLCOLORDIALOG:
		{
			FarColor* p3 = (FarColor*)OrgParam2;
			if (p3 && lRc)
			{
				FarColor_2_3(LOBYTE(lRc), *p3);
			}
		}
		break;
	case Far2::DN_CTLCOLORDLGITEM:
		{
			FarDialogItemColors* p3 = (FarDialogItemColors*)OrgParam2;
			if (p3 && lRc)
			{
				_ASSERTE(p3->StructSize==sizeof(*p3));
				if (p3->ColorsCount >= 0)
					FarColor_2_3(LOBYTE(LOWORD(lRc)), p3->Colors[0]);
				if (p3->ColorsCount >= 1)
					FarColor_2_3(HIBYTE(LOWORD(lRc)), p3->Colors[1]);
				if (p3->ColorsCount >= 2)
					FarColor_2_3(LOBYTE(HIWORD(lRc)), p3->Colors[2]);
				if (p3->ColorsCount >= 3)
					FarColor_2_3(HIBYTE(HIWORD(lRc)), p3->Colors[3]);
			}
		}
		break;
	case Far2::DN_CTLCOLORDLGLIST:
		{
			FarDialogItemColors* p3 = (FarDialogItemColors*)OrgParam2;
			Far2::FarListColors* p2 = (Far2::FarListColors*)Param2;
			if (p3 && p2 && lRc)
			{
				_ASSERTE(p3->StructSize==sizeof(*p3));
				int nMax = min(p2->ColorCount,(int)p3->ColorsCount);
				for (int i = 0; i < nMax; i++)
				{
					FarColor_2_3(p2->Colors[i], p3->Colors[i]);
				}
			}
		}
		break;
	}
}





KeyBarTitles* WrapPluginInfo::KeyBarTitles_2_3(const Far2::KeyBarTitles* KeyBar)
{
	m_KeyBar.CountLabels = 0;

	if (KeyBar)
	{
		size_t cnt = 0; // Max count. Real count may be less
		
		struct { wchar_t* const* Titles; DWORD ctrl; } src[] = 
		{
			{KeyBar->Titles, 0},
			{KeyBar->CtrlTitles, LEFT_CTRL_PRESSED},
			{KeyBar->AltTitles, LEFT_ALT_PRESSED},
			{KeyBar->ShiftTitles, SHIFT_PRESSED},
			{KeyBar->CtrlShiftTitles, LEFT_CTRL_PRESSED|SHIFT_PRESSED},
			{KeyBar->AltShiftTitles, LEFT_ALT_PRESSED|SHIFT_PRESSED},
			{KeyBar->CtrlAltTitles, LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED}
			//{KeyBar->CtrlTitles, Far2::PKF_CONTROL/*LEFT_CTRL_PRESSED*/},
			//{KeyBar->AltTitles, Far2::PKF_ALT/*LEFT_ALT_PRESSED*/},
			//{KeyBar->ShiftTitles, Far2::PKF_SHIFT/*SHIFT_PRESSED*/},
			//{KeyBar->CtrlShiftTitles, Far2::PKF_CONTROL/*LEFT_CTRL_PRESSED*/|Far2::PKF_SHIFT/*SHIFT_PRESSED*/},
			//{KeyBar->AltShiftTitles, Far2::PKF_ALT/*LEFT_ALT_PRESSED*/|Far2::PKF_SHIFT/*SHIFT_PRESSED*/},
			//{KeyBar->CtrlAltTitles, Far2::PKF_CONTROL/*LEFT_CTRL_PRESSED*/|Far2::PKF_ALT/*LEFT_ALT_PRESSED*/}
		};
		
		for (int i = 0; i < ARRAYSIZE(src); i++)
		{
			if (src[i].Titles) cnt += 12;
		}
		_ASSERTE(cnt<=ARRAYSIZE(m_KeyBarLabels));
		
		m_KeyBar.CountLabels = 0;
		
		if (cnt > 0)
		{
			KeyBarLabel *p = m_KeyBar.Labels;
			for (int i = 0; i < ARRAYSIZE(src); i++)
			{
				if (!src[i].Titles)
					continue;
				for (int j = 0; j < 12; j++)
				{
					if (!src[i].Titles[j])
						continue;
					p->Key.VirtualKeyCode = VK_F1 + j;
					p->Key.ControlKeyState = src[i].ctrl;
					p->Text = src[i].Titles[j];
					p->LongText = NULL; //src[i].Titles[j];
					p++;
				}
			}
			m_KeyBar.CountLabels = p - m_KeyBar.Labels;
		}
	}
	return &m_KeyBar;
}


// Changed functions
LONG_PTR WrapPluginInfo::FarApiDefDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	LOG_CMD(L"psi2.DefDlgProc(%i,%i,%i)",Msg,Param1,Param2);
	LONG_PTR lRc = CallDlgProc_2_3(psi3.DefDlgProc, hDlg, Msg, Param1, Param2);
	return lRc;
	//HANDLE hDlg3 = (*gpMapDlg_2_3)[(Far2Dialog*)hDlg];
	//if (!hDlg3) // ����� ���� NULL, ���� ��� ������ �� �� ����� �������
	//	hDlg3 = hDlg;
	//if (hDlg3)
	//{
	//	LONG_PTR OrgParam2 = Param2;
	//	FARMESSAGE Msg3 = FarMessage_2_3(Msg, Param1, Param2);
	//	_ASSERTE(Msg3!=DM_FIRST);
	//	lRc = psi3.DefDlgProc(hDlg3, Msg3, Param1, (void*)Param2);
	//	if (Param2 && Param2 != OrgParam2)
	//		FarMessageParam_3_2(hDlg3, Msg, Param1, Param2, OrgParam2, lRc);
	//}
	//else
	//{
	//	_ASSERTE(hDlg3!=NULL);
	//}
	//return lRc;
}
LONG_PTR WrapPluginInfo::FarApiSendDlgMessage(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	LOG_CMD(L"psi2.SendDlgMessage(%i,%i,%i)",Msg,Param1,Param2);
	LONG_PTR lRc = CallDlgProc_2_3(psi3.SendDlgMessage, hDlg, Msg, Param1, Param2);
	return lRc;
	//HANDLE hDlg3 = (*gpMapDlg_2_3)[(Far2Dialog*)hDlg];
	//if (!hDlg3) // ����� ���� NULL, ���� ��� ������ �� �� ����� �������
	//	hDlg3 = hDlg;
	//if (hDlg3)
	//{
	//	LONG_PTR OrgParam2 = Param2;
	//	FARMESSAGE Msg3 = FarMessage_2_3(Msg, Param1, Param2);
	//	_ASSERTE(Msg3!=DM_FIRST);
	//	lRc = psi3.SendDlgMessage(hDlg3, Msg3, Param1, (void*)Param2);
	//	if (Param2 && Param2 != OrgParam2)
	//		FarMessageParam_3_2(hDlg3, Msg, Param1, Param2, OrgParam2, lRc);
	//}
	//else
	//{
	//	_ASSERTE(hDlg3!=NULL);
	//}
	//return lRc;
}
BOOL WrapPluginInfo::FarApiShowHelp(const wchar_t *ModuleName, const wchar_t *Topic, DWORD Flags)
{
	LOG_CMD(L"psi2.ShowHelp",0,0,0);
	FARHELPFLAGS Flags3 = 0
		| ((Flags & Far2::FHELP_NOSHOWERROR) ? FHELP_NOSHOWERROR : 0)
		| ((Flags & Far2::FHELP_FARHELP) ? FHELP_FARHELP : 0)
		| ((Flags & Far2::FHELP_CUSTOMFILE) ? FHELP_CUSTOMFILE : 0)
		| ((Flags & Far2::FHELP_CUSTOMPATH) ? FHELP_CUSTOMPATH : 0)
		| ((Flags & Far2::FHELP_USECONTENTS) ? FHELP_USECONTENTS : 0);
	int iRc = psi3.ShowHelp(ModuleName, Topic, Flags3);
	return iRc;
}
HANDLE WrapPluginInfo::FarApiSaveScreen(int X1, int Y1, int X2, int Y2)
{
	LOG_CMD(L"psi2.SaveScreen",0,0,0);
	return psi3.SaveScreen(X1,Y1,X2,Y2);
}
void WrapPluginInfo::FarApiRestoreScreen(HANDLE hScreen)
{
	LOG_CMD(L"psi2.RestoreScreen",0,0,0);
	psi3.RestoreScreen(hScreen);
}
void WrapPluginInfo::FarApiText(int X, int Y, int Color, const wchar_t *Str)
{
	LOG_CMD(L"psi2.Text",0,0,0);
	FarColor c = {};
	if (Str)
		FarColor_2_3((BYTE)(Color&0xFF), c);
	psi3.Text(X,Y,&c,Str);
	#ifdef FORCE_TEXT_COMMIT
	if (Str)
		psi3.Text(0,0,&c,NULL);
	#endif
}
int WrapPluginInfo::FarApiEditor(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, int StartLine, int StartChar, UINT CodePage)
{
	LOG_CMD(L"psi2.Editor",0,0,0);
	EDITOR_FLAGS Flags3 = 0
		| ((Flags & Far2::EF_NONMODAL) ? EF_NONMODAL : 0)
		| ((Flags & Far2::EF_CREATENEW) ? EF_CREATENEW : 0)
		| ((Flags & Far2::EF_ENABLE_F6) ? EF_ENABLE_F6 : 0)
		| ((Flags & Far2::EF_DISABLEHISTORY) ? EF_DISABLEHISTORY : 0)
		| ((Flags & Far2::EF_DELETEONCLOSE) ? EF_DELETEONCLOSE : 0)
		| ((Flags & Far2::EF_IMMEDIATERETURN) ? EF_IMMEDIATERETURN : 0)
		| ((Flags & Far2::EF_DELETEONLYFILEONCLOSE) ? EF_DELETEONLYFILEONCLOSE : 0);
	// ������. DirSync ������ ����������� ����������
#if 0
	// ��������, � ����� �� ����������� �������� ��������?
	if (Flags3 & EF_NONMODAL)
	{
		INT_PTR nArea = psi3.MacroControl(MCTLARG(guid), MCTL_GETAREA, 0, 0);
		switch(nArea)
		{
			case MACROAREA_SHELL:
			case MACROAREA_INFOPANEL:
			case MACROAREA_QVIEWPANEL:
			case MACROAREA_TREEPANEL:
			case MACROAREA_VIEWER:
			case MACROAREA_EDITOR:
			case MACROAREA_SEARCH:
			case MACROAREA_AUTOCOMPLETION:
				break;
			case MACROAREA_DIALOG:
			case MACROAREA_DISKS:
			case MACROAREA_FINDFOLDER:
			case MACROAREA_HELP:
			case MACROAREA_MAINMENU:
			case MACROAREA_MENU:
			case MACROAREA_USERMENU:
			case MACROAREA_OTHER:
				Flags3 &= ~EF_NONMODAL;
				break;
			default:
				_ASSERTE(FALSE); // ����������� �������?
				Flags3 &= ~EF_NONMODAL;				
		}
	}
#endif
	int iRc = psi3.Editor(FileName, Title, X1,Y1,X2,Y2, Flags3, StartLine, StartChar, CodePage);
	return iRc;
}
int WrapPluginInfo::FarApiViewer(const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, UINT CodePage)
{
	LOG_CMD(L"psi2.Viewer",0,0,0);
	VIEWER_FLAGS Flags3 = 0
		| ((Flags & Far2::VF_NONMODAL) ? VF_NONMODAL : 0)
		| ((Flags & Far2::VF_DELETEONCLOSE) ? VF_DELETEONCLOSE : 0)
		| ((Flags & Far2::VF_ENABLE_F6) ? VF_ENABLE_F6 : 0)
		| ((Flags & Far2::VF_DISABLEHISTORY) ? VF_DISABLEHISTORY : 0)
		| ((Flags & Far2::VF_IMMEDIATERETURN) ? VF_IMMEDIATERETURN : 0)
		| ((Flags & Far2::VF_DELETEONLYFILEONCLOSE) ? VF_DELETEONLYFILEONCLOSE : 0);
	int iRc = psi3.Viewer(FileName, Title, X1,Y1,X2,Y2, Flags3, CodePage);
	return iRc;
};
int WrapPluginInfo::FarApiMenu(INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags, const wchar_t* Title, const wchar_t* Bottom, const wchar_t* HelpTopic, const int* BreakKeys, int* BreakCode, const struct Far2::FarMenuItem *Item, int ItemsNumber)
{
	LOG_CMD(L"psi2.Menu",0,0,0);
	int iRc = -1;
	FarMenuItem *pItems3 = NULL;
	FarKey *pBreak3 = NULL;
	
	if (Item && ItemsNumber > 0)
	{
		pItems3 = (FarMenuItem*)calloc(ItemsNumber, sizeof(*pItems3));
		if (!(Flags & Far2::FMENU_USEEXT))
		{
			const Far2::FarMenuItem* p2 = Item;
			FarMenuItem* p3 = pItems3;
			for (int i = 0; i < ItemsNumber; i++, p2++, p3++)
			{
				p3->Text = p2->Text;
				p3->Flags = 0
					| (p2->Selected ? MIF_SELECTED : 0)
					| (p2->Checked ? MIF_CHECKED : 0)
					| (p2->Separator ? MIF_SEPARATOR : 0);
			}
		}
		else
		{
			const Far2::FarMenuItemEx* p2 = (const Far2::FarMenuItemEx*)Item;
			FarMenuItem* p3 = pItems3;
			for (int i = 0; i < ItemsNumber; i++, p2++, p3++)
			{
				p3->Text = p2->Text;
				p3->Flags = 0
					| ((p2->Flags & Far2::MIF_SELECTED) ? MIF_SELECTED : 0)
					| ((p2->Flags & Far2::MIF_CHECKED) ? MIF_CHECKED : 0)
					| ((p2->Flags & Far2::MIF_SEPARATOR) ? MIF_SEPARATOR : 0)
					| ((p2->Flags & Far2::MIF_DISABLE) ? MIF_DISABLE : 0)
					| ((p2->Flags & Far2::MIF_GRAYED) ? MIF_GRAYED : 0)
					| ((p2->Flags & Far2::MIF_HIDDEN) ? MIF_HIDDEN : 0);
				#if MVV_3>=2189
				INPUT_RECORD r = {};
				WrapPluginInfo::FarKey_2_3(p2->AccelKey, &r);
				if (r.EventType == KEY_EVENT)
				{
					p3->AccelKey.VirtualKeyCode = r.Event.KeyEvent.wVirtualKeyCode;
					p3->AccelKey.ControlKeyState = r.Event.KeyEvent.dwControlKeyState;
				}
				else
				{
					_ASSERTE(r.EventType == KEY_EVENT);
					p3->AccelKey.VirtualKeyCode = p3->AccelKey.ControlKeyState = 0;
				}
				#else
				p3->AccelKey = p2->AccelKey;
				#endif
				#if MVV_3<=2798
				p3->Reserved = p2->Reserved;
				#else
				_ASSERTE(p2->Reserved==0);
				#endif
				p3->UserData = p2->UserData;
			}
		}
		
		if (BreakKeys)
		{
			int cnt = 0;
			for (int i = 0; BreakKeys[i]; i++)
				cnt++;
			pBreak3 = (FarKey*)calloc(cnt+1, sizeof(*pBreak3));
			for (int i = 0; BreakKeys[i]; i++)
			{
				pBreak3[i].VirtualKeyCode = LOWORD(BreakKeys[i]);
				pBreak3[i].ControlKeyState = 0
					| ((HIWORD(BreakKeys[i]) & Far2::PKF_CONTROL) ? LEFT_CTRL_PRESSED : 0)
					| ((HIWORD(BreakKeys[i]) & Far2::PKF_ALT) ? LEFT_ALT_PRESSED : 0)
					| ((HIWORD(BreakKeys[i]) & Far2::PKF_SHIFT) ? SHIFT_PRESSED : 0);
			}
		}
		
		FARMENUFLAGS Flags3 = 0
			| ((Flags & Far2::FMENU_SHOWAMPERSAND) ? FMENU_SHOWAMPERSAND : 0)
			| ((Flags & Far2::FMENU_WRAPMODE) ? FMENU_WRAPMODE : 0)
			| ((Flags & Far2::FMENU_AUTOHIGHLIGHT) ? FMENU_AUTOHIGHLIGHT : 0)
			| ((Flags & Far2::FMENU_REVERSEAUTOHIGHLIGHT) ? FMENU_REVERSEAUTOHIGHLIGHT : 0)
			| ((Flags & Far2::FMENU_CHANGECONSOLETITLE) ? FMENU_CHANGECONSOLETITLE : 0);
		
		iRc = psi3.Menu(WrapGuids(mguid_ApiMenu), X, Y, MaxHeight, Flags3,
				Title, Bottom, HelpTopic, pBreak3, BreakCode, pItems3, ItemsNumber);
	}
	
	if (pItems3)
		free(pItems3);
	if (pBreak3)
		free(pBreak3);
	return iRc;
};
int WrapPluginInfo::FarApiMessage(INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic, const wchar_t * const *Items, int ItemsNumber, int ButtonsNumber)
{
	LOG_CMD(L"psi2.Message",0,0,0);
	int iRc = -1;

	DWORD Far3Flags = 0
		| ((Flags & Far2::FMSG_WARNING) ? FMSG_WARNING : 0)
		| ((Flags & Far2::FMSG_ERRORTYPE) ? FMSG_ERRORTYPE : 0)
		| ((Flags & Far2::FMSG_KEEPBACKGROUND) ? FMSG_KEEPBACKGROUND : 0)
		| ((Flags & Far2::FMSG_LEFTALIGN) ? FMSG_LEFTALIGN : 0)
		| ((Flags & Far2::FMSG_ALLINONE) ? FMSG_ALLINONE : 0);
		
	if ((Flags & 0x000F0000) == Far2::FMSG_MB_OK)
		Far3Flags |= FMSG_MB_OK;
	else if ((Flags & 0x000F0000) == Far2::FMSG_MB_OKCANCEL)
		Far3Flags |= FMSG_MB_OKCANCEL;
	else if ((Flags & 0x000F0000) == Far2::FMSG_MB_ABORTRETRYIGNORE)
		Far3Flags |= FMSG_MB_ABORTRETRYIGNORE;
	else if ((Flags & 0x000F0000) == Far2::FMSG_MB_YESNO)
		Far3Flags |= FMSG_MB_YESNO;
	else if ((Flags & 0x000F0000) == Far2::FMSG_MB_YESNOCANCEL)
		Far3Flags |= FMSG_MB_YESNOCANCEL;
	else if ((Flags & 0x000F0000) == Far2::FMSG_MB_RETRYCANCEL)
		Far3Flags |= FMSG_MB_RETRYCANCEL;
	
	iRc = psi3.Message(WrapGuids(mguid_ApiMessage), Far3Flags, 
				HelpTopic, Items, ItemsNumber, ButtonsNumber);
	return iRc;
};
HANDLE WrapPluginInfo::FarApiDialogInit(INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const wchar_t* HelpTopic, struct Far2::FarDialogItem *Item, unsigned int ItemsNumber, DWORD Reserved, DWORD Flags, Far2::FARWINDOWPROC DlgProc, LONG_PTR Param)
{
	LOG_CMD(L"psi2.DialogInit",0,0,0);
	Far2Dialog *p = new Far2Dialog(this, X1, Y1, X2, Y2,
    	HelpTopic, Item, ItemsNumber, Flags, DlgProc, Param,
    	mguid_Plugin, mguid_Dialogs);
	return (HANDLE)p;
};
int WrapPluginInfo::FarApiDialogRun(HANDLE hDlg)
{
	LOG_CMD(L"psi2.DialogRun",0,0,0);
	Far2Dialog *p = (Far2Dialog*)hDlg;
	if (!p)
		return -1;
	return p->RunDlg();
}
void WrapPluginInfo::FarApiDialogFree(HANDLE hDlg)
{
	LOG_CMD(L"psi2.DialogFree",0,0,0);
	Far2Dialog *p = (Far2Dialog*)hDlg;
	if (!p)
		return;
	delete p;
};
int WrapPluginInfo::FarApiControl(HANDLE hPlugin, int Command, int Param1, LONG_PTR Param2)
{
	LOG_CMD(L"psi2.Control(%i,%i,%i)",Command, Param1, Param2);
	//TODO: ����������� ����������!
	int iRc = 0;
	switch (Command)
	{
	case Far2::FCTL_CLOSEPLUGIN:
		iRc = psi3.PanelControl(hPlugin, FCTL_CLOSEPANEL, Param1, (void*)Param2); break;
	case Far2::FCTL_GETPANELINFO:
		{
			Far2::PanelInfo* p2 = (Far2::PanelInfo*)Param2;
			PanelInfo p3 = {sizeof(PanelInfo)};
			iRc = psi3.PanelControl(hPlugin, FCTL_GETPANELINFO, Param1, &p3);
			if (iRc)
			{
				memset(p2, 0, sizeof(*p2));
				//TODO: ����������� ����
				p2->PanelType = p3.PanelType;
				p2->Plugin = (p3.Flags & PFLAGS_PLUGIN) == PFLAGS_PLUGIN;
				p2->PanelRect = p3.PanelRect;
				p2->ItemsNumber = p3.ItemsNumber;
				p2->SelectedItemsNumber = p3.SelectedItemsNumber;
				p2->CurrentItem = p3.CurrentItem;
				p2->TopPanelItem = p3.TopPanelItem;
				p2->Visible = (p3.Flags & PFLAGS_VISIBLE) == PFLAGS_VISIBLE;
				p2->Focus = (p3.Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS;
				//TODO: ����������� ������
				p2->ViewMode = p3.ViewMode;
				p2->ShortNames = (p3.Flags & PFLAGS_ALTERNATIVENAMES) == PFLAGS_ALTERNATIVENAMES;
				p2->SortMode = p3.SortMode;
				//TODO: ����������� ������
				p2->Flags = p3.Flags & 0x1FF;
			}
		}
		break;
	case Far2::FCTL_UPDATEPANEL:
		iRc = psi3.PanelControl(hPlugin, FCTL_UPDATEPANEL, Param1, (void*)Param2); break;
	case Far2::FCTL_REDRAWPANEL:
		{
		#if MVV_3>2800
		PanelRedrawInfo ri;
		if (Param2)
		{
			ADD_STRUCTSIZE(PanelRedrawInfo, ri, Param2);
		}
		#else
		ASSERTSTRUCT(PanelRedrawInfo);
		#endif
		iRc = psi3.PanelControl(hPlugin, FCTL_REDRAWPANEL, Param1, (void*)Param2);
		}
		break;
	case Far2::FCTL_GETCMDLINE:
		iRc = psi3.PanelControl(hPlugin, FCTL_GETCMDLINE, Param1, (void*)Param2); break;
	case Far2::FCTL_SETCMDLINE:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETCMDLINE, Param1, (void*)Param2); break;
	case Far2::FCTL_SETSELECTION:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETSELECTION, Param1, (void*)Param2); break;
	case Far2::FCTL_SETVIEWMODE:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETVIEWMODE, Param1, (void*)Param2); break;
	case Far2::FCTL_INSERTCMDLINE:
		iRc = psi3.PanelControl(hPlugin, FCTL_INSERTCMDLINE, Param1, (void*)Param2); break;
	case Far2::FCTL_SETUSERSCREEN:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETUSERSCREEN, Param1, (void*)Param2); break;
	case Far2::FCTL_SETPANELDIR:
		{
			_ASSERTE(!IsBadStringPtr((wchar_t*)Param2, MAX_PATH));
			GUID FarGuid = {};
			//TODO: ����� �� ���������� ����� FCTL_SETPANELDIR ���������� ����� ������? ������� FarGuid
			FarPanelDirectory setDir = {sizeof(setDir), (wchar_t*)Param2, NULL, FarGuid, NULL};
			iRc = psi3.PanelControl(hPlugin, FCTL_SETPANELDIRECTORY, 0, &setDir);
		}
		break;
	case Far2::FCTL_SETCMDLINEPOS:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETCMDLINEPOS, Param1, (void*)Param2); break;
	case Far2::FCTL_GETCMDLINEPOS:
		iRc = psi3.PanelControl(hPlugin, FCTL_GETCMDLINEPOS, Param1, (void*)Param2); break;
	case Far2::FCTL_SETSORTMODE:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETSORTMODE, Param1, (void*)Param2); break;
	case Far2::FCTL_SETSORTORDER:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETSORTORDER, Param1, (void*)Param2); break;
	case Far2::FCTL_GETCMDLINESELECTEDTEXT:
		{
			CmdLineSelect sel =
			{
				#if MVV_3>=2799
				sizeof(CmdLineSelect),
				#endif
				0
			};
			int iSel = psi3.PanelControl(hPlugin, FCTL_GETCMDLINESELECTION, 0, &sel);
			int nLen = ((sel.SelEnd > sel.SelStart)
					? (sel.SelEnd - sel.SelStart + 1)
					: (sel.SelStart - sel.SelEnd + 1));
			if (Param2 == 0)
			{
				iRc = (nLen+1) * sizeof(wchar_t);
			}
			else
			{
				int nCmdLen = psi3.PanelControl(hPlugin, FCTL_GETCMDLINE, 0, NULL);
				wchar_t* psz = (wchar_t*)calloc(nCmdLen+1,sizeof(*psz));
				nCmdLen = psi3.PanelControl(hPlugin, FCTL_GETCMDLINE, nCmdLen, psz);
				lstrcpyn((wchar_t*)Param2, psz+((sel.SelEnd > sel.SelStart) ? sel.SelStart : sel.SelEnd),
					min((nLen+1),Param1));
				iRc = TRUE;
			}
		}
		break;
	case Far2::FCTL_SETCMDLINESELECTION:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETCMDLINESELECTION, Param1, (void*)Param2); break;
	case Far2::FCTL_GETCMDLINESELECTION:
		iRc = psi3.PanelControl(hPlugin, FCTL_GETCMDLINESELECTION, Param1, (void*)Param2); break;
	case Far2::FCTL_CHECKPANELSEXIST:
		iRc = psi3.PanelControl(hPlugin, FCTL_CHECKPANELSEXIST, Param1, (void*)Param2); break;
	case Far2::FCTL_SETNUMERICSORT:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETNUMERICSORT, Param1, (void*)Param2); break;
	case Far2::FCTL_GETUSERSCREEN:
		iRc = psi3.PanelControl(hPlugin, FCTL_GETUSERSCREEN, Param1, (void*)Param2); break;
	case Far2::FCTL_ISACTIVEPANEL:
		iRc = psi3.PanelControl(hPlugin, FCTL_ISACTIVEPANEL, Param1, (void*)Param2); break;
	case Far2::FCTL_GETPANELITEM: //iRc = psi3.PanelControl(hPlugin, FCTL_GETPANELITEM, Param1, (void*)Param2); break;
	case Far2::FCTL_GETSELECTEDPANELITEM: //iRc = psi3.PanelControl(hPlugin, FCTL_GETSELECTEDPANELITEM, Param1, (void*)Param2); break;
	case Far2::FCTL_GETCURRENTPANELITEM: //iRc = psi3.PanelControl(hPlugin, FCTL_GETCURRENTPANELITEM, Param1, (void*)Param2); break;
		{
			FILE_CONTROL_COMMANDS Cmd3 = (Command == Far2::FCTL_GETPANELITEM) ? FCTL_GETPANELITEM :
					   (Command == Far2::FCTL_GETSELECTEDPANELITEM) ? FCTL_GETSELECTEDPANELITEM :
					   (Command == Far2::FCTL_GETCURRENTPANELITEM) ? FCTL_GETCURRENTPANELITEM 
					   : (FILE_CONTROL_COMMANDS)0;
			_ASSERTE(Cmd3!=(FILE_CONTROL_COMMANDS)0);
			size_t nItemSize = psi3.PanelControl(hPlugin, Cmd3, Param1, NULL);
			if (sizeof(PluginPanelItem) < sizeof(Far2::PluginPanelItem))
				nItemSize += (sizeof(Far2::PluginPanelItem) - sizeof(PluginPanelItem));
			if (Param2 == NULL)
			{
				ASSERTSTRUCTGT(PluginPanelItem);
				iRc = nItemSize;
			}
			else if (nItemSize)
			{
				Far2::PluginPanelItem* p2 = (Far2::PluginPanelItem*)Param2;
				FarGetPluginPanelItem p3 =
				{
					#if MVV_3>=2799
					sizeof(FarGetPluginPanelItem),
					#endif
					nItemSize
				};
				p3.Item = (PluginPanelItem*)malloc(nItemSize);
				if (p3.Item)
				{
					iRc = psi3.PanelControl(hPlugin, Cmd3, Param1, &p3);
					if (iRc)
					{
						PluginPanelItem_3_2(p3.Item, p2);
						// ��������� �����
						wchar_t* psz = (wchar_t*)(((LPBYTE)p2)+sizeof(*p2));
						if (p3.Item->FileName)
						{
							p2->FindData.lpwszFileName = psz;
							lstrcpy(psz, p3.Item->FileName);
							psz += lstrlen(psz)+1;
						}
						if (p3.Item->AlternateFileName)
						{
							p2->FindData.lpwszAlternateFileName = psz;
							lstrcpy(psz, p3.Item->AlternateFileName);
							psz += lstrlen(psz)+1;
						}
						if (p3.Item->Description)
						{
							p2->Description = psz;
							lstrcpy(psz, p3.Item->Description);
							psz += lstrlen(psz)+1;
						}
						if (p3.Item->Owner)
						{
							p2->Owner = psz;
							lstrcpy(psz, p3.Item->Owner);
							psz += lstrlen(psz)+1;
						}
						if (p3.Item->CustomColumnData && p3.Item->CustomColumnNumber > 0)
						{
							p2->CustomColumnNumber = p3.Item->CustomColumnNumber;
							p2->CustomColumnData = (const wchar_t * const *)psz;
							psz = (wchar_t*)(((LPBYTE)psz) + p2->CustomColumnNumber*sizeof(wchar_t*));
							for (INT_PTR i = 0; i < p2->CustomColumnNumber; i++)
							{
								if (p3.Item->CustomColumnData[i] && *(p3.Item->CustomColumnData[i]))
								{
									((wchar_t**)p2->CustomColumnData)[i] = psz;
									lstrcpy(psz, p3.Item->CustomColumnData[i]);
									psz += lstrlen(psz)+1;
								}
								else
								{
									((wchar_t**)p2->CustomColumnData)[i] = NULL;
								}
							}
						}
						else
						{
							p2->CustomColumnData = NULL;
							p2->CustomColumnNumber = 0;
						}
					}
					free(p3.Item);
				}
			}
		}
		break;
	case Far2::FCTL_GETPANELDIR:
		{
			INT_PTR iSize = psi3.PanelControl(hPlugin, FCTL_GETPANELDIRECTORY, 0, 0);
			if (iSize)
			{
				FarPanelDirectory* getDir = (FarPanelDirectory*)calloc(iSize, 1);
				if (getDir)
				{
					getDir->StructSize = sizeof(FarPanelDirectory);
					iSize = psi3.PanelControl(hPlugin, FCTL_GETPANELDIRECTORY, iSize, getDir);
					if (Param2)
					{
						lstrcpyn((wchar_t*)Param2, getDir->Name ? getDir->Name : L"", (int)Param1);
						iRc = lstrlen((wchar_t*)Param2);
					}
					else
					{
						iRc = getDir->Name ? (lstrlen(getDir->Name)+1) : 1;
					}
					free(getDir);
				}
			}
		}
		break;
	case Far2::FCTL_GETCOLUMNTYPES:
		iRc = psi3.PanelControl(hPlugin, FCTL_GETCOLUMNTYPES, Param1, (void*)Param2); break;
	case Far2::FCTL_GETCOLUMNWIDTHS:
		iRc = psi3.PanelControl(hPlugin, FCTL_GETCOLUMNWIDTHS, Param1, (void*)Param2); break;
	case Far2::FCTL_BEGINSELECTION:
		iRc = psi3.PanelControl(hPlugin, FCTL_BEGINSELECTION, Param1, (void*)Param2); break;
	case Far2::FCTL_ENDSELECTION:
		iRc = psi3.PanelControl(hPlugin, FCTL_ENDSELECTION, Param1, (void*)Param2); break;
	case Far2::FCTL_CLEARSELECTION:
		iRc = psi3.PanelControl(hPlugin, FCTL_CLEARSELECTION, Param1, (void*)Param2); break;
	case Far2::FCTL_SETDIRECTORIESFIRST:
		iRc = psi3.PanelControl(hPlugin, FCTL_SETDIRECTORIESFIRST, Param1, (void*)Param2); break;
	}
	return iRc;
};
int WrapPluginInfo::FarApiGetDirList(const wchar_t *Dir, struct Far2::FAR_FIND_DATA **pPanelItem, int *pItemsNumber)
{
	//std::map<Far2::FAR_FIND_DATA*,PluginPanelItem*> m_MapDirList;
	//std::map<Far2::PluginPanelItem*,PluginPanelItem*> m_MapPlugDirList;
	LOG_CMD(L"psi2.GetDirList",0,0,0);
	PluginPanelItem* p3 = NULL;
	size_t nItemsNumber3 = pItemsNumber ? *pItemsNumber : 0;
	int iRc = psi3.GetDirList(Dir, &p3, &nItemsNumber3);
	if (pItemsNumber)
		*pItemsNumber = nItemsNumber3;
	if (iRc)
	{
		*pPanelItem = (Far2::FAR_FIND_DATA*)calloc(sizeof(Far2::FAR_FIND_DATA),*pItemsNumber);
		if (!*pPanelItem)
		{
			_ASSERTE(*pPanelItem != NULL);
			psi3.FreeDirList(p3, *pItemsNumber);
			iRc = 0;
		}
		else
		{
			m_MapDirList[*pPanelItem] = p3;
			for (int i = *pItemsNumber; (i--) > 0;)
			{
				PluginPanelItem_3_2(p3+i, (*pPanelItem)+i);
			}
		}
	}
	return iRc;
};
void WrapPluginInfo::FarApiFreeDirList(struct Far2::FAR_FIND_DATA *PanelItem, int nItemsNumber)
{
	LOG_CMD(L"psi2.FreeDirList",0,0,0);
	PluginPanelItem* p3 = m_MapDirList[PanelItem];
	if (p3)
		psi3.FreeDirList(p3, nItemsNumber);
	m_MapDirList.erase(PanelItem);
	free(PanelItem);
};
int WrapPluginInfo::FarApiGetPluginDirList(INT_PTR PluginNumber, HANDLE hPlugin, const wchar_t *Dir, struct Far2::PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	_ASSERTE(PluginNumber == psi2.ModuleNumber);
	LOG_CMD(L"psi2.GetPluginDirList",0,0,0);
	PluginPanelItem* p3 = NULL;
	size_t nItemsNumber3 = pItemsNumber ? *pItemsNumber : 0;
	int iRc = psi3.GetPluginDirList(&mguid_Plugin, hPlugin, Dir, &p3, &nItemsNumber3);
	if (pItemsNumber)
		*pItemsNumber = nItemsNumber3;
	if (iRc)
	{
		*pPanelItem = (Far2::PluginPanelItem*)calloc(sizeof(Far2::PluginPanelItem),*pItemsNumber);
		if (!*pPanelItem)
		{
			_ASSERTE(*pPanelItem != NULL);
			psi3.FreeDirList(p3, *pItemsNumber);
			iRc = 0;
		}
		else
		{
			m_MapPlugDirList[*pPanelItem] = p3;
			for (int i = *pItemsNumber; (i--) > 0;)
			{
				PluginPanelItem_3_2(p3+i, (*pPanelItem)+i);
			}
		}
	}
	return iRc;
};
void WrapPluginInfo::FarApiFreePluginDirList(struct Far2::PluginPanelItem *PanelItem, int nItemsNumber)
{
	LOG_CMD(L"psi2.FreePluginDirList",0,0,0);
	PluginPanelItem* p3 = m_MapPlugDirList[PanelItem];
	if (p3)
	{
		#if MVV_3<=2798
		psi3.FreePluginDirList(p3, nItemsNumber);
		#else
		_ASSERTE(FALSE && "No hPanel information");
		psi3.FreePluginDirList(PANEL_ACTIVE, p3, nItemsNumber);
		#endif
	}
	m_MapPlugDirList.erase(PanelItem);
	free(PanelItem);
};
int WrapPluginInfo::FarApiCmpName(const wchar_t *Pattern, const wchar_t *String, int SkipPath)
{
	LOG_CMD(L"psi2.CmdName",0,0,0);
	wchar_t* pszFile = (wchar_t*)(SkipPath ? FSF3.PointToName(String) : String);
	int iRc = FSF3.ProcessName(Pattern, pszFile, 0, PN_CMPNAME);
	return iRc;
};
LPCWSTR WrapPluginInfo::FarApiGetMsg(INT_PTR PluginNumber, int MsgId)
{
	LOG_CMD(L"psi2.GetMsg(%i)",MsgId,0,0);
	LPCWSTR pszMsg = NULL;
	if (((INT_PTR)mh_Dll) == PluginNumber)
		pszMsg = psi3.GetMsg(&mguid_Plugin, MsgId);
#ifdef _DEBUG
	// ��������� ������� (EventViewer/AdvCmp2) ������� ������ ������ ������ � �������� >:|
	GUID guidSkip[] = {
		// EventViewer - F2BED8E4-2CDA-45E1-893C-5DAAED9BA67C
		{0xF2BED8E4, 0x2CDA, 0x45E1, {0x89, 0x3C, 0x5D, 0xAA, 0xED, 0x9B, 0xA6, 0x7C}},
		// AdvCmp2 - 0244A6CB-99AC-4689-AA86-408D4AA7F0A3
		{0x0244A6CB, 0x99AC, 0x4689, {0xAA, 0x86, 0x40, 0x8D, 0x4A, 0xA7, 0xF0, 0xA3}},
		// UserMgr - 7EB82D3C-B0A8-40F4-8A29-E0E37F22B19D
		{0x7EB82D3C, 0xB0A8, 0x40F4, {0x8A, 0x29, 0xE0, 0xE3, 0x7F, 0x22, 0xB1, 0x9D}},
		// SvcMgr - 64390498-E446-4F74-8BFC-9058E31BB4DB
		{0x64390498, 0xE446, 0x4F74, {0x8B, 0xFC, 0x90, 0x58, 0xE3, 0x1B, 0xB4, 0xDB}},
	};
	bool lbSkipPlugin = false;
	for (size_t n = 0; n < ARRAYSIZE(guidSkip); n++)
	{
		if (memcmp(guidSkip+n, &mguid_Plugin, sizeof(mguid_Plugin)) == 0)
			lbSkipPlugin = true;
	}
	_ASSERTE((pszMsg && *pszMsg) || lbSkipPlugin);
#endif
	return pszMsg ? pszMsg : L"";
};
DWORD WrapPluginInfo::GetFarSetting(HANDLE h, size_t Root, LPCWSTR Name)
{
	bool NeedClose = false;
	if (!h)
	{
		GUID FarGuid = {};
		FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
		if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
			return 0;
		h = sc.Handle;
		NeedClose = true;
	}

	int nValue = 0;
	FarSettingsItem fsi = {};
	#if MVV_3>2798
	fsi.StructSize = sizeof(fsi);
	#endif
	fsi.Root = Root;
	fsi.Name = Name;
	if (psi3.SettingsControl(h, SCTL_GET, 0, &fsi))
	{
		if (fsi.Type == FST_QWORD)
		{
			nValue = (DWORD)fsi.Number;
		}
		else
		{
			_ASSERTE(fsi.Type == FST_QWORD);
		}
	}
	else
	{
		#ifdef _DEBUG
		wchar_t szDbg[128];
		wsprintf(szDbg, L"psi3.SettingsControl(%s) failed\n", Name);
		DebugStr(szDbg);
		//_ASSERTE("psi3.SettingsControl failed" && 0);
		#endif
	}

	if (NeedClose)
	{
		psi3.SettingsControl(h, SCTL_FREE, 0, 0);
	}

	return nValue;
};
// ������������� ������ - �� ������� ����������� �������
INT_PTR WrapPluginInfo::GetFarSetting(HANDLE h, size_t Root, LPCWSTR Name, wchar_t* Result)
{
	if (Result)
		*Result = 0;

	bool NeedClose = false;
	if (!h)
	{
		GUID FarGuid = {};
		FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
		if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
			return 0;
		h = sc.Handle;
		NeedClose = true;
	}

	INT_PTR nValue = 0;
	FarSettingsItem fsi = {};
	#if MVV_3>2798
	fsi.StructSize = sizeof(fsi);
	#endif
	fsi.Root = Root;
	fsi.Name = Name;
	if (psi3.SettingsControl(h, SCTL_GET, 0, &fsi))
	{
		if ((fsi.Type == FST_STRING) && fsi.String)
		{
			nValue = wcslen(fsi.String);
			if (Result)
				lstrcpy(Result, fsi.String);
		}
		else
		{
			_ASSERTE((fsi.Type == FST_STRING) && fsi.String);
		}
	}
	else
	{
		#ifdef _DEBUG
		wchar_t szDbg[128];
		wsprintf(szDbg, L"psi3.SettingsControl(%s) failed\n", Name);
		DebugStr(szDbg);
		//_ASSERTE("psi3.SettingsControl failed" && 0);
		#endif
	}

	if (NeedClose)
	{
		psi3.SettingsControl(h, SCTL_FREE, 0, 0);
	}

	return nValue;
};
DWORD WrapPluginInfo::GetFarSystemSettings()
{
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
		return 0;

	DWORD nFlags = Far2::FSS_CLEARROATTRIBUTE; // Far2::FarSystemSettings

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_SYSTEM, N)) \
			nFlags |= F;

	Get(L"DeleteToRecycleBin", Far2::FSS_DELETETORECYCLEBIN);
	Get(L"UseSystemCopy", Far2::FSS_USESYSTEMCOPYROUTINE);
	Get(L"CopyOpened", Far2::FSS_COPYFILESOPENEDFORWRITING);
	Get(L"CreateUppercaseFolders", Far2::FSS_CREATEFOLDERSINUPPERCASE);
	Get(L"SaveHistory", Far2::FSS_SAVECOMMANDSHISTORY);
	Get(L"SaveFoldersHistory", Far2::FSS_SAVEFOLDERSHISTORY);
	Get(L"SaveViewHistory", Far2::FSS_SAVEVIEWANDEDITHISTORY);
	Get(L"UseRegisteredTypes", Far2::FSS_USEWINDOWSREGISTEREDTYPES);
	Get(L"AutoSaveSetup", Far2::FSS_AUTOSAVESETUP);
	Get(L"ScanSymlinks", Far2::FSS_SCANSYMLINK);

	#undef Get

	psi3.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

	return nFlags;
}
DWORD WrapPluginInfo::GetFarPanelSettings()
{
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
		return 0;

	DWORD nFlags = 0; // Far2::FarPanelSettings

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_PANEL, N)) \
			nFlags |= F;

	Get(L"ShowHidden", Far2::FPS_SHOWHIDDENANDSYSTEMFILES);
	Get(L"Highlight", Far2::FPS_HIGHLIGHTFILES);
	Get(L"AutoChangeFolder", Far2::FPS_AUTOCHANGEFOLDER);
	Get(L"SelectFolders", Far2::FPS_SELECTFOLDERS);
	Get(L"ReverseSort", Far2::FPS_ALLOWREVERSESORTMODES);
	Get(L"TotalInfo", Far2::FPS_SHOWFILESTOTALINFORMATION);
	Get(L"FreeInfo", Far2::FPS_SHOWFREESIZE);
	Get(L"Scrollbar", Far2::FPS_SHOWSCROLLBAR);
	Get(L"ScreensNumber", Far2::FPS_SHOWBACKGROUNDSCREENSNUMBER);

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_PANELLAYOUT, N)) \
			nFlags |= F;

	Get(L"ColumnTitles", Far2::FPS_SHOWCOLUMNTITLES);
	Get(L"StatusLine", Far2::FPS_SHOWSTATUSLINE);
	Get(L"SortMode", Far2::FPS_SHOWSORTMODELETTER);

	#undef Get

	psi3.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

	return nFlags;
}
DWORD WrapPluginInfo::GetFarInterfaceSettings()
{
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
		return 0;

	DWORD nFlags = 0; // Far2::FarInterfaceSettings

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_SCREEN, N)) \
			nFlags |= F;

	Get(L"Clock", Far2::FIS_CLOCKINPANELS);
	Get(L"ViewerEditorClock", Far2::FIS_CLOCKINVIEWERANDEDITOR);
	Get(L"Mouse", Far2::FIS_MOUSE);
	Get(L"KeyBar", Far2::FIS_SHOWKEYBAR);
	Get(L"CopyShowTotal", Far2::FIS_SHOWTOTALCOPYPROGRESSINDICATOR);
	Get(L"CopyTimeRule", Far2::FIS_SHOWCOPYINGTIMEINFO);
	Get(L"CtrlPgUp", Far2::FIS_USECTRLPGUPTOCHANGEDRIVE);
	Get(L"DelShowTotal", Far2::FIS_SHOWTOTALDELPROGRESSINDICATOR);

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_INTERFACE, N)) \
			nFlags |= F;

	Get(L"ShowMenuBar", Far2::FIS_ALWAYSSHOWMENUBAR);

	#undef Get

	psi3.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

	return nFlags;
}
DWORD WrapPluginInfo::GetFarConfirmations()
{
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
		return 0;

	DWORD nFlags = 0; // Far2::FarConfirmationsSettings

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_CONFIRMATIONS, N)) \
			nFlags |= F;

	Get(L"Copy", Far2::FCS_COPYOVERWRITE);
	Get(L"Move", Far2::FCS_MOVEOVERWRITE);
	Get(L"Drag", Far2::FCS_DRAGANDDROP);
	Get(L"Delete", Far2::FCS_DELETE);
	Get(L"DeleteFolder", Far2::FCS_DELETENONEMPTYFOLDERS);
	Get(L"Esc", Far2::FCS_INTERRUPTOPERATION);
	Get(L"RemoveConnection", Far2::FCS_DISCONNECTNETWORKDRIVE);
	Get(L"AllowReedit", Far2::FCS_RELOADEDITEDFILE);
	Get(L"HistoryClear", Far2::FCS_CLEARHISTORYLIST);
	Get(L"Exit", Far2::FCS_EXIT);
	Get(L"RO", Far2::FCS_OVERWRITEDELETEROFILES);

	#undef Get

	psi3.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

	return nFlags;
}
DWORD WrapPluginInfo::GetFarDescSettings()
{
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
		return 0;

	DWORD nFlags = 0; // Far2::FarDescriptionSettings

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_ROOT/*Private*/, N)) \
			nFlags |= F;

	DWORD nUpd = GetFarSetting(sc.Handle, FSSF_ROOT/*Private*/, L"UpdateMode");
	if (nUpd == 1)
		nFlags |= Far2::FDS_UPDATEIFDISPLAYED;
	else if (nUpd == 2)
		nFlags |= Far2::FDS_UPDATEALWAYS;
	Get(L"SetHidden", Far2::FDS_SETHIDDEN);
	Get(L"ROUpdate", Far2::FDS_UPDATEREADONLY);

	#undef Get

	psi3.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

	return nFlags;
}
DWORD WrapPluginInfo::GetFarDialogSettings()
{
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (!psi3.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc) || !sc.Handle)
		return 0;

	DWORD nFlags = 0; // Far2::FarDialogSettings

	#undef Get
	#define Get(N,F) \
		if (GetFarSetting(sc.Handle, FSSF_DIALOG, N)) \
			nFlags |= F;

	Get(L"EditHistory", Far2::FDIS_HISTORYINDIALOGEDITCONTROLS);
	Get(L"EditBlock", Far2::FDIS_PERSISTENTBLOCKSINEDITCONTROLS);
	Get(L"AutoComplete", Far2::FDIS_AUTOCOMPLETEININPUTLINES);
	Get(L"EULBsClear", Far2::FDIS_BSDELETEUNCHANGEDTEXT);
	Get(L"DelRemovesBlocks", Far2::FDIS_DELREMOVESBLOCKS);
	//TODO: ...
	//Get(L"MouseButton", Far2::FDIS_MOUSECLICKOUTSIDECLOSESDIALOG);

	#undef Get

	psi3.SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

	return nFlags;
}
INT_PTR WrapPluginInfo::FarApiAdvControl(INT_PTR ModuleNumber, int Command, void *Param)
{
	LOG_CMD(L"psi2.AdvControl(%i,x%08X)",Command,(DWORD)Param,0);
	INT_PTR iRc = 0;
	GUID guid = {0};
	if (((INT_PTR)mh_Dll) == ModuleNumber)
	{
		_ASSERTE(((INT_PTR)mh_Dll) == ModuleNumber);
		guid = mguid_Plugin;
	}
	
	switch (Command)
	{
	case Far2::ACTL_GETFARVERSION:
		{
			if (mn_FakeFarVersion != 0)
			{
				iRc = mn_FakeFarVersion;
			}
			else
			{
				VersionInfo vi = {0};
				iRc = psi3.AdvControl(&guid, ACTL_GETFARMANAGERVERSION, 0, &vi);
				if (iRc)
				{
					DWORD ver = MAKEFARVERSION2(vi.Major, vi.Minor, vi.Build);
					if (Param)
						*((DWORD*)Param) = ver;
					iRc = ver;
				}
			}
			break;
		}
	case Far2::ACTL_GETSYSWORDDIV:
		// ������������� ������ - �� ������� ����������� �������
		{
			#if MVV_3<=2540
			INT_PTR nSize = Param ? psi3.AdvControl(&guid, ACTL_GETSYSWORDDIV, 0, NULL) : 0;
			iRc = psi3.AdvControl(&guid, ACTL_GETSYSWORDDIV, nSize, Param);
			#else
			iRc = GetFarSetting(NULL, FSSF_EDITOR, L"WordDiv", (wchar_t*)Param);
			#endif
		}
		break;
	case Far2::ACTL_WAITKEY:
		iRc = psi3.AdvControl(&guid, ACTL_WAITKEY, 0, Param); break;
	case Far2::ACTL_GETCOLOR:
		{
			FarColor fc3 = {};
			//_ASSERTE(Far2::COL_DIALOGDISABLED == COL_DIALOGDISABLED); -- ��� ��� �� ���
			int Far3Index = FarColorIndex_2_3((int)(INT_PTR)Param);
			iRc = psi3.AdvControl(&guid, ACTL_GETCOLOR, Far3Index, &fc3);
			if (iRc)
				iRc = FarColor_3_2(fc3);
		}
		break;
	case Far2::ACTL_GETARRAYCOLOR:
		{
			if (!Param)
			{
				iRc = Far2::COL_LASTPALETTECOLOR+1; // fixed for Far2
			}
			else
			{
				memset(Param, 0, Far2::COL_LASTPALETTECOLOR+1);
				iRc = psi3.AdvControl(&guid, ACTL_GETARRAYCOLOR, 0, 0);
				if (iRc > 0)
				{
					FarColor* pColors = (FarColor*)calloc(iRc,sizeof(*pColors));
					iRc = pColors ? psi3.AdvControl(&guid, ACTL_GETARRAYCOLOR, iRc, pColors) : 0;
					for (INT_PTR i = 0; i < iRc; i++)
					{
						UINT Far2Index = FarColorIndex_3_2(i);
						if (Far2Index < Far2::COL_LASTPALETTECOLOR)
							((LPBYTE)Param)[Far2Index] = FarColor_3_2(pColors[i]);
					}
					free(pColors);
				}
			}
		}
		break;
	case Far2::ACTL_EJECTMEDIA:
		#if MVV_3<=2798
		ASSERTSTRUCT(ActlEjectMedia);
		iRc = psi3.AdvControl(&guid, ACTL_EJECTMEDIA, 0, Param);
		#endif
		break;
	case Far2::ACTL_KEYMACRO:
		{
			if (Param)
			{
				Far2::ActlKeyMacro* p2 = (Far2::ActlKeyMacro*)Param;
				#ifdef LOG_COMMANDS
				wchar_t szDbgInfo[255]; szDbgInfo[0] = 0;
				switch (p2->Command)
				{
				case Far2::MCMD_LOADALL:
					lstrcpy(szDbgInfo, L"Far2::MCMD_LOADALL");
					break;
				case Far2::MCMD_SAVEALL:
					lstrcpy(szDbgInfo, L"Far2::MCMD_SAVEALL");
					break;
				case Far2::MCMD_POSTMACROSTRING:
					lstrcpy(szDbgInfo, L"Far2::MCMD_POSTMACROSTRING: ");
					lstrcpyn(szDbgInfo+lstrlen(szDbgInfo), p2->Param.PlainText.SequenceText, 200);
					break;
				case Far2::MCMD_CHECKMACRO:
					lstrcpy(szDbgInfo, L"Far2::MCMD_CHECKMACRO: ");
					lstrcpyn(szDbgInfo+lstrlen(szDbgInfo), p2->Param.PlainText.SequenceText, 200);
					break;
				case Far2::MCMD_GETSTATE:
					lstrcpy(szDbgInfo, L"Far2::MCMD_GETSTATE");
					break;
				case Far2::MCMD_GETAREA:
					lstrcpy(szDbgInfo, L"Far2::MCMD_GETAREA");
					break;
				default:
					wsprintf(szDbgInfo, L"Far2::ACTL_KEYMACRO(%i)", p2->Command);
				}
				LOG_CMD(L"%s", szDbgInfo, 0,0);
				#endif
				//iRc = psi3.AdvControl(&guid, ACTL_KEYMACRO, Param);
				switch (p2->Command)
				{
				case Far2::MCMD_LOADALL:
					iRc = psi3.MacroControl(MCTLARG(guid), MCTL_LOADALL, 0, 0);
					break;
				case Far2::MCMD_SAVEALL:
					iRc = psi3.MacroControl(MCTLARG(guid), MCTL_LOADALL, 0, 0);
					break;
				case Far2::MCMD_POSTMACROSTRING:
					{
						MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
						mcr.SequenceText = p2->Param.PlainText.SequenceText;
						wchar_t *pszUpper = NULL, *pszChanged = NULL, *pszDeMultiSz = NULL;
						if (p2->Param.PlainText.Flags & Far2::KSFLAGS_REG_MULTI_SZ)
						{
							if ((pszDeMultiSz = MacroFromMultiSZ(p2->Param.PlainText.SequenceText)) != NULL)
								mcr.SequenceText = pszDeMultiSz;
						}
						// ���� ������ ������������ AKey - ����������
						if (HIWORD(m_MinFarVersion) >= 1800)
						{
							FarKey_2_3(p2->Param.PlainText.AKey, &mcr.AKey);
						}
						// ������ ����� ����� ��� ���� ����� CallPlugin, 
						// � ���� ������ ����� �������� ������ PluginID (DWORD) �� GUID
						if (mcr.SequenceText && *mcr.SequenceText /*&& m_Info.Reserved*/)
						{
							pszUpper = lstrdup(mcr.SequenceText);
							size_t nOrigLen = lstrlen(mcr.SequenceText);
							size_t nAddPos = 0;
							CharUpperBuff(pszUpper, lstrlen(pszUpper));
							wchar_t* pszFrom = pszUpper;
							wchar_t* pszCall;
							WCHAR /*szIdDec[32], szIdHex1[32], szIdHex2[32], szId[32],*/ szGuid[64];
							//wsprintf(szIdDec, L"%u", m_Info.Reserved);
							//wsprintf(szIdHex1, L"0X%X", m_Info.Reserved);
							//wsprintf(szIdHex2, L"0X%08X", m_Info.Reserved);
							//FormatGuid(&mguid_Plugin, szGuid, TRUE);
							int nCchAdd = 0, nCchGuidsAdd = 0;
							while ((pszCall = wcsstr(pszFrom, L"CALLPLUGIN")) != NULL)
							{
								pszCall = pszCall + 10; // lstrlen(L"CALLPLUGIN")
								pszFrom = pszCall; // �����, ����� �� ������ � �� �����������
								while (*pszCall == L' ' || *pszCall == L'\t') pszCall++;
								if (*pszCall != L'(') continue;
								pszCall++;
								while (*pszCall == L' ' || *pszCall == L'\t') pszCall++;
								if (*pszCall < L'0' || *pszCall > L'9') continue; // ����������� ������ �����

								szGuid[0] = 0;
								DWORD nID = 0;
								wchar_t* pszEnd = NULL;
								if (pszCall[1] == L'X')
									nID = wcstoul(pszCall+2, &pszEnd, 16);
								else
									nID = wcstoul(pszCall, &pszEnd, 10);
								if (!nID || !pszEnd || (pszEnd <= pszCall))
									continue; // ������ � ID?

								if (nID == m_Info.Reserved)
									FormatGuid(&mguid_Plugin, szGuid, TRUE);
								else
								{
									// ���� ����� ����������� ���������
									std::map<DWORD,WrapPluginInfo*>::iterator iter;
									for (iter = (*gpMapSysID).begin(); iter != (*gpMapSysID).end(); iter++)
									{
										if (iter->first == nID)
										{
											FormatGuid(&iter->second->mguid_Plugin, szGuid, TRUE);
											break;
										}
									}

									// ���� �� "���������"
									if (szGuid[0] == 0)
									{
										LPCWSTR pszGuid = NULL;
										switch (nID)
										{
										case 0x43454D55: // ConEmu
											pszGuid = L"4b675d80-1d4a-4ea9-8436-fdc23f2fc14b"; break;
										case 0x43455568: // ConEmuTh
											pszGuid = L"bd454d48-448e-46cc-909d-b6cf789c2d65"; break;
										// Wrapper!
										case 0x424C434D: // MacroLib
											pszGuid = L"46FC0BF1-CC99-4652-B41D-C7B8705D52AF"; break;
										case 0x436C4678: // ClipFixer
											pszGuid = L"AF0DD773-7193-4F50-9904-AB24673EB42C"; break;
										//const SameFolder = 0x44464D53
										case 0x444E4645: // EditFind
											pszGuid = L"8EF28982-957E-4BCE-AD73-7E67DB443969"; break;
										case 0x44654272: // DeepBrowser
											pszGuid = L"D1778FAF-B6B1-4604-9D9C-CBCF3B15F7A8"; break;
										case 0x466C5470: // FileTypes
											pszGuid = L"32C72FF2-8762-4485-9BAE-2020B9143AA0"; break;
										case 0x4D426C6B: // MBlockEditor
											pszGuid = L"D82D6847-0C7B-4BF4-9A31-B0B929707854"; break;
										case 0x4D4D5657: // MMView
											pszGuid = L"44E0DA00-F361-4ACC-BF8F-DDC7D2E0494F"; break;
										case 0x52674564: // RegEditor
											pszGuid = L"F6A1E51C-1C11-4BD5-ADD2-8677348BC106"; break;
										//const FarHints = 0x544E4948
										case 0x5774654E: // Network
											pszGuid = L"9E724C40-D0B6-4DC3-9F30-CC7AF5292C5B"; break;
										case 0xA91B3F07: // PanelTabs
											pszGuid = L"66D5D731-EE8E-4113-87F3-56883CC321DA"; break;
										}
										if (pszGuid)
										{
											szGuid[0] = L'"'; lstrcpy(szGuid+1, pszGuid); lstrcat(szGuid, L"\"");
										}
									}
								}

								if (szGuid[0] == 0)
									continue; // ����������� ID, ������� ������ �� ������

								size_t nNewIdLen = lstrlen(szGuid);
								_ASSERTE(nNewIdLen==38);

								if (nCchAdd < 38 || !pszChanged)
								{
									wchar_t* pszNew = NULL;
									int nLen = 0;
									nCchAdd = 38*10; // 10 ������ � ���������
									if (pszChanged == NULL)
									{
										nLen = lstrlen(mcr.SequenceText);
										pszNew = (wchar_t*)calloc(nLen+nCchAdd+1, sizeof(wchar_t));
										if (!pszNew) { _ASSERTE(pszNew!=NULL); break; }
										lstrcpy(pszNew, mcr.SequenceText);
									}
									else
									{
										nLen = lstrlen(pszChanged);
										pszNew = (wchar_t*)calloc(nLen+nCchAdd+1, sizeof(wchar_t));
										if (!pszNew) { _ASSERTE(pszNew!=NULL); break; }
										lstrcpy(pszNew, pszChanged);
										free(pszChanged);
									}
									pszChanged = pszNew;
								}

								size_t nPos = (pszCall - pszUpper);
								size_t nOrigIdLen = pszEnd - pszCall;
								if (nOrigLen < (nOrigIdLen+nPos))
								{
									_ASSERTE(nOrigLen > (nOrigIdLen+nPos));
									break;
								}
								// ���������� ����� ��� GUID (������ �����, �.�. ������ �������� calloc)
								memmove(pszChanged+nNewIdLen+nPos+nAddPos, pszChanged+nPos+nAddPos+nOrigIdLen, (nOrigLen-nOrigIdLen-nPos)*sizeof(wchar_t));
								// �������� ����� ��
								memmove(pszChanged+nPos+nAddPos, szGuid, nNewIdLen*sizeof(wchar_t));

								nCchAdd -= nNewIdLen;
								pszFrom = pszCall + nOrigIdLen;
								nAddPos += nNewIdLen - nOrigIdLen;

								//// ������ - ����������� ID � szId
								//for (int i = 0; i < 12 
								//		&& ( (pszCall[i] >= L'0' && pszCall[i] <= L'9')
								//		  || (pszCall[i] >= L'A' && pszCall[i] <= L'F') // ��� CharUpperBuff
								//		  || (pszCall[i] >= L'X') // HEX
								//		    ) ; i++)
								//{
								//	szId[i] = pszCall[i]; szId[i+1] = 0;
								//}
								////
								//if (lstrcmp(szId, szIdDec) && lstrcmp(szId, szIdHex1) && lstrcmp(szId, szIdHex2))
								//	continue; 
								//if (szGuid[0] == 0)
								//	continue; // ����������� ID, ������� ������ �� ������
							}

							if (pszChanged != NULL)
								mcr.SequenceText = pszChanged;
						}
						mcr.Flags = 0
							| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_DISABLEOUTPUT) ? KMFLAGS_DISABLEOUTPUT : 0)
							| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_NOSENDKEYSTOPLUGINS) ? KMFLAGS_NOSENDKEYSTOPLUGINS : 0)
							//| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_REG_MULTI_SZ) ? KMFLAGS_REG_MULTI_SZ : 0)
							| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_SILENTCHECK) ? KMFLAGS_SILENTCHECK : 0);
						iRc = psi3.MacroControl(MCTLARG(guid), MCTL_SENDSTRING, 0, &mcr);
						if (pszUpper)
							free(pszUpper);
						if (pszChanged)
							free(pszChanged);
						if (pszDeMultiSz)
							free(pszDeMultiSz);
					}
					break;
				case Far2::MCMD_CHECKMACRO:
					{
						MacroSendMacroText mcr = {sizeof(mcr)};
						mcr.SequenceText = p2->Param.PlainText.SequenceText;
						wchar_t *pszDeMultiSz = NULL;
						if (p2->Param.PlainText.Flags & Far2::KSFLAGS_REG_MULTI_SZ)
						{
							if ((pszDeMultiSz = MacroFromMultiSZ(p2->Param.PlainText.SequenceText)) != NULL)
								mcr.SequenceText = pszDeMultiSz;
						}
						mcr.Flags = 0
							| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_DISABLEOUTPUT) ? KMFLAGS_DISABLEOUTPUT : 0)
							| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_NOSENDKEYSTOPLUGINS) ? KMFLAGS_NOSENDKEYSTOPLUGINS : 0)
							//| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_REG_MULTI_SZ) ? KMFLAGS_REG_MULTI_SZ : 0)
							| ((p2->Param.PlainText.Flags & Far2::KSFLAGS_SILENTCHECK) ? KMFLAGS_SILENTCHECK : 0);
						iRc = psi3.MacroControl(MCTLARG(guid), MCTL_SENDSTRING, MSSC_CHECK, &mcr);
						#if MVV_3<=2375
						// ���� ��������� ��� �������
						p2->Param.MacroResult.ErrCode = mcr.Result.ErrCode;
						p2->Param.MacroResult.ErrPos = mcr.Result.ErrPos;
						p2->Param.MacroResult.ErrSrc = mcr.Result.ErrSrc;
						#else
						if (iRc == FALSE)
						{
							p2->Param.MacroResult.ErrCode = -1;
							p2->Param.MacroResult.ErrPos.X = p2->Param.MacroResult.ErrPos.Y = 0;
							p2->Param.MacroResult.ErrSrc = L"";

							size_t iRcSize = psi3.MacroControl(MCTLARG(guid), MCTL_GETLASTERROR, 0, NULL);
							if (!mp_MacroResult || mn_MaxMacroResult < iRcSize)
							{
								if (mp_MacroResult) free(mp_MacroResult);
								mn_MaxMacroResult = iRcSize;
								mp_MacroResult = (MacroParseResult*)malloc(mn_MaxMacroResult);
								if (mp_MacroResult)
								{
									mp_MacroResult->StructSize = sizeof(*mp_MacroResult);
									psi3.MacroControl(MCTLARG(guid), MCTL_GETLASTERROR, iRcSize, mp_MacroResult);
									p2->Param.MacroResult.ErrCode = mp_MacroResult->ErrCode;
									p2->Param.MacroResult.ErrPos = mp_MacroResult->ErrPos;
									p2->Param.MacroResult.ErrSrc = mp_MacroResult->ErrSrc;
								}
							}
						}
						else
						{
							//OK
							p2->Param.MacroResult.ErrCode = 0;
							p2->Param.MacroResult.ErrPos.X = p2->Param.MacroResult.ErrPos.Y = 0;
							p2->Param.MacroResult.ErrSrc = NULL;
						}
						#endif
						// Fin
						if (pszDeMultiSz)
							free(pszDeMultiSz);
					}
					break;
				case Far2::MCMD_GETSTATE:
					iRc = psi3.MacroControl(MCTLARG(guid), MCTL_GETSTATE, 0, 0);
					break;
				case Far2::MCMD_GETAREA:
					_ASSERTE(Far2::MACROAREA_OTHER==MACROAREA_OTHER);
					_ASSERTE(Far2::MACROAREA_SHELL==MACROAREA_SHELL);
					_ASSERTE(Far2::MACROAREA_VIEWER==MACROAREA_VIEWER);
					_ASSERTE(Far2::MACROAREA_EDITOR==MACROAREA_EDITOR);
					_ASSERTE(Far2::MACROAREA_DIALOG==MACROAREA_DIALOG);
					_ASSERTE(Far2::MACROAREA_SEARCH==MACROAREA_SEARCH);
					_ASSERTE(Far2::MACROAREA_DISKS==MACROAREA_DISKS);
					_ASSERTE(Far2::MACROAREA_MAINMENU==MACROAREA_MAINMENU);
					_ASSERTE(Far2::MACROAREA_MENU==MACROAREA_MENU);
					_ASSERTE(Far2::MACROAREA_HELP==MACROAREA_HELP);
					_ASSERTE(Far2::MACROAREA_INFOPANEL==MACROAREA_INFOPANEL);
					_ASSERTE(Far2::MACROAREA_QVIEWPANEL==MACROAREA_QVIEWPANEL);
					_ASSERTE(Far2::MACROAREA_TREEPANEL==MACROAREA_TREEPANEL);
					_ASSERTE(Far2::MACROAREA_FINDFOLDER==MACROAREA_FINDFOLDER);
					_ASSERTE(Far2::MACROAREA_USERMENU==MACROAREA_USERMENU);
					_ASSERTE(Far2::MACROAREA_AUTOCOMPLETION==MACROAREA_SHELLAUTOCOMPLETION);
					iRc = psi3.MacroControl(MCTLARG(guid), MCTL_GETAREA, 0, 0);
					if (iRc == MACROAREA_DIALOGAUTOCOMPLETION)
						iRc = Far2::MACROAREA_AUTOCOMPLETION;
					break;
				}
			}
			break;
		}
	case Far2::ACTL_POSTKEYSEQUENCE:
		//TODO: ����� ���������� �� MCTL_SENDSTRING
		{
			Far2::KeySequence* p2 = (Far2::KeySequence*)Param;
			if (p2 && p2->Count)
			{
				int i;
				size_t nStrLen = 2, nKeyLen;
				for (i = 0; i < p2->Count; i++)
					nStrLen += FarKeyToName3(p2->Sequence[i], NULL, 0)+1;
				#if MVV_3>=2799
				nStrLen += 10; // Keys('...')
				#endif
				wchar_t* pszMacro = (wchar_t*)malloc(nStrLen*sizeof(wchar_t));
				if (pszMacro)
				{
					wchar_t* psz = pszMacro;
					#if MVV_3>=2799
					lstrcpy(psz, L"Keys('");
					psz += 6;
					#endif
					for (i = 0; i < p2->Count; i++)
					{
						*psz = 0;
						// �� ����, ��� ������ ��� ����������...
						FarKeyToName3(p2->Sequence[i], psz, nStrLen);
						nKeyLen = lstrlen(psz);
						#if MVV_3>=2799
						_ASSERTE(nKeyLen && psz[nKeyLen-1]!=L'\''); // ������������ ��������� ������� ��� lua?
						#endif
						psz += nKeyLen;
						if (nKeyLen)
							*(psz++) = L' ';
						nStrLen -= nKeyLen + 1;
					}
					if ((psz > pszMacro) && (*(psz-1) == L' '))
						psz--;
					#if MVV_3>=2799
					lstrcpy(psz, L"')");
					#else
					*psz = 0;
					#endif
					
					
					if (*pszMacro)
					{
						MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
						mcr.SequenceText = pszMacro;
						mcr.Flags = 0
							| ((p2->Flags & Far2::KSFLAGS_DISABLEOUTPUT) ? KMFLAGS_DISABLEOUTPUT : 0)
							| ((p2->Flags & Far2::KSFLAGS_NOSENDKEYSTOPLUGINS) ? KMFLAGS_NOSENDKEYSTOPLUGINS : 0)
							;
						iRc = psi3.MacroControl(MCTLARG(guid), MCTL_SENDSTRING, 0, &mcr);
					}
					
					free(pszMacro);
				}
			}
		}
		break;
	case Far2::ACTL_GETWINDOWINFO:
		//iRc = psi3.AdvControl(&guid, ACTL_GETWINDOWINFO, 0, Param);
		{
			Far2::WindowInfo* p2 = (Far2::WindowInfo*)Param;
			WindowInfo wi = {sizeof(WindowInfo)};
			wi.Pos = p2->Pos;
			wi.TypeName = p2->TypeName; wi.TypeNameSize = p2->TypeNameSize;
			wi.Name = p2->Name; wi.NameSize = p2->NameSize;
			iRc = psi3.AdvControl(&guid, ACTL_GETWINDOWINFO, 0, &wi);
			if (iRc)
			{
				p2->Pos = wi.Pos;
				p2->Type = wi.Type; //TODO: ����������� ����?
				p2->Modified = (wi.Flags & WIF_MODIFIED) == WIF_MODIFIED;
				p2->Current = (wi.Flags & WIF_CURRENT) == WIF_CURRENT;
				p2->TypeNameSize = wi.TypeNameSize;
				p2->NameSize = wi.NameSize;
			}
		}
		break;
	case Far2::ACTL_GETWINDOWCOUNT:
		iRc = psi3.AdvControl(&guid, ACTL_GETWINDOWCOUNT, 0, Param); break;
	case Far2::ACTL_SETCURRENTWINDOW:
		iRc = psi3.AdvControl(&guid, ACTL_SETCURRENTWINDOW, 0, Param); break;
	case Far2::ACTL_COMMIT:
		iRc = psi3.AdvControl(&guid, ACTL_COMMIT, 0, Param); break;
	case Far2::ACTL_GETFARHWND:
		iRc = psi3.AdvControl(&guid, ACTL_GETFARHWND, 0, Param); break;
	case Far2::ACTL_GETSYSTEMSETTINGS:
		{
		#if MVV_3<=2540
			iRc = psi3.AdvControl(&guid, ACTL_GETSYSTEMSETTINGS, 0, Param);
		#else
			iRc = GetFarSystemSettings();
		#endif	
		}
		break;
	case Far2::ACTL_GETPANELSETTINGS:
		{
		#if MVV_3<=2540
			iRc = psi3.AdvControl(&guid, ACTL_GETPANELSETTINGS, 0, Param);
		#else
			iRc = GetFarPanelSettings();
		#endif	
		}
		break;
	case Far2::ACTL_GETINTERFACESETTINGS:
		{
		#if MVV_3<=2540
			iRc = psi3.AdvControl(&guid, ACTL_GETINTERFACESETTINGS, 0, Param);
		#else
			iRc = GetFarInterfaceSettings();
		#endif	
		}
		break;
	case Far2::ACTL_GETCONFIRMATIONS:
		{
		#if MVV_3<=2540
			iRc = psi3.AdvControl(&guid, ACTL_GETCONFIRMATIONS, 0, Param);
		#else
			iRc = GetFarConfirmations();
		#endif	
		}
		break;
	case Far2::ACTL_GETDESCSETTINGS:
		{
		#if MVV_3<=2540
			iRc = psi3.AdvControl(&guid, ACTL_GETDESCSETTINGS, 0, Param);
		#else
			iRc = GetFarDescSettings();
		#endif	
		}
		break;
	case Far2::ACTL_SETARRAYCOLOR:
		{
			Far2::FarSetColors* p2 = (Far2::FarSetColors*)Param;
			FarSetColors p3 = {};
			#if MVV_3>2798
			p3.StructSize = sizeof(p3);
			#endif
			p3.Flags = (p2->Flags & Far2::FCLR_REDRAW) ? FSETCLR_REDRAW : FSETCLR_NONE;
			p3.Colors = (FarColor*)calloc(p2->ColorCount,sizeof(*p3.Colors));
			for (int i = 0; i < p2->ColorCount; i++)
				FarColor_2_3(p2->Colors[i], p3.Colors[i]);
			p3.StartIndex = p2->StartIndex;
			p3.ColorsCount = p2->ColorCount;
			iRc = psi3.AdvControl(&guid, ACTL_SETARRAYCOLOR, 0, &p3);
			free(p3.Colors);
		}
		break;
	case Far2::ACTL_GETPLUGINMAXREADDATA:
		{
		#if MVV_3<=2540
			iRc = psi3.AdvControl(&guid, ACTL_GETPLUGINMAXREADDATA, 0, Param);
		#else
			iRc = GetFarSetting(NULL, FSSF_SYSTEM, L"PluginMaxReadData");
		#endif	
		}
		break;
	case Far2::ACTL_GETDIALOGSETTINGS:
		{
		#if MVV_3<=2540
			iRc = psi3.AdvControl(&guid, ACTL_GETDIALOGSETTINGS, 0, Param);
		#else
			iRc = GetFarDialogSettings();
		#endif	
		}
		break;
	case Far2::ACTL_GETSHORTWINDOWINFO:
		{
			Far2::WindowInfo* p2 = (Far2::WindowInfo*)Param;
			if (p2)
			{
				if (GetCurrentThreadId() == gnMainThreadId)
				{
					WindowInfo wi = {sizeof(WindowInfo)};
					wi.Pos = p2->Pos;
					iRc = psi3.AdvControl(&guid, ACTL_GETWINDOWINFO, 0, &wi);
					if (iRc)
					{
						p2->Pos = wi.Pos;
						p2->Type = wi.Type; //TODO: ����������� ����?
						p2->Modified = (wi.Flags & WIF_MODIFIED) == WIF_MODIFIED;
						p2->Current = (wi.Flags & WIF_CURRENT) == WIF_CURRENT;
						p2->TypeNameSize = wi.TypeNameSize;
						p2->NameSize = wi.NameSize;
					}
				}
				else if (p2->Pos == -1)
				{
					memset(p2, 0, sizeof(*p2));
					p2->Pos = -1;
					//BUGBUG: ��������� ACTL_GETWINDOWINFO ������ �� ThreadSafe - ����� MCTL_GETAREA
					iRc = TRUE;
					//iRc = psi3.AdvControl(&guid, ACTL_GETSHORTWINDOWINFO, Param);
					INT_PTR nArea = psi3.MacroControl(MCTLARG(guid), MCTL_GETAREA, 0, 0);
					switch(nArea)
					{
						case MACROAREA_SHELL:
						case MACROAREA_INFOPANEL:
						case MACROAREA_QVIEWPANEL:
						case MACROAREA_TREEPANEL:
							p2->Type = WTYPE_PANELS; break;
						case MACROAREA_VIEWER:
							p2->Type = WTYPE_VIEWER; break;
						case MACROAREA_EDITOR:
							p2->Type = WTYPE_EDITOR; break;
						case MACROAREA_DIALOG:
						case MACROAREA_SEARCH:
						case MACROAREA_DISKS:
						case MACROAREA_FINDFOLDER:
						case MACROAREA_SHELLAUTOCOMPLETION:
						case MACROAREA_DIALOGAUTOCOMPLETION:
							p2->Type = WTYPE_DIALOG; break;
						case MACROAREA_HELP:
							p2->Type = WTYPE_HELP; break;
						case MACROAREA_MAINMENU:
						case MACROAREA_MENU:
						case MACROAREA_USERMENU:
							p2->Type = WTYPE_VMENU; break;
						//case MACROAREA_OTHER: // Grabber
						//	return -1;
						default:
							iRc = 0;
					}
					p2->Current = (iRc != 0);
				}
			}
		}
		break;
	case Far2::ACTL_REDRAWALL:
		iRc = psi3.AdvControl(&guid, ACTL_REDRAWALL, 0, Param); break;
	case Far2::ACTL_SYNCHRO:
		iRc = psi3.AdvControl(&guid, ACTL_SYNCHRO, 0, Param); break;
	case Far2::ACTL_SETPROGRESSSTATE:
		iRc = psi3.AdvControl(&guid, ACTL_SETPROGRESSSTATE, 0, Param);
		break;
	case Far2::ACTL_SETPROGRESSVALUE:
		{
		#if MVV_3>2800
		Far2::PROGRESSVALUE *pv2 = (Far2::PROGRESSVALUE*)Param;
		ProgressValue pv3 = {};
		if (Param)
		{
			pv3.StructSize = sizeof(pv3);
			pv3.Completed = pv2->Completed;
			pv3.Total = pv2->Total;
			Param = &pv3;
		}
		#endif
		iRc = psi3.AdvControl(&guid, ACTL_SETPROGRESSVALUE, 0, Param);
		}
		break;
	case Far2::ACTL_QUIT:
		iRc = psi3.AdvControl(&guid, ACTL_QUIT, 0, Param); break;
	case Far2::ACTL_GETFARRECT:
		iRc = psi3.AdvControl(&guid, ACTL_GETFARRECT, 0, Param); break;
	case Far2::ACTL_GETCURSORPOS:
		iRc = psi3.AdvControl(&guid, ACTL_GETCURSORPOS, 0, Param); break;
	case Far2::ACTL_SETCURSORPOS:
		iRc = psi3.AdvControl(&guid, ACTL_SETCURSORPOS, 0, Param); break;
	}
	return iRc;
};
int WrapPluginInfo::FarApiViewerControl(int Command, void *Param)
{
	LOG_CMD(L"psi2.ViewerControl",0,0,0);
	int nRc = 0; //psi3.ViewerControl(-1, Command, 0, Param);
	//TODO: ��� ��������� ������� ����� ����������� ����������
	switch (Command)
	{
	case Far2::VCTL_GETINFO:
		{
			Far2::ViewerInfo* p2 = (Far2::ViewerInfo*)Param;
			ViewerInfo p3 = {sizeof(ViewerInfo)};
			p3.ViewerID = p2->ViewerID;
			nRc = psi3.ViewerControl(-1, VCTL_GETINFO, 0, &p3);
			p2->ViewerID = p3.ViewerID;
			#if MVV_3<=2798
			p2->FileName = p3.FileName;
			#else
			size_t nSize = psi3.ViewerControl(-1, VCTL_GETFILENAME, 0, NULL);
			if (!mpsz_ViewerFileName || (mcch_ViewerFileName < nSize))
			{
				SafeFree(mpsz_ViewerFileName);
				mpsz_ViewerFileName = (wchar_t*)malloc((nSize+1)*sizeof(wchar_t));
				mcch_ViewerFileName = nSize;
				nSize = psi3.ViewerControl(-1, VCTL_GETFILENAME, mcch_ViewerFileName, mpsz_ViewerFileName);
			}
			p2->FileName = mpsz_ViewerFileName ? mpsz_ViewerFileName : L"???";
			#endif
			p2->FileSize = p3.FileSize;
			p2->FilePos = p3.FilePos;
			p2->WindowSizeX = p3.WindowSizeX;
			p2->WindowSizeY = p3.WindowSizeY;
			p2->Options = p3.Options;
			p2->TabSize = p3.TabSize;
			p2->CurMode.CodePage = p3.CurMode.CodePage;
			#if MVV_3<=2798
			p2->CurMode.Wrap = p3.CurMode.Wrap;
			p2->CurMode.WordWrap = p3.CurMode.WordWrap;
			p2->CurMode.Hex = p3.CurMode.Hex;
			#else
			p2->CurMode.Wrap = (p3.CurMode.Flags & VMF_WRAP) == VMF_WRAP;
			p2->CurMode.WordWrap = (p3.CurMode.Flags & VMF_WORDWRAP) == VMF_WORDWRAP;
			p2->CurMode.Hex = (int)p3.CurMode.ViewMode;
			#endif
			p2->LeftPos = p3.LeftPos;
		}
		break;
	case Far2::VCTL_QUIT:
		nRc = psi3.ViewerControl(-1, VCTL_QUIT, 0, (void*)Param); break;
	case Far2::VCTL_REDRAW:
		nRc = psi3.ViewerControl(-1, VCTL_REDRAW, 0, (void*)Param); break;
	case Far2::VCTL_SETKEYBAR:
		{
			KeyBarTitles* p3 = (KeyBarTitles*)Param;
			if (Param && (Param != (void*)-1))
			{
				const Far2::KeyBarTitles* p2 = (const Far2::KeyBarTitles*)Param;
				p3 = KeyBarTitles_2_3(p2);
			}
			nRc = psi3.ViewerControl(-1, VCTL_SETKEYBAR, 0, p3);
		}
		break;
	case Far2::VCTL_SETPOSITION:
		{
			const Far2::ViewerSetPosition* p2 = (const Far2::ViewerSetPosition*)Param;
			//TODO: ����������� ������
			ViewerSetPosition p3 = {};
			#if MVV_3>2799
			p3.StructSize = sizeof(p3);
			#endif
			p3.Flags = (VIEWER_SETPOS_FLAGS)p2->Flags;
			p3.StartPos = p2->StartPos;
			p3.LeftPos = p2->LeftPos;
			nRc = psi3.ViewerControl(-1, VCTL_SETPOSITION, 0, &p3);
		}
		break;
	case Far2::VCTL_SELECT:
		{
			//ASSERTSTRUCT(ViewerSelect);
			ViewerSelect vs3 = {};
			if (Param)
			{
				Far2::ViewerSelect* pvs2 = (Far2::ViewerSelect*)Param;
				#if MVV_3<=2798
				_ASSERTE(sizeof(vs3)==16 && sizeof(vs3.BlockStartPos)==8 && sizeof(vs3.BlockLen)==4);
				#else
				_ASSERTE(sizeof(vs3)==(16+sizeof(size_t)) && sizeof(vs3.BlockStartPos)==8 && sizeof(vs3.BlockLen)==4);
				vs3.StructSize = sizeof(vs3);
				#endif
				vs3.BlockStartPos = pvs2->BlockStartPos;
				vs3.BlockLen = pvs2->BlockLen;
				Param = &vs3;
			}
			nRc = psi3.ViewerControl(-1, VCTL_SELECT, 0, (void*)Param);
		}
		break;
	case Far2::VCTL_SETMODE:
		{
			const Far2::ViewerSetMode* p2 = (const Far2::ViewerSetMode*)Param;
			//TODO: ����������� ����
			ViewerSetMode p3 = {};
			#if MVV_3>2799
			p3.StructSize = sizeof(p3);
			#endif
			p3.Type = (VIEWER_SETMODE_TYPES)p2->Type;
			p3.wszParam = p2->Param.wszParam; // �������� ������� �� union
			//TODO: ����������� ������
			p3.Flags = p2->Flags;
			nRc = psi3.ViewerControl(-1, VCTL_SETMODE, 0, &p3);
		}
		break;
	}
	return nRc;
};
int WrapPluginInfo::FarApiEditorControl(int Command, void *Param)
{
	LOG_CMD0(L"psi2.EditorControl(%i)",Command,0,0);
	int nRc = 0; //psi3.EditorControl(-1, Command, 0, Param);
	//TODO: ��� ��������� ������� ����� ����������� ����������
	switch (Command)
	{
	case Far2::ECTL_GETSTRING:
		{
			if (Param)
			{
				EditorGetString egs3 = {};
				Far2::EditorGetString* egs2 = (Far2::EditorGetString*)Param;
				#if MVV_3>2800
				egs3.StructSize = sizeof(egs3);
				#endif
				egs3.StringNumber = egs2->StringNumber;
				egs3.StringText = egs2->StringText;
				egs3.StringEOL = egs2->StringEOL;
				egs3.StringLength = egs2->StringLength;
				egs3.SelStart = egs2->SelStart;
				egs3.SelEnd = egs2->SelEnd;
				nRc = psi3.EditorControl(-1, ECTL_GETSTRING, 0, &egs3);
				egs2->StringNumber = egs3.StringNumber;
				egs2->StringText = egs3.StringText;
				egs2->StringEOL = egs3.StringEOL;
				egs2->StringLength = egs3.StringLength;
				egs2->SelStart = egs3.SelStart;
				egs2->SelEnd = egs3.SelEnd;
			}
			else
			{
				nRc = psi3.EditorControl(-1, ECTL_GETSTRING, 0, NULL);
			}
		}
		break;
	case Far2::ECTL_SETSTRING:
		{
			if (Param)
			{
				EditorSetString ess3 = {};
				Far2::EditorSetString* ess2 = (Far2::EditorSetString*)Param;
				#if MVV_3>2800
				ess3.StructSize = sizeof(ess3);
				#endif
				ess3.StringNumber = ess2->StringNumber;
				ess3.StringText = ess2->StringText;
				ess3.StringEOL = ess2->StringEOL;
				ess3.StringLength = ess2->StringLength;
				nRc = psi3.EditorControl(-1, ECTL_SETSTRING, 0, &ess3);
				ess2->StringNumber = ess3.StringNumber;
				ess2->StringText = ess3.StringText;
				ess2->StringEOL = ess3.StringEOL;
				ess2->StringLength = ess3.StringLength;
			}
			else
			{
				_ASSERTE(Param!=NULL);
				nRc = psi3.EditorControl(-1, ECTL_SETSTRING, 0, NULL);
			}
		}
		break;
	case Far2::ECTL_INSERTSTRING:
		nRc = psi3.EditorControl(-1, ECTL_INSERTSTRING, 0, (void*)Param);
		break;
	case Far2::ECTL_DELETESTRING:
		nRc = psi3.EditorControl(-1, ECTL_DELETESTRING, 0, (void*)Param);
		break;
	case Far2::ECTL_DELETECHAR:
		nRc = psi3.EditorControl(-1, ECTL_DELETECHAR, 0, (void*)Param);
		break;
	case Far2::ECTL_INSERTTEXT:
		nRc = psi3.EditorControl(-1, ECTL_INSERTTEXT, 0, (void*)Param);
		break;
	case Far2::ECTL_GETINFO:
		{
			Far2::EditorInfo* p2 = (Far2::EditorInfo*)Param;
			EditorInfo p3 = {0};
			#if MVV_3>2800
			p3.StructSize = sizeof(p3);
			#endif
			p3.EditorID = p2->EditorID;
			nRc = psi3.EditorControl(-1, ECTL_GETINFO, 0, &p3);
			p2->EditorID = p3.EditorID;
			p2->WindowSizeX = p3.WindowSizeX;
			p2->WindowSizeY = p3.WindowSizeY;
			p2->TotalLines = p3.TotalLines;
			p2->CurLine = p3.CurLine;
			p2->CurPos = p3.CurPos;
			p2->CurTabPos = p3.CurTabPos;
			p2->TopScreenLine = p3.TopScreenLine;
			p2->LeftPos = p3.LeftPos;
			p2->Overtype = p3.Overtype;
			p2->BlockType = p3.BlockType;
			p2->BlockStartLine = p3.BlockStartLine;
			p2->Options = p3.Options;
			p2->TabSize = p3.TabSize;
			#if MVV_3<=2798
			p2->BookMarkCount = p3.BookMarkCount;
			#else
			p2->BookMarkCount = p3.BookmarkCount;
			#endif
			p2->CurState = p3.CurState;
			p2->CodePage = p3.CodePage;
		}
		break;
	case Far2::ECTL_SETPOSITION:
		#if MVV_3>2800
		ADD_STRUCTSIZE_HERE(EditorSetPosition, Param);
		#else
		ASSERTSTRUCT(EditorSetPosition);
		#endif
		nRc = psi3.EditorControl(-1, ECTL_SETPOSITION, 0, (void*)Param);
		break;
	case Far2::ECTL_SELECT:
		#if MVV_3>2800
		ADD_STRUCTSIZE_HERE(EditorSelect, Param);
		#else
		ASSERTSTRUCT(EditorSelect);
		#endif
		nRc = psi3.EditorControl(-1, ECTL_SELECT, 0, (void*)Param);
		break;
	case Far2::ECTL_REDRAW:
		nRc = psi3.EditorControl(-1, ECTL_REDRAW, 0, (void*)Param); break;
	case Far2::ECTL_TABTOREAL:
	case Far2::ECTL_REALTOTAB:
		#if MVV_3>2800
		ADD_STRUCTSIZE_HERE(EditorConvertPos, Param);
		#else
		ASSERTSTRUCT(EditorConvertPos);
		#endif
		switch (Command)
		{
		case Far2::ECTL_TABTOREAL: nRc = psi3.EditorControl(-1, ECTL_TABTOREAL, 0, (void*)Param); break;
		case Far2::ECTL_REALTOTAB: nRc = psi3.EditorControl(-1, ECTL_REALTOTAB, 0, (void*)Param); break;
		}
		break;
	case Far2::ECTL_EXPANDTABS:
		nRc = psi3.EditorControl(-1, ECTL_EXPANDTABS, 0, (void*)Param); break;
	case Far2::ECTL_SETTITLE:
		nRc = psi3.EditorControl(-1, ECTL_SETTITLE, 0, (void*)Param); break;
	case Far2::ECTL_READINPUT:
		nRc = psi3.EditorControl(-1, ECTL_READINPUT, 0, (void*)Param); break;
	case Far2::ECTL_PROCESSINPUT:
		nRc = psi3.EditorControl(-1, ECTL_PROCESSINPUT, 0, (void*)Param); break;
	case Far2::ECTL_ADDCOLOR:
		{
			const Far2::EditorColor* p2 = (const Far2::EditorColor*)Param;
			if (p2->Color)
			{
				EditorColor p3 = {sizeof(EditorColor)};
				p3.StringNumber = p2->StringNumber;
				p3.ColorItem = 0; // p2->ColorItem; -- ��� ���� �� ������������ � ECTL_ADDCOLOR. ����� ���� ������ ����������� �� ���� �������
				p3.StartPos = p2->StartPos;
				p3.EndPos = p2->EndPos;
				// ����� ������ ������� ���� ������������
				FarColor_2_3((BYTE)(p2->Color & 0xFF), p3.Color);
				//p3.Color.Flags = FCF_FG_4BIT|FCF_BG_4BIT;
				//p3.Color.ForegroundColor = p2->Color & 0xF;
				//p3.Color.BackgroundColor = (p2->Color & 0xF0) >> 4;
				p3.Owner = mguid_Plugin;
				p3.Priority = mn_EditorColorPriority;
				nRc = psi3.EditorControl(-1, ECTL_ADDCOLOR, 0, &p3);
			}
			else
			{
				EditorDeleteColor p3 = {sizeof(EditorDeleteColor)};
				p3.Owner = mguid_Plugin;
				p3.StringNumber = p2->StringNumber;
				p3.StartPos = p2->StartPos;
                nRc = psi3.EditorControl(-1, ECTL_DELCOLOR, 0, &p3);
			}
		}
		break;
	case Far2::ECTL_GETCOLOR:
		{
			Far2::EditorColor* p2 = (Far2::EditorColor*)Param;
			EditorColor p3 = {sizeof(EditorColor)};
			p3.StringNumber = p2->StringNumber;
			p3.ColorItem = p2->ColorItem;
			p3.StartPos = p2->StartPos;
			p3.EndPos = p2->EndPos;
			p3.Color.Flags = FCF_FG_4BIT|FCF_BG_4BIT;
			nRc = psi3.EditorControl(-1, ECTL_GETCOLOR, 0, &p3);
			if (nRc)
			{
				_ASSERTE(p3.Color.Flags == (FCF_FG_4BIT|FCF_BG_4BIT));
				//p2->Color = (p3.Color.ForegroundColor & 0xF) | ((p3.Color.BackgroundColor & 0xF) << 4);
				p2->Color = FarColor_3_2(p3.Color);
			}
		}
		break;
	case Far2::ECTL_SAVEFILE:
		#if MVV_3>2800
		ADD_STRUCTSIZE_HERE(EditorSaveFile, Param);
		#else
		ASSERTSTRUCT(EditorSaveFile);
		#endif
		nRc = psi3.EditorControl(-1, ECTL_SAVEFILE, 0, (void*)Param);
		break;
	case Far2::ECTL_QUIT:
		nRc = psi3.EditorControl(-1, ECTL_QUIT, 0, (void*)Param); break;
	case Far2::ECTL_SETKEYBAR:
		{
			KeyBarTitles* p3 = (KeyBarTitles*)Param;
			if (Param && (Param != (void*)-1))
			{
				const Far2::KeyBarTitles* p2 = (const Far2::KeyBarTitles*)Param;
				p3 = KeyBarTitles_2_3(p2);
			}
			nRc = psi3.EditorControl(-1, ECTL_SETKEYBAR, 0, p3);
		}
		break;
	case Far2::ECTL_PROCESSKEY:
		{
			#if MVV_3>=2184
			INPUT_RECORD r = {};
			int VirtKey = 0, ControlState = 0;
			TranslateKeyToVK((DWORD_PTR)Param, VirtKey, ControlState, &r);
			nRc = psi3.EditorControl(-1, ECTL_PROCESSINPUT, 0, &r);
			#else
			nRc = psi3.EditorControl(-1, ECTL_PROCESSKEY, 0, (void*)Param);
			#endif
		}
		break;
	case Far2::ECTL_SETPARAM:
		{
			const Far2::EditorSetParameter* p2 = (const Far2::EditorSetParameter*)Param;
			EditorSetParameter p3 = {};
			#if MVV_3>2800
			p3.StructSize = sizeof(p3);
			#endif
			p3.Type = (EDITOR_SETPARAMETER_TYPES)-1;
			p3.wszParam = p2->Param.wszParam; // �������� "��������" �� union
			//TODO: ����������� ������?
			p3.Flags = p2->Flags;
			p3.Size = p2->Size;
			switch (p2->Type)
			{
			case Far2::ESPT_TABSIZE: p3.Type = ESPT_TABSIZE; break;
			case Far2::ESPT_EXPANDTABS: p3.Type = ESPT_EXPANDTABS; break;
			case Far2::ESPT_AUTOINDENT: p3.Type = ESPT_AUTOINDENT; break;
			case Far2::ESPT_CURSORBEYONDEOL: p3.Type = ESPT_CURSORBEYONDEOL; break;
			case Far2::ESPT_CHARCODEBASE: p3.Type = ESPT_CHARCODEBASE; break;
			case Far2::ESPT_CODEPAGE: p3.Type = ESPT_CODEPAGE; break;
			case Far2::ESPT_SAVEFILEPOSITION: p3.Type = ESPT_SAVEFILEPOSITION; break;
			case Far2::ESPT_LOCKMODE: p3.Type = ESPT_LOCKMODE; break;
			case Far2::ESPT_SETWORDDIV: p3.Type = ESPT_SETWORDDIV; break;
			case Far2::ESPT_GETWORDDIV: p3.Type = ESPT_GETWORDDIV; break;
			case Far2::ESPT_SHOWWHITESPACE: p3.Type = ESPT_SHOWWHITESPACE; break;
			case Far2::ESPT_SETBOM: p3.Type = ESPT_SETBOM; break;
			}
			if (p3.Type != (EDITOR_SETPARAMETER_TYPES)-1)
				nRc = psi3.EditorControl(-1, ECTL_SETPARAM, 0, &p3);
		}
		break;
	case Far2::ECTL_GETBOOKMARKS:
		nRc = psi3.EditorControl(-1, ECTL_GETBOOKMARKS, 0, (void*)Param); break;
	case Far2::ECTL_TURNOFFMARKINGBLOCK:
		#if MVV_3<=2558
		nRc = psi3.EditorControl(-1, ECTL_TURNOFFMARKINGBLOCK, 0, (void*)Param);
		#else
		nRc = 0;
		#endif
		break;
	case Far2::ECTL_DELETEBLOCK:
		nRc = psi3.EditorControl(-1, ECTL_DELETEBLOCK, 0, (void*)Param); break;
	case Far2::ECTL_ADDSTACKBOOKMARK:
		nRc = psi3.EditorControl(-1, ECTL_ADDSESSIONBOOKMARK, 0, (void*)Param); break;
	case Far2::ECTL_PREVSTACKBOOKMARK:
		nRc = psi3.EditorControl(-1, ECTL_PREVSESSIONBOOKMARK, 0, (void*)Param); break;
	case Far2::ECTL_NEXTSTACKBOOKMARK:
		nRc = psi3.EditorControl(-1, ECTL_NEXTSESSIONBOOKMARK, 0, (void*)Param); break;
	case Far2::ECTL_CLEARSTACKBOOKMARKS:
		nRc = psi3.EditorControl(-1, ECTL_CLEARSESSIONBOOKMARKS, 0, (void*)Param); break;
	case Far2::ECTL_DELETESTACKBOOKMARK:
		nRc = psi3.EditorControl(-1, ECTL_DELETESESSIONBOOKMARK, 0, (void*)Param); break;
	case Far2::ECTL_GETSTACKBOOKMARKS:
		nRc = psi3.EditorControl(-1, ECTL_GETSESSIONBOOKMARKS, 0, (void*)Param); break;
	case Far2::ECTL_UNDOREDO:
		{
			const Far2::EditorUndoRedo* p2 = (const Far2::EditorUndoRedo*)Param;
			EditorUndoRedo p3 = {};
			#if MVV_3>2800
			p3.StructSize = sizeof(p3);
			#endif
			//TODO: ����������� �������
			p3.Command = (EDITOR_UNDOREDO_COMMANDS)p2->Command;
			nRc = psi3.EditorControl(-1, ECTL_UNDOREDO, 0, &p3);
		}
		break;
	case Far2::ECTL_GETFILENAME:
		nRc = psi3.EditorControl(-1, ECTL_GETFILENAME, 0, (void*)Param); break;
	}
	return nRc;
};
int WrapPluginInfo::FarApiInputBox(const wchar_t *Title, const wchar_t *SubTitle, const wchar_t *HistoryName, const wchar_t *SrcText, wchar_t *DestText, int   DestLength, const wchar_t *HelpTopic, DWORD Flags)
{
	LOG_CMD(L"psi2.InputBox",0,0,0);
	INPUTBOXFLAGS Flags3 = 0
		| ((Flags & Far2::FIB_ENABLEEMPTY) ? FIB_ENABLEEMPTY : 0)
		| ((Flags & Far2::FIB_PASSWORD) ? FIB_PASSWORD : 0)
		| ((Flags & Far2::FIB_EXPANDENV) ? FIB_EXPANDENV : 0)
		| ((Flags & Far2::FIB_NOUSELASTHISTORY) ? FIB_NOUSELASTHISTORY : 0)
		| ((Flags & Far2::FIB_BUTTONS) ? FIB_BUTTONS : 0)
		| ((Flags & Far2::FIB_NOAMPERSAND) ? FIB_NOAMPERSAND : 0)
		| ((Flags & Far2::FIB_EDITPATH) ? FIB_EDITPATH : 0)
		;
	int nRc = psi3.InputBox(WrapGuids(mguid_ApiInput), Title, SubTitle, HistoryName, SrcText, DestText, DestLength, HelpTopic, Flags);
	return nRc;
};
int WrapPluginInfo::FarApiPluginsControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2)
{
	LOG_CMD(L"psi2.PluginsControl",0,0,0);
	int nRc = 0; //psi3.PluginsControl(hHandle, Command, Param1, Param2);
	switch (Command)
	{
	case Far2::PCTL_LOADPLUGIN:
		nRc = psi3.PluginsControl(hHandle, PCTL_LOADPLUGIN, Param1, (void*)Param2); break;
	case Far2::PCTL_UNLOADPLUGIN:
		nRc = psi3.PluginsControl(hHandle, PCTL_UNLOADPLUGIN, Param1, (void*)Param2); break;
	case 2/*Far2::PCTL_FORCEDLOADPLUGIN*/:
		nRc = psi3.PluginsControl(hHandle, PCTL_FORCEDLOADPLUGIN, Param1, (void*)Param2); break;
	}
	return nRc;
};
int WrapPluginInfo::FarApiFileFilterControl(HANDLE hHandle, int Command, int Param1, LONG_PTR Param2)
{
	LOG_CMD(L"psi2.FileFilterControl",0,0,0);
	int nRc = 0; //psi3.FileFilterControl(hHandle, Command, Param1, Param2);
	switch (Command)
	{
	case Far2::FFCTL_CREATEFILEFILTER:
		nRc = psi3.FileFilterControl(hHandle, FFCTL_CREATEFILEFILTER, Param1, (void*)Param2); break;
	case Far2::FFCTL_FREEFILEFILTER:
		nRc = psi3.FileFilterControl(hHandle, FFCTL_FREEFILEFILTER, Param1, (void*)Param2); break;
	case Far2::FFCTL_OPENFILTERSMENU:
		nRc = psi3.FileFilterControl(hHandle, FFCTL_OPENFILTERSMENU, Param1, (void*)Param2); break;
	case Far2::FFCTL_STARTINGTOFILTER:
		nRc = psi3.FileFilterControl(hHandle, FFCTL_STARTINGTOFILTER, Param1, (void*)Param2); break;
	case Far2::FFCTL_ISFILEINFILTER:
		{
			Far2::FAR_FIND_DATA* p2 = (Far2::FAR_FIND_DATA*)Param2;
			PluginPanelItem p3;
			PluginPanelItem_2_3(p2, &p3);
			nRc = psi3.FileFilterControl(hHandle, FFCTL_ISFILEINFILTER, Param1, (void*)&p3);
		}
		break;
	}
	return nRc;
};
int WrapPluginInfo::FarApiRegExpControl(HANDLE hHandle, int Command, LONG_PTR Param)
{
	LOG_CMD(L"psi2.RegExpControl",0,0,0);
	int nRc = 0; //psi3.RegExpControl(hHandle, Command, 0, (void*)Param);
	switch (Command)
	{
	case Far2::RECTL_CREATE:
		nRc = psi3.RegExpControl(hHandle, RECTL_CREATE, 0, (void*)Param); break;
	case Far2::RECTL_FREE:
		nRc = psi3.RegExpControl(hHandle, RECTL_FREE, 0, (void*)Param); break;
	case Far2::RECTL_COMPILE:
		nRc = psi3.RegExpControl(hHandle, RECTL_COMPILE, 0, (void*)Param); break;
	case Far2::RECTL_OPTIMIZE:
		nRc = psi3.RegExpControl(hHandle, RECTL_OPTIMIZE, 0, (void*)Param); break;
	case Far2::RECTL_MATCHEX:
		nRc = psi3.RegExpControl(hHandle, RECTL_MATCHEX, 0, (void*)Param); break;
	case Far2::RECTL_SEARCHEX:
		nRc = psi3.RegExpControl(hHandle, RECTL_SEARCHEX, 0, (void*)Param); break;
	case Far2::RECTL_BRACKETSCOUNT:
		nRc = psi3.RegExpControl(hHandle, RECTL_BRACKETSCOUNT, 0, (void*)Param); break;
	}
	return nRc;
};
//struct int WINAPIV FARSTDSPRINTF(wchar_t *Buffer,const wchar_t *Format,...);
//struct int WINAPIV FARSTDSNPRINTF(wchar_t *Buffer,size_t Sizebuf,const wchar_t *Format,...);
//struct int WINAPIV FARSTDSSCANF(const wchar_t *Buffer, const wchar_t *Format,...);
//struct void WINAPI FARSTDQSORT(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
//struct void WINAPI FARSTDQSORTEX(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam);
//struct void   *WINAPI FARSTDBSEARCH(const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
int WrapPluginInfo::FarStdGetFileOwner(const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,int Size)
{
	LOG_CMD(L"fsf2.GetFileOwner",0,0,0);
	__int64 nRc = FSF3.GetFileOwner(Computer, Name, Owner, Size);
	return (int)nRc;
};
//struct int WINAPI FARSTDGETNUMBEROFLINKS(const wchar_t *Name);
//struct int WINAPI FARSTDATOI(const wchar_t *s);
//struct __int64 WINAPI FARSTDATOI64(const wchar_t *s);
//struct wchar_t   *WINAPI FARSTDITOA64(__int64 value, wchar_t *string, int radix);
//struct wchar_t   *WINAPI FARSTDITOA(int value, wchar_t *string, int radix);
//struct wchar_t   *WINAPI FARSTDLTRIM(wchar_t *Str);
//struct wchar_t   *WINAPI FARSTDRTRIM(wchar_t *Str);
//struct wchar_t   *WINAPI FARSTDTRIM(wchar_t *Str);
//struct wchar_t   *WINAPI FARSTDTRUNCSTR(wchar_t *Str,int MaxLength);
//struct wchar_t   *WINAPI FARSTDTRUNCPATHSTR(wchar_t *Str,int MaxLength);
//struct wchar_t   *WINAPI FARSTDQUOTESPACEONLY(wchar_t *Str);
//struct const wchar_t*WINAPI FARSTDPOINTTONAME(const wchar_t *Path);
int WrapPluginInfo::FarStdGetPathRoot(const wchar_t *Path,wchar_t *Root, int DestSize)
{
	LOG_CMD(L"fsf2.GetPathRoot",0,0,0);
	__int64 nRc = FSF3.GetPathRoot(Path, Root, DestSize);
	return (int)nRc;
};
int WrapPluginInfo::FarStdCopyToClipboard(const wchar_t *Data)
{
	LOG_CMD(L"fsf2.CopyToClipboardWrap",0,0,0);
	int iRc = FSF3.CopyToClipboard(FCT_ANY, Data);
	return iRc;
}
wchar_t* WrapPluginInfo::FarStdPasteFromClipboard()
{
	LOG_CMD(L"fsf2.PasteFromClipboard",0,0,0);
	wchar_t* psz = NULL;
	INT_PTR iRc = FSF3.PasteFromClipboard(FCT_ANY, NULL, 0);
	if (iRc)
	{
		psz = (wchar_t*)malloc((iRc+1)*sizeof(wchar_t));
		iRc = FSF3.PasteFromClipboard(FCT_ANY, psz, iRc);
	}
	return psz;
}
//struct BOOL WINAPI FARSTDADDENDSLASH(wchar_t *Path);
//struct int WINAPI FARSTDCOPYTOCLIPBOARD(const wchar_t *Data);
//struct wchar_t *WINAPI FARSTDPASTEFROMCLIPBOARD(void);
//struct int WINAPI FARSTDINPUTRECORDTOKEY(const INPUT_RECORD *r);
//struct int WINAPI FARSTDLOCALISLOWER(wchar_t Ch);
//struct int WINAPI FARSTDLOCALISUPPER(wchar_t Ch);
//struct int WINAPI FARSTDLOCALISALPHA(wchar_t Ch);
//struct int WINAPI FARSTDLOCALISALPHANUM(wchar_t Ch);
//struct wchar_t WINAPI FARSTDLOCALUPPER(wchar_t LowerChar);
//struct wchar_t WINAPI FARSTDLOCALLOWER(wchar_t UpperChar);
//struct void WINAPI FARSTDLOCALUPPERBUF(wchar_t *Buf,int Length);
//struct void WINAPI FARSTDLOCALLOWERBUF(wchar_t *Buf,int Length);
//struct void WINAPI FARSTDLOCALSTRUPR(wchar_t *s1);
//struct void WINAPI FARSTDLOCALSTRLWR(wchar_t *s1);
//struct int WINAPI FARSTDLOCALSTRICMP(const wchar_t *s1,const wchar_t *s2);
//struct int WINAPI FARSTDLOCALSTRNICMP(const wchar_t *s1,const wchar_t *s2,int n);
wchar_t* WrapPluginInfo::FarStdXlatW3(wchar_t *Line,int StartPos,int EndPos,DWORD Flags)
{
	LOG_CMD(L"fsf2.XLat",0,0,0);
	wchar_t* pszRc = FSF3.XLat(Line, StartPos, EndPos, Flags);
	return pszRc;
};
int WrapPluginInfo::RecSearchUserFn(const struct PluginPanelItem *FData, const wchar_t *FullName, void *Param)
{
	RecSearchUserFnArg* p = (RecSearchUserFnArg*)Param;
	Far2::FAR_FIND_DATA ffd;
	PluginPanelItem_3_2(FData, &ffd);
	return p->UserFn2(&ffd, FullName, p->Param2);
}
int WrapPluginInfo::GetNumberOfLinks(const wchar_t *Name)
{
	size_t nRc = FSF3.GetNumberOfLinks(Name);
	return nRc;
}
void WrapPluginInfo::FarStdRecursiveSearch(const wchar_t *InitDir,const wchar_t *Mask,Far2::FRSUSERFUNC Func,DWORD Flags,void *Param)
{
	LOG_CMD(L"fsf2.RecursiveSearch",0,0,0);
	RecSearchUserFnArg arg = {Func, Param};
	FRSMODE Flags3 = (FRSMODE)0;
	if (Flags & Far2::FRS_RETUPDIR)
		Flags3 |= FRS_RETUPDIR;
	if (Flags & Far2::FRS_RECUR)
		Flags3 |= FRS_RECUR;
	if (Flags & Far2::FRS_SCANSYMLINK)
		Flags3 |= FRS_SCANSYMLINK;
	FSF3.FarRecursiveSearch(InitDir, Mask, RecSearchUserFn, Flags3, &arg);
};
int WrapPluginInfo::FarStdMkTemp(wchar_t *Dest, DWORD size, const wchar_t *Prefix)
{
	LOG_CMD(L"fsf2.MkTemp",0,0,0);
	__int64 nRc = FSF3.MkTemp(Dest, size, Prefix);
	return (int)nRc;
};
int WrapPluginInfo::FarStdProcessName(const wchar_t *param1, wchar_t *param2, DWORD size, DWORD flags)
{
	LOG_CMD(L"fsf2.ProcessName",0,0,0);
	__int64 nRc = FSF3.ProcessName(param1, param2, size, flags);
	return (int)nRc;
};
int WrapPluginInfo::FarStdMkLink(const wchar_t *Src,const wchar_t *Dest,DWORD Flags)
{
	LOG_CMD(L"fsf2.MkLink",0,0,0);
	LINK_TYPE Type3 = (LINK_TYPE)(Flags & 7);
	MKLINK_FLAGS Flags3 = (MKLINK_FLAGS)(Flags & 0x30000);
	BOOL nRc = FSF3.MkLink(Src, Dest, Type3, Flags3);
	return nRc;
};
int WrapPluginInfo::FarConvertPath(enum Far2::CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize)
{
	LOG_CMD(L"fsf2.ConvertPath",0,0,0);
	__int64 nRc = 0;
	switch (Mode)
	{
	case Far2::CPM_FULL:
		nRc = FSF3.ConvertPath(CPM_FULL, Src, Dest, DestSize); break;
	case Far2::CPM_REAL:
		nRc = FSF3.ConvertPath(CPM_REAL, Src, Dest, DestSize); break;
	case Far2::CPM_NATIVE:
		nRc = FSF3.ConvertPath(CPM_NATIVE, Src, Dest, DestSize); break;
	}
	return (int)nRc;
};
int WrapPluginInfo::FarGetReparsePointInfo(const wchar_t *Src, wchar_t *Dest,int DestSize)
{
	LOG_CMD(L"fsf2.GetReparsePointInfo",0,0,0);
	__int64 nRc = FSF3.GetReparsePointInfo(Src, Dest, DestSize);
	return (int)nRc;
};
DWORD WrapPluginInfo::FarGetCurrentDirectory(DWORD Size,wchar_t* Buffer)
{
	LOG_CMD(L"fsf2.GetCurrentDirectory",0,0,0);
	size_t nRc = FSF3.GetCurrentDirectory(Size, Buffer);
	return (DWORD)nRc;
};
struct WrapQSortParam
{
	int (__cdecl *fcmp)(const void *, const void *);
	static int WINAPI Far3Cmp(const void *p1, const void *p2, void *userparam)
	{
		return ((WrapQSortParam*)userparam)->fcmp(p1, p2);
	};
};
void WrapPluginInfo::FarStdQSort(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *))
{
	LOG_CMD(L"fsf2.FarStdQSort",0,0,0);
	WrapQSortParam prm = {fcmp};
	FSF3.qsort(base, nelem, width, WrapQSortParam::Far3Cmp, &prm);
};
struct WrapQSortExParam
{
	int (__cdecl *fcmp)(const void *, const void *, void *userparam);
	void *userparam;
	static int WINAPI Far3Cmp(const void *p1, const void *p2, void *userparam)
	{
		return ((WrapQSortExParam*)userparam)->fcmp(p1, p2, userparam);
	};
};
void WrapPluginInfo::FarStdQSortEx(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam)
{
	LOG_CMD(L"fsf2.FarStdQSortEx",0,0,0);
	WrapQSortExParam prm = {fcmp, userparam};
	FSF3.qsort(base, nelem, width, WrapQSortExParam::Far3Cmp, &prm);
};
struct WrapBSearchParam
{
	int (__cdecl *fcmp)(const void *, const void *);
	static int WINAPI Far3Cmp(const void *p1, const void *p2, void *userparam)
	{
		return ((WrapBSearchParam*)userparam)->fcmp(p1, p2);
	};
};
void* WrapPluginInfo::FarStdBSearch(const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *))
{
	LOG_CMD(L"fsf2.FarStdBSearch",0,0,0);
	WrapQSortParam prm = {fcmp};
	void* pRc = FSF3.bsearch(key, base, nelem, width, WrapBSearchParam::Far3Cmp, &prm);
	return pRc;
};












void WrapPluginInfo::SetStartupInfoW3(PluginStartupInfo *Info)
{
#ifdef _DEBUG
	if (Info->StructSize != sizeof(*Info)
		|| IsBadReadPtr(Info->FSF, sizeof(*Info->FSF))
		|| Info->FSF->StructSize != sizeof(*Info->FSF))
	{
		_ASSERTE(Info->StructSize == sizeof(*Info));
		_ASSERTE(!IsBadReadPtr(Info->FSF, sizeof(*Info->FSF)));
		_ASSERTE(Info->FSF->StructSize == sizeof(*Info->FSF));
	}
#endif

	memmove(&psi3, Info, sizeof(psi3));
	memmove(&FSF3, Info->FSF, sizeof(FSF3));
	psi3.FSF = &FSF3;
	lbPsi3 = TRUE;

#if MVV_3>=2103
	if (!FarInputRecordToName)
		FarInputRecordToName = Info->FSF->FarInputRecordToName;
	if (!FarNameToInputRecord)
		FarNameToInputRecord = Info->FSF->FarNameToInputRecord;
	
#ifdef _DEBUG
	static bool bFirstCall = false;
	if (!bFirstCall)
	{
		bFirstCall = true;
		struct {
			LPCWSTR pszKey;
			wchar_t szDbg[128];
			bool Cmp;
			INPUT_RECORD r;
		} Keys[] = {
			{L"MsWheelUp"},
			{L"AltF"}, 
			{L"AltBS"},
			{L"RAltF"},
			{L"RAltBS"},
			{L"CtrlR"},
			{L"RCtrlR"},
			{L"AltShiftBS"},
			{L"CtrlAltBS"},
			{L"CtrlBreak"},
			{L"CtrlAlt["},
			{L"Ctrl["},
			{L"["},
		};
		int lFail = -1;
		BOOL b1; size_t nDbgSize;
		for (UINT i = 0; i < ARRAYSIZE(Keys); i++)
		{
			b1 = FarNameToInputRecord(Keys[i].pszKey, &Keys[i].r);
			nDbgSize = FarInputRecordToName(&Keys[i].r, Keys[i].szDbg, ARRAYSIZE(Keys[i].szDbg));
			Keys[i].Cmp = lstrcmpi(Keys[i].pszKey, Keys[i].szDbg)==0;
			if (lFail==-1 && !Keys[i].Cmp)
			{
				lFail = i;
				_ASSERTE(lFail==-1);
				FarNameToInputRecord(Keys[i].pszKey, &Keys[i].r);
			}
		}
		_ASSERTE(lFail==-1);
	}
#endif
#endif // #if MVV_3>=2103

	LoadPluginInfo();
	
	if (!LoadPlugin(TRUE))
		return;

	LOG_CMD(L"SetStartupInfoW",0,0,0);	
	// ��������� � FSF2 & psi2
	
	// *** FARSTANDARDFUNCTIONS *** 
	FSF2.StructSize = sizeof(FSF2);
	FSF2.atoi = FSF3.atoi;
	FSF2.atoi64 = FSF3.atoi64;
	FSF2.itoa = FSF3.itoa;
	FSF2.itoa64 = FSF3.itoa64;
	// <C&C++>
	FSF2.sprintf = FSF3.sprintf;
	FSF2.sscanf = FSF3.sscanf;
	// </C&C++>
	FSF2.qsort = WrapPluginInfo::FarStdQSortExp;
	FSF2.bsearch = WrapPluginInfo::FarStdBSearchExp;
	FSF2.qsortex = WrapPluginInfo::FarStdQSortExExp;
	// <C&C++>
	FSF2.snprintf = FSF3.snprintf;
	// </C&C++>
	FSF2.LIsLower = FSF3.LIsLower;
	FSF2.LIsUpper = FSF3.LIsUpper;
	FSF2.LIsAlpha = FSF3.LIsAlpha;
	FSF2.LIsAlphanum = FSF3.LIsAlphanum;
	FSF2.LUpper = FSF3.LUpper;
	FSF2.LLower = FSF3.LLower;
	FSF2.LUpperBuf = FSF3.LUpperBuf;
	FSF2.LLowerBuf = FSF3.LLowerBuf;
	FSF2.LStrupr = FSF3.LStrupr;
	FSF2.LStrlwr = FSF3.LStrlwr;
	FSF2.LStricmp = FSF3.LStricmp;
	FSF2.LStrnicmp = FSF3.LStrnicmp;
	FSF2.Unquote = FSF3.Unquote;
	FSF2.LTrim = FSF3.LTrim;
	FSF2.RTrim = FSF3.RTrim;
	FSF2.Trim = FSF3.Trim;
	FSF2.TruncStr = FSF3.TruncStr;
	FSF2.TruncPathStr = FSF3.TruncPathStr;
	FSF2.QuoteSpaceOnly = FSF3.QuoteSpaceOnly;
	FSF2.PointToName = FSF3.PointToName;
	FSF2.GetPathRoot = WrapPluginInfo::FarStdGetPathRootExp;
	FSF2.AddEndSlash = FSF3.AddEndSlash;
	FSF2.CopyToClipboard = WrapPluginInfo::FarStdCopyToClipboardExp;
	FSF2.PasteFromClipboard = WrapPluginInfo::FarStdPasteFromClipboardExp;
#if MVV_3>=2103
	FSF2.FarKeyToName = WrapPluginInfo::FarKeyToName3;
	FSF2.FarNameToKey = WrapPluginInfo::FarNameToKey3;
	FSF2.FarInputRecordToKey = WrapPluginInfo::FarKey_3_2;
#else
	FSF2.FarKeyToName = FSF3.FarKeyToName;
	FSF2.FarNameToKey = FSF3.FarNameToKey;
	FSF2.FarInputRecordToKey = FSF3.FarInputRecordToKey;
#endif
	FSF2.XLat = WrapPluginInfo::FarStdXlatExp;
	FSF2.GetFileOwner = WrapPluginInfo::FarStdGetFileOwnerExp;
	FSF2.GetNumberOfLinks = WrapPluginInfo::GetNumberOfLinksExp;
	FSF2.FarRecursiveSearch = WrapPluginInfo::FarStdRecursiveSearchExp;
	FSF2.MkTemp = WrapPluginInfo::FarStdMkTempExp;
	#if MVV_3<=2798
	FSF2.DeleteBuffer = FSF3.DeleteBuffer;
	#else
	FSF2.DeleteBuffer = WrapPluginInfo::DeleteBufferWrap;
	#endif
	FSF2.ProcessName = WrapPluginInfo::FarStdProcessNameExp;
	FSF2.MkLink = WrapPluginInfo::FarStdMkLinkExp;
	FSF2.ConvertPath = WrapPluginInfo::FarConvertPathExp;
	FSF2.GetReparsePointInfo = WrapPluginInfo::FarGetReparsePointInfoExp;
	FSF2.GetCurrentDirectory = WrapPluginInfo::FarGetCurrentDirectoryExp;
	FSF2.qsort = WrapPluginInfo::FarStdQSortExp;
	FSF2.qsortex = WrapPluginInfo::FarStdQSortExExp;
	FSF2.bsearch = WrapPluginInfo::FarStdBSearchExp;
	
	// *** PluginStartupInfo ***
	psi2.StructSize = sizeof(psi2);
	psi2.ModuleName = ms_PluginDll;
	psi2.ModuleNumber = (INT_PTR)mh_Dll;
	psi2.RootKey = ms_RegRoot;
	psi2.Menu = WrapPluginInfo::FarApiMenuExp;
	psi2.Message = WrapPluginInfo::FarApiMessageExp;
	psi2.GetMsg = WrapPluginInfo::FarApiGetMsgExp;
	psi2.Control = WrapPluginInfo::FarApiControlExp;
	psi2.SaveScreen = WrapPluginInfo::FarApiSaveScreenExp;
	psi2.RestoreScreen = WrapPluginInfo::FarApiRestoreScreenExp;
	psi2.GetDirList = WrapPluginInfo::FarApiGetDirListExp;
	psi2.GetPluginDirList = WrapPluginInfo::FarApiGetPluginDirListExp;
	psi2.FreeDirList = WrapPluginInfo::FarApiFreeDirListExp;
	psi2.FreePluginDirList = WrapPluginInfo::FarApiFreePluginDirListExp;
	psi2.Viewer = WrapPluginInfo::FarApiViewerExp;
	psi2.Editor = WrapPluginInfo::FarApiEditorExp;
	psi2.CmpName = WrapPluginInfo::FarApiCmpNameExp;
	psi2.Text = WrapPluginInfo::FarApiTextExp;
	psi2.EditorControl = WrapPluginInfo::FarApiEditorControlExp;

	psi2.FSF = &FSF2;

	psi2.ShowHelp = WrapPluginInfo::FarApiShowHelpExp;
	psi2.AdvControl = WrapPluginInfo::FarApiAdvControlExp;
	psi2.InputBox = WrapPluginInfo::FarApiInputBoxExp;
	psi2.DialogInit = WrapPluginInfo::FarApiDialogInitExp;
	psi2.DialogRun = WrapPluginInfo::FarApiDialogRunExp;
	psi2.DialogFree = WrapPluginInfo::FarApiDialogFreeExp;

	psi2.SendDlgMessage = WrapPluginInfo::FarApiSendDlgMessageExp;
	psi2.DefDlgProc = WrapPluginInfo::FarApiDefDlgProcExp;
	//DWORD_PTR              Reserved;
	psi2.ViewerControl = WrapPluginInfo::FarApiViewerControlExp;
	psi2.PluginsControl = WrapPluginInfo::FarApiPluginsControlExp;
	psi2.FileFilterControl = WrapPluginInfo::FarApiFileFilterControlExp;
	psi2.RegExpControl = WrapPluginInfo::FarApiRegExpControlExp;

	lbPsi2 = TRUE;
	
	if (SetStartupInfoW && lbPsi2)
	{
		SetStartupInfoW(&psi2);
	}

	// ���� ���� GetPluginInfoW==NULL - ��� ����� ����� GetPluginInfoInternal (��� ���� ����������)
	_ASSERTE(m_Info.StructSize == 0); // �� ����, ��� �� ������ ��� ���� ������, ��� ��� m_Info ��� �� ����������������!
	GetPluginInfoInternal();
}

void WrapPluginInfo::GetGlobalInfoW3(GlobalInfo *Info)
{
	//LOG_CMD(L"GetGlobalInfoW",0,0,0);
	LoadPluginInfo();
	
	//Info->StructSize = sizeof(GlobalInfo);
	_ASSERTE(Info->StructSize >= sizeof(GlobalInfo));
	Info->MinFarVersion = FARMANAGERVERSION;

	Info->Version = m_Version;
	Info->Guid = mguid_Plugin;
	Info->Title = ms_Title;
	Info->Description = ms_Desc;
	Info->Author = ms_Author;

#ifdef _DEBUG
	wchar_t szDebugInfo[2048], szGUID[64];
	FormatGuid(&mguid_Plugin, szGUID, FALSE);
	wsprintf(szDebugInfo, L"GetGlobalInfoW3: %s: %s\n", szGUID, ms_PluginDll);
	DebugStr(szDebugInfo);
#endif
}

void WrapPluginInfo::GetPluginInfoInternal()
{
	LOG_CMD(L"GetPluginInfoW",0,0,0);
	DWORD nOldSysID = m_Info.Reserved;
	ZeroStruct(m_Info);
	m_Info.StructSize = sizeof(m_Info);

	if (GetPluginInfoW)
	{
		GetPluginInfoW(&m_Info);
	}

	if (nOldSysID && nOldSysID != m_Info.Reserved)
		(*gpMapSysID).erase(nOldSysID);
	if (m_Info.Reserved)
		(*gpMapSysID)[m_Info.Reserved] = this;

	if (mn_LoadOnStartup)
		m_Info.Flags |= PF_PRELOAD;
}

void WrapPluginInfo::GetPluginInfoW3(PluginInfo *Info)
{
    //memset(Info, 0, sizeof(PluginInfo)); -- ������ ���������� ������ ����� ���� ������, ��� ������� ������, memset ����
	//Info->StructSize = sizeof(*Info);
	//_ASSERTE(Info->StructSize>0 && (Info->StructSize >= (size_t)(((LPBYTE)&Info->MacroFunctionNumber) - (LPBYTE)Info)));

	_ASSERTE(Info && Info->StructSize>=sizeof(*Info));

	//Yac, ����.
	if (Info && !Info->StructSize)
	{
		//if (!IsBadWritePtr(Info, sizeof(Far2::PluginInfo)))
		//{
		//	GetPluginInfoInternal();
		//	memmove(Info, &m_Info, sizeof(m_Info));
		//}
		return;
	}

	//_ASSERTE(lbPsi2 && lbPsi3);
	if (!lbPsi3)
	{
		DebugStr(L"GetPluginInfoW3 called before SetStartupInfoW3\n");
	}

	// �� ����, mh_Dll �� ����� ���� NULL - ����� Loader ����� FreeLibrary �� Far3Wrap � �������� ��� ����
    if (!lbPsi2 || !mh_Dll)
	{
		if (!*ms_DllFailTitle)
		{
			wchar_t szSelf[MAX_PATH+1]; szSelf[0] = 0;
			GetModuleFileName(mh_Loader, szSelf, ARRAYSIZE(szSelf)-4);
			wchar_t* pszPlugin = *ms_File ? ms_File : szSelf;

			wsprintf(ms_DllFailTitle, L"[%s] %s x%08X",
				pszPlugin,
				*ms_DllFailFunc ? ms_DllFailFunc : !mh_Dll ? L"mh_Dll" : L"lbPsi2",
				mn_DllFailCode);
		}

		static wchar_t *pszFailMenu[1]; pszFailMenu[0] = ms_DllFailTitle;
		Info->PluginMenu.Count = 1;
		Info->PluginMenu.Strings = pszFailMenu;
		Info->PluginMenu.Guids = &mguid_Plugin;
	}
	else if (GetPluginInfoW || mn_LoadOnStartup)
    {
    	GetPluginInfoInternal();
    	
    	if (m_Info.Flags & Far2::PF_PRELOAD)
    		Info->Flags |= PF_PRELOAD;
    	if (m_Info.Flags & Far2::PF_DISABLEPANELS)
    		Info->Flags |= PF_DISABLEPANELS;
    	if (m_Info.Flags & Far2::PF_EDITOR)
    		Info->Flags |= PF_EDITOR;
    	if (m_Info.Flags & Far2::PF_VIEWER)
    		Info->Flags |= PF_VIEWER;
    	if (m_Info.Flags & Far2::PF_FULLCMDLINE)
    		Info->Flags |= PF_FULLCMDLINE;
    	if (m_Info.Flags & Far2::PF_DIALOG)
    		Info->Flags |= PF_DIALOG;

		//if (GetPrivateProfileString(L"Plugin", L"MenuGUID", L"", szTemp, ARRAYSIZE(szTemp), szIni))
		//{
		//	if (
		//		guid_PluginMenu = guid;
		//	else
		//		guid_PluginMenu = ::guid_DefPluginMenu;
		//}

		wchar_t szValName[64], szGUID[128]; GUID guid;
		struct { LPCWSTR Fmt; int Count; const wchar_t * const * Strings;
				 PluginMenuItem *Menu; int* GuidCount; GUID** Guids; }
			Menus[] = 
		{
			{L"PluginMenuGUID%i", m_Info.PluginMenuStringsNumber, m_Info.PluginMenuStrings, 
				&Info->PluginMenu, &mn_PluginMenu, &mguids_PluginMenu},
			{L"DiskMenuGUID%i", m_Info.DiskMenuStringsNumber, m_Info.DiskMenuStrings, 
				&Info->DiskMenu, &mn_PluginDisks, &mguids_PluginDisks},
			{L"PluginConfigGUID%i", m_Info.PluginConfigStringsNumber, m_Info.PluginConfigStrings, 
				&Info->PluginConfig, &mn_PluginConfig, &mguids_PluginConfig}
		};
		for (int k = 0; k < ARRAYSIZE(Menus); k++)
		{
			if (Menus[k].Count < 1)
				continue;
			
    		if (*Menus[k].GuidCount < Menus[k].Count || !*Menus[k].Guids)
    		{
    			if (*Menus[k].Guids)
    				free(*Menus[k].Guids);
    			*Menus[k].GuidCount = 0;
    			*Menus[k].Guids = (GUID*)calloc(sizeof(GUID),Menus[k].Count);
    			for (int i = 1; i <= Menus[k].Count; i++)
    			{
    				wsprintf(szValName, Menus[k].Fmt, i);
					BOOL lbGot = FALSE;
					szGUID[0] = 0;
    				lbGot = (GetPrivateProfileString(L"Plugin", szValName, L"", szGUID, ARRAYSIZE(szGUID), ms_IniFile)
						&& (UuidFromStringW((RPC_WSTR)szGUID, &guid) == RPC_S_OK));
    				if (!lbGot)
					{
						RPC_STATUS hr = UuidCreate(&guid);
						if (FAILED(hr))
							break;
						WritePrivateProfileString(L"Plugin", szValName, FormatGuid(&guid, szGUID), ms_IniFile);
					}
    				// OK
    				(*Menus[k].Guids)[i-1] = guid;
    				(*Menus[k].GuidCount)++;
    			}
				if (Menus[k].Count != *Menus[k].GuidCount)
				{
					//TODO: ������� ������������ ��� ������ � ���������
					_ASSERTE(Menus[k].Count == *Menus[k].GuidCount);
    				Menus[k].Count = *Menus[k].GuidCount; // ����� �� �� ���� ������ �������
				}
    		}
    		if (Menus[k].Count > 0)
    		{
				Menus[k].Menu->Guids = *Menus[k].Guids;
				Menus[k].Menu->Strings = Menus[k].Strings;
				Menus[k].Menu->Count = Menus[k].Count;
			}
    	}
		//const wchar_t * const *DiskMenuStrings;
		//int *DiskMenuNumbers;
		//int DiskMenuStringsNumber;
		// -- >struct PluginMenuItem DiskMenu;
		//const wchar_t * const *PluginMenuStrings;
		//int PluginMenuStringsNumber;
		// -->struct PluginMenuItem PluginMenu;
		//const wchar_t * const *PluginConfigStrings;
		//int PluginConfigStringsNumber;
		// -->struct PluginMenuItem PluginConfig;
		
		if (ms_ForcePrefix[0] && (!m_Info.CommandPrefix || !*m_Info.CommandPrefix) && this->OpenFilePluginW)
			Info->CommandPrefix = ms_ForcePrefix;
		else
			Info->CommandPrefix = m_Info.CommandPrefix;
    }
}

HANDLE WrapPluginInfo::OpenFilePluginHelper(LPCWSTR asFile)
{
	unsigned char Buffer[8192];
	DWORD nRead = 0;
	// ��������� ������� ������� ����� - ����� ������� ��������� �����
	// � ��������������, ����������� ���� � \\UNC ������
	size_t nLen = FSF3.ConvertPath(CPM_NATIVE, asFile, NULL, 0);
	wchar_t* pszUnc = (wchar_t*)malloc(nLen*sizeof(wchar_t));
	FSF3.ConvertPath(CPM_NATIVE, asFile, pszUnc, nLen);
	HANDLE hFile = CreateFile(pszUnc, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (!ReadFile(hFile, Buffer, sizeof(Buffer), &nRead, NULL))
			nRead = 0;
		CloseHandle(hFile);
	}
	free(pszUnc);
	
	HANDLE h = OpenFilePluginW(asFile, Buffer, nRead, 0);
	return h;
}

HANDLE WrapPluginInfo::OpenW3(const OpenInfo *Info)
{
	LOG_CMD(L"OpenW(0x%08X)", (DWORD)Info->OpenFrom,0,0);
#ifdef _DEBUG
	wchar_t szDebugInfo[2048], szGUID[64], szMenuGUID[64];
	FormatGuid(&mguid_Plugin, szGUID, FALSE);
	FormatGuid(Info->Guid, szMenuGUID, FALSE);
	wsprintf(szDebugInfo, L"OpenW3(x%X, x%X) %s\\%s: %s\n", (DWORD)Info->OpenFrom, (DWORD)Info->Data, szGUID, szMenuGUID, ms_PluginDll);
	DebugStr(szDebugInfo);
#endif

	HANDLE h = INVALID_HANDLE_VALUE;

	if (
		#if MVV_3>=2458
		(Info->OpenFrom == OPEN_FROMMACRO)
		#else
		(Info->OpenFrom & OPEN_FROMMACRO_MASK)
		#endif
	   )
	{
		_ASSERTE(m_Info.StructSize!=0);
		if (m_Info.Reserved 
			#if MVV_3<2458
			&& (((Info->OpenFrom & OPEN_FROMMACRO_MASK) == OPEN_FROMMACRO)
				|| ((Info->OpenFrom & OPEN_FROMMACRO_MASK) == OPEN_FROMMACROSTRING)
				|| ((Info->OpenFrom & OPEN_FROMMACRO_MASK) == (OPEN_FROMMACRO|OPEN_FROMMACROSTRING)))
			#endif
		    && OpenPluginW)
		{
			#if MVV_3>=2458
			INT_PTR Item = 0;
			DWORD nOpen2 = OpenFrom_3_2(Info->OpenFrom, Info->Data, Item);
			#else
			DWORD nOpen2 = Far2::OPEN_FROMMACRO;
			INT_PTR Item = Info->Data;
			if ((HIBYTE(LOWORD(m_MinFarVersion)) > 2)
				|| ((HIBYTE(LOWORD(m_MinFarVersion)) == 2) && (HIWORD(m_MinFarVersion) >= 1800)))
			{
				nOpen2 |= ((Info->OpenFrom & OPEN_FROMMACROSTRING)?Far2::OPEN_FROMMACROSTRING:0);
				nOpen2 |= LOWORD(Info->OpenFrom);
			}
			#endif
			h = OpenPluginW(nOpen2, Item);
			#ifdef _DEBUG
			if (h)
				goto trap2;
			#endif
		}
		goto trap2;
	}
	
	int nPluginItemNumber = 0;
	GUID* pGuids = NULL;
	int nGuids = 0;
	//if ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_PLUGINSMENU)
	//{
	//	pGuids = mguids_PluginMenu; nGuids = mn_PluginMenu;
	//}
	if ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_LEFTDISKMENU || (Info->OpenFrom & OPEN_FROM_MASK) == OPEN_RIGHTDISKMENU)
	{
		pGuids = mguids_PluginDisks; nGuids = mn_PluginDisks;
	}
	else //if ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_PLUGINSMENU || (Info->OpenFrom & OPEN_FROM_MASK) == OPEN_DIALOG)
	{
		pGuids = mguids_PluginMenu; nGuids = mn_PluginMenu;
	}
	if (Info->Guid && memcmp(Info->Guid, &GUID_NULL, sizeof(GUID)) && pGuids && nGuids > 0)
	{
		for (int i = 0; i < nGuids; i++)
		{
			if (memcmp(Info->Guid, pGuids+i, sizeof(GUID)) == 0)
			{
				nPluginItemNumber = i;
				break;
			}
		}
	}
	
	// Fake: ForcePrefix
	if (((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_COMMANDLINE)
		&& (ms_ForcePrefix[0] && (!m_Info.CommandPrefix || !*m_Info.CommandPrefix) && OpenFilePluginW))
	{
		// ���� �� �������� ������ ���� �������� ������ ��������� OpenFilePluginW, �� �� ���������� �������� �������
		LPCWSTR wszCmdLine;
		#if MVV_3>2801
		OpenCommandLineInfo* pocli = (OpenCommandLineInfo*)Info->Data;
		wszCmdLine = pocli->CommandLine;
		#else
		wszCmdLine = (LPCWSTR)Info->Data;
		#endif
		h = OpenFilePluginHelper(wszCmdLine);
		goto trap;
	}

	if ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_ANALYSE)
	{
		#if MVV_3>=2462
		OpenAnalyseInfo* p3 = (OpenAnalyseInfo*)Info->Data;
		if (p3 && (p3->StructSize >= sizeof(*p3)))
		{
			h = p3->Handle;
			#if MVV_3>=2472
			if (h == PANEL_STOP)
				return PANEL_STOP;
			#endif
			_ASSERTE(h!=INVALID_HANDLE_VALUE);
			// ���� � ������� ���� AnalyseW, � ���� ������ OpenFilePlugin
			// �� � p3->Handle ��� ������� �����. � ��� ����� AnalyseW,
			// ������ ��� ����� �������
			_ASSERTE(h || AnalyseW);
			if ((h == NULL || h == (HANDLE)TRUE) && AnalyseW && OpenPluginW)
			{
				Far2::AnalyseData fad2 = {sizeof(fad2), p3->Info->FileName, (const unsigned char *)p3->Info->Buffer, p3->Info->BufferSize, OpMode_3_2(p3->Info->OpMode)};
				h = OpenPluginW(Far2::OPEN_ANALYSE, (INT_PTR)&fad2);
			}
		}
		goto trap;
		#else
		if (AnalyseW)
		{
			// � ��������, � Far2 ���� ����� �������, ��� ��� ���� ���� �� ��������� - �����
			AnalyseInfo* fad3 = (AnalyseInfo*)Info->Data;
			_ASSERTE(fad3 && fad3->StructSize >= sizeof(fad3));
			Far2::AnalyseData fad2 = {sizeof(fad2), fad3->FileName, (const unsigned char *)fad3->Buffer, fad3->BufferSize, OpMode_3_2(fad3->OpMode)};
			if (OpenPluginW)
				h = OpenPluginW(Far2::OPEN_ANALYSE, (INT_PTR)&fad2);
			goto trap;
		}
		if (mn_AnalyzeReturn == -2)
		{
			_ASSERTE(mp_Analyze == NULL);
			h = (HANDLE)-2;
			// ��� �� ����� ����, ������ ���� ������ � ���� ���� ������ (������� ������ TRUE � AnalyseW)
			if (gpsz_LastAnalyzeFile)
			{
				_ASSERTE(mpsz_LastAnalyzeFile == gpsz_LastAnalyzeFile);
				free(gpsz_LastAnalyzeFile);
				gpsz_LastAnalyzeFile = NULL;
			}
			mpsz_LastAnalyzeFile = NULL;
			goto trap;
		}
		if (!mp_Analyze || !OpenFilePluginW)
			goto trap;
		h = OpenFilePluginW(mp_Analyze->FileName,
				(const unsigned char*)mp_Analyze->Buffer,
				mp_Analyze->BufferSize,
				OpMode_3_2(mp_Analyze->OpMode));
		goto trap;
		#endif
	}
	
	if (OpenPluginW)
	{
		switch (Info->OpenFrom & OPEN_FROM_MASK)
		{
		case OPEN_LEFTDISKMENU:
		case OPEN_RIGHTDISKMENU:
			h = OpenPluginW(Far2::OPEN_DISKMENU, nPluginItemNumber); break;
		case OPEN_PLUGINSMENU:
			h = OpenPluginW(Far2::OPEN_PLUGINSMENU, nPluginItemNumber); break;
		case OPEN_FINDLIST:
			h = SetFindListW ? OpenPluginW(Far2::OPEN_FINDLIST, Info->Data) : INVALID_HANDLE_VALUE; break;
		case OPEN_SHORTCUT:
			// Far3 build 
			{
				INT_PTR Item = Info->Data;
				#if MVV_3>=2556
				OpenShortcutInfo* pOSI = (OpenShortcutInfo*)Info->Data;
				Item = (INT_PTR)pOSI->ShortcutData;
				#endif
				// ������ �����, ������ �� ������������ OPEN_SHORTCUT (�� ����� �����)
				if (OpenFilePluginW)
				{
					if (pOSI->HostFile)
						h = OpenFilePluginHelper(pOSI->HostFile);
					else if (m_Info.PluginMenuStringsNumber == 1)
						h = OpenPluginW(Far2::OPEN_PLUGINSMENU, 0);
					else
						h = OpenPluginW(Far2::OPEN_COMMANDLINE, (INT_PTR)L"");
				}
				else
				{
					h = OpenPluginW(Far2::OPEN_SHORTCUT, Item);
				}
			}
			break;
		case OPEN_COMMANDLINE:
			#if MVV_3>2801
			h = OpenPluginW(Far2::OPEN_COMMANDLINE, (INT_PTR)((OpenCommandLineInfo*)Info->Data)->CommandLine);
			#else
			h = OpenPluginW(Far2::OPEN_COMMANDLINE, Info->Data);
			#endif
			break;
		case OPEN_EDITOR:
			h = OpenPluginW(Far2::OPEN_EDITOR, Info->Data); break;
		case OPEN_VIEWER:
			h = OpenPluginW(Far2::OPEN_VIEWER, Info->Data); break;
		case OPEN_FILEPANEL:
			h = OpenPluginW(Far2::OPEN_FILEPANEL, Info->Data); break;
		case OPEN_DIALOG:
			{
				const OpenDlgPluginData* p3 = (OpenDlgPluginData*)Info->Data;
				Far2::OpenDlgPluginData p2 = {nPluginItemNumber};
				if (p3)
				{
					Far2Dialog* p = (*gpMapDlg_3_2)[p3->hDlg];
					// ����� ���� NULL, ���� ��� ������ �� �� ����� �������
					p2.hDlg = p ? ((HANDLE)p) : p3->hDlg;
					p2.ItemNumber = nPluginItemNumber;
				}
				h = OpenPluginW(Far2::OPEN_DIALOG, (INT_PTR)&p2);
			}
			break;
		}
	}
trap:
	// ������ �� ���������
	if (mp_Analyze)
	{
		free(mp_Analyze);
		mp_Analyze = NULL;
	}

	#if MVV_3>=2472
	if ((h == INVALID_HANDLE_VALUE) || (h == (HANDLE)-2))
	{
		if ((h == (HANDLE)-2) && ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_ANALYSE))
			h = PANEL_STOP;
		else
			h = NULL;
	}
	#endif
trap2:
	return h;
}

HANDLE    WrapPluginInfo::AnalyseW3(const AnalyseInfo *Info)
{
	LOG_CMD(L"AnalyseW",0,0,0);
	mn_AnalyzeReturn = 0;
	if (gpsz_LastAnalyzeFile)
	{
		if (mpsz_LastAnalyzeFile != gpsz_LastAnalyzeFile)
		{
			if (Info->FileName && !lstrcmp(Info->FileName, gpsz_LastAnalyzeFile))
			{
				// ���� ��� ��������� ������ �������� (�������� PicView2)
				#if MVV_3>=2472
				return FALSE;
				#elif MVV_3>=2462
				return INVALID_HANDLE_VALUE;
				#else
				return FALSE;
				#endif
			}
		}
		free(gpsz_LastAnalyzeFile);
		gpsz_LastAnalyzeFile = NULL;
	}
	gpsz_LastAnalyzeFile = NULL;
	
	if (AnalyseW) // � Far2 ���� ����� �������, ��� ��� ���� ������� ���� - �����
	{
		Far2::AnalyseData fad2 = {sizeof(fad2), Info->FileName, (const unsigned char *)Info->Buffer, Info->BufferSize, OpMode_3_2(Info->OpMode)};
		#if MVV_3>=2462
		int lb2 = AnalyseW(&fad2);
		if (!lb2)
			#if MVV_3>=2472
			return FALSE; // ������ ���������
			#else
			return INVALID_HANDLE_VALUE; // ������ ���������
			#endif
		else
			return (HANDLE)TRUE; // � "OpenW" ������ ����� ����� "�������"
		#else
		return AnalyseW(&fad2);
		#endif
	}
	if (mp_Analyze)
	{
		free(mp_Analyze);
		mp_Analyze = NULL;
	}

	if (!OpenFilePluginW)
	{
		#if MVV_3>=2472
		return FALSE;
		#elif MVV_3>=2462
		return INVALID_HANDLE_VALUE;
		#else
		return FALSE;
		#endif
	}
	
	size_t nNewSize = sizeof(*mp_Analyze) + Info->BufferSize
			+ ((Info->FileName ? lstrlen(Info->FileName) : 0)+1)*sizeof(wchar_t);
	mp_Analyze = (AnalyseInfo*)malloc(nNewSize);
	if (!mp_Analyze)
		return FALSE;
	LPBYTE ptr = (LPBYTE)mp_Analyze;
	memmove(ptr, Info, sizeof(*mp_Analyze));
	ptr += sizeof(*mp_Analyze);
	if (Info->BufferSize && Info->Buffer)
	{
		memmove(ptr, Info->Buffer, Info->BufferSize);
		mp_Analyze->Buffer = ptr;
		ptr += Info->BufferSize;
	}
	else
	{
		mp_Analyze->Buffer = NULL;
	}
	if (Info->FileName)
	{
		lstrcpy((LPWSTR)ptr, Info->FileName);
		mp_Analyze->FileName = (LPCWSTR)ptr;
	}
	else
		mp_Analyze->FileName = NULL;
	
	HANDLE h = OpenFilePluginW(mp_Analyze->FileName,
		(const unsigned char*)mp_Analyze->Buffer,
		mp_Analyze->BufferSize,
		OpMode_3_2(Info->OpMode));
	
	if (h && h != INVALID_HANDLE_VALUE && ((INT_PTR)h) != -2)
	{
		#if MVV_3>=2462
		return h;
		#else
		if (m_AnalyzeMode == 1)
		{
			//TODO: �� ���������. ������ ������ - ����� ��� ����� ����� �������, ���� OpenW �� ����� ������?
		}
	
		//TODO: ������ �� �����������, � �� ��������� ���� ��� ���� ������, �� ���������
		//TODO: � ����� ������ ����� ������� Far2 ������, ���� �� OpenW ���� �� �����...
		if (ClosePluginW)
			ClosePluginW(h);
		return TRUE;
		#endif
	}
	if (mp_Analyze)
	{
		free(mp_Analyze);
		mp_Analyze = NULL;
	}
	if (((INT_PTR)h) == -2)
	{
		// ������ ������ "������ ����� �� ���������".
		// ��� � ��������, �� ����� ������, �.�. ���������� �� AnalyseW,
		// �� ��� "��������������" ������ �������� ������, � ��������� ����������
		mn_AnalyzeReturn = -2;
		_ASSERTE(gpsz_LastAnalyzeFile == NULL);
		if (Info->FileName && *Info->FileName)
		{
			wchar_t* pszNew = lstrdup(Info->FileName);
			gpsz_LastAnalyzeFile = pszNew;
			mpsz_LastAnalyzeFile = pszNew;
		}
		// ����� ���� ANSI �������, ������� ���� ���������� ������� ����?
		if (m_AnalyzeMode == 3)
		{
			MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
			#if MVV_3>2850
			mcr.SequenceText = L"if Area.Menu then Keys('Esc') end";
			#else
			mcr.SequenceText = L"$if (Menu) Esc $end";
			#endif
			mcr.Flags = KMFLAGS_DISABLEOUTPUT;
			psi3.MacroControl(MCTLARG(mguid_Plugin), MCTL_SENDSTRING, 0, &mcr);
		}
		#if MVV_3>=2472
		return PANEL_STOP;
		#elif MVV_3>=2462
		return (HANDLE)-2; // "������ ����� �� ���������"
		#else
		return TRUE;
		#endif
	}
	
	#if MVV_3>=2472
	return FALSE;
	#elif MVV_3>=2462
	return INVALID_HANDLE_VALUE;
	#else
	return 0;
	#endif
}
void    WrapPluginInfo::CloseAnalyseW3(const CloseAnalyseInfo *Info)
{
	LOG_CMD(L"CloseAnalyseW",0,0,0);
	if (ClosePluginW && Info && (Info->StructSize >= sizeof(*Info)))
	{
		#if MVV_3>=2462
		_ASSERTE((Info->Handle == NULL || Info->Handle == (HANDLE)TRUE) || (AnalyseW==NULL));
		// ���� � Far2 ������� ���� ������� AnalyseW, �� ����� ������ �� ����������!
		if ((Info->Handle == NULL || Info->Handle == (HANDLE)TRUE) && AnalyseW)
		{
			return;
		}
		else
		#endif
		{
			if (Info->Handle && (Info->Handle != INVALID_HANDLE_VALUE) && (((INT_PTR)Info->Handle) != -2))
				ClosePluginW(Info->Handle);
		}
	}
	if (mp_Analyze)
	{
		free(mp_Analyze);
		mp_Analyze = NULL;
	}
}
void   WrapPluginInfo::ClosePanelW3(const struct ClosePanelInfo *Info)
{
	LOG_CMD(L"ClosePanelW",0,0,0);
	if (ClosePluginW)
		ClosePluginW(Info->hPanel);
}
int    WrapPluginInfo::CompareW3(const CompareInfo *Info)
{
	LOG_CMD(L"CompareW",0,0,0);
	int iRc = -2;
	if (CompareW)
	{
		Far2::PluginPanelItem Item1 = {{0}};
		Far2::PluginPanelItem Item2 = {{0}};
		PluginPanelItem_3_2(Info->Item1, &Item1);
		PluginPanelItem_3_2(Info->Item2, &Item2);
		iRc = CompareW(Info->hPanel, &Item1, &Item2, SortMode_3_2(Info->Mode));
	}
	return iRc;
}
int    WrapPluginInfo::ConfigureW3(const struct ConfigureInfo *Info)
{
	LOG_CMD(L"ConfigureW",0,0,0);
	int iRc = 0;
	if (ConfigureW)
	{
		if (mn_PluginConfig > 0 && mguids_PluginConfig)
		{
			for (int i = 0; i < mn_PluginConfig; i++)
			{
				if (memcmp(Info->Guid, &GUID_NULL, sizeof(GUID)) == 0 ||
					memcmp(Info->Guid, mguids_PluginConfig+i, sizeof(GUID)) == 0)
				{
					iRc = ConfigureW(i);
					break;
				}
			}
		}
	}
	return iRc;
}
int    WrapPluginInfo::DeleteFilesW3(const DeleteFilesInfo *Info)
{
	LOG_CMD(L"DeleteFilesW",0,0,0);
	int iRc = 0;
	if (DeleteFilesW)
	{
		Far2::PluginPanelItem *p2 = PluginPanelItems_3_2(Info->PanelItem, Info->ItemsNumber);
		if (p2)
		{
			iRc = DeleteFilesW(Info->hPanel, p2, Info->ItemsNumber, OpMode_3_2(Info->OpMode));
			free(p2);
		}
	}
	return iRc;
}
void   WrapPluginInfo::ExitFARW3(const struct ExitInfo *Info)
{
	LOG_CMD(L"ExitFARW",0,0,0);
	if (ExitFARW)
	{
		if (GetGlobalInfoWPlugin != NULL)
		{
			// ����� �� ����� ExitFarW � ����� ������...
			_ASSERTE(GetGlobalInfoWPlugin == NULL);
			//((_ExitFARW3)ExitFARW)(NULL); -- Far3 ������ ����� ��� �������� ������, � �������� ��� ����� ������� "��������" - �� ������
		}
		else
		{
			ExitFARW();
		}
	}
	UnloadPlugin();
	//// ���� ����� � ���������� ������ (��� ������������) �����-�� �������� - ����� ����� ��������� ��� � �������
	//if (mn_OldAbsentFunctions != mn_NewAbsentFunctions && *ms_PluginDll)
	//{
	//	WrapUpdateFunctions upd = {mn_OldAbsentFunctions, mn_NewAbsentFunctions};
	//	if (GetModuleFileName(mh_Loader, upd.ms_Loader, ARRAYSIZE(upd.ms_Loader)))
	//	{
	//		lstrcpy(upd.ms_IniFile, ms_IniFile);
	//		UpdateFunc.push_back(upd);
	//	}
	//}
	delete this;
}
void   WrapPluginInfo::FreeVirtualFindDataW3(const FreeFindDataInfo *Info)
{
	LOG_CMD(L"FreeVirtualFindDataW",0,0,0);
	//TODO:
}
int    WrapPluginInfo::GetFilesW3(GetFilesInfo *Info)
{
	LOG_CMD(L"GetFilesW",0,0,0);
	int iRc = 0;
	if (GetFilesW)
	{
		Far2::PluginPanelItem *p2 = PluginPanelItems_3_2(Info->PanelItem, Info->ItemsNumber);
		if (p2)
		{
			iRc = GetFilesW(Info->hPanel, p2, Info->ItemsNumber, Info->Move, &Info->DestPath, OpMode_3_2(Info->OpMode));
			free(p2);
		}
	}
	return iRc;
}
int    WrapPluginInfo::GetFindDataW3(GetFindDataInfo *Info)
{
	LOG_CMD(L"GetFindDataW",0,0,0);
	//GetFindDataW(HANDLE hPlugin,Far2::PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
	if (!GetFindDataW)
		return 0;
	Far2::PluginPanelItem *pPanelItem = NULL;
	int ItemsNumber = 0;
	int iRc = GetFindDataW(Info->hPanel, &pPanelItem, &ItemsNumber, OpMode_3_2(Info->OpMode));
	if (iRc && ItemsNumber > 0)
	{
		Info->PanelItem = PluginPanelItems_2_3(pPanelItem, ItemsNumber);
		Info->ItemsNumber = ItemsNumber;
		m_MapPanelItems[Info->PanelItem] = pPanelItem;
		//PluginPanelItem* p3 = Info->PanelItem;
		//Far2::PluginPanelItem* p2 = pPanelItem;
		//for (int i = 0; i < ItemsNumber; i++, p2++, p3++)
		//{
		//	p3->FileAttributes = p2->FindData.dwFileAttributes;
		//	p3->CreationTime = p2->FindData.ftCreationTime;
		//	p3->LastAccessTime = p2->FindData.ftLastAccessTime;
		//	p3->LastWriteTime = p2->FindData.ftLastWriteTime;
		//	p3->ChangeTime = p2->FindData.ftLastWriteTime;
		//	p3->FileSize = p2->FindData.nFileSize;
		//	p3->AllocationSize = p2->FindData.nPackSize;
		//	p3->FileName = p2->FindData.lpwszFileName;
		//	p3->AlternateFileName = p2->FindData.lpwszAlternateFileName;
		//	p3->Flags = PluginPanelItemFlags_3_2(p2->Flags);
		//	p3->NumberOfLinks = p2->NumberOfLinks;
		//	p3->Description = p2->Description;
		//	p3->Owner = p2->Owner;
		//	p3->CustomColumnData = p2->CustomColumnData;
		//	p3->CustomColumnNumber = p2->CustomColumnNumber;
		//	p3->UserData = p2->UserData;
		//	p3->CRC32 = p2->CRC32;
		//}
	}
	return iRc;
}
void   WrapPluginInfo::FreeFindDataW3(const FreeFindDataInfo *Info)
{
	LOG_CMD(L"FreeFindDataW",0,0,0);
	if (Info->PanelItem)
	{
		if (FreeFindDataW)
		{
			Far2::PluginPanelItem *pPanelItem = m_MapPanelItems[Info->PanelItem];
			if (pPanelItem)
				FreeFindDataW(Info->hPanel, pPanelItem, Info->ItemsNumber);
		}
		free(Info->PanelItem);
		
		m_MapPanelItems.erase(Info->PanelItem);
		//std::map<PluginPanelItem*,Far2::PluginPanelItem*>::iterator iter;
		//for (iter = MapPanelItems.begin( ); iter!=MapPanelItems.end( ); iter++)
		//{
		//	if (iter->first == Info->PanelItem)
		//	{
		//		MapPanelItems.erase(iter);
		//		break;
		//	}
		//}
	}
}
void   WrapPluginInfo::GetOpenPanelInfoW3(OpenPanelInfo *Info)
{
	LOG_CMD(L"GetOpenPluginInfoW",0,0,0);
	if (GetOpenPluginInfoW)
	{
		Far2::OpenPluginInfo ofi = {sizeof(Far2::OpenPluginInfo)};
		GetOpenPluginInfoW(Info->hPanel, &ofi);
		
		//memset(Info, 0, sizeof(*Info));
		//Info->StructSize = sizeof(*Info);
		//HANDLE hPanel;
		Info->Flags = OpenPanelInfoFlags_2_3(ofi.Flags);
		Info->HostFile = ofi.HostFile;
		Info->CurDir = ofi.CurDir;
		Info->Format = ofi.Format;
		Info->PanelTitle = ofi.PanelTitle;
		Info->InfoLines = InfoLines_2_3(ofi.InfoLines, ofi.InfoLinesNumber);
		Info->InfoLinesNumber = m_InfoLinesNumber;
		Info->DescrFiles = ofi.DescrFiles;
		Info->DescrFilesNumber = ofi.DescrFilesNumber;
		Info->PanelModesArray = PanelModes_2_3(ofi.PanelModesArray, ofi.PanelModesNumber);
		Info->PanelModesNumber = m_PanelModesNumber;
		Info->StartPanelMode = ofi.StartPanelMode;
		Info->StartSortMode = SortMode_2_3(ofi.StartSortMode);
		Info->StartSortOrder = ofi.StartSortOrder;
		Info->KeyBar = KeyBarTitles_2_3(ofi.KeyBar);
		Info->ShortcutData = ofi.ShortcutData;
		Info->FreeSize = 0;
	}
}
#if MVV_3<=2798
int    WrapPluginInfo::GetVirtualFindDataW3(GetVirtualFindDataInfo *Info)
{
	LOG_CMD(L"GetVirtualFindDataW",0,0,0);
	//TODO:
	return 0;
}
#endif
int    WrapPluginInfo::MakeDirectoryW3(MakeDirectoryInfo *Info)
{
	LOG_CMD(L"MakeDirectoryW",0,0,0);
	int iRc = 0;
	if (MakeDirectoryW)
		iRc = MakeDirectoryW(Info->hPanel, &Info->Name, OpMode_3_2(Info->OpMode));
	return iRc;
}
int    WrapPluginInfo::ProcessDialogEventW3(const struct ProcessDialogEventInfo *Info)
{
	LOG_CMD0(L"ProcessDialogEventW",0,0,0);
	int lRc = 0;
	if (ProcessDialogEventW)
	{
		Far2::DIALOG_EVENTS Event2 = Far2::DE_DLGPROCINIT;
		switch (Info->Event)
		{
		case DE_DLGPROCINIT:
			Event2 = Far2::DE_DLGPROCINIT; break;
		case DE_DEFDLGPROCINIT:
			Event2 = Far2::DE_DEFDLGPROCINIT; break;
		case DE_DLGPROCEND:
			Event2 = Far2::DE_DLGPROCEND; break;
		default:
			return FALSE;
		}
		FarDialogEvent* p3 = (FarDialogEvent*)Info->Param;
		Far2::FarDialogEvent p2 = {p3->hDlg, 0, p3->Param1, (LONG_PTR)p3->Param2, p3->Result};
		Far2Dialog* pDlg = (*gpMapDlg_3_2)[p3->hDlg];
		//TODO: ����������� VBuf?
		p2.Msg = FarMessage_3_2(p3->Msg, p2.Param1, (void*&)p2.Param2, pDlg);
		if (p2.Msg != DM_FIRST)
		{
			lRc = ProcessDialogEventW(Event2, &p2);
			if (lRc)
				p3->Result = p2.Result;
		}
	}
	return lRc;
}
int    WrapPluginInfo::ProcessEditorEventW3(const struct ProcessEditorEventInfo *Info)
{
	LOG_CMD0(L"ProcessEditorEventW(%i)",Info->Event,0,0);
	int iRc = 0;
	if (Info->Event == EE_CHANGE)
	{
		mb_EditorChanged = true;
	}
	else if (ProcessEditorEventW && Info->Event >= EE_READ && Info->Event <= EE_KILLFOCUS)
	{
		void* Param = Info->Param;
		if (Info->Event == EE_CLOSE || Info->Event == EE_KILLFOCUS || Info->Event == EE_GOTFOCUS)
		{
			Param = (void*)&Info->EditorID;
		}
		else if (Info->Event == EE_REDRAW)
		{
			Param = mb_EditorChanged ? EEREDRAW_CHANGE : EEREDRAW_ALL;
			mb_EditorChanged = false;
		}
		iRc = ProcessEditorEventW(Info->Event, Param);
	}
	return iRc;
}
int    WrapPluginInfo::ProcessEditorInputW3(const ProcessEditorInputInfo *Info)
{
	LOG_CMD(L"ProcessEditorInputW",0,0,0);
	int iRc = 0;
	_ASSERTE(Info->StructSize==sizeof(*Info));
	if (ProcessEditorInputW)
		iRc = ProcessEditorInputW(&Info->Rec);
	return iRc;
}
int    WrapPluginInfo::ProcessPanelEventW3(const struct ProcessPanelEventInfo *Info)
{
	LOG_CMD0(L"ProcessEventW(%i)",Info->Event,0,0);
	int iRc = 0;
	if (ProcessEventW)
		iRc = ProcessEventW(Info->hPanel, Info->Event, Info->Param);
	return iRc;
}
int    WrapPluginInfo::ProcessHostFileW3(const ProcessHostFileInfo *Info)
{
	LOG_CMD(L"ProcessHostFileW",0,0,0);
	int iRc = 0;
	if (ProcessHostFileW)
	{
		Far2::PluginPanelItem *p2 = PluginPanelItems_3_2(Info->PanelItem, Info->ItemsNumber);
		iRc = ProcessHostFileW(Info->hPanel, p2, Info->ItemsNumber, OpMode_3_2(Info->OpMode));
		if (p2)
			free(p2);
	}
	return iRc;
}
int    WrapPluginInfo::ProcessPanelInputW3(const struct ProcessPanelInputInfo *Info)
{
	LOG_CMD(L"ProcessPanelInputW(%s,0x%X,0x%X)",(Info->Rec.EventType==KEY_EVENT?L"Key":L"???"),Info->Rec.Event.KeyEvent.wVirtualKeyCode,Info->Rec.Event.KeyEvent.dwControlKeyState);
	_ASSERTE(Info->StructSize == sizeof(*Info));
	int iRc = 0;
	if (ProcessKeyW)
	{
		int Key3 = FarKey_3_2(&Info->Rec);
		int VirtKey = 0, ControlState = 0;
		// ProcessKeyW ������� VK � �� FarKey
		TranslateKeyToVK(Key3, VirtKey, ControlState, NULL);
		
		//DWORD FShift = Key3 & 0x7F000000; // ������� ��� ������������ � ������ �����!
		//DWORD ControlState =
		//#if MVV_3>=2103
		//	(FShift & Far2::KEY_SHIFT ? Far2::PKF_SHIFT : 0)|
		//	(FShift & Far2::KEY_ALT ? Far2::PKF_ALT : 0)|
		//	(FShift & Far2::KEY_CTRL ? Far2::PKF_CONTROL : 0)
		//#else
		//	(FShift & KEY_SHIFT ? Far2::PKF_SHIFT : 0)|
		//	(FShift & KEY_ALT ? Far2::PKF_ALT : 0)|
		//	(FShift & KEY_CTRL ? Far2::PKF_CONTROL : 0)
		//#endif
		//	;
		////DWORD PreProcess = (Info->Flags & PKIF_PREPROCESS) ? Far2::PKF_PREPROCESS : 0;
		//int Key2;
		//if ((Key3 & 0x00030000) == 0x00010000)
		//	Key2 = Key3 & 0x0000FFFF; // KEY_BREAK .. KEY_F24, KEY_BROWSER_BACK, KEY_MEDIA_NEXT_TRACK � �.�. 
		//else
		//	Key2 = Key3 & 0x0003FFFF;
		
		// ProcessKeyW ������� VK � �� FarKey
		//iRc = ProcessKeyW(Info->hPanel, Key2 /*| PreProcess*/, ControlState);
		
		//TODO: ������������, ��������� ������� ����� ������ Far2::PKF_PREPROCESS, �� ���� ��� ��
		iRc = ProcessKeyW(Info->hPanel, VirtKey, ControlState);
	}
	return iRc;
}
int    WrapPluginInfo::ProcessConsoleInputW3(ProcessConsoleInputInfo *Info)
{
	//TODO: � ����� ����� �� ������ � Far2 �� ��� ������ ���� PKF_PREPROCESS
	return 0;
}
int    WrapPluginInfo::ProcessSynchroEventW3(const struct ProcessSynchroEventInfo *Info)
{
	LOG_CMD(L"ProcessSynchroEventW",0,0,0);
	int iRc = 0;
	if (ProcessSynchroEventW)
		iRc = ProcessSynchroEventW(Info->Event, Info->Param);
	return iRc;
}
int    WrapPluginInfo::ProcessViewerEventW3(const struct ProcessViewerEventInfo *Info)
{
	LOG_CMD(L"ProcessViewerEventW",0,0,0);
	int iRc = 0;
	if (ProcessViewerEventW)
		iRc = ProcessViewerEventW(Info->Event, Info->Param);
	return iRc;
}
int    WrapPluginInfo::PutFilesW3(const PutFilesInfo *Info)
{
	LOG_CMD(L"PutFilesW",0,0,0);
	int iRc = 0;
	if (PutFilesW)
	{
		Far2::PluginPanelItem *p2 = PluginPanelItems_3_2(Info->PanelItem, Info->ItemsNumber);
		if (p2)
		{
			if (!m_OldPutFilesParams)
				iRc = PutFilesW(Info->hPanel, p2, Info->ItemsNumber, Info->Move, Info->SrcPath, OpMode_3_2(Info->OpMode));
			else
			{
				wchar_t szOldPath[MAX_PATH]; szOldPath[0] = 0;
				if (Info->SrcPath)
				{
					GetCurrentDirectory(ARRAYSIZE(szOldPath), szOldPath);
					// ��� ����� ����� �� "�������" ��� "������������" �����
					SetCurrentDirectory(Info->SrcPath);
				}
				iRc = ((WrapPluginInfo::_PutFilesOldW)PutFilesW)(Info->hPanel, p2, Info->ItemsNumber, Info->Move, OpMode_3_2(Info->OpMode));
				if (szOldPath[0])
					SetCurrentDirectory(szOldPath);
			}
			free(p2);
		}
	}
	return iRc;
}
int    WrapPluginInfo::SetDirectoryW3(const SetDirectoryInfo *Info)
{
	int iRc = 0;
	LOG_CMD(L"SetDirectoryW",0,0,0);
	if (SetDirectoryW)
		iRc = SetDirectoryW(Info->hPanel, Info->Dir, OpMode_3_2(Info->OpMode));
	return iRc;
}
int    WrapPluginInfo::SetFindListW3(const SetFindListInfo *Info)
{
	LOG_CMD(L"SetFindListW(count=%u)",(DWORD)Info->ItemsNumber,0,0);
	int iRc = 0;
	if (SetFindListW)
	{
		Far2::PluginPanelItem *p2 = PluginPanelItems_3_2(Info->PanelItem, Info->ItemsNumber);
		if (p2)
		{
			iRc = SetFindListW(Info->hPanel, p2, Info->ItemsNumber);
			free(p2);
		}
	}
	return iRc;
}
int    WrapPluginInfo::GetCustomDataW3(const wchar_t *FilePath, wchar_t **CustomData)
{
	int iRc = 0;
	if (GetCustomDataW)
		iRc = GetCustomDataW(FilePath, CustomData);
	return iRc;
}
void   WrapPluginInfo::FreeCustomDataW3(wchar_t *CustomData)
{
	if (FreeCustomDataW)
		FreeCustomDataW(CustomData);
}


/* *** */
FARPROC WrapPluginInfo::GetProcAddressWrap(struct WrapPluginInfo* wpi, HMODULE hModule, LPCSTR lpProcName)
{
	return wpi->GetProcAddressW3(hModule, lpProcName);
}
FARPROC WrapPluginInfo::GetProcAddressW3(HMODULE hModule, LPCSTR lpProcName)
{
	if (!lstrcmpA(lpProcName, "AnalyseW"))
		if (!(mn_PluginFunctions & LF3_Analyse))
			return NULL;
	if (!lstrcmpA(lpProcName, "GetCustomDataW") || !lstrcmpA(lpProcName, "FreeCustomDataW"))
		if (!(mn_PluginFunctions & LF3_CustomData))
			return NULL;
	if (!lstrcmpA(lpProcName, "ProcessDialogEventW"))
		if (!(mn_PluginFunctions & LF3_Dialog))
			return NULL;
	if (!lstrcmpA(lpProcName, "ProcessEditorEventW") || !lstrcmpA(lpProcName, "ProcessEditorInputW"))
		if (!(mn_PluginFunctions & LF3_Editor))
			return NULL;
	if (!lstrcmpA(lpProcName, "SetFindListW"))
		if (!(mn_PluginFunctions & LF3_FindList))
			return NULL;
	if (!lstrcmpA(lpProcName, "ProcessViewerEventW"))
		if (!(mn_PluginFunctions & LF3_Viewer))
			return NULL;
	if (!lstrcmpA(lpProcName, "GetFilesW"))
		if (!(mn_PluginFunctions & LF3_GetFiles))
			return NULL;
	if (!lstrcmpA(lpProcName, "PutFilesW"))
		if (!(mn_PluginFunctions & LF3_PutFiles))
			return NULL;
	if (!lstrcmpA(lpProcName, "DeleteFilesW"))
		if (!(mn_PluginFunctions & LF3_DeleteFiles))
			return NULL;
	
	// OK, �����
	return GetProcAddress(hModule, lpProcName);
}
/* *** */
FARPROC WrapPluginInfo::GetOldProcAddressWrap(struct WrapPluginInfo* wpi, HMODULE hModule, LPCSTR lpProcName)
{
	return wpi->GetOldProcAddressW3(hModule, lpProcName);
}
FARPROC WrapPluginInfo::GetOldProcAddressW3(HMODULE hModule, LPCSTR lpProcName)
{
	if (hModule != mh_Loader)
		return NULL;
	return GetProcAddress(mh_Dll, lpProcName);
}
/* *** */
void WrapPluginInfo::SetStartupInfoWrap(struct WrapPluginInfo* wpi, PluginStartupInfo *Info)
{
	wpi->SetStartupInfoW3(Info);
}
void WrapPluginInfo::GetGlobalInfoWrap(struct WrapPluginInfo* wpi, GlobalInfo *Info)
{
	wpi->GetGlobalInfoW3(Info);
}
void WrapPluginInfo::GetPluginInfoWrap(struct WrapPluginInfo* wpi, PluginInfo *Info)
{
	wpi->GetPluginInfoW3(Info);
}
HANDLE WrapPluginInfo::OpenWrap(struct WrapPluginInfo* wpi, const OpenInfo *Info)
{
	return wpi->OpenW3(Info);
}
HANDLE    WrapPluginInfo::AnalyseWrap(struct WrapPluginInfo* wpi, const AnalyseInfo *Info)
{
	return wpi->AnalyseW3(Info);
}
void    WrapPluginInfo::CloseAnalyseWrap(struct WrapPluginInfo* wpi, const CloseAnalyseInfo *Info)
{
	wpi->CloseAnalyseW3(Info);
}
void   WrapPluginInfo::ClosePanelWrap(struct WrapPluginInfo* wpi, const struct ClosePanelInfo *Info)
{
	return wpi->ClosePanelW3(Info);
}
int    WrapPluginInfo::CompareWrap(struct WrapPluginInfo* wpi, const CompareInfo *Info)
{
	return wpi->CompareW3(Info);
}
int    WrapPluginInfo::ConfigureWrap(struct WrapPluginInfo* wpi, const struct ConfigureInfo *Info)
{
	return wpi->ConfigureW3(Info);
}
int    WrapPluginInfo::DeleteFilesWrap(struct WrapPluginInfo* wpi, const DeleteFilesInfo *Info)
{
	return wpi->DeleteFilesW3(Info);
}
void   WrapPluginInfo::ExitFARWrap(struct WrapPluginInfo* wpi, const struct ExitInfo *Info)
{
	return wpi->ExitFARW3(Info);
}
void   WrapPluginInfo::FreeVirtualFindDataWrap(struct WrapPluginInfo* wpi, const FreeFindDataInfo *Info)
{
	return wpi->FreeVirtualFindDataW3(Info);
}
int    WrapPluginInfo::GetFilesWrap(struct WrapPluginInfo* wpi, GetFilesInfo *Info)
{
	return wpi->GetFilesW3(Info);
}
int    WrapPluginInfo::GetFindDataWrap(struct WrapPluginInfo* wpi, GetFindDataInfo *Info)
{
	return wpi->GetFindDataW3(Info);
}
void   WrapPluginInfo::FreeFindDataWrap(struct WrapPluginInfo* wpi, const FreeFindDataInfo *Info)
{
	return wpi->FreeFindDataW3(Info);
}
void   WrapPluginInfo::GetOpenPanelInfoWrap(struct WrapPluginInfo* wpi, OpenPanelInfo *Info)
{
	return wpi->GetOpenPanelInfoW3(Info);
}
#if MVV_3<=2798
int    WrapPluginInfo::GetVirtualFindDataWrap(struct WrapPluginInfo* wpi, GetVirtualFindDataInfo *Info)
{
	return wpi->GetVirtualFindDataW3(Info);
}
#endif
int    WrapPluginInfo::MakeDirectoryWrap(struct WrapPluginInfo* wpi, MakeDirectoryInfo *Info)
{
	return wpi->MakeDirectoryW3(Info);
}
int    WrapPluginInfo::ProcessDialogEventWrap(struct WrapPluginInfo* wpi, const struct ProcessDialogEventInfo *Info)
{
	return wpi->ProcessDialogEventW3(Info);
}
int    WrapPluginInfo::ProcessEditorEventWrap(struct WrapPluginInfo* wpi, const struct ProcessEditorEventInfo *Info)
{
	return wpi->ProcessEditorEventW3(Info);
}
int    WrapPluginInfo::ProcessEditorInputWrap(struct WrapPluginInfo* wpi, const ProcessEditorInputInfo *Info)
{
	return wpi->ProcessEditorInputW3(Info);
}
int    WrapPluginInfo::ProcessPanelEventWrap(struct WrapPluginInfo* wpi, const struct ProcessPanelEventInfo *Info)
{
	return wpi->ProcessPanelEventW3(Info);
}
int    WrapPluginInfo::ProcessHostFileWrap(struct WrapPluginInfo* wpi, const ProcessHostFileInfo *Info)
{
	return wpi->ProcessHostFileW3(Info);
}
int    WrapPluginInfo::ProcessPanelInputWrap(struct WrapPluginInfo* wpi, const struct ProcessPanelInputInfo *Info)
{
	return wpi->ProcessPanelInputW3(Info);
}
int    WrapPluginInfo::ProcessConsoleInputWrap(struct WrapPluginInfo* wpi, ProcessConsoleInputInfo *Info)
{
	return wpi->ProcessConsoleInputW3(Info);
}
int    WrapPluginInfo::ProcessSynchroEventWrap(struct WrapPluginInfo* wpi, const struct ProcessSynchroEventInfo *Info)
{
	return wpi->ProcessSynchroEventW3(Info);
}
int    WrapPluginInfo::ProcessViewerEventWrap(struct WrapPluginInfo* wpi, const struct ProcessViewerEventInfo *Info)
{
	return wpi->ProcessViewerEventW3(Info);
}
int    WrapPluginInfo::PutFilesWrap(struct WrapPluginInfo* wpi, const PutFilesInfo *Info)
{
	return wpi->PutFilesW3(Info);
}
int    WrapPluginInfo::SetDirectoryWrap(struct WrapPluginInfo* wpi, const SetDirectoryInfo *Info)
{
	return wpi->SetDirectoryW3(Info);
}
int    WrapPluginInfo::SetFindListWrap(struct WrapPluginInfo* wpi, const SetFindListInfo *Info)
{
	return wpi->SetFindListW3(Info);
}
int    WrapPluginInfo::GetCustomDataWrap(struct WrapPluginInfo* wpi, const wchar_t *FilePath, wchar_t **CustomData)
{
	return wpi->GetCustomDataW3(FilePath, CustomData);
}
void   WrapPluginInfo::FreeCustomDataWrap(struct WrapPluginInfo* wpi, wchar_t *CustomData)
{
	return wpi->FreeCustomDataW3(CustomData);
}
/* *** */
/* *** */
/* *** */
LONG_PTR WrapPluginInfo::FarApiDefDlgProcWrap(WrapPluginInfo* wpi, HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	return wpi->FarApiDefDlgProc(hDlg, Msg, Param1, Param2);
}
LONG_PTR WrapPluginInfo::FarApiSendDlgMessageWrap(WrapPluginInfo* wpi, HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	return wpi->FarApiSendDlgMessage(hDlg, Msg, Param1, Param2);
}
BOOL WrapPluginInfo::FarApiShowHelpWrap(WrapPluginInfo* wpi, const wchar_t *ModuleName, const wchar_t *Topic, DWORD Flags)
{
	return wpi->FarApiShowHelp(ModuleName, Topic, Flags);
}
HANDLE WrapPluginInfo::FarApiSaveScreenWrap(WrapPluginInfo* wpi, int X1, int Y1, int X2, int Y2)
{
	return wpi->FarApiSaveScreen(X1, Y1, X2, Y2);
}
void WrapPluginInfo::FarApiRestoreScreenWrap(WrapPluginInfo* wpi, HANDLE hScreen)
{
	wpi->FarApiRestoreScreen(hScreen);
}
void WrapPluginInfo::FarApiTextWrap(WrapPluginInfo* wpi, int X, int Y, int Color, const wchar_t *Str)
{
	wpi->FarApiText(X, Y, Color, Str);
}
int WrapPluginInfo::FarApiEditorWrap(WrapPluginInfo* wpi, const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, int StartLine, int StartChar, UINT CodePage)
{
	return wpi->FarApiEditor(FileName, Title, X1, Y1, X2, Y2, Flags, StartLine, StartChar, CodePage);
}
int WrapPluginInfo::FarApiViewerWrap(WrapPluginInfo* wpi, const wchar_t *FileName, const wchar_t *Title, int X1, int Y1, int X2, int Y2, DWORD Flags, UINT CodePage)
{
	return wpi->FarApiViewer(FileName, Title, X1, Y1, X2, Y2, Flags, CodePage);
}
int WrapPluginInfo::FarApiMenuWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, int X, int Y, int MaxHeight, DWORD Flags, const wchar_t* Title, const wchar_t* Bottom, const wchar_t* HelpTopic, const int* BreakKeys, int* BreakCode, const struct Far2::FarMenuItem *Item, int ItemsNumber)
{
	return wpi->FarApiMenu(PluginNumber, X, Y, MaxHeight, Flags, Title, Bottom, HelpTopic, BreakKeys, BreakCode, Item, ItemsNumber);
}
int WrapPluginInfo::FarApiMessageWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, DWORD Flags, const wchar_t *HelpTopic, const wchar_t * const *Items, int ItemsNumber, int ButtonsNumber)
{
	return wpi->FarApiMessage(PluginNumber, Flags, HelpTopic, Items, ItemsNumber, ButtonsNumber);
}
HANDLE WrapPluginInfo::FarApiDialogInitWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, int X1, int Y1, int X2, int Y2, const wchar_t* HelpTopic, struct Far2::FarDialogItem *Item, unsigned int ItemsNumber, DWORD Reserved, DWORD Flags, Far2::FARWINDOWPROC DlgProc, LONG_PTR Param)
{
	return wpi->FarApiDialogInit(PluginNumber, X1, Y1, X2, Y2, HelpTopic, Item, ItemsNumber, Reserved, Flags, DlgProc, Param);
}
int WrapPluginInfo::FarApiDialogRunWrap(WrapPluginInfo* wpi, HANDLE hDlg)
{
	return wpi->FarApiDialogRun(hDlg);
}
void WrapPluginInfo::FarApiDialogFreeWrap(WrapPluginInfo* wpi, HANDLE hDlg)
{
	wpi->FarApiDialogFree(hDlg);
}
int WrapPluginInfo::FarApiControlWrap(WrapPluginInfo* wpi, HANDLE hPlugin, int Command, int Param1, LONG_PTR Param2)
{
	return wpi->FarApiControl(hPlugin, Command, Param1, Param2);
}
int WrapPluginInfo::FarApiGetDirListWrap(WrapPluginInfo* wpi, const wchar_t *Dir, struct Far2::FAR_FIND_DATA **pPanelItem, int *pItemsNumber)
{
	return wpi->FarApiGetDirList(Dir, pPanelItem, pItemsNumber);
}
void WrapPluginInfo::FarApiFreeDirListWrap(WrapPluginInfo* wpi, struct Far2::FAR_FIND_DATA *PanelItem, int nItemsNumber)
{
	wpi->FarApiFreeDirList(PanelItem, nItemsNumber);
}
int WrapPluginInfo::FarApiGetPluginDirListWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, HANDLE hPlugin, const wchar_t *Dir, struct Far2::PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	return wpi->FarApiGetPluginDirList(PluginNumber, hPlugin, Dir, pPanelItem, pItemsNumber);
}
void WrapPluginInfo::FarApiFreePluginDirListWrap(WrapPluginInfo* wpi, struct Far2::PluginPanelItem *PanelItem, int nItemsNumber)
{
	wpi->FarApiFreePluginDirList(PanelItem, nItemsNumber);
}
int WrapPluginInfo::FarApiCmpNameWrap(WrapPluginInfo* wpi, const wchar_t *Pattern, const wchar_t *String, int SkipPath)
{
	return wpi->FarApiCmpName(Pattern, String, SkipPath);
}
LPCWSTR WrapPluginInfo::FarApiGetMsgWrap(WrapPluginInfo* wpi, INT_PTR PluginNumber, int MsgId)
{
	return wpi->FarApiGetMsg(PluginNumber, MsgId);
}
INT_PTR WrapPluginInfo::FarApiAdvControlWrap(WrapPluginInfo* wpi, INT_PTR ModuleNumber, int Command, void *Param)
{
	return wpi->FarApiAdvControl(ModuleNumber, Command, Param);
}
int WrapPluginInfo::FarApiViewerControlWrap(WrapPluginInfo* wpi, int Command, void *Param)
{
	return wpi->FarApiViewerControl(Command, Param);
}
int WrapPluginInfo::FarApiEditorControlWrap(WrapPluginInfo* wpi, int Command, void *Param)
{
	return wpi->FarApiEditorControl(Command, Param);
}
int WrapPluginInfo::FarApiInputBoxWrap(WrapPluginInfo* wpi, const wchar_t *Title, const wchar_t *SubTitle, const wchar_t *HistoryName, const wchar_t *SrcText, wchar_t *DestText, int   DestLength, const wchar_t *HelpTopic, DWORD Flags)
{
	return wpi->FarApiInputBox(Title, SubTitle, HistoryName, SrcText, DestText, DestLength, HelpTopic, Flags);
}
int WrapPluginInfo::FarApiPluginsControlWrap(WrapPluginInfo* wpi, HANDLE hHandle, int Command, int Param1, LONG_PTR Param2)
{
	return wpi->FarApiPluginsControl(hHandle, Command, Param1, Param2);
}
int WrapPluginInfo::FarApiFileFilterControlWrap(WrapPluginInfo* wpi, HANDLE hHandle, int Command, int Param1, LONG_PTR Param2)
{
	return wpi->FarApiFileFilterControl(hHandle, Command, Param1, Param2);
}
int WrapPluginInfo::FarApiRegExpControlWrap(WrapPluginInfo* wpi, HANDLE hHandle, int Command, LONG_PTR Param)
{
	return wpi->FarApiRegExpControl(hHandle, Command, Param);
}
int WrapPluginInfo::FarStdGetFileOwnerWrap(WrapPluginInfo* wpi, const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,int Size)
{
	return wpi->FarStdGetFileOwner(Computer, Name, Owner, Size);
}
int WrapPluginInfo::FarStdGetPathRootWrap(WrapPluginInfo* wpi, const wchar_t *Path,wchar_t *Root, int DestSize)
{
	return wpi->FarStdGetPathRoot(Path, Root, DestSize);
}
int WrapPluginInfo::FarStdCopyToClipboardWrap(WrapPluginInfo* wpi, const wchar_t *Data)
{
	return wpi->FarStdCopyToClipboard(Data);
}
wchar_t* WrapPluginInfo::FarStdPasteFromClipboardWrap(WrapPluginInfo* wpi)
{
	return wpi->FarStdPasteFromClipboard();
}
#if MVV_3>2798
void WrapPluginInfo::DeleteBufferWrap(void *Buffer)
{
	if (Buffer) free(Buffer);
}
#endif
wchar_t* WrapPluginInfo::FarStdXlatWrap(WrapPluginInfo* wpi, wchar_t *Line,int StartPos,int EndPos,DWORD Flags)
{
	return wpi->FarStdXlatW3(Line, StartPos, EndPos, Flags);
}
int WrapPluginInfo::GetNumberOfLinksWrap(WrapPluginInfo* wpi, const wchar_t *Name)
{
	return wpi->GetNumberOfLinks(Name);
}
void WrapPluginInfo::FarStdRecursiveSearchWrap(WrapPluginInfo* wpi, const wchar_t *InitDir,const wchar_t *Mask,Far2::FRSUSERFUNC Func,DWORD Flags,void *Param)
{
	wpi->FarStdRecursiveSearch(InitDir, Mask, Func, Flags, Param);
}
int WrapPluginInfo::FarStdMkTempWrap(WrapPluginInfo* wpi, wchar_t *Dest, DWORD size, const wchar_t *Prefix)
{
	return wpi->FarStdMkTemp(Dest, size, Prefix);
}
int WrapPluginInfo::FarStdProcessNameWrap(WrapPluginInfo* wpi, const wchar_t *param1, wchar_t *param2, DWORD size, DWORD flags)
{
	return wpi->FarStdProcessName(param1, param2, size, flags);
}
int WrapPluginInfo::FarStdMkLinkWrap(WrapPluginInfo* wpi, const wchar_t *Src,const wchar_t *Dest,DWORD Flags)
{
	return wpi->FarStdMkLink(Src, Dest, Flags);
}
int WrapPluginInfo::FarConvertPathWrap(WrapPluginInfo* wpi, enum Far2::CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize)
{
	return wpi->FarConvertPath(Mode, Src, Dest, DestSize);
}
int WrapPluginInfo::FarGetReparsePointInfoWrap(WrapPluginInfo* wpi, const wchar_t *Src, wchar_t *Dest,int DestSize)
{
	return wpi->FarGetReparsePointInfo(Src, Dest, DestSize);
}
DWORD WrapPluginInfo::FarGetCurrentDirectoryWrap(WrapPluginInfo* wpi, DWORD Size,wchar_t* Buffer)
{
	return wpi->FarGetCurrentDirectory(Size, Buffer);
}
void WrapPluginInfo::FarStdQSortWrap(WrapPluginInfo* wpi, void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *))
{
	wpi->FarStdQSort(base, nelem, width, fcmp);
}
void WrapPluginInfo::FarStdQSortExWrap(WrapPluginInfo* wpi, void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam)
{
	wpi->FarStdQSortEx(base, nelem, width, fcmp, userparam);
}
void* WrapPluginInfo::FarStdBSearchWrap(WrapPluginInfo* wpi, const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *))
{
	return wpi->FarStdBSearch(key, base, nelem, width, fcmp);
}
/* *** */





Far2Dialog::Far2Dialog(WrapPluginInfo* pwpi,
	int X1, int Y1, int X2, int Y2,
    const wchar_t *HelpTopic, Far2::FarDialogItem *Items, UINT ItemsNumber,
    DWORD Flags, Far2::FARWINDOWPROC DlgProc, LONG_PTR Param,
    GUID PluginGuid, GUID DefGuid)
{
	WrapMagic = 'F2WR';

	wpi = pwpi;
	hDlg3 = NULL; m_Items3 = NULL; mp_ListInit3 = NULL;
	m_PluginGuid = PluginGuid;
	
    m_X1 = X1; m_Y1 = Y1; m_X2 = X2; m_Y2 = Y2;
    m_HelpTopic = HelpTopic ? lstrdup(HelpTopic) : NULL;
    
    m_ItemsNumber = ItemsNumber;
    m_Items2 = NULL;
    m_Colors2 = NULL;
	mpp_FarCharInfo2 = NULL;
	mp_FarCharInfoDummy2.p2 = NULL;
    mpp_FarCharInfo3 = NULL;
	mp_FarCharInfoDummy3.p = NULL;
    
    if (ItemsNumber > 0 && Items)
    {
		for (UINT i = 0; i < ItemsNumber; i++)
		{
			if (Items[i].MaxLen)
			{
				_ASSERTE(Items[i].MaxLen<0xFFFFFF);
				if (Items[i].Type != DI_EDIT && Items[i].Type != DI_FIXEDIT && Items[i].Type != DI_PSWEDIT && Items[i].Type != DI_COMBOBOX)
				{
					Items[i].MaxLen = 0;
				}
			}
		}
    	//m_Items2 = (Far2::FarDialogItem*)calloc(ItemsNumber, sizeof(*m_Items2));
    	//memmove(m_Items2, Items, ItemsNumber*sizeof(*m_Items2));
		m_Items2 = Items;
    	m_Colors2 = (DWORD*)calloc(ItemsNumber, sizeof(*m_Colors2));
		mpp_FarCharInfo2 = (WRAP_CHAR_INFO*)calloc(ItemsNumber,sizeof(*mpp_FarCharInfo2));
    	mpp_FarCharInfo3 = (WRAP_FAR_CHAR_INFO*)calloc(ItemsNumber,sizeof(*mpp_FarCharInfo3));
    	for (UINT i = 0; i < ItemsNumber; i++)
		{
    		m_Colors2[i] = Items[i].Flags & (Far2::DIF_COLORMASK|Far2::DIF_SETCOLOR);
			if (Items[i].Type == DI_USERCONTROL)
			{
				if (Items[i].Param.VBuf != NULL)
				{
					mpp_FarCharInfo2[i].nCount = VBufDim(Items+i);
					mpp_FarCharInfo2[i].isExt = true;
					mpp_FarCharInfo2[i].p2 = Items[i].Param.VBuf;
				}
			}
			//mpp_FarCharInfo2[i] = (Items[i].Type == DI_USERCONTROL) ? Items[i].VBuf : NULL;
		}
    }
    m_Flags = Flags;
    m_DlgProc = DlgProc;
    m_Param = Param;
    // GUID
    m_GuidChecked = FALSE;
    memset(&m_Guid, 0, sizeof(m_Guid));
    m_DefGuid = DefGuid;
};

Far2Dialog::~Far2Dialog()
{
	// Far3 call
	FreeDlg();
	// Release memory
	if (m_HelpTopic)
	{
		free(m_HelpTopic);
		m_HelpTopic = NULL;
	}
	//if (m_Items2) -- ��� ������, �� ���� ������!
	//{
	//	free(m_Items2);
	//	m_Items2 = NULL;
	//}
	if (m_Colors2)
	{
		free(m_Colors2);
		m_Colors2 = NULL;
	}
	if (mpp_FarCharInfo2)
	{
		for (UINT i = 0; i < m_ItemsNumber; i++)
		{
			if (!mpp_FarCharInfo2[i].isExt && mpp_FarCharInfo2[i].p2)
				free(mpp_FarCharInfo2[i].p2);
		}
		free(mpp_FarCharInfo2);
		mpp_FarCharInfo2 = NULL;
	}
	if (mp_FarCharInfoDummy2.p2)
	{
		free(mp_FarCharInfoDummy2.p2);
		mp_FarCharInfoDummy2.p2 = NULL;
	}
	if (mpp_FarCharInfo3)
	{
		for (UINT i = 0; i < m_ItemsNumber; i++)
		{
			if (mpp_FarCharInfo3[i].p)
				free(mpp_FarCharInfo3[i].p);
		}
		free(mpp_FarCharInfo3);
		mpp_FarCharInfo3 = NULL;
	}
	if (mp_FarCharInfoDummy3.p)
	{
		free(mp_FarCharInfoDummy3.p);
		mp_FarCharInfoDummy3.p = NULL;
	}
	if (mp_ListInit3)
	{
		for (UINT i = 0; i < m_ItemsNumber; i++)
		{
			if (mp_ListInit3[i].Items)
				free(mp_ListInit3[i].Items);
		}
		free(mp_ListInit3);
	}
	if (m_Items3)
	{
		free(m_Items3);
		m_Items3 = NULL;
	}
};

WRAP_CHAR_INFO& Far2Dialog::GetVBufPtr2(UINT DlgItem)
{
	if (mpp_FarCharInfo2 && (DlgItem >= 0) && (DlgItem < this->m_ItemsNumber))
		return mpp_FarCharInfo2[DlgItem];
	return mp_FarCharInfoDummy2;
}

WRAP_FAR_CHAR_INFO& Far2Dialog::GetVBufPtr3(UINT DlgItem)
{
	if (mpp_FarCharInfo3 && (DlgItem >= 0) && (DlgItem < this->m_ItemsNumber))
		return mpp_FarCharInfo3[DlgItem];
	return mp_FarCharInfoDummy3;
}

void Far2Dialog::FreeDlg()
{
	(*gpMapDlg_2_3).erase(this);
	if (hDlg3 != NULL)
	{
		wpi->psi3.DialogFree(hDlg3);
		(*gpMapDlg_3_2).erase(hDlg3);
		hDlg3 = NULL;
	}
}

INT_PTR Far2Dialog::Far3DlgProc(HANDLE hDlg, int Msg, int Param1, void* Param2)
{
#ifdef _DEBUG
	FARMESSAGE Msg3 = (FARMESSAGE)Msg;
#endif

	Far2Dialog* p = (*gpMapDlg_3_2)[hDlg];
	INT_PTR lRc = 0;
	
	if (!p)
	{
		_ASSERTE(p!=NULL);
		return 0;
	}
	
	void* OrgParam2 = Param2;
	bool lbNeedColor = (Msg == DN_CTLCOLORDLGITEM /* || ...*/)
		&& (Param1 >= 0) && (Param1 < (int)p->m_ItemsNumber)
		&& (p->m_Colors2[Param1] & Far2::DIF_SETCOLOR);
	
	if (p->m_DlgProc)
	{
		Far2::FarMessagesProc Msg2 = p->wpi->FarMessage_3_2(Msg, Param1, Param2, p);
		_ASSERTE(Msg2!=Far2::DM_FIRST);
		if (Msg > DM_FIRST && Msg < DM_USER && Msg2 != Far2::DM_FIRST)
		{
			if (OrgParam2 && Param2)
			{
				p->wpi->gnMsg_3 = (FARMESSAGE)Msg;
				p->wpi->gnParam1_2 = p->wpi->gnParam1_3 = Param1;
				p->wpi->gpParam2_3 = OrgParam2;
				p->wpi->gnMsg_2 = Msg2;
				p->wpi->gnParam2_2 = (LONG_PTR)Param2;
			}
		}
		else
		{
			p->wpi->gnMsg_3 = DM_FIRST;
		}
		if (Msg == DM_KEY || Msg == DN_CONTROLINPUT)
			p->wpi->gnMsgKey_3 = (FARMESSAGE)Msg;
		if (Msg == DM_CLOSE || Msg == DN_CLOSE)
			p->wpi->gnMsgClose_3 = (FARMESSAGE)Msg;
			
		/* ********* ���������� ����� ********** */
		lRc = p->m_DlgProc((HANDLE)p, Msg2, Param1, (LONG_PTR)Param2);
		/* ************************************* */
		
		if (Msg == DM_KEY || Msg == DN_CONTROLINPUT)
			p->wpi->gnMsgKey_3 = DM_FIRST;
		if (Msg == DM_CLOSE || Msg == DN_CLOSE)
			p->wpi->gnMsgClose_3 = DM_FIRST;
		if (Param2 && Param2 != OrgParam2)
			p->wpi->FarMessageParam_2_3(Msg, Param1, Param2, OrgParam2, lRc);
			
		if (lbNeedColor && lRc)
			lbNeedColor = false;
	}
	else
	{
		lRc = p->wpi->psi3.DefDlgProc(hDlg, Msg, Param1, (void*)Param2);
	}
	
	
	// �� Far3 ����� DIF_SETCOLOR
	if (lbNeedColor)
	{
		switch (Msg)
		{
		case DN_CTLCOLORDLGITEM:
			{
				FarDialogItemColors* p3 = (FarDialogItemColors*)OrgParam2;
				_ASSERTE(p3->StructSize==sizeof(*p3));
				// ���������� ���������� ����� ���� DIF_SETCOLOR
				WrapPluginInfo::FarColor_2_3((BYTE)(p->m_Colors2[Param1] & 0xFF), p3->Colors[0]);
				//TODO: � ����� ������� (0..3) ����� ���������?
				// ��������� ����� ������ ������������
				//lRc = TRUE;
			}
			break;
		}
	}
	
	return lRc;
}

HANDLE Far2Dialog::InitDlg()
{	
	if (!m_GuidChecked)
	{
		/*
		-- ���� �� ����������. ��������, UCharMap ����� DM_GETDLGDATA, � �������-�� ��� ���...
		if (m_DlgProc)
		{
			Far2::DialogInfo info = {sizeof(Far2::DialogInfo)};
			if (m_DlgProc((HANDLE)this, Far2::DM_GETDIALOGINFO, 0, (LONG_PTR)&info)
				&& memcmp(&info.Id, &m_Guid, sizeof(m_Guid)))
			{
				m_Guid = info.Id;
				m_GuidChecked = TRUE;
			}
		}
		*/
		if (!m_GuidChecked)
		{
			m_Guid = m_DefGuid;
			m_GuidChecked = TRUE;
		}
	}
	
	if (!m_Items3 && m_Items2 && m_ItemsNumber > 0)
	{
		m_Items3 = (FarDialogItem*)calloc(m_ItemsNumber, sizeof(*m_Items3));
		if (!mp_ListInit3)
		{
			mp_ListInit3 = (FarList*)calloc(m_ItemsNumber,sizeof(*mp_ListInit3));
		}
		Far2::FarDialogItem *p2 = m_Items2;
		FarDialogItem *p3 = m_Items3;
		for (UINT i = 0; i < m_ItemsNumber; i++, p2++, p3++)
		{
			wpi->FarDialogItem_2_3(p2, p3, mp_ListInit3+i, mpp_FarCharInfo3[i]);
		}
	}

	if (hDlg3 == NULL)
	{
		wpi->m_LastFar2Dlg = this;
		FARDIALOGFLAGS Flags3 = 0
			| ((m_Flags & Far2::FDLG_WARNING) ? FDLG_WARNING : 0)
			| ((m_Flags & Far2::FDLG_SMALLDIALOG) ? FDLG_SMALLDIALOG : 0)
			| ((m_Flags & Far2::FDLG_NODRAWSHADOW) ? FDLG_NODRAWSHADOW : 0)
			| ((m_Flags & Far2::FDLG_NODRAWPANEL) ? FDLG_NODRAWPANEL : 0);
		hDlg3 = wpi->psi3.DialogInit(&m_PluginGuid, &m_Guid,
			m_X1, m_Y1, m_X2, m_Y2, m_HelpTopic, m_Items3, m_ItemsNumber, 0,
			Flags3, Far3DlgProc, (void*)m_Param);
		if (hDlg3)
		{
			(*gpMapDlg_2_3)[this] = hDlg3;
			(*gpMapDlg_3_2)[hDlg3] = this;
		}
	}

	return hDlg3;
}

int Far2Dialog::RunDlg()
{
	int iRc = -1;

	if (hDlg3 == NULL)
	{
		InitDlg();
	}

	if (hDlg3 != NULL)
	{
		wpi->m_LastFar2Dlg = this;
		iRc = wpi->psi3.DialogRun(hDlg3);

		// ��������� ��������� ����� ���������� ������� � ������ 2
		if (m_Items3 && m_Items2 && m_ItemsNumber > 0)
		{
			Far2::FarDialogItem *p2 = m_Items2;
			FarDialogItem *p3 = m_Items3;
			for (UINT i = 0; i < m_ItemsNumber; i++, p2++, p3++)
			{
				if (p3->Type == DI_COMBOBOX || p3->Type == DI_LISTBOX)
					p2->Param.Selected = p3->Selected;
				else if (p3->Type == DI_CHECKBOX || p3->Type == DI_RADIOBUTTON)
					p2->Param.Selected = p3->Selected;

				p2->Flags &= ~(Far2::DIF_SETCOLOR|Far2::DIF_COLORMASK);
				p2->Flags |= m_Colors2[i];
			}

		}

		// ����������� - � DialogFree
		//(*gpMapDlg_2_3).erase(this);
		//for (std::map<Far2Dialog*,HANDLE>::iterator i1 = (*gpMapDlg_2_3).begin();
		//	 i1 != (*gpMapDlg_2_3).end(); i++)
		//{
		//	if (i->first == this)
		//	{
		//		(*gpMapDlg_2_3).erase(i1);
		//		break;
		//	}
		//}
		//(*gpMapDlg_3_2).erase(hDlg3);
		//for (std::map<HANDLE,Far2Dialog*>::iterator i2 = (*gpMapDlg_3_2).begin();
		//	 i2 != (*gpMapDlg_3_2).end(); i++)
		//{
		//	if (i->first == hDlg3)
		//	{
		//		(*gpMapDlg_3_2).erase(i2);
		//		break;
		//	}
		//}
	}
	
	wpi->m_LastFar2Dlg = NULL;
	return iRc;
}



















typedef FARPROC (WINAPI* GetProcAddressT)(HMODULE hModule, LPCSTR lpProcName);
GetProcAddressT _GetProcAddress = NULL;
FARPROC WINAPI WrapGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	// If this parameter is an ordinal value, it must be in the low-order word;
	// the high-order word must be zero.
	if ( (((DWORD_PTR)lpProcName) & 0xFFFF) != ((DWORD_PTR)lpProcName) )
	{
		GetProcAddressT fn = (GetProcAddressT)GetProcAddress(hModule, "FarWrapGetProcAddress");
		if (fn)
			return fn(hModule, lpProcName);
	}
	
	// ���� ������ �� ���������� - �� ������� �������
	if (_GetProcAddress)
		return _GetProcAddress(hModule, lpProcName);
	_ASSERTE(_GetProcAddress!=NULL);
	return GetProcAddress(hModule, lpProcName);
}


bool SetProcAddressHook()
{
	#define GetPtrFromRVA(rva,pNTHeader,imageBase) (PVOID)((imageBase)+(rva))
	
	IMAGE_IMPORT_DESCRIPTOR* Import = 0;
	DWORD Size = 0;
	HMODULE Module = GetModuleHandle(0);

	if (!Module || (Module == INVALID_HANDLE_VALUE))
		return false;

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
	IMAGE_NT_HEADERS* nt_header = NULL;

	if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);

		if (nt_header->Signature != 0x004550)
			return false;
		else
		{
			Import = (IMAGE_IMPORT_DESCRIPTOR*)((char*)Module +
			                                    (DWORD)(nt_header->OptionalHeader.
			                                            DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
			                                            VirtualAddress));
			Size = nt_header->OptionalHeader.
			       DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		}
	}
	else
		return false;

	// if wrong module or no import table
	if (!Import)
		return false;

#ifdef _DEBUG
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt_header);
#endif
#ifdef _WIN64
	_ASSERTE(sizeof(DWORD_PTR)==8);
#else
	_ASSERTE(sizeof(DWORD_PTR)==4);
#endif
#ifdef _WIN64
#define TOP_SHIFT 60
#else
#define TOP_SHIFT 28
#endif


	bool bHooked = false;
	
	int nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
	//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- ����� ���� �� �������
	for(int i = 0; !bHooked && i < nCount; i++)
	{
		if (Import[i].Name == 0)
			break;

		//DebugString( ToTchar( (char*)Module + Import[i].Name ) );
#ifdef _DEBUG
		char* mod_name = (char*)Module + Import[i].Name;
#endif
		DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
		DWORD_PTR rvaIAT = Import[i].FirstThunk;

		if (rvaINT == 0)      // No Characteristics field?
		{
			// Yes! Gotta have a non-zero FirstThunk field then.
			rvaINT = rvaIAT;

			if (rvaINT == 0)       // No FirstThunk field?  Ooops!!!
			{
				_ASSERTE(rvaINT!=0);
				break;
			}
		}

		PIMAGE_IMPORT_BY_NAME pOrdinalNameO = NULL;
		IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaIAT, nt_header, (PBYTE)Module);
		IMAGE_THUNK_DATA* thunkO = (IMAGE_THUNK_DATA*)GetPtrFromRVA(rvaINT, nt_header, (PBYTE)Module);

		if (!thunk ||  !thunkO)
		{
			_ASSERTE(thunk && thunkO);
			continue;
		}

		for (int f = 0; !bHooked && thunk->u1.Function; thunk++, thunkO++, f++)
		{
			const char* pszFuncName = NULL;
			ULONGLONG ordinalO = -1;

			if (thunk->u1.Function != thunkO->u1.Function)
			{
				// Ordinal � ��� ���� �� ������������
				if (IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
				{
					//WARNING("��� �� ORDINAL, ��� Hint!!!");
					ordinalO = IMAGE_ORDINAL(thunkO->u1.Ordinal);
					pOrdinalNameO = NULL;
				}

				if (!IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
				{
					pOrdinalNameO = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, (PBYTE)Module);
					//WARNING("������������� ������ IsBad???Ptr ����� �������");
					BOOL lbValidPtr = !IsBadReadPtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME));
					_ASSERTE(lbValidPtr);

					if (lbValidPtr)
					{
						lbValidPtr = !IsBadStringPtrA((LPCSTR)pOrdinalNameO->Name, 10);
						_ASSERTE(lbValidPtr);

						if (lbValidPtr)
							pszFuncName = (LPCSTR)pOrdinalNameO->Name;
					}
				}
			}
			
			if (!pszFuncName)
				continue; // ���� ��� ������� ���������� �� ������� - ����������
			else if (lstrcmpA(pszFuncName, "GetProcAddress"))
				continue;
			
			
			bHooked = true;
			DWORD old_protect = 0; DWORD dwErr = 0;

			if (thunk->u1.Function == (DWORD_PTR)WrapGetProcAddress)
			{
				// ��������� �������� � ������ ����? ������ ���� �� ������
				_ASSERTE(thunk->u1.Function != (DWORD_PTR)WrapGetProcAddress);
			}
			else
			{
				if (!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function),
				                   PAGE_READWRITE, &old_protect))
				{
					dwErr = GetLastError();
					_ASSERTE(FALSE);
				}
				else
				{
					_GetProcAddress = (GetProcAddressT)thunk->u1.Function;
					thunk->u1.Function = (DWORD_PTR)WrapGetProcAddress;
					VirtualProtect(&thunk->u1.Function, sizeof(DWORD), old_protect, &old_protect);
				}
			}
		}
	}

	return bHooked;
}


// ������ ����� ���� ������ � ������ ��� �� ������� ���� (������ ������ ��� ������ � �������)
// ������� ������� "gnMainThreadId = GetCurrentThreadId();" �� ��������. ����� ������ ������ ���� ��������!
DWORD GetMainThreadId()
{
	DWORD nThreadID = 0;
	DWORD nProcID = GetCurrentProcessId();
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ti = {sizeof(THREADENTRY32)};
		if (Thread32First(h, &ti))
		{
			do {
				// ����� ����� ������ ���� ��������
				if (ti.th32OwnerProcessID == nProcID) {
					nThreadID = ti.th32ThreadID;
					break;
				}
			} while (Thread32Next(h, &ti));
		}
		CloseHandle(h);
	}

	// ��������. ������ ���� �������. ������ ���� ���-�� (������� ����)
	if (!nThreadID) {
		_ASSERTE(nThreadID!=0);
		nThreadID = GetCurrentThreadId();
	}
	return nThreadID;
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    if (ghFar3Wrap == NULL)
    {
		ghFar3Wrap = (HMODULE)hModule;
		gnMainThreadId = GetMainThreadId();
		HeapInitialize();

		gpMapDlg_2_3 = new std::map<Far2Dialog*,HANDLE>;
		gpMapDlg_3_2 = new std::map<HANDLE,Far2Dialog*>;
		gpMapSysID = new std::map<DWORD,WrapPluginInfo*>;
		
		// ������� GetProcAddress, ����� � ��� �� �������� �� �������, 
		// ������� ��� � ������������ �������. 
		// ��� ����� ������� ������ � ������ ������� ��������.
		bool lbHooked = SetProcAddressHook();
		if (!lbHooked)
		{
			_ASSERTE(lbHooked==true);
		}

		//lbPsi2 = FALSE;
		//memset(&psi3, 0, sizeof(psi3));
		//memset(&psi2, 0, sizeof(psi2));
		//memset(&FSF3, 0, sizeof(FSF3));
		//memset(&FSF2, 0, sizeof(FSF2));

		//wpi = NULL;
		//LoadPluginInfo(); // ����� ������ �� �����, �������� � SetStartupInfoW?
    }
    if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
    	#if 0
		//TODO: !!! ���� ����� � ���������� ������ (��� ������������) �����-�� �������� - ����� ����� ��� ���������
		std::vector<WrapUpdateFunctions>::iterator iter;
		#undef SET_EXP
		#define SET_EXP(v,n) { lstrcpyA(strChange[nIdx].OldName, n); lstrcpyA(strChange[nIdx].NewName, n); if (upd.mn_NewAbsentFunctions & v) strChange[nIdx].NewName[0] = '_'; else strChange[nIdx].OldName[0] = '_'; nIdx++; }
		for (iter = UpdateFunc.begin(); iter != UpdateFunc.end(); iter++)
		{
			WrapUpdateFunctions upd = *iter;
			HRSRC hRc = FindResource(ghFar3Wrap, (LPCTSTR)upd.nResourceID, _T("LOADERS"));
			if (hRc == NULL)
			{
				_ASSERTE(hRc != NULL);
			}
			else
			{
				DWORD nSize = SizeofResource(ghFar3Wrap, hRc);
				HGLOBAL hRes = LoadResource(ghFar3Wrap, hRc);
				if (hRes == NULL && nSize)
				{
					_ASSERTE(hRes != NULL && nSize);
				}
				else
				{
					LPVOID ptrLoader = LockResource(hRes);
					if (ptrLoader == NULL)
					{
						_ASSERTE(ptrLoader!=NULL);
					}
					else
					{
						HANDLE hFile = CreateFile(upd.ms_Loader, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
							CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
						if ( hFile == INVALID_HANDLE_VALUE )
							continue;
						DWORD nWrite = 0;
						BOOL lbWrite = WriteFile(hFile, ptrLoader, nSize, &nWrite, NULL);
						_ASSERTE(lbWrite && nWrite == nSize);
						CloseHandle(hFile);
					}
				}
			}
			#if 0
			HANDLE hFile = CreateFile(upd.ms_Loader, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if ( hFile == INVALID_HANDLE_VALUE )
				continue;
			HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
			if (hFileMapping)
			{
				PBYTE pMappedFileBase = (PBYTE)MapViewOfFile(hFileMapping,FILE_MAP_READ|FILE_MAP_WRITE,0,0,0);
				if (pMappedFileBase)
				{
					ExportFunc strChange[64] = {{NULL}};
					int nIdx = 0;
					if ((upd.mn_OldAbsentFunctions & ALF3_Analyse) != (upd.mn_NewAbsentFunctions & ALF3_Analyse))
					{
						SET_EXP(ALF3_Analyse, "AnalyseW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_Open) != (upd.mn_NewAbsentFunctions & ALF3_Open))
					{
						SET_EXP(ALF3_Open, "OpenW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_Configure) != (upd.mn_NewAbsentFunctions & ALF3_Configure))
					{
						SET_EXP(ALF3_Configure, "ConfigureW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_Compare) != (upd.mn_NewAbsentFunctions & ALF3_Compare))
					{
						SET_EXP(ALF3_Compare, "CompareW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_GetFiles) != (upd.mn_NewAbsentFunctions & ALF3_GetFiles))
					{
						SET_EXP(ALF3_GetFiles, "GetFilesW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_PutFiles) != (upd.mn_NewAbsentFunctions & ALF3_PutFiles))
					{
						SET_EXP(ALF3_PutFiles, "PutFilesW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_FindData) != (upd.mn_NewAbsentFunctions & ALF3_FindData))
					{
						SET_EXP(ALF3_FindData, "GetFindDataW");
						SET_EXP(ALF3_FindData, "FreeFindDataW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_VirtualFindData) != (upd.mn_NewAbsentFunctions & ALF3_VirtualFindData))
					{
						SET_EXP(ALF3_VirtualFindData, "GetVirtualFindDataW");
						SET_EXP(ALF3_VirtualFindData, "FreeVirtualFindDataW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_ProcessHostFile) != (upd.mn_NewAbsentFunctions & ALF3_ProcessHostFile))
					{
						SET_EXP(ALF3_ProcessHostFile, "ProcessHostFileW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_ProcessDialogEvent) != (upd.mn_NewAbsentFunctions & ALF3_ProcessDialogEvent))
					{
						SET_EXP(ALF3_ProcessDialogEvent, "ProcessDialogEventW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_ProcessEditorEvent) != (upd.mn_NewAbsentFunctions & ALF3_ProcessEditorEvent))
					{
						SET_EXP(ALF3_ProcessEditorEvent, "ProcessEditorEventW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_ProcessEditorInput) != (upd.mn_NewAbsentFunctions & ALF3_ProcessEditorInput))
					{
						SET_EXP(ALF3_ProcessEditorInput, "ProcessEditorInputW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_ProcessViewerEvent) != (upd.mn_NewAbsentFunctions & ALF3_ProcessViewerEvent))
					{
						SET_EXP(ALF3_ProcessViewerEvent, "ProcessViewerEventW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_SetFindList) != (upd.mn_NewAbsentFunctions & ALF3_SetFindList))
					{
						SET_EXP(ALF3_SetFindList, "SetFindListW");
					}
					if ((upd.mn_OldAbsentFunctions & ALF3_CustomData) != (upd.mn_NewAbsentFunctions & ALF3_CustomData))
					{
						SET_EXP(ALF3_CustomData, "GetCustomDataW");
						SET_EXP(ALF3_CustomData, "FreeCustomDataW");
					}
					ChangeExports(strChange, pMappedFileBase);
					wchar_t szNew[32];
					wsprintf(szNew, L"%i", upd.mn_NewAbsentFunctions);
					WritePrivateProfileString(L"Plugin", L"DisabledFunctions", szNew, upd.ms_IniFile);
				}
				CloseHandle(hFileMapping);
			}
			// ���-�� �� �������� LastWriteDate
			SYSTEMTIME st; GetSystemTime(&st);
			FILETIME ft; SystemTimeToFileTime(&st, &ft);
			SetFileTime(hFile, NULL, NULL, &ft);
			CloseHandle(hFile);
			#endif
		}
		#undef SET_EXP
		#endif

		if (WrapPluginInfo::gpsz_LastAnalyzeFile != NULL)
		{
			free(WrapPluginInfo::gpsz_LastAnalyzeFile);
			WrapPluginInfo::gpsz_LastAnalyzeFile = NULL;
		}

		if (gpMapDlg_2_3)
		{
			delete gpMapDlg_2_3;
			gpMapDlg_2_3 = NULL;
		}
		if (gpMapDlg_3_2)
		{
			delete gpMapDlg_3_2;
			gpMapDlg_3_2 = NULL;
		}
		if (gpMapSysID)
		{
			delete gpMapSysID;
			gpMapSysID = NULL;
		}

		HeapDeinitialize();
		return TRUE;
    	//if (wpi)
    	//{
    	//	delete wpi;
    	//	wpi = NULL;
    	//}
    }
    return TRUE;
}

int WINAPI InitPlugin(struct Far3WrapFunctions *pInfo2)
{
#ifdef _DEBUG
	_InitPlugin fDbg = InitPlugin; // ��� �������� ���������� �� ����� ����������
#endif
	if (!pInfo2)
	{
		#ifdef _DEBUG
		{
			DebugDump(L"InitPlugin failed (pInfo2)!", L"Far3Wrap");
		}
		#endif
		return 1;
	}
	if (pInfo2->StructSize != sizeof(*pInfo2))
	{
		_ASSERTE(pInfo2->StructSize == sizeof(*pInfo2));
		if (pInfo2->ErrorInfo && (pInfo2->ErrorInfoMax >= 255 && pInfo2->ErrorInfoMax <= 4096))
			wsprintf(pInfo2->ErrorInfo, L"Far3Wrap\nInitPlugin failed. Invalid value of pInfo2->StructSize\nRequired: %u, received: %u", (DWORD)sizeof(*pInfo2), (DWORD)pInfo2->StructSize);
		if (pInfo2->ErrorTitle && pInfo2->ErrorTitleMax >= 255)
		{
			wchar_t sModule[MAX_PATH+1] = {0};
			GetModuleFileName(pInfo2->hLoader, sModule, ARRAYSIZE(sModule));
			int nLen = lstrlen(sModule);
			int nMaxLen = pInfo2->ErrorTitleMax - 64;
			wsprintf(pInfo2->ErrorTitle, 
				L"<%s> Incorrect struct size # (%u!=%u)",
				*sModule
				? ((nLen < pInfo2->ErrorTitleMax) ? sModule : (sModule + nLen - pInfo2->ErrorTitleMax + 1) )
				: L"[Far3wrap]",
				(DWORD)pInfo2->StructSize, (DWORD)sizeof(*pInfo2));
		}
		#ifdef _DEBUG
		{
			DebugDump(L"InitPlugin failed (StructSize)!", L"Far3Wrap");
		}
		#endif
		return 2;
	}
	if (pInfo2->Far3Build != MVV_3)
	{
		_ASSERTE(pInfo2->Far3Build == MVV_3);
		if (pInfo2->ErrorInfo && (pInfo2->ErrorInfoMax >= 255 && pInfo2->ErrorInfoMax <= 4096))
			wsprintf(pInfo2->ErrorInfo, L"Far3Wrap\nInitPlugin failed. Invalid value of pInfo2->Far3Build\nRequired: %u, received: %u", MVV_3, pInfo2->Far3Build);
		if (pInfo2->ErrorTitle && pInfo2->ErrorTitleMax >= 255)
		{
			wchar_t sModule[MAX_PATH+1] = {0};
			GetModuleFileName(pInfo2->hLoader, sModule, ARRAYSIZE(sModule));
			int nLen = lstrlen(sModule);
			int nMaxLen = pInfo2->ErrorTitleMax - 64;
			wsprintf(pInfo2->ErrorTitle, 
				L"<%s> Incorrect build # (%u!=%u)",
				*sModule
				? ((nLen < pInfo2->ErrorTitleMax) ? sModule : (sModule + nLen - pInfo2->ErrorTitleMax + 1) )
				: L"[Far3wrap]",
				pInfo2->Far3Build, MVV_3);
		}
		#ifdef _DEBUG
		{
			DebugDump(L"InitPlugin failed (Build)!", L"Far3Wrap");
		}
		#endif
		return 3;
	}

	_ASSERTE(ghFar3Wrap==NULL || ghFar3Wrap==pInfo2->hFar3Wrap);
	if (!gnMainThreadId)
		gnMainThreadId = GetMainThreadId();
	ghFar3Wrap = pInfo2->hFar3Wrap;
	
	pInfo2->wpi = new WrapPluginInfo(pInfo2);

	pInfo2->wpi->LoadPluginInfo();
	if (!pInfo2->wpi->LoadPlugin(TRUE/*abSilent*/))
	{
		if (pInfo2->ErrorTitle && pInfo2->ErrorTitleMax > 0 && *pInfo2->wpi->ms_DllFailTitle)
			lstrcpyn(pInfo2->ErrorTitle, pInfo2->wpi->ms_DllFailTitle, pInfo2->ErrorTitleMax);
		// ���� pInfo2->wpi->mh_Dll, ������ �������� �� ������� 
		// ��������� ���������� ������ (���� �����������?)
		//_ASSERTE(pInfo2->wpi->mh_Dll!=NULL); -- �� ����, ��� DebugDump �������
		delete pInfo2->wpi;
		pInfo2->wpi = NULL;
		return 4;
	}

	pInfo2->PluginGuid = pInfo2->wpi->mguid_Plugin;

	// ������� � Loader ������ ������� ��������
	#undef SET_FN
	#define SET_FN(n) pInfo2->n = WrapPluginInfo::n;
	SET_FN(GetProcAddressWrap);
	SET_FN(GetOldProcAddressWrap);
	SET_FN(SetStartupInfoWrap);
	SET_FN(GetGlobalInfoWrap);
	SET_FN(GetPluginInfoWrap);
	SET_FN(OpenWrap);
	SET_FN(AnalyseWrap);
	SET_FN(CloseAnalyseWrap);
	SET_FN(ClosePanelWrap);
	SET_FN(CompareWrap);
	SET_FN(ConfigureWrap);
	SET_FN(DeleteFilesWrap);
	SET_FN(ExitFARWrap);
	SET_FN(FreeVirtualFindDataWrap);
	SET_FN(GetFilesWrap);
	SET_FN(GetFindDataWrap);
	SET_FN(FreeFindDataWrap);
	SET_FN(GetOpenPanelInfoWrap);
	#if MVV_3<=2798
	SET_FN(GetVirtualFindDataWrap);
	#endif
	SET_FN(MakeDirectoryWrap);
	SET_FN(ProcessDialogEventWrap);
	SET_FN(ProcessEditorEventWrap);
	SET_FN(ProcessEditorInputWrap);
	SET_FN(ProcessPanelEventWrap);
	SET_FN(ProcessHostFileWrap);
	SET_FN(ProcessPanelInputWrap);
	SET_FN(ProcessConsoleInputWrap);
	SET_FN(ProcessSynchroEventWrap);
	SET_FN(ProcessViewerEventWrap);
	SET_FN(PutFilesWrap);
	SET_FN(SetDirectoryWrap);
	SET_FN(SetFindListWrap);
	SET_FN(GetCustomDataWrap);
	SET_FN(FreeCustomDataWrap);
	//
	SET_FN(FarApiDefDlgProcWrap);
	SET_FN(FarApiSendDlgMessageWrap);
	SET_FN(FarApiShowHelpWrap);
	SET_FN(FarApiSaveScreenWrap);
	SET_FN(FarApiRestoreScreenWrap);
	SET_FN(FarApiTextWrap);
	SET_FN(FarApiEditorWrap);
	SET_FN(FarApiViewerWrap);
	SET_FN(FarApiMenuWrap);
	SET_FN(FarApiMessageWrap);
	SET_FN(FarApiDialogInitWrap);
	SET_FN(FarApiDialogRunWrap);
	SET_FN(FarApiDialogFreeWrap);
	SET_FN(FarApiControlWrap);
	SET_FN(FarApiGetDirListWrap);
	SET_FN(FarApiFreeDirListWrap);
	SET_FN(FarApiGetPluginDirListWrap);
	SET_FN(FarApiFreePluginDirListWrap);
	SET_FN(FarApiCmpNameWrap);
	SET_FN(FarApiGetMsgWrap);
	SET_FN(FarApiAdvControlWrap);
	SET_FN(FarApiViewerControlWrap);
	SET_FN(FarApiEditorControlWrap);
	SET_FN(FarApiInputBoxWrap);
	SET_FN(FarApiPluginsControlWrap);
	SET_FN(FarApiFileFilterControlWrap);
	SET_FN(FarApiRegExpControlWrap);
	SET_FN(FarStdGetFileOwnerWrap);
	SET_FN(FarStdGetPathRootWrap);
	SET_FN(FarStdCopyToClipboardWrap);
	SET_FN(FarStdPasteFromClipboardWrap);
	SET_FN(FarStdXlatWrap);
	SET_FN(GetNumberOfLinksWrap);
	SET_FN(FarStdRecursiveSearchWrap);
	SET_FN(FarStdMkTempWrap);
	SET_FN(FarStdProcessNameWrap);
	SET_FN(FarStdMkLinkWrap);
	SET_FN(FarConvertPathWrap);
	SET_FN(FarGetReparsePointInfoWrap);
	SET_FN(FarGetCurrentDirectoryWrap);
	SET_FN(FarStdQSortWrap);
	SET_FN(FarStdQSortExWrap);
	SET_FN(FarStdBSearchWrap);
	#undef SET_FN

	return 0;
}
