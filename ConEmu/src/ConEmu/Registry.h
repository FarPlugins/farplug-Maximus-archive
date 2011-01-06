
/*
Copyright (c) 2009-2010 Maximus5
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

#ifndef __GNUC__
#include <msxml.h>
#endif

#ifndef LPCBYTE
#define LPCBYTE const BYTE*
#endif

class SettingsBase
{
public:
	virtual bool OpenKey(const wchar_t *regPath, uint access) = 0;
	virtual void CloseKey() = 0;

	virtual bool Load(const wchar_t *regName, wchar_t **value) = 0;
	virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize) = 0;
	virtual bool Load(const wchar_t *regName, wchar_t *value, int maxLen) = 0; // ����, ��� �������� ���������� ���� �������
	/*template <class T> bool Load(const wchar_t *regName, T &value)
	{
		DWORD len = sizeof(T);
		bool lbRc = Load(regName, (LPBYTE)&(value), len);
		return lbRc;
	}*/
	bool Load(const wchar_t *regName, char &value) { return Load(regName, (LPBYTE)&value, 1); }
	bool Load(const wchar_t *regName, bool &value) { return Load(regName, (LPBYTE)&value, 1); }
	bool Load(const wchar_t *regName, BYTE &value) { return Load(regName, (LPBYTE)&value, 1); }
	bool Load(const wchar_t *regName, DWORD &value) { return Load(regName, (LPBYTE)&value, 4); }
	bool Load(const wchar_t *regName, LONG &value) { return Load(regName, (LPBYTE)&value, 4); }
	bool Load(const wchar_t *regName, int &value) { return Load(regName, (LPBYTE)&value, 4); }
	bool Load(const wchar_t *regName, RECT &value) { return Load(regName, (LPBYTE)&value, sizeof(value)); }
	
	virtual void Delete(const wchar_t *regName) = 0;
	
	virtual void Save(const wchar_t *regName, const wchar_t *value) = 0; // value = _T(""); // ���� ��� ������ � NULL
	virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize) = 0;
	
	void SaveMSZ(const wchar_t *regName, const wchar_t *value, DWORD nSize) // size in BYTES!!!
	{
		if (!value || !*value)
			Delete(regName);
		else
			Save(regName, (LPBYTE)value, REG_MULTI_SZ, nSize); 
	}
	void Save(const wchar_t *regName, wchar_t *value)
	{	// ����, ����� ���� � template �� ����������
		Save(regName, (const wchar_t*)value);
	}
	// bool, dword, rect, etc.
	template <class T> void Save(const wchar_t *regName, T value)
	{
		DWORD len = sizeof(T);
		Save(regName, (LPBYTE)(&value), (len == 4) ? REG_DWORD : REG_BINARY, len); 
	}
	
public:
	wchar_t Type[16];

	SettingsBase() {Type[0] = 0;};
	virtual ~SettingsBase() {};
};

class SettingsRegistry : public SettingsBase
{
public:
	HKEY regMy;

	bool OpenKey(HKEY inHKEY, const wchar_t *regPath, uint access);
	virtual bool OpenKey(const wchar_t *regPath, uint access);
	virtual void CloseKey();

	virtual bool Load(const wchar_t *regName, wchar_t **value);
	virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize);
	virtual bool Load(const wchar_t *regName, wchar_t *value, int maxLen); // ����, ��� �������� ���������� ���� �������
	
	virtual void Delete(const wchar_t *regName);
	
	virtual void Save(const wchar_t *regName, const wchar_t *value); // value = _T(""); // ���� ��� ������ � NULL
	virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize);

public:
	SettingsRegistry();
	virtual ~SettingsRegistry();
};

#ifndef __GNUC__
class SettingsXML : public SettingsBase
{
protected:
	IXMLDOMDocument* mp_File;
	IXMLDOMNode* mp_Key;
	bool mb_Modified;
	int mi_Level;
	//wchar_t ms_LevelPrefix[64];
	//BSTR mbs_LevelPrefix, mbs_LevelSuffix;
public:
	bool IsXmlAllowed();
	virtual bool OpenKey(const wchar_t *regPath, uint access);
	virtual void CloseKey();

	virtual bool Load(const wchar_t *regName, wchar_t **value);
	virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize);
	virtual bool Load(const wchar_t *regName, wchar_t *value, int maxLen); // ����, ��� �������� ���������� ���� �������
	
	virtual void Delete(const wchar_t *regName);
	
	virtual void Save(const wchar_t *regName, const wchar_t *value); // value = _T(""); // ���� ��� ������ � NULL
	virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize);
	
protected:
	IXMLDOMNode* FindItem(IXMLDOMNode* apFrom, const wchar_t* asType, const wchar_t* asName, bool abAllowCreate);
	bool SetAttr(IXMLDOMNode* apNode, const wchar_t* asName, const wchar_t* asValue);
	bool SetAttr(IXMLDOMNode* apNode, IXMLDOMNamedNodeMap* apAttrs, const wchar_t* asName, const wchar_t* asValue);
	BSTR GetAttr(IXMLDOMNode* apNode, const wchar_t* asName);
	BSTR GetAttr(IXMLDOMNode* apNode, IXMLDOMNamedNodeMap* apAttrs, const wchar_t* asName);

	void AppendIndent(IXMLDOMNode* apFrom, int nLevel);
	void AppendNewLine(IXMLDOMNode* apFrom);
	void AppendText(IXMLDOMNode* apFrom, BSTR asText);
	
public:
	SettingsXML();
	virtual ~SettingsXML();
};
#endif
