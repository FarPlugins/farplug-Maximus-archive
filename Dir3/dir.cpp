
/* **************************
   This module may be used as
   1) Observer module (*.so)
   2) MultiArc module (*.fmt)
 * ************************** */

#include "plugin.hpp"
#include "fmt.hpp"
#include <crtdbg.h>

/******************************************************
 ��� � ���������� D ����� ����� DIST_ALX
 �������� ����� ����: 356D-B9B3

 ���������� ����� D:\!Develop!\Dir

15.11.2008  17:36    <DIR>          .
15.11.2008  17:36    <DIR>          ..
15.11.2008  17:39    <DIR>          dir010327
15.11.2008  17:46    <DIR>          dirparse
15.11.2008  18:11    <DIR>          [2008.11.15]
15.11.2008  18:36                 0 !!
               1 ������              0 ����

 ���������� ����� D:\!Develop!\Dir\dir010327

15.11.2008  17:39    <DIR>          .
15.11.2008  17:39    <DIR>          ..
27.03.2001  08:47             7�581 dir.cpp
27.03.2001  09:03             6�144 dir.fmt
29.03.2001  18:11             2�600 dir.txt
               3 ������         16�325 ����

     ����� ������:
               9 ������         26�795 ����
              11 �����     396�525�568 ���� ��������
 Volume in drive C is w2k3
 Volume Serial Number is F0AC-A340

 Directory of C:\Programs\Develop\mingw32\mingw

31.10.2008  12:30    <DIR>          .
31.10.2008  12:30    <DIR>          ..
31.10.2008  12:30    <DIR>          bin
04.02.2008  04:19            35�821 COPYING-gcc-tdm.txt
09.08.2007  17:18            26�930 COPYING.lib-gcc-tdm.txt
31.10.2008  12:30    <DIR>          doc
31.10.2008  13:06    <DIR>          include
31.10.2008  12:30    <DIR>          info
31.10.2008  13:06    <DIR>          lib
28.08.2008  16:39    <DIR>          libexec
31.10.2008  12:30    <DIR>          man
31.10.2008  12:30    <DIR>          mingw32
26.05.2005  10:15            21�927 pthreads-win32-README
28.08.2008  16:58            14�042 README-gcc-tdm.txt
               4 File(s)         98�720 bytes

******************************************************/

/*
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
*/

enum {  // �������� ��� WIN XP/2003/VISTA:
	DATA_DISP = 0,
	TIME_DISP = 12,
	SIZE_DISP = 19,
	NAME_DISP = 36,

	ID_DIR_DISP = 21,
};

struct DirInfo;
DirInfo* gpCur = NULL;

//static struct PluginStartupInfo Info;
struct DirInfo
{
	HANDLE Handle, MapHandle;
	char *Data, *Pos, *Edge, Prefix[MAX_PATH-2], Descrip[80], Oem2Ansi[4096];
	int nPrefixLen;
	wchar_t Utf8[4096], Wide[MAX_PATH];
	char Buf[8192]; // ��� ��������� �������
	size_t nStrLen, nSkipLen;
	bool isBtanch;
	enum
	{
	  cp_NonChecked = 0,
	  cp_OEM = 1,
	  cp_ANSI = 2,
	  cp_UTF8 = 3,
	} CP_Check;
	bool isRus;

	__int64 AtoI(char *Str, int Len)
	{
		__int64 Ret=0;
		for (; Len && *Str; Len--, Str++)
			if (*Str>='0' && *Str<='9')
				(Ret*=10)+=*Str-'0';
		return Ret;
	};

	bool Compare(char *Str, char *Mask, bool NonEng = false, int Len=-1)
	{
		int nCmp = (Len==-1) ? lstrlenA(Mask) : Len;

		if (CompareString(LOCALE_USER_DEFAULT, 0, Str, nCmp, Mask, nCmp)==CSTR_EQUAL) 
			return true;

		if (NonEng)
		{
			if ((CP_Check == cp_NonChecked) && (nCmp < ARRAYSIZE(Oem2Ansi)))
			{
				CharToOemBuff(Mask, Oem2Ansi, nCmp);
				if (CompareString(LOCALE_USER_DEFAULT, 0, Str, nCmp, Oem2Ansi, nCmp)==CSTR_EQUAL)
				{
					CP_Check = cp_OEM;
					return true;
				}
			}
			if ((CP_Check == cp_NonChecked || CP_Check == cp_UTF8) && (nCmp < ARRAYSIZE(Oem2Ansi)))
			{
				int nWide = MultiByteToWideChar(CP_ACP, 0, Mask, nCmp, Utf8, ARRAYSIZE(Utf8)-1);
				Utf8[nWide] = 0;
				int nUtf8 = WideCharToMultiByte(CP_UTF8, 0, Utf8, nWide+1, Oem2Ansi, ARRAYSIZE(Utf8)-1, 0, 0);
				//BUGBUG:???
				if (memcmp(Str, Oem2Ansi, min(nUtf8,nCmp)) == 0)
				{
					CP_Check = cp_UTF8;
					return true;
				}
			}
		}

		return false;
	};

	bool GetS(char *pBuf, DWORD nBufLen)
	{
		if (!pBuf || !nBufLen)
		{
			_ASSERTE(pBuf && nBufLen);
			return false;
		}
		const char *Start=pBuf;
		if (Pos >= Edge)
		{
			nStrLen = 0;
			return false;
		}
		while (Pos < Edge && *Pos!='\r' && *Pos!='\n' && (--nBufLen)) //Maks " && (--BufLen)"
			*pBuf++ = *Pos++;
		*pBuf=0;
		for (; Pos < Edge && (*Pos=='\r' || *Pos=='\n'); Pos++)
			;
		nStrLen = pBuf - Start;
		return true;
	};

	BOOL openArchive(int *Type)
	{
		//Handle=CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		//                  FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (Handle!=INVALID_HANDLE_VALUE)
		{
			MapHandle=CreateFileMapping(Handle, NULL, PAGE_READONLY, 0, 0, NULL);
			if (MapHandle)
			{
				Pos = Data = (char *)MapViewOfFile(MapHandle, FILE_MAP_READ, 0, 0, 0);
				if (Data)
				{
					*Type=0;
					*Prefix=0; nPrefixLen = 0;
					*Descrip=0;
					nSkipLen=0;
					Edge = Data + GetFileSize(Handle, NULL);

					PluginPanelItem* Item = (PluginPanelItem*)calloc(1,sizeof(PluginPanelItem));
					ArcItemInfo* Info = (ArcItemInfo*)calloc(1,sizeof(ArcItemInfo));
					bool lbRc = (getArcItem(Item, Info) == GETARC_SUCCESS);
					free(Item); free(Info);

					nStrLen = nSkipLen = 0;
					*Prefix=0; nPrefixLen = 0;
					Pos = Data;

					if (lbRc)
					{
						//static const char *MsgItems[]={ "", "In mode branch?"};
						//isBtanch=!Info.Message(Info.ModuleNumber, FMSG_MB_YESNO, 0, MsgItems, 2, 0);
						return true;
					}
				}
				CloseHandle(MapHandle); MapHandle = NULL;
			}
			CloseHandle(Handle);
		}
		Handle = NULL;
		return false;
	};

	int getArcItem(struct PluginPanelItem *Item, struct ArcItemInfo *Info)
	{
		static char *LabelHeader[]={ " ���", " Volume in" };
		static char *SerialHeader[]={ " ��������", " Volume Serial" };
		static char *DirHeader[]={ " ���������� �����", " Directory of" };
		static char *DirEndHeader[]={ "     ����� ������:", "     Total Files" };
		static char  DirID[]="<DIR>";
		static char  JunID[]="<JUNCTION>";
		//char* Buf = (char*)malloc(4096);
		bool First=false, FormatOk=false;
		size_t nLines=0;

		//if (isBtanch) nSkipLen=0;
		while (GetS(Buf,ARRAYSIZE(Buf)-2))
		{
			nLines++;
			if ((First = Compare(Buf, LabelHeader[0], true)) || Compare(Buf, LabelHeader[1]))
			{
				FormatOk = true;
				isRus = First;
				//if (isRus && strncmp(Buf, LabelHeader[0], lstrlenA(LabelHeader[0])))
				//{
				//	OemToCharBuff(Buf, Buf, lstrlenA(Buf));
				//}
				if (CP_Check == cp_OEM)
				{
					OemToCharBuff(Buf, Buf, lstrlenA(Buf));
					CP_Check = cp_ANSI;
				}

				*Prefix = 0; nPrefixLen = 0; nSkipLen = 0;

				// 26.11.2008 Maks - 
				//lstrcpy(Descrip, "  Label:");
				Descrip[0]=0;
				// 26.11.2008 Maks - ���������� ������ �� �����
				//if (isRus?*(Buf+20)=='�':*(Buf+19)=='i' ) lstrcat(Descrip, Buf+(isRus?31:21));
				char *lpszPtr = NULL;
				if (isRus?*(Buf+20)=='�':*(Buf+19)=='i' ) lpszPtr = Buf+(isRus?31:21);
				if (!lpszPtr) continue;
				while (*lpszPtr==' ') lpszPtr++;

				// 26.11.2008 Maks - ����� ������ ������� ���� ���� ����� �����
				if (!*lpszPtr) continue;
				memset(Item, 0, sizeof(PluginPanelItem));

				lstrcpy(Item->FindData.cFileName, "<VOLUMELABEL:");
				// ���������� ����� �����
				if (lstrlenA(lpszPtr)>=(MAX_PATH-20)) lpszPtr[MAX_PATH-20] = 0;
				lstrcat(Item->FindData.cFileName, lpszPtr);
				lstrcat(Item->FindData.cFileName, ">");

				Item->FindData.dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY;

				SYSTEMTIME st;
				GetSystemTime(&st);
				FILETIME ft;
				SystemTimeToFileTime(&st,&ft);
				LocalFileTimeToFileTime(&ft,&Item->FindData.ftLastWriteTime);

				lstrcpy(Info->Description, lpszPtr);
				return GETARC_SUCCESS;
			}
			if (Compare(Buf, SerialHeader[0]) || Compare(Buf, SerialHeader[1]))
			{
				FormatOk = true;
				/* // 26.11.2008 Maks - Volume Serial - �� ���������
				char temp[80];
				lstrcpy(temp, Descrip);
				lstrcpy(Descrip, Buf+(isRus?21:24));
				lstrcat(Descrip, temp);
				*/
				continue;
			}
			if (Compare(Buf, DirHeader[0]) || Compare(Buf, DirHeader[1]))
			{
				FormatOk = true;
				if (!nSkipLen)
					nSkipLen = nStrLen;
				else
				{
					lstrcpyn(Prefix, Buf+nSkipLen+(Buf[nSkipLen]=='\\'?1:0), ARRAYSIZE(Prefix)-2);
					nPrefixLen = lstrlen(Prefix);
					if (nPrefixLen && Prefix[nPrefixLen-1] != '\\')
					{
						Prefix[nPrefixLen++] = '\\';
						Prefix[nPrefixLen] = 0;
					}
				}
				continue;
			}

			if (!FormatOk)
			{
				// ���� ���� ��������� �� ����� � ������ 128 ������� - �������
				if (nLines > 128)
					break;
				else
					continue;
			}

			if ( (nStrLen==NAME_DISP+1 && *(Buf+NAME_DISP)=='.') ||
				(nStrLen==NAME_DISP+2 && *(Buf+NAME_DISP+1)=='.') )
				continue;
			if (Compare(Buf, DirEndHeader[0]) || Compare(Buf, DirEndHeader[1]))
			{
				nSkipLen=0; *Prefix=0; nPrefixLen = 0;
				continue;
			}
			//2008-11-30 ������ ���, ���� 
			if (nStrLen>NAME_DISP && *Buf!=' ' && *(Buf+1)!=' ')
			{
				memset(Item, 0, sizeof(PluginPanelItem));

				//BUGBUG: �������� �� LongUnicodeString
				if (*Prefix)
				{
					lstrcpy(Item->FindData.cFileName, Prefix);
				}
				if (CP_Check == cp_NonChecked)
				{
					//TODO: ���������, ����� ��� UTF8?
					int iUtf = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Buf+NAME_DISP, -1, Utf8, ARRAYSIZE(Utf8));
					//BUGBUG: ?? OEM ?
					int iWide = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, Buf+NAME_DISP, -1, Wide, ARRAYSIZE(Wide));
					if (iUtf > 0 && iWide > 0 && iUtf < iWide)
					{
						CP_Check = cp_UTF8;
					}
				}
				lstrcpyn(Item->FindData.cFileName+nPrefixLen, Buf+NAME_DISP, ARRAYSIZE(Item->FindData.cFileName)-nPrefixLen);

				if (Compare(Buf+ID_DIR_DISP, DirID) || Compare(Buf+ID_DIR_DISP, JunID))
					Item->FindData.dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY;
				else
				{
					FARINT64 nFileSize;
					nFileSize.i64=AtoI(Buf+SIZE_DISP, 16);
					Item->FindData.nFileSizeLow=nFileSize.Part.LowPart;
					Item->FindData.nFileSizeHigh=nFileSize.Part.HighPart;
				}

				SYSTEMTIME st;
				st.wDayOfWeek=st.wSecond=st.wMilliseconds=0;
				st.wDay=(WORD)AtoI(Buf+DATA_DISP, 2);
				st.wMonth=(WORD)AtoI(Buf+DATA_DISP+3, 2);
				st.wYear=(WORD)AtoI(Buf+DATA_DISP+6, 4);
				st.wHour=(WORD)AtoI(Buf+TIME_DISP, 2);
				st.wMinute=(WORD)AtoI(Buf+TIME_DISP+3, 2);
				//st.wYear+=st.wYear<50?2000:1900;
				FILETIME ft;
				SystemTimeToFileTime(&st,&ft);
				LocalFileTimeToFileTime(&ft,&Item->FindData.ftLastWriteTime);

				//lstrcpy(Info->Description, Descrip); -- Maks ��������� VolumeID � Description
				char *Temp = Pos; //�������� ��������� �������� ������ DIR ����� 
				if(GetS(Buf, 4095))
					if(Compare(Buf, " @"))
						lstrcpyn(Info->Description, Buf+2, 256);
					else
						Pos = Temp;

				return GETARC_SUCCESS;
			}
		}
		return GETARC_EOF;
	};

	BOOL closeArchive()
	{
		if (Data)
			UnmapViewOfFile(Data);
		Data = NULL;

		if (MapHandle && (MapHandle != INVALID_HANDLE_VALUE))
			CloseHandle(MapHandle);
		MapHandle = NULL;

		if (Handle && (Handle != INVALID_HANDLE_VALUE))
			CloseHandle(Handle);
		Handle = NULL;

		DirInfo* p = this;
		if (p == gpCur)
			gpCur = NULL;
		free(p);

		return true;
	};
};



BOOL WINAPI _export IsArchive(char *Name, const unsigned char *Data, int DataSize)
{
	_ASSERTE(gpCur!=NULL);
	static char *ID[]={ " ��� � ���������� ", " Volume in drive ", "Queued to drive " };
	BOOL lbRc = 
		gpCur->Compare((char *)Data, ID[0], true, min(lstrlen(ID[0]), DataSize)) ||
		gpCur->Compare((char *)Data, ID[1], false, min(lstrlen(ID[1]), DataSize)) ||
		gpCur->Compare((char *)Data, ID[2], false, min(lstrlen(ID[2]), DataSize))  ;
	return lbRc;
}
/*
void WINAPI SetFarInfo(const struct PluginStartupInfo *Info)
{
::Info=*Info;
}
*/
BOOL WINAPI _export OpenArchive(char *Name, int *Type)
{
	_ASSERTE(gpCur==NULL);
	gpCur = (DirInfo*)calloc(1,sizeof(DirInfo));
	gpCur->Handle=CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	return gpCur->openArchive(Type);
}

int WINAPI _export GetArcItem(struct PluginPanelItem *Item, struct ArcItemInfo *Info)
{
	if (!gpCur)
		return GETARC_EOF;
	return gpCur->getArcItem(Item, Info);
}

BOOL WINAPI _export CloseArchive(struct ArcInfo *Info)
{
	if (gpCur)
	{
		gpCur->closeArchive();
	}
	return TRUE;
}

//??��� ������� ���� �� ���������, ����� ������ �� ��������� � ���� MultiArc,
//�� ����� ���������� ������ ������ MultiArc :(
BOOL WINAPI _export GetFormatName(int Type, char *FormatName, char *DefaultExt)
{
	if (Type==0)
	{
		lstrcpy(FormatName, "DIR");
		lstrcpy(DefaultExt,"dir");
		return true;
	}
	return false;
}

BOOL WINAPI _export GetDefaultCommands(int Type,int Command,char *Dest)
{
	if (Type==0)
	{
		if (Command==0)
		{
			//extract (����������)
			lstrcpyA(Dest, "dir.fmt.exe x %%A %%L");
		}
		else if (Command==1)
		{
			//extract without path (���������� ��� �����)
			lstrcpyA(Dest, "dir.fmt.exe e %%A %%L");
		}
		else
		{
			*Dest=0;
		}
		return true;
	}
	return false;
}

#include "ModuleDef.h"

int nLastItemIndex = -1;

int MODULE_EXPORT OpenStorage(StorageOpenParams params, HANDLE *storage, StorageGeneralInfo* info)
{
	nLastItemIndex = -1;
	gpCur = (DirInfo*)calloc(1,sizeof(DirInfo));
	gpCur->Handle = CreateFileW(params.FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	int Type = 0;
	if (gpCur->openArchive(&Type))
	{
		*storage = (HANDLE)gpCur;
		lstrcpyW(info->Format, L"Dir");
		lstrcpyW(info->Compression, L"");
		lstrcpyW(info->Comment, L"");
		return SOR_SUCCESS;
	}
	gpCur->closeArchive();
	return SOR_INVALID_FILE;
}

void MODULE_EXPORT CloseStorage(HANDLE storage)
{
	DirInfo* p = (DirInfo*)storage;
	if (p)
		p->closeArchive();
}

int MODULE_EXPORT GetStorageItem(HANDLE storage, int item_index, StorageItemInfo* item_info)
{
	int iRc = GET_ITEM_NOMOREITEMS;
	_ASSERTE(item_index>nLastItemIndex);
	DirInfo* p = (DirInfo*)storage;
	nLastItemIndex = item_index;
	PluginPanelItem* Item = (PluginPanelItem*)calloc(1,sizeof(PluginPanelItem));
	ArcItemInfo* Info = (ArcItemInfo*)calloc(1,sizeof(ArcItemInfo));
	if (p->getArcItem(Item, Info) == GETARC_SUCCESS)
	{
		item_info->Attributes = Item->FindData.dwFileAttributes;
		LARGE_INTEGER li; li.LowPart = Item->FindData.nFileSizeLow; li.HighPart = Item->FindData.nFileSizeHigh;
		item_info->Size = li.QuadPart;
		item_info->CreationTime = Item->FindData.ftLastWriteTime;
		item_info->ModificationTime = Item->FindData.ftLastWriteTime;
		MultiByteToWideChar((p->CP_Check == p->cp_UTF8) ? CP_UTF8 : CP_OEMCP, 0, Item->FindData.cFileName, -1, item_info->Path, ARRAYSIZE(item_info->Path));
		iRc = GET_ITEM_OK;
	}
	free(Item); free(Info);
	return iRc;
}

int MODULE_EXPORT ExtractItem(HANDLE storage, ExtractOperationParams params)
{
	return SER_ERROR_SYSTEM;
}

//////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////

int MODULE_EXPORT LoadSubModule(ModuleLoadParameters* LoadParams)
{
	LoadParams->ModuleVersion = MAKEMODULEVERSION(1, 0);
	LoadParams->ApiVersion = ACTUAL_API_VERSION;
	LoadParams->OpenStorage = OpenStorage;
	LoadParams->CloseStorage = CloseStorage;
	LoadParams->GetItem = GetStorageItem;
	LoadParams->ExtractItem = ExtractItem;

	return TRUE;
}

void MODULE_EXPORT UnloadSubModule()
{
}
