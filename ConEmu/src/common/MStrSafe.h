
#pragma once

#define STRSAFE_NO_DEPRECATE

//#define MSTRSAFE_NO_DEPRECATE

#ifdef __GNUC__
	#define StringCchCopyA(d,n,s) lstrcpyA(d,s)
	#define StringCchCopyW(d,n,s) lstrcpyW(d,s)
	#define StringCchCatA(d,n,s) lstrcatA(d,s)
	#define StringCchCatW(d,n,s) lstrcatW(d,s)
	#define StringCchCopyNA(d,n,s,l) lstrcpynA(d,s,l)
	#define StringCchCopyNW(d,n,s,l) lstrcpynW(d,s,l)
	#ifndef _ASSERTE
		#define _ASSERTE(x) // {bool b = (x);}
	#endif
#else
	#include <strsafe.h>
#endif



#ifndef STRSAFE_NO_DEPRECATE

// First all the names that are a/w variants (or shouldn't be #defined by now anyway)
#pragma deprecated(lstrcpyA)
#pragma deprecated(lstrcpyW)
#pragma deprecated(lstrcatA)
#pragma deprecated(lstrcatW)
#ifdef _DEBUG
#pragma deprecated(wsprintfA)
#pragma deprecated(wsprintfW)
#endif

#pragma deprecated(StrCpyW)
#pragma deprecated(StrCatW)
#pragma deprecated(StrNCatA)
#pragma deprecated(StrNCatW)
#pragma deprecated(StrCatNA)
#pragma deprecated(StrCatNW)
#pragma deprecated(wvsprintfA)
#pragma deprecated(wvsprintfW)

#pragma deprecated(strcpy)
#pragma deprecated(wcscpy)
#pragma deprecated(strcat)
#pragma deprecated(wcscat)
#pragma deprecated(sprintf)
#pragma deprecated(swprintf)
#pragma deprecated(vsprintf)
#pragma deprecated(vswprintf)
#pragma deprecated(_snprintf)
#pragma deprecated(_snwprintf)
#pragma deprecated(_vsnprintf)
#pragma deprecated(_vsnwprintf)
#pragma deprecated(gets)
#pragma deprecated(_getws)

// Then all the windows.h names
#undef lstrcpy
#undef lstrcat
#undef wsprintf
#undef wvsprintf
#pragma deprecated(lstrcpy)
#pragma deprecated(lstrcat)
#pragma deprecated(wsprintf)
#pragma deprecated(wvsprintf)
#ifdef UNICODE
#define lstrcpy    lstrcpyW
#define lstrcat    lstrcatW
#define wsprintf   wsprintfW
#define wvsprintf  wvsprintfW
#else
#define lstrcpy    lstrcpyA
#define lstrcat    lstrcatA
#define wsprintf   wsprintfA
#define wvsprintf  wvsprintfA
#endif

// Then the shlwapi names
#undef StrCpyA
#undef StrCpy
#undef StrCatA
#undef StrCat
#undef StrNCat
#undef StrCatN
#pragma deprecated(StrCpyA)
#pragma deprecated(StrCatA)
#pragma deprecated(StrCatN)
#pragma deprecated(StrCpy)
#pragma deprecated(StrCat)
#pragma deprecated(StrNCat)
#define StrCpyA lstrcpyA
#define StrCatA lstrcatA
#define StrCatN StrNCat
#ifdef UNICODE
#define StrCpy  StrCpyW
#define StrCat  StrCatW
#define StrNCat StrNCatW
#else
#define StrCpy  lstrcpyA
#define StrCat  lstrcatA
#define StrNCat StrNCatA
#endif

#undef _tcscpy
#undef _ftcscpy
#undef _tcscat
#undef _ftcscat
#undef _stprintf
#undef _sntprintf
#undef _vstprintf
#undef _vsntprintf
#undef _getts
#pragma deprecated(_tcscpy)
#pragma deprecated(_ftcscpy)
#pragma deprecated(_tcscat)
#pragma deprecated(_ftcscat)
#pragma deprecated(_stprintf)
#pragma deprecated(_sntprintf)
#pragma deprecated(_vstprintf)
#pragma deprecated(_vsntprintf)
#pragma deprecated(_getts)
#ifdef _UNICODE
#define _tcscpy     wcscpy
#define _ftcscpy    wcscpy
#define _tcscat     wcscat
#define _ftcscat    wcscat
#define _stprintf   swprintf
#define _sntprintf  _snwprintf
#define _vstprintf  vswprintf
#define _vsntprintf _vsnwprintf
#define _getts      _getws
#else
#define _tcscpy     strcpy
#define _ftcscpy    strcpy
#define _tcscat     strcat
#define _ftcscat    strcat
#define _stprintf   sprintf
#define _sntprintf  _snprintf
#define _vstprintf  vsprintf
#define _vsntprintf _vsnprintf
#define _getts      gets
#endif

#endif // MSTRSAFE_NO_DEPRECATE


#ifndef _DEBUG
#undef StringCchPrintf
#define StringCchPrintf    dont_use_StringCchPrintf_in_release;

#undef StringCchPrintfA
#define StringCchPrintfA    dont_use_StringCchPrintfA_in_release;

#undef StringCchPrintfW
#define StringCchPrintfW    dont_use_StringCchPrintfW_in_release;
#endif


#ifdef _DEBUG
#define SKIPLEN(l) (l),
#else
#define SKIPLEN(l)
#endif

#ifdef _DEBUG
// ������ ��� �������, �.�. StringCchVPrintf �������� vsprintf, ������� �� ��������� � ������ ��������.
inline int swprintf_c(wchar_t* Buffer, INT_PTR size, const wchar_t *Format, ...)
{
	if (size < 0)
		DebugBreak();
	va_list argList;
	va_start(argList, Format);
	int nRc;
	nRc = StringCchVPrintfW(Buffer, size, Format, argList);
	return nRc;
};
inline int sprintf_c(char* Buffer, INT_PTR size, const char *Format, ...)
{
	if (size < 0)
		DebugBreak();
	va_list argList;
	va_start(argList, Format);
	int nRc;
	nRc = StringCchVPrintfA(Buffer, size, Format, argList);
	return nRc;
};

#define _wsprintf  swprintf_c
#define _wsprintfA sprintf_c
#else
#define _wsprintf  wsprintfW
#define _wsprintfA wsprintfA
#endif


//template <size_t size>
//int swprintf_c(wchar_t (&Buffer)[size], const wchar_t *Format, ...)
//{
//	va_list argList;
//	va_start(argList, Format);
//	int nRc;
//#ifdef _DEBUG
//	nRc = StringCchVPrintfW(Buffer, size, Format, argList);
//#else
//	nRc = wvsprintf(Buffer, Format, argList);
//#endif
//	return nRc;
//};
//template <size_t size>
//int swprintf_add(wchar_t* Buffer, wchar_t (&BufferStart)[size], const wchar_t *Format, ...)
//{
//	va_list argList;
//	va_start(argList, Format);
//	size_t SizeLeft = size - (Buffer - BufferStart);
//	if (SizeLeft > size)
//	{
//		_ASSERTE(SizeLeft <= size);
//		return E_POINTER;
//	}
//	int nRc;
//#ifdef _DEBUG
//	nRc = StringCchVPrintfW(Buffer, SizeLeft, Format, argList);
//#else
//	nRc = wvsprintf(Buffer, Format, argList);
//#endif
//	return nRc;
//};
//template <size_t size>
//int swprintf_add(int Shift, wchar_t (&BufferStart)[size], const wchar_t *Format, ...)
//{
//	va_list argList;
//	va_start(argList, Format);
//	size_t SizeLeft = size - Shift;
//	if (SizeLeft > size)
//	{
//		_ASSERTE(SizeLeft <= size);
//		return E_POINTER;
//	}
//	int nRc;
//#ifdef _DEBUG
//	nRc = StringCchVPrintfW(BufferStart+Shift, SizeLeft, Format, argList);
//#else
//	nRc = wvsprintf(BufferStart+Shift, Format, argList);
//#endif
//	return nRc;
//};
//template <size_t size>
//int swprintf_cat(wchar_t (&BufferStart)[size], const wchar_t *Format, ...)
//{
//	va_list argList;
//	va_start(argList, Format);
//	int Shift = lstrlenW(BufferStart);
//	size_t SizeLeft = size - Shift;
//	if (SizeLeft > size)
//	{
//		_ASSERTE(SizeLeft <= size);
//		return E_POINTER;
//	}
//	int nRc;
//#ifdef _DEBUG
//	nRc = StringCchVPrintfW(BufferStart+Shift, SizeLeft, Format, argList);
//#else
//	nRc = wvsprintf(BufferStart+Shift, Format, argList);
//#endif
//	return nRc;
//};

#ifdef __GNUC__
inline int wcscpy_add(wchar_t* Buffer, wchar_t* /*BufferStart*/, const wchar_t *Str)
{
	lstrcpyW(Buffer, Str);
	return S_OK;
};
inline int wcscpy_add(int Shift, wchar_t* BufferStart, const wchar_t *Str)
{
	lstrcpyW(BufferStart+Shift, Str);
	return S_OK;
};
inline int wcscat_add(wchar_t* Buffer, wchar_t* /*BufferStart*/, const wchar_t *Str)
{
	lstrcatW(Buffer, Str);
	return S_OK;
};
inline int wcscat_add(int Shift, wchar_t* BufferStart, const wchar_t* Str)
{
	lstrcatW(BufferStart+Shift, Str);
	return S_OK;
};
inline int wcscpy_c(wchar_t* Dst, const wchar_t *Src)
{
	lstrcpyW(Dst, Src);
	return S_OK;
}
inline int wcscat_c(wchar_t* Dst, const wchar_t *Src)
{
	lstrcatW(Dst, Src);
	return S_OK;
}

#else

template <size_t size>
int wcscpy_add(wchar_t* Buffer, wchar_t (&BufferStart)[size], const wchar_t *Str)
{
	size_t SizeLeft = size - (Buffer - BufferStart);

	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}

	int nRc = StringCchCopyW(Buffer, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscpy_add(int Shift, wchar_t (&BufferStart)[size], const wchar_t* Str)
{
	size_t SizeLeft = size - Shift;

	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}

	int nRc = StringCchCopyW(BufferStart+Shift, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscat_add(wchar_t* Buffer, wchar_t (&BufferStart)[size], const wchar_t* Str)
{
	size_t SizeLeft = size - (Buffer - BufferStart);

	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}

	int nRc = StringCchCatW(Buffer, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscat_add(int Shift, wchar_t (&BufferStart)[size], const wchar_t *Str)
{
	size_t SizeLeft = size - Shift;

	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}

	int nRc = StringCchCatW(BufferStart+Shift, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscpy_c(wchar_t (&Dst)[size], const wchar_t *Src)
{
	int nRc = StringCchCopyW(Dst, size, Src);
	return nRc;
}
template <size_t size>
int wcscat_c(wchar_t (&Dst)[size], const wchar_t *Src)
{
	int nRc = StringCchCatW(Dst, size, Src);
	return nRc;
}
#endif
//#undef _wcscpy_c
//inline errno_t _wcscpy_c(wchar_t *Dst, size_t size, const wchar_t *Src)
//{
//	return StringCchCopyW(Dst, size, Src);
//}
//#undef _wcscat_c
//inline errno_t _wcscat_c(wchar_t *Dst, size_t size, const wchar_t *Src)
//{
//	return StringCchCatW(Dst, size, Src);
//}

#define _wcscpy_c(Dst,cchDest,Src) StringCchCopyW(Dst, cchDest, Src)
#define _wcscpyn_c(Dst,cchDest,Src,cchSrc) { _ASSERTE(((INT_PTR)cchDest)>=((INT_PTR)cchSrc)); StringCchCopyNW(Dst, cchDest, Src, cchSrc); }
#define _wcscat_c(Dst,cchDest,Src) StringCchCatW(Dst, cchDest, Src)
#define _wcscatn_c(Dst,cchDest,Src,cchSrc) { \
			_ASSERTE(((INT_PTR)cchDest)>=((INT_PTR)cchSrc)); size_t nDestLen = wcslen(Dst); \
			if (((INT_PTR)(nDestLen+1)) >= ((INT_PTR)cchDest)) DebugBreak(); else StringCchCopyNW(Dst+nDestLen,cchDest-nDestLen,Src,cchSrc); \
		}
#define _strcpy_c(Dst,cchDest,Src) StringCchCopyA(Dst, cchDest, Src)
#define _strcat_c(Dst,cchDest,Src) StringCchCatA(Dst, cchDest, Src)
#define _strcpyn_c(Dst,cchDest,Src,cchSrc) { _ASSERTE(((INT_PTR)cchDest)>=((INT_PTR)cchSrc)); StringCchCopyNA(Dst, cchDest, Src, cchSrc); }