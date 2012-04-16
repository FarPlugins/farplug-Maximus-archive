
/*
Copyright (c) 2009-2012 Maximus5
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


#pragma once

#define CONEMU_ROOT_KEY L"Software\\ConEmu"

#define MIN_ALPHA_VALUE 40
#define MAX_FONT_STYLES 8  //normal/(bold|italic|underline)
#define MAX_FONT_GROUPS 20 // Main, Borders, Japan, Cyrillic, ...

#define LONGOUTPUTHEIGHT_MIN 300
#define LONGOUTPUTHEIGHT_MAX 9999

#include <pshpack1.h>
typedef struct tagMYRGB
{
	union
	{
		COLORREF color;
		struct
		{
			BYTE    rgbBlue;
			BYTE    rgbGreen;
			BYTE    rgbRed;
			BYTE    rgbReserved;
		};
	};
} MYRGB, MYCOLORREF;
#include <poppack.h>

enum BackgroundOp
{
	eUpLeft = 0,
	eStretch = 1,
	eTile = 2,
};

#define BgImageColorsDefaults (1|2)

#include "UpdateSet.h"

class CSettings;
class CVirtualConsole;

#define TaskBracketLeft   L'{'
#define TaskBracketRight  L'}'

struct Settings
{
	public:
		Settings();
		~Settings();
	protected:
		friend class CSettings;
	
		void ReleasePointers();		
	public:

		wchar_t Type[16]; // �������������: L"[reg]" ��� L"[xml]"

		//reg->Load(L"DefaultBufferHeight", DefaultBufferHeight);
		int DefaultBufferHeight;
		
		//bool AutoScroll;
		
		//reg->Load(L"AutoBufferHeight", AutoBufferHeight);
		bool AutoBufferHeight; // Long console output
		//reg->Load(L"CmdOutputCP", nCmdOutputCP);
		int nCmdOutputCP;

	public:
		struct ColorPalette
		{
			wchar_t* pszName;
			bool bPredefined;

			//reg->Load(L"ExtendColors", isExtendColors);
			bool isExtendColors;
			//reg->Load(L"ExtendColorIdx", nExtendColorIdx);
			char nExtendColorIdx; // 0..15

			COLORREF Colors[0x20];

			void FreePtr()
			{
				SafeFree(pszName);
                ColorPalette* p = this;
                SafeFree(p);
			};
		};

		struct AppSettings
		{
			size_t   cchNameMax;
			wchar_t* AppNames; // "far.exe|far64.exe"
			wchar_t* AppNamesLwr; // For internal use
			BYTE Elevated; // 00 - unimportant, 01 - elevated, 02 - nonelevated
			
			//const COLORREF* Palette/*[0x20]*/; // ������� ������� (Fade/�� Fade)

			bool OverridePalette; // Palette+Extend
			wchar_t szPaletteName[128];
			//reg->Load(L"ExtendColors", isExtendColors);
			bool isExtendColors;
			char ExtendColors() const { return (OverridePalette || !AppNames) ? isExtendColors : gpSet->AppStd.isExtendColors; };
			//reg->Load(L"ExtendColorIdx", nExtendColorIdx);
			char nExtendColorIdx; // 0..15
			char ExtendColorIdx() const { return (OverridePalette || !AppNames) ? nExtendColorIdx : gpSet->AppStd.nExtendColorIdx; };

			bool OverrideExtendFonts;
			//reg->Load(L"ExtendFonts", isExtendFonts);
			bool isExtendFonts;
			bool ExtendFonts() const { return (OverrideExtendFonts || !AppNames) ? isExtendFonts : gpSet->AppStd.isExtendFonts; };
			//reg->Load(L"ExtendFontNormalIdx", nFontNormalColor);
			BYTE nFontNormalColor; // 0..15
			BYTE FontNormalColor() const { return (OverrideExtendFonts || !AppNames) ? nFontNormalColor : gpSet->AppStd.nFontNormalColor; };
			//reg->Load(L"ExtendFontBoldIdx", nFontBoldColor);
			BYTE nFontBoldColor;   // 0..15
			BYTE FontBoldColor() const { return (OverrideExtendFonts || !AppNames) ? nFontBoldColor : gpSet->AppStd.nFontBoldColor; };
			//reg->Load(L"ExtendFontItalicIdx", nFontItalicColor);
			BYTE nFontItalicColor; // 0..15
			BYTE FontItalicColor() const { return (OverrideExtendFonts || !AppNames) ? nFontItalicColor : gpSet->AppStd.nFontItalicColor; };

			bool OverrideCursor;
			//reg->Load(L"CursorType", isCursorV);
			bool isCursorV;
			bool CursorV() const { return (OverrideCursor || !AppNames) ? isCursorV : gpSet->AppStd.isCursorV; };
			//reg->Load(L"CursorBlink", isCursorBlink);
			bool isCursorBlink;
			bool CursorBlink() const { return (OverrideCursor || !AppNames) ? isCursorBlink : gpSet->AppStd.isCursorBlink; };
			//reg->Load(L"CursorColor", isCursorColor);
			bool isCursorColor;
			bool CursorColor() const { return (OverrideCursor || !AppNames) ? isCursorColor : gpSet->AppStd.isCursorColor; };
			//reg->Load(L"CursorBlockInactive", isCursorBlockInactive);
			bool isCursorBlockInactive;
			bool CursorBlockInactive() const { return (OverrideCursor || !AppNames) ? isCursorBlockInactive : gpSet->AppStd.isCursorBlockInactive; };

			void SetNames(LPCWSTR asAppNames)
			{
				size_t iLen = wcslen(asAppNames);

				if (!AppNames || !AppNamesLwr || (iLen >= cchNameMax))
				{
					SafeFree(AppNames);
					SafeFree(AppNamesLwr);

					cchNameMax = iLen+32;
					AppNames = (wchar_t*)malloc(cchNameMax*sizeof(wchar_t));
					AppNamesLwr = (wchar_t*)malloc(cchNameMax*sizeof(wchar_t));
					if (!AppNames || !AppNamesLwr)
					{
						_ASSERTE(AppNames!=NULL && AppNamesLwr!=NULL);
						return;
					}
				}

				_wcscpy_c(AppNames, iLen+1, asAppNames);
				_wcscpy_c(AppNamesLwr, iLen+1, asAppNames);
				CharLowerBuff(AppNamesLwr, iLen);
			};

			void FreeApps()
			{
				SafeFree(AppNames);
				SafeFree(AppNamesLwr);
				cchNameMax = 0;
			};
		};
		int GetAppSettingsId(LPCWSTR asExeAppName, bool abElevated);
		const AppSettings* GetAppSettings(int anAppId=-1);
		COLORREF* GetColors(int anAppId=-1, BOOL abFade = FALSE);
		COLORREF GetFadeColor(COLORREF cr);
		void ResetFadeColors();

		struct CommandTasks
		{
			size_t   cchNameMax;
			wchar_t* pszName;
			size_t   cchCmdMax;
			wchar_t* pszCommands; // "\r\n" separated commands

			void FreePtr()
			{
				SafeFree(pszName);
				cchNameMax = 0;
				SafeFree(pszCommands);
				cchCmdMax = 0;
                CommandTasks* p = this;
                SafeFree(p);
			};

			void SetName(LPCWSTR asName, int anCmdIndex)
			{
				wchar_t szCmd[16];
				if (!asName || !*asName)
				{
					_wsprintf(szCmd, SKIPLEN(countof(szCmd)) L"Group%i", (anCmdIndex+1));
					asName = szCmd;
				}

				// ��� �������� ���������� ������ - ��� ������ ���� ��������� � ������� ������
				size_t iLen = wcslen(asName);

				if (!pszName || ((iLen+2) >= cchNameMax))
				{
					SafeFree(pszName);

					cchNameMax = iLen+16;
					pszName = (wchar_t*)malloc(cchNameMax*sizeof(wchar_t));
					if (!pszName)
					{
						_ASSERTE(pszName!=NULL);
						return;
					}
				}

				if (asName[0] == TaskBracketLeft)
				{
					_wcscpy_c(pszName, iLen+1, asName);
				}
				else
				{
					*pszName = TaskBracketLeft;
					_wcscpy_c(pszName+1, iLen+1, asName);
				}
				if (asName[iLen-1] != TaskBracketRight)
				{
					iLen = wcslen(pszName);
					pszName[iLen++] = TaskBracketRight; pszName[iLen] = 0;
				}
			};

			void SetCommands(LPCWSTR asCommands)
			{
				if (!asCommands)
					asCommands = L"";

				size_t iLen = wcslen(asCommands);

				if (!pszCommands || (iLen >= cchCmdMax))
				{
					SafeFree(pszCommands);

					cchCmdMax = iLen+1024;
					pszCommands = (wchar_t*)malloc(cchCmdMax*sizeof(wchar_t));
					if (!pszCommands)
					{
						_ASSERTE(pszCommands!=NULL);
						return;
					}
				}

				_wcscpy_c(pszCommands, cchCmdMax, asCommands);
			};
		};
		const CommandTasks* CmdTaskGet(int anIndex); // 0-based, index of CmdTasks
		void CmdTaskSet(int anIndex, LPCWSTR asName, LPCWSTR asCommands); // 0-based, index of CmdTasks
		bool CmdTaskXch(int anIndex1, int anIndex2); // 0-based, index of CmdTasks

		const ColorPalette* PaletteGet(int anIndex); // 0-based, index of Palettes
		void PaletteSaveAs(LPCWSTR asName); // Save active colors to named palette
		void PaletteDelete(LPCWSTR asName); // Delete named palette

	protected:
		AppSettings AppStd;
		int AppCount;
		AppSettings* Apps;
		// ��� CSettings
		AppSettings* GetAppSettingsPtr(int anAppId);

		int CmdTaskCount;
		CommandTasks** CmdTasks;

		int PaletteCount;
		ColorPalette** Palettes;

		int PaletteGetIndex(LPCWSTR asName);
		void SavePalettes(SettingsBase* reg);
		void SortPalettes();

	private:	
		// reg->Load(L"ColorTableNN", Colors[i]);
		COLORREF Colors[0x20];
		COLORREF ColorsFade[0x20];
		bool mb_FadeInitialized;

		struct CEAppColors
		{
			COLORREF Colors[0x20];
			COLORREF ColorsFade[0x20];
			bool FadeInitialized;
		} *AppColors; // [AppCount]

		void LoadAppSettings(SettingsBase* reg);
		void LoadAppSettings(SettingsBase* reg, AppSettings* pApp, COLORREF* pColors);
		void SaveAppSettings(SettingsBase* reg, AppSettings* pApp, COLORREF* pColors);

		DWORD mn_FadeMul;
		inline BYTE GetFadeColorItem(BYTE c);

	public:
		//reg->Load(L"FontAutoSize", isFontAutoSize);
		bool isFontAutoSize;
		//reg->Load(L"AutoRegisterFonts", isAutoRegisterFonts);
		bool isAutoRegisterFonts;
		
		//reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		//reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		//reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		LOGFONT ConsoleFont;
		
		bool NeedDialogDetect();
		
		
		//reg->Load(L"TrueColorerSupport", isTrueColorer);
		bool isTrueColorer;
		

		/* *** Background image *** */
		//reg->Load(L"BackGround Image show", isShowBgImage);
		char isShowBgImage;
		//reg->Load(L"BackGround Image", sBgImage, countof(sBgImage));		
		WCHAR sBgImage[MAX_PATH];
		//reg->Load(L"bgImageDarker", bgImageDarker);
		u8 bgImageDarker;		
		//reg->Load(L"bgImageColors", nBgImageColors);
		DWORD nBgImageColors;
		//reg->Load(L"bgOperation", bgOperation);
		char bgOperation; // BackgroundOp {eUpLeft = 0, eStretch = 1, eTile = 2}
		//reg->Load(L"bgPluginAllowed", isBgPluginAllowed);
		char isBgPluginAllowed;
		
		
		//bool isBackgroundImageValid;
		
		

		/* *** Transparency *** */
		//reg->Load(L"AlphaValue", nTransparent);
		u8 nTransparent;
		//reg->Load(L"UserScreenTransparent", isUserScreenTransparent);
		bool isUserScreenTransparent;

		/* *** Command Line History (from start dialog) *** */
		//reg->Load(L"CmdLineHistory", &psCmdHistory);
		LPWSTR psCmdHistory;
		//nCmdHistorySize = 0; HistoryCheck();
		DWORD nCmdHistorySize;

		/* *** Command Line (Registry) *** */
		//reg->Load(L"CmdLine", &psCmd);
		LPTSTR psCmd;
		
		/* Command Line ("/cmd" arg) */
		LPTSTR psCurCmd;

		/* 'Default' command line (if nor Registry, nor /cmd specified) */
		//WCHAR  szDefCmd[16];
	public:
		/* "Active" command line */
		LPCTSTR GetCmd();
		/* "Default" command line "far/cmd", based on /BufferHeight switch */
		//LPCTSTR GetDefaultCmd();
		///* OUR(!) startup info */
		//STARTUPINFOW ourSI;
		/* If Attach to PID requested */
		//DWORD nAttachPID; HWND hAttachConWnd;

		//reg->Load(L"FontName", inFont, countof(inFont))
		wchar_t inFont[LF_FACESIZE];
		//reg->Load(L"FontName2", inFont2, countof(inFont2))
		wchar_t inFont2[LF_FACESIZE];
		//reg->Load(L"FontBold", isBold);
		bool isBold;
		//reg->Load(L"FontItalic", isItalic);
		bool isItalic;
		//reg->Load(L"Anti-aliasing", Quality);
		DWORD mn_AntiAlias; //���������� ��� Quality
		//reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		BYTE mn_LoadFontCharSet; // �� ��� ��������� ���������� (��� ��� ��������� � ������)
		//reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		BOOL mb_CharSetWasSet;
		//reg->Load(L"FontSize", FontSizeY);
		DWORD FontSizeY;  // ������ ��������� ������ (����������� �� ��������!)
		//reg->Load(L"FontSizeX", FontSizeX);
		DWORD FontSizeX;  // ������ ��������� ������
		//reg->Load(L"FontSizeX2", FontSizeX2);
		DWORD FontSizeX2; // ������ ��� FixFarBorders (������ ������������ ������ ��� ��������� �����, �� ������ �� �����������)
		//reg->Load(L"FontSizeX3", FontSizeX3);
		DWORD FontSizeX3; // ������ ���������� ��� ���������� ������ (�� ������ � FontSizeX2)
		
		//bool isFullScreen;
		//reg->Load(L"HideCaption", isHideCaption);
		bool isHideCaption;
		protected:
		//reg->Load(L"HideCaptionAlways", mb_HideCaptionAlways);
		bool mb_HideCaptionAlways;
		public:
		bool isHideCaptionAlways(); //<<mb_HideCaptionAlways
		//reg->Load(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);
		BYTE nHideCaptionAlwaysFrame;
		//reg->Load(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);
		DWORD nHideCaptionAlwaysDelay;
		//reg->Load(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);
		DWORD nHideCaptionAlwaysDisappear;
		//reg->Load(L"DownShowHiddenMessage", isDownShowHiddenMessage);
		bool isDownShowHiddenMessage;
		//reg->Load(L"AlwaysOnTop", isAlwaysOnTop);
		bool isAlwaysOnTop;
		//reg->Load(L"DesktopMode", isDesktopMode);
		bool isDesktopMode;
		//reg->Load(L"FixFarBorders", isFixFarBorders)
		BYTE isFixFarBorders;
		//reg->Load(L"ExtendUCharMap", isExtendUCharMap);
		bool isExtendUCharMap;
		//reg->Load(L"DisableMouse", isDisableMouse);
		bool isDisableMouse;
		//reg->Load(L"MouseSkipActivation", isMouseSkipActivation);
		bool isMouseSkipActivation;
		//reg->Load(L"MouseSkipMoving", isMouseSkipMoving);
		bool isMouseSkipMoving;
		//reg->Load(L"FarHourglass", isFarHourglass);
		bool isFarHourglass;
		//reg->Load(L"FarHourglassDelay", nFarHourglassDelay);
		DWORD nFarHourglassDelay;
		//reg->Load(L"DisableFarFlashing", isDisableFarFlashing); if (isDisableFarFlashing>2) isDisableFarFlashing = 2;
		BYTE isDisableFarFlashing;
		//reg->Load(L"DisableAllFlashing", isDisableAllFlashing); if (isDisableAllFlashing>2) isDisableAllFlashing = 2;
		BYTE isDisableAllFlashing;
		/* *** Text selection *** */
		//reg->Load(L"ConsoleTextSelection", isConsoleTextSelection);
		BYTE isConsoleTextSelection;
		//reg->Load(L"CTS.SelectBlock", isCTSSelectBlock);
		bool isCTSSelectBlock;
		//reg->Load(L"CTS.SelectText", isCTSSelectText);
		bool isCTSSelectText;
		//reg->Load(L"CTS.VkBlock", isCTSVkBlock);
		BYTE isCTSVkBlock; // ����������� ������� ��������� ������
		//reg->Load(L"CTS.VkBlockStart", isCTSVkBlockStart);
		BYTE isCTSVkBlockStart; // ������ ������ ��������� ������������� �����
		//reg->Load(L"CTS.VkText", isCTSVkText);
		BYTE isCTSVkText; // ����������� ������� ��������� ������
		//reg->Load(L"CTS.VkTextStart", isCTSVkTextStart);
		BYTE isCTSVkTextStart; // ������ ������ ��������� ���������� �����
		//reg->Load(L"CTS.ActMode", isCTSActMode);
		BYTE isCTSActMode; // ����� � ����������� ���������� �������� ������ � ������� ������ �����
		//reg->Load(L"CTS.VkAct", isCTSVkAct);
		BYTE isCTSVkAct; // ����� � ����������� ���������� �������� ������ � ������� ������ �����
		//reg->Load(L"CTS.RBtnAction", isCTSRBtnAction);
		BYTE isCTSRBtnAction; // 0-off, 1-copy, 2-paste
		//reg->Load(L"CTS.MBtnAction", isCTSMBtnAction);
		BYTE isCTSMBtnAction; // 0-off, 1-copy, 2-paste
		//reg->Load(L"CTS.ColorIndex", isCTSColorIndex);
		BYTE isCTSColorIndex;
		//reg->Load(L"FarGotoEditor", isFarGotoEditor);
		bool isFarGotoEditor; // ������������ � ���������� �� ����/������ (������ �����������)
		//reg->Load(L"FarGotoEditorVk", isFarGotoEditorVk);
		BYTE isFarGotoEditorVk; // �������-����������� ��� isFarGotoEditor
		
		bool isModifierPressed(DWORD vk);
		//bool isSelectionModifierPressed();
		//typedef struct tag_CharRanges
		//{
		//	bool bUsed;
		//	wchar_t cBegin, cEnd;
		//} CharRanges;
		//wchar_t mszCharRanges[120];
		//CharRanges icFixFarBorderRanges[10];
		
		// !!! ������� �� �������� Init/Load... !!!
		int ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue = TRUE); // ��������, L"2013-25C3,25C4"
		wchar_t* CreateCharRanges(BYTE (&Chars)[0x10000]); // caller must free(result)
		BYTE mpc_FixFarBorderValues[0x10000];
		
		//reg->Load(L"KeyboardHooks", m_isKeyboardHooks); if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;
		BYTE m_isKeyboardHooks;
	public:
		bool isKeyboardHooks();

		//bool CheckUpdatesWanted();

		bool isCharBorder(wchar_t inChar);
		
		//reg->Load(L"PartBrush75", isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		BYTE isPartBrush75;
		//reg->Load(L"PartBrush50", isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		BYTE isPartBrush50;
		//reg->Load(L"PartBrush25", isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		BYTE isPartBrush25;
		//reg->Load(L"PartBrushBlack", isPartBrushBlack);
		BYTE isPartBrushBlack;
		
		//reg->Load(L"RightClick opens context menu", isRClickSendKey);
		// 0 - �� ����� EMenu, 1 - ����� ������, 2 - ����� �� �������� �����
		char isRClickSendKey;
		//��� ���������� - ������� �� �������� ���� ���������� ����,
		// � �� �������� (Press and tap) ��������� ��������� ������
		// �������, ���� isRClickTouch, �� "�������"/"��������" ���� �������������
		// --> isRClickSendKey==1 - ����� ������ (isRClickTouchInvert �� ������)
		// --> isRClickSendKey==2 - ����� �� �������� ���� (������ �������� RClick)
		// ��� ����, PressAndTap ������ �������� RClick � ������� (��� ��������� ������).
		bool isRClickTouchInvert();
		//reg->Load(L"RightClickMacro2", &sRClickMacro);
		wchar_t *sRClickMacro;
		LPCWSTR RClickMacro();
		LPCWSTR RClickMacroDefault();
		
		//reg->Load(L"SafeFarClose", isSafeFarClose);
		bool isSafeFarClose;
		//reg->Load(L"SafeFarCloseMacro", &sSafeFarCloseMacro);
		wchar_t *sSafeFarCloseMacro;
		LPCWSTR SafeFarCloseMacro();
		LPCWSTR SafeFarCloseMacroDefault();
		
		//reg->Load(L"AltEnter", isSendAltEnter);
		bool isSendAltEnter;
		//reg->Load(L"AltSpace", isSendAltSpace);
		bool isSendAltSpace;
		//reg->Load(L"SendAltTab", isSendAltTab);
		bool isSendAltTab;
		//reg->Load(L"SendAltEsc", isSendAltEsc);
		bool isSendAltEsc;
		//reg->Load(L"SendAltPrintScrn", isSendAltPrintScrn);
		bool isSendAltPrintScrn;
		//reg->Load(L"SendPrintScrn", isSendPrintScrn);
		bool isSendPrintScrn;
		//reg->Load(L"SendCtrlEsc", isSendCtrlEsc);
		bool isSendCtrlEsc;
		//reg->Load(L"SendAltF9", isSendAltF9);
		bool isSendAltF9;
		
		//reg->Load(L"Min2Tray", isMinToTray);
		bool isMinToTray;
		//reg->Load(L"AlwaysShowTrayIcon", isAlwaysShowTrayIcon);
		bool isAlwaysShowTrayIcon;
		//bool isForceMonospace, isProportional;
		//reg->Load(L"Monospace", isMonospace)
		BYTE isMonospace; // 0 - proportional, 1 - monospace, 2 - forcemonospace
		//bool isUpdConHandle;
		//reg->Load(L"RSelectionFix", isRSelFix);
		bool isRSelFix;
		
		/* *** Drag *** */
		//reg->Load(L"Dnd", isDragEnabled);
		BYTE isDragEnabled;
		//reg->Load(L"DndDrop", isDropEnabled);
		BYTE isDropEnabled;
		//reg->Load(L"DndLKey", nLDragKey);
		BYTE nLDragKey;
		//reg->Load(L"DndRKey", nRDragKey);
		BYTE nRDragKey; // ��� DWORD
		//reg->Load(L"DefCopy", isDefCopy);
		bool isDefCopy;
		//reg->Load(L"DragOverlay", isDragOverlay);
		bool isDragOverlay;
		//reg->Load(L"DragShowIcons", isDragShowIcons);
		bool isDragShowIcons;
		//reg->Load(L"DragPanel", isDragPanel); if (isDragPanel > 2) isDragPanel = 1;
		BYTE isDragPanel; // ��������� ������� ������� ������
		//reg->Load(L"DragPanelBothEdges", isDragPanelBothEdges);
		bool isDragPanelBothEdges; // ������� �� ��� ����� (������-����� ������ � �����-������ ������)

		//reg->Load(L"KeyBarRClick", isKeyBarRClick);
		bool isKeyBarRClick; // ������ ���� �� ������� - �������� PopupMenu
		
		//reg->Load(L"DebugSteps", isDebugSteps);
		bool isDebugSteps;
		
		//reg->Load(L"EnhanceGraphics", isEnhanceGraphics);
		bool isEnhanceGraphics; // Progressbars and scrollbars (pseudographics)
		//reg->Load(L"EnhanceButtons", isEnhanceButtons);
		bool isEnhanceButtons; // Buttons, CheckBoxes and RadioButtons (pseudographics)
		
		//reg->Load(L"FadeInactive", isFadeInactive);
		bool isFadeInactive;
		protected:
		//reg->Load(L"FadeInactiveLow", mn_FadeLow);
		BYTE mn_FadeLow;
		//reg->Load(L"FadeInactiveHigh", mn_FadeHigh);		
		BYTE mn_FadeHigh;
		//mn_LastFadeSrc = mn_LastFadeDst = -1;
		COLORREF mn_LastFadeSrc;
		//mn_LastFadeSrc = mn_LastFadeDst = -1;		
		COLORREF mn_LastFadeDst;
		public:
		
		//reg->Load(L"Tabs", isTabs);
		char isTabs;
		//reg->Load(L"TabSelf", isTabSelf);
		bool isTabSelf;
		//reg->Load(L"TabRecent", isTabRecent);
		bool isTabRecent;
		//reg->Load(L"TabLazy", isTabLazy);
		bool isTabLazy;
		
		//TODO:
		bool isTabsInCaption;

		// Tab theme properties
		int ilDragHeight;

		protected:
		//reg->Load(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		char m_isTabsOnTaskBar;
		public:
		bool isTabsOnTaskBar();
		
		//reg->Load(L"TabFontFace", sTabFontFace, countof(sTabFontFace));
		wchar_t sTabFontFace[LF_FACESIZE];
		//reg->Load(L"TabFontCharSet", nTabFontCharSet);
		DWORD nTabFontCharSet;
		//reg->Load(L"TabFontHeight", nTabFontHeight);
		int nTabFontHeight;
		
		//if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { if (sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = NULL; } }
		wchar_t *sTabCloseMacro;
		LPCWSTR TabCloseMacro();
		LPCWSTR TabCloseMacroDefault();
		
		//if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro)) { sSaveAllMacro = lstrdup(L"...
		wchar_t *sSaveAllMacro;
		LPCWSTR SaveAllMacro();
		LPCWSTR SaveAllMacroDefault();
		
		//reg->Load(L"ToolbarAddSpace", nToolbarAddSpace);
		int nToolbarAddSpace;
		//reg->Load(L"ConWnd Width", wndWidth);
		DWORD wndWidth;
		//reg->Load(L"ConWnd Height", wndHeight);
		DWORD wndHeight;
		//reg->Load(L"16bit Height", ntvdmHeight);
		DWORD ntvdmHeight; // � ��������
		//reg->Load(L"ConWnd X", wndX);
		int wndX; // � ��������
		//reg->Load(L"ConWnd Y", wndY);
		int wndY; // � ��������
		//reg->Load(L"WindowMode", WindowMode); if (WindowMode!=rFullScreen && WindowMode!=rMaximized && WindowMode!=rNormal) WindowMode = rNormal;
		DWORD WindowMode;
		//reg->Load(L"Cascaded", wndCascade);
		bool wndCascade;
		//reg->Load(L"AutoSaveSizePos", isAutoSaveSizePos);
		bool isAutoSaveSizePos;
	private:
		// ��� �������� ���� ��������� - ��������� ������ ���� ���,
		// � �� ������ ����� � �������� �������� �������� ����������
		bool mb_SizePosAutoSaved;
	public:
		//reg->Load(L"SlideShowElapse", nSlideShowElapse);
		DWORD nSlideShowElapse;
		//reg->Load(L"IconID", nIconID);
		DWORD nIconID;
		//reg->Load(L"TryToCenter", isTryToCenter);
		bool isTryToCenter;
		//reg->Load(L"ShowScrollbar", isAlwaysShowScrollbar); if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2;
		BYTE isAlwaysShowScrollbar;
		
		//reg->Load(L"TabMargins", rcTabMargins);
		RECT rcTabMargins;
		//reg->Load(L"TabFrame", isTabFrame);
		bool isTabFrame;
		
		//reg->Load(L"MinimizeRestore", icMinimizeRestore);
		BYTE icMinimizeRestore;
		//reg->Load(L"Multi", isMulti);
		bool isMulti;
		private:
		//reg->Load(L"Multi.Modifier", nMultiHotkeyModifier); TestHostkeyModifiers();
		DWORD nMultiHotkeyModifier;
		public:
		//reg->Load(L"Multi.NewConsole", icMultiNew);
		BYTE icMultiNew;
		//reg->Load(L"Multi.Next", icMultiNext);
		BYTE icMultiNext;
		//reg->Load(L"Multi.Recreate", icMultiRecreate);
		BYTE icMultiRecreate;
		//reg->Load(L"Multi.Buffer", icMultiBuffer);
		BYTE icMultiBuffer;
		//reg->Load(L"Multi.Close", icMultiClose);
		BYTE icMultiClose;
		//reg->Load(L"Multi.CmdKey", icMultiCmd);
		BYTE icMultiCmd;
		//reg->Load(L"Multi.AutoCreate", isMultiAutoCreate);
		bool isMultiAutoCreate;
		//reg->Load(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		bool isMultiLeaveOnClose;
		//reg->Load(L"Multi.Iterate", isMultiIterate);
		bool isMultiIterate;
		//reg->Load(L"Multi.NewConfirm", isMultiNewConfirm);
		bool isMultiNewConfirm;
		//reg->Load(L"Multi.UseNumbers", isUseWinNumber);
		bool isUseWinNumber;
		//reg->Load(L"Multi.UseWinTab", isUseWinTab);
		bool isUseWinTab;
		//reg->Load(L"Multi.UseArrows", isUseWinArrows);
		bool isUseWinArrows;
		//reg->Load(L"FARuseASCIIsort", isFARuseASCIIsort);
		bool isFARuseASCIIsort;
		//reg->Load(L"FixAltOnAltTab", isFixAltOnAltTab);
		bool isFixAltOnAltTab;
		//reg->Load(L"ShellNoZoneCheck", isShellNoZoneCheck);
		bool isShellNoZoneCheck;
		
		bool IsHostkey(WORD vk);
		bool IsHostkeySingle(WORD vk);
		bool IsHostkeyPressed();
		WORD GetPressedHostkey();
		UINT GetHostKeyMod(); // ����� ������ MOD_xxx ��� RegisterHotKey

		/* *** ��������� ����� *** */
		//reg->Load(L"TabConsole", szTabConsole, countof(szTabConsole));
		WCHAR szTabConsole[32];
		//reg->Load(L"TabEditor", szTabEditor, countof(szTabEditor));
		WCHAR szTabEditor[32];
		//reg->Load(L"TabEditorModified", szTabEditorModified, countof(szTabEditorModified));
		WCHAR szTabEditorModified[32];
		//reg->Load(L"TabViewer", szTabViewer, countof(szTabViewer));
		WCHAR szTabViewer[32];
		//reg->Load(L"TabLenMax", nTabLenMax); if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;
		DWORD nTabLenMax;

		//reg->Load(L"AdminTitleSuffix", szAdminTitleSuffix, countof(szAdminTitleSuffix)); szAdminTitleSuffix[countof(szAdminTitleSuffix)-1] = 0;
		wchar_t szAdminTitleSuffix[64]; //" (Admin)"
		//reg->Load(L"AdminShowShield", bAdminShield);
		bool bAdminShield;
		//reg->Load(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		bool bHideInactiveConsoleTabs;

		TODO("��������/���������� bHideDisabledTabs");
		bool bHideDisabledTabs;
		
		//reg->Load(L"ShowFarWindows", bShowFarWindows);
		bool bShowFarWindows;

		bool NeedCreateAppWindow();
		
		//reg->Load(L"MainTimerElapse", nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
		DWORD nMainTimerElapse; // �������������, � ������� �� ������� ����������� �����
		//reg->Load(L"MainTimerInactiveElapse", nMainTimerInactiveElapse); if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000;
		DWORD nMainTimerInactiveElapse; // ������������� ��� ������������
		
		//bool isAdvLangChange; // � ����� ��� ConIme � ����� ������� �� �������� ����, ���� �� ������� WM_SETFOCUS. �� ��� ���� �������� ������ �������� ������
		
		//reg->Load(L"SkipFocusEvents", isSkipFocusEvents);
		bool isSkipFocusEvents;
		
		//bool isLangChangeWsPlugin;
		
		//reg->Load(L"MonitorConsoleLang", isMonitorConsoleLang);
		char isMonitorConsoleLang;
		
		//reg->Load(L"SleepInBackground", isSleepInBackground);
		bool isSleepInBackground;

		//reg->Load(L"AffinityMask", nAffinity);
		DWORD nAffinity;

		//reg->Load(L"UseInjects", isUseInjects);
		bool isUseInjects;
		//reg->Load(L"PortableReg", isPortableReg);
		bool isPortableReg;

		/* *** Debugging *** */
		//reg->Load(L"ConVisible", isConVisible);
		bool isConVisible;
		//reg->Load(L"ConInMode", nConInMode);
		DWORD nConInMode;

		/* *** Thumbnails and Tiles *** */
		//reg->Load(L"PanView.BackColor", ThSet.crBackground.RawColor);
		//reg->Load(L"PanView.PFrame", ThSet.nPreviewFrame); if (ThSet.nPreviewFrame!=0 && ThSet.nPreviewFrame!=1) ThSet.nPreviewFrame = 1;
		//� �.�...
		PanelViewSetMapping ThSet;
		
		/* *** AutoUpdate *** */
		ConEmuUpdateSettings UpdSet;
		//wchar_t *szUpdateVerLocation; // ConEmu latest version location info
		//bool isUpdateCheckOnStartup;
		//bool isUpdateCheckHourly;
		//bool isUpdateConfirmDownload;
		//BYTE isUpdateUseBuilds; // 1-stable only, 2-latest
		//bool isUpdateUseProxy;
		//wchar_t *szUpdateProxy; // "Server:port"
		//wchar_t *szUpdateProxyUser;
		//wchar_t *szUpdateProxyPassword;
		//BYTE isUpdateDownloadSetup; // 1-Installer (ConEmuSetup.exe), 2-7z archieve (ConEmu.7z), WinRar or 7z required
		//wchar_t *szUpdateArcCmdLine; // "%1"-archive file, "%2"-ConEmu base dir
		//wchar_t *szUpdateDownloadPath; // "%TEMP%"
		//bool isUpdateLeavePackages;
		//wchar_t *szUpdatePostUpdateCmd; // ���� ����� ����-�� ���� ������ � �������������� �������


		/* *** HotKeys & GuiMacros *** */
		//reg->Load(L"GuiMacro<N>.Key", &Macros.vk);
		//reg->Load(L"GuiMacro<N>.Macro", &Macros.szGuiMacro);
		struct HotGuiMacro
		{
			union {
				BYTE vk;
				LPVOID dummy;
			};
			wchar_t* szGuiMacro;
		};
		HotGuiMacro Macros[24];
		
	public:
		void LoadSettings();
		void InitSettings();
		void LoadCmdTasks(SettingsBase* reg, bool abFromOpDlg = false);
		void LoadPalettes(SettingsBase* reg);
		BOOL SaveSettings(BOOL abSilent = FALSE);
		void SaveAppSettings(SettingsBase* reg);
		void SaveCmdTasks(SettingsBase* reg);
		void SaveSizePosOnExit();
		void SaveConsoleFont();
		void UpdateMargins(RECT arcMargins);
	public:
		void HistoryCheck();
		void HistoryAdd(LPCWSTR asCmd);
		LPCWSTR HistoryGet();
		//void UpdateConsoleMode(DWORD nMode);
		//BOOL CheckConIme();
		void CheckConsoleSettings();
		
		SettingsBase* CreateSettings();
		
		void GetSettingsType(wchar_t (&szType)[8], bool& ReadOnly);
		
	private:
		bool TestHostkeyModifiers();
		static BYTE CheckHostkeyModifier(BYTE vk);
		static void ReplaceHostkey(BYTE vk, BYTE vkNew);
		static void AddHostkey(BYTE vk);
		static void TrimHostkeys();
		static bool MakeHostkeyModifier();
		static BYTE HostkeyCtrlId2Vk(WORD nID);
		BYTE mn_HostModOk[15], mn_HostModSkip[15];
		bool isHostkeySingleLR(WORD vk, WORD vkC, WORD vkL, WORD vkR);
	private:
		struct CEFontRange
		{
		public:
			bool    bUsed;              // ��� �������� ���������/����������
			wchar_t sTitle[64];         // "Title"="Borders and scrollbars"
			wchar_t sRange[1024];       // "CharRange"="2013-25C4;"
			wchar_t sFace[LF_FACESIZE]; // "FontFace"="Andale Mono"
			LONG    nHeight;            // "Height"=dword:00000012
			LONG    nWidth;             // "Width"=dword:00000000
			LONG    nLoadHeight;        // ��� ����������� ���������� ������������� �������� ������ ���
			LONG    nLoadWidth;         // ������� GUI, ��� ��������� �� ������� - ��������� ��������
			BYTE    nCharset;           // "Charset"=hex:00
			bool    bBold;              // "Bold"=hex:00
			bool    bItalic;            // "Italic"=hex:00
			BYTE    nQuality;           // "Anti-aliasing"=hex:03
			/*    */
		private:
			LOGFONT LF;
		public:
			LOGFONT& LogFont()
			{
				LF.lfHeight = nHeight;
				LF.lfWidth = nWidth;
				LF.lfEscapement = 0;
				LF.lfOrientation = 0;
				LF.lfWeight = bBold?FW_BOLD:FW_NORMAL;
				LF.lfItalic = bItalic ? 1 : 0;
				LF.lfUnderline = 0;
				LF.lfStrikeOut = 0;
				LF.lfQuality = nQuality;
				LF.lfCharSet = nCharset;
				LF.lfOutPrecision = OUT_TT_PRECIS;
				LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
				LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
				lstrcpyn(LF.lfFaceName, sFace, countof(LF.lfFaceName));
				return LF;
			};
			void Scale(LONG anMainHeight, LONG anMainLoadHeight)
			{
				// ������
				if (nLoadHeight == 0 || !anMainLoadHeight)
				{
					nHeight = anMainHeight; // ������ ����� ������ � Main
				}
				else
				{
					nHeight = nLoadHeight * anMainHeight / anMainLoadHeight;
				}
				// ������
				if (nLoadWidth == 0 || !anMainLoadHeight)
				{
					nWidth = 0;
				}
				else
				{
					nWidth = nLoadWidth * anMainHeight / anMainLoadHeight;
				}
			};
			/*    */
			HFONT hFonts[MAX_FONT_STYLES]; //normal/(bold|italic|underline)
			/*    */
			BYTE RangeData[0x10000];
		};
		CEFontRange m_Fonts[MAX_FONT_GROUPS]; // 0-Main, 1-Borders, 2 � ����� - user defined
		BOOL FontRangeLoad(SettingsBase* reg, int Idx);
		BOOL FontRangeSave(SettingsBase* reg, int Idx);
};

#include "OptionsClass.h"
