#pragma once

/*
stddlg.hpp

���� ������ ����������� ��������
*/
/*
Copyright � 1996 Eugene Roshal
Copyright � 2000 Far Group
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

/*
  ������� GetSearchReplaceString ������� ������ ������ ��� ������, ���������
  �� ������������ ������ � � ������ ��������� ���������� ������� ����������
  TRUE.
  ���������:
    IsReplaceMode
      true  - ���� ����� ��������
      false - ���� ����� ������

    SearchStr
      ������ ������.
      ��������� ��������� ������� ��������� � ��� ��.

    ReplaceStr,
      ������ ������.
      ��������� ��������� ������� ��������� � ��� ��.
      ��� ������, ���� IsReplaceMode=FALSE ����� ���� ����� nullptr

    TextHistoryName
      ��� ������� ������ ������.
      ���� ����������� � nullptr, �� �� ���������
      ����������� �������� "SearchText"
      ���� ����������� � ������ ������, �� ������� ������� �� �����

    ReplaceHistoryName
      ��� ������� ������ ������.
      ���� ����������� � nullptr, �� �� ���������
      ����������� �������� "ReplaceText"
      ���� ����������� � ������ ������, �� ������� ������� �� �����

    Case
      ������ �� ����������, ����������� �� �������� ����� "Case sensitive"

    WholeWords
      ������ �� ����������, ����������� �� �������� ����� "Whole words"

    Reverse
      ������ �� ����������, ����������� �� �������� ����� "Reverse search"

    SelectFound
      ������ �� ����������, ����������� �� �������� ����� "Select found"

    Regexp
      ������ �� ����������, ����������� �� �������� ����� "Regular expressions"

    HelpTopic
      ��� ���� ������.
      ���� nullptr ��� ������ ������ - ���� ������ �� �����������.

  ������������ ��������:
  0 - ������������ ��������� �� ������� (Esc)
    1  - ������������ ���������� ���� ���������
    2 - ������ ����� ���� ���������

*/
int GetSearchReplaceString(
    bool IsReplaceMode,
    string& SearchStr,
    string& ReplaceStr,
    const wchar_t *TextHistoryName,
    const wchar_t *ReplaceHistoryName,
    bool& Case,
    bool& WholeWords,
    bool& Reverse,
    bool& SelectFound,
    bool& Regexp,
    const wchar_t *HelpTopic=nullptr);

int GetString(
    const wchar_t *Title,
    const wchar_t *SubTitle,
    const wchar_t *HistoryName,
    const wchar_t *SrcText,
    string &strDestText,
    const wchar_t *HelpTopic = nullptr,
    DWORD Flags = 0,
    int *CheckBoxValue = nullptr,
    const wchar_t *CheckBoxText = nullptr,
    class Plugin* PluginNumber = nullptr,
    const GUID* Id = nullptr
);

// ��� ������� GetNameAndPassword()
enum FlagsNameAndPassword
{
	GNP_USELAST      = 0x00000001UL, // ������������ ��������� ��������� ������
};

int GetNameAndPassword(const wchar_t *Title,string &strUserName, string &strPassword, const wchar_t *HelpTopic,DWORD Flags);

int OperationFailed(const string& Object, LNGID Title, const wchar_t* Description, bool AllowSkip = true);
