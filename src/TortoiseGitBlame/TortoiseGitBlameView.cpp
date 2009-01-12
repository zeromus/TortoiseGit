// TortoiseGitBlame - a Viewer for Git Blames

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// CTortoiseGitBlameView.cpp : implementation of the CTortoiseGitBlameView class
//

#include "stdafx.h"
#include "TortoiseGitBlame.h"

#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTortoiseGitBlameView

IMPLEMENT_DYNCREATE(CTortoiseGitBlameView, CView)

BEGIN_MESSAGE_MAP(CTortoiseGitBlameView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CTortoiseGitBlameView::OnFilePrintPreview)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(SCN_PAINTED,0,OnSciPainted)
END_MESSAGE_MAP()

// CTortoiseGitBlameView construction/destruction

CTortoiseGitBlameView::CTortoiseGitBlameView()
{
	// TODO: add construction code here
	hInstance = 0;
	hResource = 0;
	currentDialog = 0;
	wMain = 0;
	m_wEditor = 0;
	wLocator = 0;

	m_font = 0;
	m_italicfont = 0;
	m_blamewidth = 100;
	m_revwidth = 0;
	m_datewidth = 0;
	m_authorwidth = 0;
	m_pathwidth = 0;
	m_linewidth = 0;

	m_windowcolor = ::GetSysColor(COLOR_WINDOW);
	m_textcolor = ::GetSysColor(COLOR_WINDOWTEXT);
	m_texthighlightcolor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_mouserevcolor = InterColor(m_windowcolor, m_textcolor, 20);
	m_mouseauthorcolor = InterColor(m_windowcolor, m_textcolor, 10);
	m_selectedrevcolor = ::GetSysColor(COLOR_HIGHLIGHT);
	m_selectedauthorcolor = InterColor(m_selectedrevcolor, m_texthighlightcolor, 35);
	m_mouserev = -2;

	m_selectedrev = -1;
	m_selectedorigrev = -1;
	m_SelectedLine = -1;
	m_directPointer = 0;
	m_directFunction = 0;

	m_lowestrev = LONG_MAX;
	m_highestrev = 0;
	m_colorage = true;

	m_bShowLine=true;
}

CTortoiseGitBlameView::~CTortoiseGitBlameView()
{
	if (m_font)
		DeleteObject(m_font);
	if (m_italicfont)
		DeleteObject(m_italicfont);
}


int CTortoiseGitBlameView::OnCreate(LPCREATESTRUCT lpcs)
{

	CRect rect,rect1;
	this->GetWindowRect(&rect1);
	rect.left=m_blamewidth;
	rect.right=rect.Width();
	rect.top=0;
	rect.bottom=rect.Height();
	BOOL b=m_TextView.Create(_T("Scintilla"),_T("source"),0,rect,this,0,0);
	m_TextView.Init(0);
	m_TextView.ShowWindow( SW_SHOW);
	//m_TextView.InsertText(_T("Abdadfasdf"));
	m_wEditor = m_TextView.m_hWnd;
	CreateFont();
	InitialiseEditor();
	return CView::OnCreate(lpcs);
}

void CTortoiseGitBlameView::OnSize(UINT nType,int cx, int cy)
{

	CRect rect;
	rect.left=m_blamewidth;
	rect.right=cx;
	rect.top=0;
	rect.bottom=cy;

	m_TextView.MoveWindow(&rect);

}
BOOL CTortoiseGitBlameView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CTortoiseGitBlameView drawing

void CTortoiseGitBlameView::OnDraw(CDC* /*pDC*/)
{
	CTortoiseGitBlameDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	DrawBlame(this->GetDC()->m_hDC);
	// TODO: add draw code for native data here
}


// CTortoiseGitBlameView printing


void CTortoiseGitBlameView::OnFilePrintPreview()
{
	AFXPrintPreview(this);
}

BOOL CTortoiseGitBlameView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTortoiseGitBlameView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTortoiseGitBlameView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CTortoiseGitBlameView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CTortoiseGitBlameView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}


// CTortoiseGitBlameView diagnostics

#ifdef _DEBUG
void CTortoiseGitBlameView::AssertValid() const
{
	CView::AssertValid();
}

void CTortoiseGitBlameView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTortoiseGitBlameDoc* CTortoiseGitBlameView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTortoiseGitBlameDoc)));
	return (CTortoiseGitBlameDoc*)m_pDocument;
}
#endif //_DEBUG


// CTortoiseGitBlameView message handlers
CString CTortoiseGitBlameView::GetAppDirectory()
{
	CString path;
	DWORD len = 0;
	DWORD bufferlen = MAX_PATH;		// MAX_PATH is not the limit here!
	do 
	{
		bufferlen += MAX_PATH;		// MAX_PATH is not the limit here!
		TCHAR * pBuf = new TCHAR[bufferlen];
		len = GetModuleFileName(NULL, pBuf, bufferlen);	
		path = CString(pBuf, len);
		delete [] pBuf;
	} while(len == bufferlen);

	path = path.Left(path.ReverseFind(_T('\\')));
	//path = path.substr(0, path.rfind('\\') + 1);

	return path;
}

// Return a color which is interpolated between c1 and c2.
// Slider controls the relative proportions as a percentage:
// Slider = 0 	represents pure c1
// Slider = 50	represents equal mixture
// Slider = 100	represents pure c2
COLORREF CTortoiseGitBlameView::InterColor(COLORREF c1, COLORREF c2, int Slider)
{
	int r, g, b;
	
	// Limit Slider to 0..100% range
	if (Slider < 0)
		Slider = 0;
	if (Slider > 100)
		Slider = 100;
	
	// The color components have to be treated individually.
	r = (GetRValue(c2) * Slider + GetRValue(c1) * (100 - Slider)) / 100;
	g = (GetGValue(c2) * Slider + GetGValue(c1) * (100 - Slider)) / 100;
	b = (GetBValue(c2) * Slider + GetBValue(c1) * (100 - Slider)) / 100;
	
	return RGB(r, g, b);
}

LRESULT CTortoiseGitBlameView::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_directFunction)
	{
		return ((SciFnDirect) m_directFunction)(m_directPointer, Msg, wParam, lParam);
	}
	return ::SendMessage(m_wEditor, Msg, wParam, lParam);	
}

void CTortoiseGitBlameView::GetRange(int start, int end, char *text) 
{
#if 0
	TEXTRANGE tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;

	SendMessage(m_wEditor, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
#endif
}

void CTortoiseGitBlameView::SetTitle() 
{
#if 0
	char title[MAX_PATH + 100];
	strcpy_s(title, MAX_PATH + 100, szTitle);
	strcat_s(title, MAX_PATH + 100, " - ");
	strcat_s(title, MAX_PATH + 100, szViewtitle);
	::SetWindowText(wMain, title);
#endif
}

BOOL CTortoiseGitBlameView::OpenLogFile(const char *fileName)
{
#if 0
	char logmsgbuf[10000+1];
	FILE * File;
	fopen_s(&File, fileName, "rb");
	if (File == 0)
	{
		return FALSE;
	}
	LONG rev = 0;
	CString msg;
	int slength = 0;
	int reallength = 0;
	size_t len = 0;
	wchar_t wbuf[MAX_LOG_LENGTH+6];
	for (;;)
	{
		len = fread(&rev, sizeof(LONG), 1, File);
		if (len == 0)
		{
			fclose(File);
            InitSize();
			return TRUE;
		}
		len = fread(&slength, sizeof(int), 1, File);
		if (len == 0)
		{
			fclose(File);
            InitSize();
			return FALSE;
		}
		if (slength > MAX_LOG_LENGTH)
		{
			reallength = slength;
			slength = MAX_LOG_LENGTH;
		}
		else
			reallength = 0;
		len = fread(logmsgbuf, sizeof(char), slength, File);
		if (len < (size_t)slength)
		{
			fclose(File);
            InitSize();
			return FALSE;
		}
		msg = CString(logmsgbuf, slength);
		if (reallength)
		{
			fseek(File, reallength-MAX_LOG_LENGTH, SEEK_CUR);
			msg = msg + _T("\n...");
		}
		int len2 = ::MultiByteToWideChar(CP_UTF8, NULL, msg.c_str(), min(msg.size(), MAX_LOG_LENGTH+5), wbuf, MAX_LOG_LENGTH+5);
		wbuf[len2] = 0;
		len2 = ::WideCharToMultiByte(CP_ACP, NULL, wbuf, len2, logmsgbuf, MAX_LOG_LENGTH+5, NULL, NULL);
		logmsgbuf[len2] = 0;
		msg = CString(logmsgbuf);
		logmessages[rev] = msg;
	}
#endif
	return TRUE;
}

BOOL CTortoiseGitBlameView::OpenFile(const char *fileName) 
{
#if 0
	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SetTitle();
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);
	::ShowWindow(m_wEditor, SW_HIDE);
	std::ifstream File;
	File.open(fileName);
	if (!File.good())
	{
		return FALSE;
	}
	char line[100*1024];
	char * lineptr = NULL;
	char * trimptr = NULL;
	//ignore the first two lines, they're of no interest to us
	File.getline(line, sizeof(line)/sizeof(char));
	File.getline(line, sizeof(line)/sizeof(char));
	m_lowestrev = LONG_MAX;
	m_highestrev = 0;
	bool bUTF8 = true;
	do
	{
		File.getline(line, sizeof(line)/sizeof(TCHAR));
		if (File.gcount()>139)
		{
			mergelines.push_back((line[0] != ' '));
			lineptr = &line[9];
			long rev = _ttol(lineptr);
			revs.push_back(rev);
			m_lowestrev = min(m_lowestrev, rev);
			m_highestrev = max(m_highestrev, rev);
			lineptr += 7;
			rev = _ttol(lineptr);
			origrevs.push_back(rev);
			lineptr += 7;
			dates.push_back(CString(lineptr, 30));
			lineptr += 31;
			// unfortunately, the 'path' entry can be longer than the 60 chars
			// we made the column. We therefore have to step through the path
			// string until we find a space
			trimptr = lineptr;
			do 
			{
				// TODO: how can we deal with the situation where the path has
				// a space in it, but the space is after the 60 chars reserved
				// for it?
				// The only way to deal with that would be to use a custom
				// binary format for the blame file.
				trimptr++;
				trimptr = _tcschr(trimptr, ' ');
			} while ((trimptr)&&(trimptr+1 < lineptr+61));
			if (trimptr)
				*trimptr = 0;
			else
				trimptr = lineptr;
			paths.push_back(CString(lineptr));
			if (trimptr+1 < lineptr+61)
				lineptr +=61;
			else
				lineptr = (trimptr+1);
			trimptr = lineptr+30;
			while ((*trimptr == ' ')&&(trimptr > lineptr))
				trimptr--;
			*(trimptr+1) = 0;
			authors.push_back(CString(lineptr));
			lineptr += 31;
			// in case we find an UTF8 BOM at the beginning of the line, we remove it
			if (((unsigned char)lineptr[0] == 0xEF)&&((unsigned char)lineptr[1] == 0xBB)&&((unsigned char)lineptr[2] == 0xBF))
			{
				lineptr += 3;
			}
			if (((unsigned char)lineptr[0] == 0xBB)&&((unsigned char)lineptr[1] == 0xEF)&&((unsigned char)lineptr[2] == 0xBF))
			{
				lineptr += 3;
			}
			// check each line for illegal utf8 sequences. If one is found, we treat
			// the file as ASCII, otherwise we assume an UTF8 file.
			char * utf8CheckBuf = lineptr;
			while ((bUTF8)&&(*utf8CheckBuf))
			{
				if ((*utf8CheckBuf == 0xC0)||(*utf8CheckBuf == 0xC1)||(*utf8CheckBuf >= 0xF5))
				{
					bUTF8 = false;
					break;
				}
				if ((*utf8CheckBuf & 0xE0)==0xC0)
				{
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
				}
				if ((*utf8CheckBuf & 0xF0)==0xE0)
				{
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
				}
				if ((*utf8CheckBuf & 0xF8)==0xF0)
				{
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
				}

				utf8CheckBuf++;
			}
			SendEditor(SCI_ADDTEXT, _tcslen(lineptr), reinterpret_cast<LPARAM>(static_cast<char *>(lineptr)));
			SendEditor(SCI_ADDTEXT, 2, (LPARAM)_T("\r\n"));
		}
	} while (File.gcount() > 0);

	if (bUTF8)
		SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);

	SendEditor(SCI_SETUNDOCOLLECTION, 1);
	::SetFocus(m_wEditor);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETSCROLLWIDTHTRACKING, TRUE);
	SendEditor(SCI_SETREADONLY, TRUE);

	//check which lexer to use, depending on the filetype
	SetupLexer(fileName);
	::ShowWindow(m_wEditor, SW_SHOW);
	m_blamewidth = 0;
	::InvalidateRect(wMain, NULL, TRUE);
	RECT rc;
	GetWindowRect(wMain, &rc);
	SetWindowPos(wMain, 0, rc.left, rc.top, rc.right-rc.left-1, rc.bottom - rc.top, 0);
#endif
	return TRUE;
}

void CTortoiseGitBlameView::SetAStyle(int style, COLORREF fore, COLORREF back, int size, CString *face) 
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face) 
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(this->m_TextView.StringForControl(*face).GetBuffer()));
}

void CTortoiseGitBlameView::InitialiseEditor() 
{

	m_directFunction = ::SendMessage(m_wEditor, SCI_GETDIRECTFUNCTION, 0, 0);
	m_directPointer = ::SendMessage(m_wEditor, SCI_GETDIRECTPOINTER, 0, 0);
	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT, 
			  black, 
			  white, 
			(DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10), 
			&CString(((stdstring)CRegStdString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New"))).c_str())
			);
	SendEditor(SCI_SETTABWIDTH, (DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\BlameTabSize"), 4));
	SendEditor(SCI_SETREADONLY, TRUE);
	LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)this->m_TextView.StringForControl(_T("_99999")).GetBuffer());
	if (m_bShowLine)
		SendEditor(SCI_SETMARGINWIDTHN, 0, pix);
	else
		SendEditor(SCI_SETMARGINWIDTHN, 0);
	SendEditor(SCI_SETMARGINWIDTHN, 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	SendEditor(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	SendEditor(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	SendEditor(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	SendEditor(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	SendEditor(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));
	m_regOldLinesColor = CRegStdWORD(_T("Software\\TortoiseGit\\BlameOldColor"), RGB(230, 230, 255));
	m_regNewLinesColor = CRegStdWORD(_T("Software\\TortoiseGit\\BlameNewColor"), RGB(255, 230, 230));
	
	this->m_TextView.Call(SCI_SETWRAPMODE, SC_WRAP_NONE);

}

void CTortoiseGitBlameView::StartSearch()
{
	if (currentDialog)
		return;
	bool bCase = false;
	// Initialize FINDREPLACE
	if (fr.Flags & FR_MATCHCASE)
		bCase = true;
	SecureZeroMemory(&fr, sizeof(fr));
	fr.lStructSize = sizeof(fr);
	fr.hwndOwner = wMain;
	fr.lpstrFindWhat = szFindWhat;
	fr.wFindWhatLen = 80;
	fr.Flags = FR_HIDEUPDOWN | FR_HIDEWHOLEWORD;
	fr.Flags |= bCase ? FR_MATCHCASE : 0;

	currentDialog = FindText(&fr);
}

bool CTortoiseGitBlameView::DoSearch(LPSTR what, DWORD flags)
{
#if 0
	TCHAR szWhat[80];
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int line = SendEditor(SCI_LINEFROMPOSITION, pos);
	bool bFound = false;
	bool bCaseSensitive = !!(flags & FR_MATCHCASE);

	strcpy_s(szWhat, sizeof(szWhat), what);

	if(!bCaseSensitive)
	{
		char *p;
		size_t len = strlen(szWhat);
		for (p = szWhat; p < szWhat + len; p++)
		{
			if (isupper(*p)&&__isascii(*p))
				*p = _tolower(*p);
		}
	}

	CString sWhat = CString(szWhat);
	
	char buf[20];
	int i=0;
	for (i=line; (i<(int)authors.size())&&(!bFound); ++i)
	{
		int bufsize = SendEditor(SCI_GETLINE, i);
		char * linebuf = new char[bufsize+1];
		SecureZeroMemory(linebuf, bufsize+1);
		SendEditor(SCI_GETLINE, i, (LPARAM)linebuf);
		if (!bCaseSensitive)
		{
			char *p;
			for (p = linebuf; p < linebuf + bufsize; p++)
			{
				if (isupper(*p)&&__isascii(*p))
					*p = _tolower(*p);
			}
		}
		_stprintf_s(buf, 20, _T("%ld"), revs[i]);
		if (authors[i].compare(sWhat)==0)
			bFound = true;
		else if ((!bCaseSensitive)&&(_stricmp(authors[i].c_str(), szWhat)==0))
			bFound = true;
		else if (strcmp(buf, szWhat) == 0)
			bFound = true;
		else if (strstr(linebuf, szWhat))
			bFound = true;
		delete [] linebuf;
	}
	if (!bFound)
	{
		for (i=0; (i<line)&&(!bFound); ++i)
		{
			int bufsize = SendEditor(SCI_GETLINE, i);
			char * linebuf = new char[bufsize+1];
			SecureZeroMemory(linebuf, bufsize+1);
			SendEditor(SCI_GETLINE, i, (LPARAM)linebuf);
			if (!bCaseSensitive)
			{
				char *p;
				for (p = linebuf; p < linebuf + bufsize; p++)
				{
					if (isupper(*p)&&__isascii(*p))
						*p = _tolower(*p);
				}
			}
			_stprintf_s(buf, 20, _T("%ld"), revs[i]);
			if (authors[i].compare(sWhat)==0)
				bFound = true;
			else if ((!bCaseSensitive)&&(_stricmp(authors[i].c_str(), szWhat)==0))
				bFound = true;
			else if (strcmp(buf, szWhat) == 0)
				bFound = true;
			else if (strstr(linebuf, szWhat))
				bFound = true;
			delete [] linebuf;
		}
	}
	if (bFound)
	{
		GotoLine(i);
		int selstart = SendEditor(SCI_GETCURRENTPOS);
		int selend = SendEditor(SCI_POSITIONFROMLINE, i);
		SendEditor(SCI_SETSELECTIONSTART, selstart);
		SendEditor(SCI_SETSELECTIONEND, selend);
		m_SelectedLine = i-1;
	}
	else
	{
		::MessageBox(wMain, searchstringnotfound, "CTortoiseGitBlameView", MB_ICONINFORMATION);
	}
#endif
	return true;
}

bool CTortoiseGitBlameView::GotoLine(long line)
{
#if 0
	--line;
	if (line < 0)
		return false;
	if ((unsigned long)line >= authors.size())
	{
		line = authors.size()-1;
	}

	int nCurrentPos = SendEditor(SCI_GETCURRENTPOS);
	int nCurrentLine = SendEditor(SCI_LINEFROMPOSITION,nCurrentPos);
	int nFirstVisibleLine = SendEditor(SCI_GETFIRSTVISIBLELINE);
	int nLinesOnScreen = SendEditor(SCI_LINESONSCREEN);

	if ( line>=nFirstVisibleLine && line<=nFirstVisibleLine+nLinesOnScreen)
	{
		// no need to scroll
		SendEditor(SCI_GOTOLINE, line);
	}
	else
	{
		// Place the requested line one third from the top
		if ( line > nCurrentLine )
		{
			SendEditor(SCI_GOTOLINE, (WPARAM)(line+(int)nLinesOnScreen*(2/3.0)));
		}
		else
		{
			SendEditor(SCI_GOTOLINE, (WPARAM)(line-(int)nLinesOnScreen*(1/3.0)));
		}
	}

	// Highlight the line
	int nPosStart = SendEditor(SCI_POSITIONFROMLINE,line);
	int nPosEnd = SendEditor(SCI_GETLINEENDPOSITION,line);
	SendEditor(SCI_SETSEL,nPosEnd,nPosStart);
#endif
	return true;
}

bool CTortoiseGitBlameView::ScrollToLine(long line)
{
	if (line < 0)
		return false;

	int nCurrentLine = SendEditor(SCI_GETFIRSTVISIBLELINE);

	int scrolldelta = line - nCurrentLine;
	SendEditor(SCI_LINESCROLL, 0, scrolldelta);

	return true;
}

void CTortoiseGitBlameView::CopySelectedLogToClipboard()
{
#if 0
	if (m_selectedrev <= 0)
		return;
	std::map<LONG, CString>::iterator iter;
	if ((iter = app.logmessages.find(m_selectedrev)) != app.logmessages.end())
	{
		CString msg;
		msg += m_selectedauthor;
		msg += "  ";
		msg += app.m_selecteddate;
		msg += '\n';
		msg += iter->second;
		msg += _T("\n");
		if (OpenClipboard(app.wBlame))
		{
			EmptyClipboard();
			HGLOBAL hClipboardData;
			hClipboardData = GlobalAlloc(GMEM_DDESHARE, msg.size()+1);
			char * pchData;
			pchData = (char*)GlobalLock(hClipboardData);
			strcpy_s(pchData, msg.size()+1, msg.c_str());
			GlobalUnlock(hClipboardData);
			SetClipboardData(CF_TEXT,hClipboardData);
			CloseClipboard();
		}
	}
#endif
}

void CTortoiseGitBlameView::BlamePreviousRevision()
{
#if 0
	LONG nRevisionTo = m_selectedorigrev - 1;
	if ( nRevisionTo<1 )
	{
		return;
	}

	// We now determine the smallest revision number in the blame file (but ignore "-1")
	// We do this for two reasons:
	// 1. we respect the "From revision" which the user entered
	// 2. we speed up the call of "svn blame" because previous smaller revision numbers don't have any effect on the result
	LONG nSmallestRevision = -1;
	for (LONG line=0;line<(LONG)app.revs.size();line++)
	{
		const LONG nRevision = app.revs[line];
		if ( nRevision > 0 )
		{
			if ( nSmallestRevision < 1 )
			{
				nSmallestRevision = nRevision;
			}
			else
			{
				nSmallestRevision = min(nSmallestRevision,nRevision);
			}
		}
	}

	char bufStartRev[20];
	_stprintf_s(bufStartRev, 20, _T("%d"), nSmallestRevision);

	char bufEndRev[20];
	_stprintf_s(bufEndRev, 20, _T("%d"), nRevisionTo);

	char bufLine[20];
	_stprintf_s(bufLine, 20, _T("%d"), m_SelectedLine+1); //using the current line is a good guess.

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	stdstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
	stdstring svnCmd = _T(" /command:blame ");
	svnCmd += _T(" /path:\"");
	svnCmd += szOrigPath;
	svnCmd += _T("\"");
	svnCmd += _T(" /startrev:");
	svnCmd += bufStartRev;
	svnCmd += _T(" /endrev:");
	svnCmd += bufEndRev;
	svnCmd += _T(" /line:");
	svnCmd += bufLine;
	if (bIgnoreEOL)
		svnCmd += _T(" /ignoreeol");
	if (bIgnoreSpaces)
		svnCmd += _T(" /ignorespaces");
	if (bIgnoreAllSpaces)
		svnCmd += _T(" /ignoreallspaces");
    if (CreateProcess(tortoiseProcPath.c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
	{
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
	}
#endif
}

void CTortoiseGitBlameView::DiffPreviousRevision()
{
#if 0
	LONG nRevisionTo = m_selectedorigrev;
	if ( nRevisionTo<1 )
	{
		return;
	}

	LONG nRevisionFrom = nRevisionTo-1;

	char bufStartRev[20];
	_stprintf_s(bufStartRev, 20, _T("%d"), nRevisionFrom);

	char bufEndRev[20];
	_stprintf_s(bufEndRev, 20, _T("%d"), nRevisionTo);

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	stdstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
	stdstring svnCmd = _T(" /command:diff ");
	svnCmd += _T(" /path:\"");
	svnCmd += szOrigPath;
	svnCmd += _T("\"");
	svnCmd += _T(" /startrev:");
	svnCmd += bufStartRev;
	svnCmd += _T(" /endrev:");
	svnCmd += bufEndRev;
	if (CreateProcess(tortoiseProcPath.c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
	{
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
	}
#endif
}

void CTortoiseGitBlameView::ShowLog()
{
#if 0
	char bufRev[20];
	_stprintf_s(bufRev, 20, _T("%d"), m_selectedorigrev);

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	stdstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
	stdstring svnCmd = _T(" /command:log ");
	svnCmd += _T(" /path:\"");
	svnCmd += szOrigPath;
	svnCmd += _T("\"");
	svnCmd += _T(" /startrev:");
	svnCmd += bufRev;
	svnCmd += _T(" /pegrev:");
	svnCmd += bufRev;
	if (CreateProcess(tortoiseProcPath.c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
	{
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
	}
#endif
}

void CTortoiseGitBlameView::Notify(SCNotification *notification) 
{
	switch (notification->nmhdr.code) 
	{
	case SCN_SAVEPOINTREACHED:
		break;

	case SCN_SAVEPOINTLEFT:
		break;
	case SCN_PAINTED:
//		InvalidateRect(wBlame, NULL, FALSE);
//		InvalidateRect(wLocator, NULL, FALSE);
		break;
	case SCN_GETBKCOLOR:
//		if ((m_colorage)&&(notification->line < (int)revs.size()))
//		{
//			notification->lParam = InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (revs[notification->line]-m_lowestrev)*100/((m_highestrev-m_lowestrev)+1));
//		}
		break;
	}
}

void CTortoiseGitBlameView::Command(int id)
{
#if 0
	switch (id) 
	{
//	case IDM_EXIT:
//		::PostQuitMessage(0);
//		break;
	case ID_EDIT_FIND:
		StartSearch();
		break;
	case ID_COPYTOCLIPBOARD:
		CopySelectedLogToClipboard();
		break;
	case ID_BLAME_PREVIOUS_REVISION:
		BlamePreviousRevision();
		break;
	case ID_DIFF_PREVIOUS_REVISION:
		DiffPreviousRevision();
		break;
	case ID_SHOWLOG:
		ShowLog();
		break;
	case ID_EDIT_GOTOLINE:
		GotoLineDlg();
		break;
	case ID_VIEW_COLORAGEOFLINES:
		{
			m_colorage = !m_colorage;
			HMENU hMenu = GetMenu(wMain);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= m_colorage ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_COLORAGEOFLINES, uCheck);
			m_blamewidth = 0;
			InitSize();
		}
		break;
	case ID_VIEW_MERGEPATH:
		{
			ShowPath = !ShowPath;
			HMENU hMenu = GetMenu(wMain);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= ShowPath ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_MERGEPATH, uCheck);
			m_blamewidth = 0;
			InitSize();
		}
	default:
		break;
	};
#endif
}

void CTortoiseGitBlameView::GotoLineDlg()
{
#if 0
	if (DialogBox(hResource, MAKEINTRESOURCE(IDD_GOTODLG), wMain, GotoDlgProc)==IDOK)
	{
		GotoLine(m_gotoline);
	}
#endif
}

LONG CTortoiseGitBlameView::GetBlameWidth()
{
#if 0
	if (m_blamewidth)
		return m_blamewidth;
	LONG blamewidth = 0;
	SIZE width;
	CreateFont();
	HDC hDC = ::GetDC(wBlame);
	HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);
	TCHAR buf[MAX_PATH];
	_stprintf_s(buf, MAX_PATH, _T("%8ld "), 88888888);
	::GetTextExtentPoint(hDC, buf, _tcslen(buf), &width);
	m_revwidth = width.cx + BLAMESPACE;
	blamewidth += m_revwidth;
	if (ShowDate)
	{
		_stprintf_s(buf, MAX_PATH, _T("%30s"), _T("31.08.2001 06:24:14"));
		::GetTextExtentPoint32(hDC, buf, _tcslen(buf), &width);
		m_datewidth = width.cx + BLAMESPACE;
		blamewidth += m_datewidth;
	}
	if (ShowAuthor)
	{
		SIZE maxwidth = {0};
		for (std::vector<CString>::iterator I = authors.begin(); I != authors.end(); ++I)
		{
			::GetTextExtentPoint32(hDC, I->c_str(), I->size(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_authorwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_authorwidth;
	}
	if (ShowPath)
	{
		SIZE maxwidth = {0};
		for (std::vector<CString>::iterator I = paths.begin(); I != paths.end(); ++I)
		{
			::GetTextExtentPoint32(hDC, I->c_str(), I->size(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_pathwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_pathwidth;
	}
	::SelectObject(hDC, oldfont);
	POINT pt = {blamewidth, 0};
	LPtoDP(hDC, &pt, 1);
	m_blamewidth = pt.x;
	ReleaseDC(wBlame, hDC);
#endif
	//return m_blamewidth;
	return 100;
}

void CTortoiseGitBlameView::CreateFont()
{
	if (m_font)
		return;
	LOGFONT lf = {0};
	lf.lfWeight = 400;
	HDC hDC = ::GetDC(wBlame);
	lf.lfHeight = -MulDiv((DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	lf.lfCharSet = DEFAULT_CHARSET;
	CRegStdString fontname = CRegStdString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New"));
	_tcscpy_s(lf.lfFaceName, 32, ((stdstring)fontname).c_str());
	m_font = ::CreateFontIndirect(&lf);

	lf.lfItalic = TRUE;
	m_italicfont = ::CreateFontIndirect(&lf);

	::ReleaseDC(wBlame, hDC);

	//m_TextView.SetFont(lf.lfFaceName,((DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10)));
}

void CTortoiseGitBlameView::DrawBlame(HDC hDC)
{

	if (hDC == NULL)
		return;
	if (m_font == NULL)
		return;

	HFONT oldfont = NULL;
	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
	LONG_PTR height = SendEditor(SCI_TEXTHEIGHT);
	LONG_PTR Y = 0;
	TCHAR buf[MAX_PATH];
	RECT rc;
	BOOL sel = FALSE;
	//::GetClientRect(this->m_hWnd, &rc);
	for (LRESULT i=line; i<(line+linesonscreen); ++i)
	{
		sel = FALSE;
		if (i < (int)m_CommitHash.size())
		{
		//	if (mergelines[i])
		//		oldfont = (HFONT)::SelectObject(hDC, m_italicfont);
		//	else
				oldfont = (HFONT)::SelectObject(hDC, m_font);
			::SetBkColor(hDC, m_windowcolor);
			::SetTextColor(hDC, m_textcolor);
			if (m_CommitHash[i].GetLength()>0)
			{
				if (m_CommitHash[i].Compare(m_MouseHash)==0)
					::SetBkColor(hDC, m_mouseauthorcolor);
				if (m_CommitHash[i].Compare(m_SelectedHash)==0)
				{
					::SetBkColor(hDC, m_selectedauthorcolor);
					::SetTextColor(hDC, m_texthighlightcolor);
					sel = TRUE;
				}
			}
			//if ((revs[i] == m_mouserev)&&(!sel))
			//	::SetBkColor(hDC, m_mouserevcolor);
			//if (revs[i] == m_selectedrev)
			//{
			//	::SetBkColor(hDC, m_selectedrevcolor);
			//	::SetTextColor(hDC, m_texthighlightcolor);
			//}

			CString str;
			str.Format(_T("%d.%s"),m_ID[i],m_Authors[i]);

			//_stprintf_s(buf, MAX_PATH, _T("%8ld       "), revs[i]);
			rc.top=Y;rc.left=0;		
			rc.bottom=Y+height;
			rc.right = rc.left + 100;
			::ExtTextOut(hDC, 0, Y, ETO_CLIPPED, &rc, str, str.GetLength(), 0);
			int Left = m_revwidth;
#if 0
			if (ShowDate)
			{
				rc.right = rc.left + Left + m_datewidth;
				_stprintf_s(buf, MAX_PATH, _T("%30s            "), dates[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_datewidth;
			}
			if (ShowAuthor)
			{
				rc.right = rc.left + Left + m_authorwidth;
				_stprintf_s(buf, MAX_PATH, _T("%-30s            "), authors[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_authorwidth;
			}
#endif
#if 0
			if (ShowPath)
			{
				rc.right = rc.left + Left + m_pathwidth;
				_stprintf_s(buf, MAX_PATH, _T("%-60s            "), paths[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_authorwidth;
			}
#endif
			if ((i==m_SelectedLine)&&(currentDialog))
			{
				LOGBRUSH brush;
				brush.lbColor = m_textcolor;
				brush.lbHatch = 0;
				brush.lbStyle = BS_SOLID;
				HPEN pen = ExtCreatePen(PS_SOLID | PS_GEOMETRIC, 2, &brush, 0, NULL);
				HGDIOBJ hPenOld = SelectObject(hDC, pen);
				RECT rc2 = rc;
				rc2.top = Y;
				rc2.bottom = Y + height;
				::MoveToEx(hDC, rc2.left, rc2.top, NULL);
				::LineTo(hDC, rc2.right, rc2.top);
				::LineTo(hDC, rc2.right, rc2.bottom);
				::LineTo(hDC, rc2.left, rc2.bottom);
				::LineTo(hDC, rc2.left, rc2.top);
				SelectObject(hDC, hPenOld); 
				DeleteObject(pen); 
			}
			Y += height;
			::SelectObject(hDC, oldfont);
		}
		else
		{
			::SetBkColor(hDC, m_windowcolor);
			for (int j=0; j< MAX_PATH; ++j)
				buf[j]=' ';
			::ExtTextOut(hDC, 0, Y, ETO_CLIPPED, &rc, buf, MAX_PATH-1, 0);
			Y += height;
		}
	}
}

void CTortoiseGitBlameView::DrawHeader(HDC hDC)
{
#if 0
	if (hDC == NULL)
		return;

	RECT rc;
	HFONT oldfont = (HFONT)::SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	::GetClientRect(wHeader, &rc);

	::SetBkColor(hDC, ::GetSysColor(COLOR_BTNFACE));

	TCHAR szText[MAX_LOADSTRING];
	LoadString(app.hResource, IDS_HEADER_REVISION, szText, MAX_LOADSTRING);
	::ExtTextOut(hDC, LOCATOR_WIDTH, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
	int Left = m_revwidth+LOCATOR_WIDTH;
	if (ShowDate)
	{
		LoadString(app.hResource, IDS_HEADER_DATE, szText, MAX_LOADSTRING);
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
		Left += m_datewidth;
	}
	if (ShowAuthor)
	{
		LoadString(app.hResource, IDS_HEADER_AUTHOR, szText, MAX_LOADSTRING);
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
		Left += m_authorwidth;
	}
	if (ShowPath)
	{
		LoadString(app.hResource, IDS_HEADER_PATH, szText, MAX_LOADSTRING);
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
		Left += m_pathwidth;
	}
	LoadString(app.hResource, IDS_HEADER_LINE, szText, MAX_LOADSTRING);
	::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);

	::SelectObject(hDC, oldfont);
#endif
}

void CTortoiseGitBlameView::DrawLocatorBar(HDC hDC)
{
#if 0
	if (hDC == NULL)
		return;

	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
	LONG_PTR Y = 0;
	COLORREF blackColor = GetSysColor(COLOR_WINDOWTEXT);

	RECT rc;
	::GetClientRect(wLocator, &rc);
	RECT lineRect = rc;
	LONG height = rc.bottom-rc.top;
	LONG currentLine = 0;

	// draw the colored bar
	for (std::vector<LONG>::const_iterator it = revs.begin(); it != revs.end(); ++it)
	{
		currentLine++;
		// get the line color
		COLORREF cr = InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (*it - m_lowestrev)*100/((m_highestrev-m_lowestrev)+1));
		if ((currentLine > line)&&(currentLine <= (line + linesonscreen)))
		{
			cr = InterColor(cr, blackColor, 10);
		}
		SetBkColor(hDC, cr);
		lineRect.top = Y;
		lineRect.bottom = (currentLine * height / revs.size());
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		Y = lineRect.bottom;
	}

	if (revs.size())
	{
		// now draw two lines indicating the scroll position of the source view
		SetBkColor(hDC, blackColor);
		lineRect.top = line * height / revs.size();
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		lineRect.top = (line + linesonscreen) * height / revs.size();
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
	}
#endif
}

void CTortoiseGitBlameView::StringExpand(LPSTR str)
{
	char * cPos = str;
	do
	{
		cPos = strchr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, strlen(cPos)*sizeof(char));
			*cPos = '\r';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}
void CTortoiseGitBlameView::StringExpand(LPWSTR str)
{
	wchar_t * cPos = str;
	do
	{
		cPos = wcschr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, wcslen(cPos)*sizeof(wchar_t));
			*cPos = '\r';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hResource);
ATOM				MyRegisterBlameClass(HINSTANCE hResource);
ATOM				MyRegisterHeaderClass(HINSTANCE hResource);
ATOM				MyRegisterLocatorClass(HINSTANCE hResource);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndBlameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndHeaderProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndLocatorProc(HWND, UINT, WPARAM, LPARAM);
UINT				uFindReplaceMsg;

#if 0
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	app.hInstance = hInstance;
	MSG msg;
	HACCEL hAccelTable;

	if (::LoadLibrary("SciLexer.DLL") == NULL)
		return FALSE;

	CRegStdWORD loc = CRegStdWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;

	CLangDll langDLL;
	app.hResource = langDLL.Init(_T("CTortoiseGitBlameView"), langId);
	if (app.hResource == NULL)
		app.hResource = app.hInstance;

	// Initialize global strings
	LoadString(app.hResource, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(app.hResource, IDC_TortoiseGitBlameView, szWindowClass, MAX_LOADSTRING);
	LoadString(app.hResource, IDS_SEARCHNOTFOUND, searchstringnotfound, MAX_LOADSTRING);
	MyRegisterClass(app.hResource);
	MyRegisterBlameClass(app.hResource);
	MyRegisterHeaderClass(app.hResource);
	MyRegisterLocatorClass(app.hResource);

	// Perform application initialization:
	if (!InitInstance (app.hResource, nCmdShow)) 
	{
		langDLL.Close();
		return FALSE;
	}

	SecureZeroMemory(szViewtitle, MAX_PATH);
	SecureZeroMemory(szOrigPath, MAX_PATH);
	char blamefile[MAX_PATH] = {0};
	char logfile[MAX_PATH] = {0};

	CCmdLineParser parser(lpCmdLine);


	if (__argc > 1)
	{
		_tcscpy_s(blamefile, MAX_PATH, __argv[1]);
	}
	if (__argc > 2)
	{
		_tcscpy_s(logfile, MAX_PATH, __argv[2]);
	}
	if (__argc > 3)
	{
		_tcscpy_s(szViewtitle, MAX_PATH, __argv[3]);
		if (parser.HasVal(_T("revrange")))
		{
			_tcscat_s(szViewtitle, MAX_PATH, _T(" : "));
			_tcscat_s(szViewtitle, MAX_PATH, parser.GetVal(_T("revrange")));
		}
	}
	if ((_tcslen(blamefile)==0) || parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
	{
		TCHAR szInfo[MAX_LOADSTRING];
		LoadString(app.hResource, IDS_COMMANDLINE_INFO, szInfo, MAX_LOADSTRING);
		MessageBox(NULL, szInfo, _T("CTortoiseGitBlameView"), MB_ICONERROR);
		langDLL.Close();
		return 0;
	}

	if ( parser.HasKey(_T("path")) )
	{
		_tcscpy_s(szOrigPath, MAX_PATH, parser.GetVal(_T("path")));
	}
	app.bIgnoreEOL = parser.HasKey(_T("ignoreeol"));
	app.bIgnoreSpaces = parser.HasKey(_T("ignorespaces"));
	app.bIgnoreAllSpaces = parser.HasKey(_T("ignoreallspaces"));

	app.SendEditor(SCI_SETCODEPAGE, GetACP());
	app.OpenFile(blamefile);
	if (_tcslen(logfile)>0)
		app.OpenLogFile(logfile);

	if (parser.HasKey(_T("line")))
	{
		app.GotoLine(parser.GetLongVal(_T("line")));
	}

	CheckMenuItem(GetMenu(app.wMain), ID_VIEW_COLORAGEOFLINES, MF_CHECKED|MF_BYCOMMAND);


	hAccelTable = LoadAccelerators(app.hResource, (LPCTSTR)IDC_TortoiseGitBlameView);

	BOOL going = TRUE;
	msg.wParam = 0;
	while (going) 
	{
		going = GetMessage(&msg, NULL, 0, 0);
		if (app.currentDialog && going) 
		{
			if (!IsDialogMessage(app.currentDialog, &msg)) 
			{
				if (TranslateAccelerator(msg.hwnd, hAccelTable, &msg) == 0) 
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		} 
		else if (going) 
		{
			if (TranslateAccelerator(app.wMain, hAccelTable, &msg) == 0) 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	langDLL.Close();
	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_TortoiseGitBlameView;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterBlameClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndBlameProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseGitBlameViewBlame");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterHeaderClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndHeaderProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseGitBlameViewHeader");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterLocatorClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndLocatorProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseGitBlameViewLocator");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hResource, int nCmdShow)
{
   app.wMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hResource, NULL);   

   if (!app.wMain)
   {
      return FALSE;
   }

   CRegStdWORD pos(_T("Software\\TortoiseGit\\TBlamePos"), 0);
   CRegStdWORD width(_T("Software\\TortoiseGit\\TBlameSize"), 0);
   CRegStdWORD state(_T("Software\\TortoiseGit\\TBlameState"), 0);
   if (DWORD(pos) && DWORD(width))
   {
	   RECT rc;
	   rc.left = LOWORD(DWORD(pos));
	   rc.top = HIWORD(DWORD(pos));
	   rc.right = rc.left + LOWORD(DWORD(width));
	   rc.bottom = rc.top + HIWORD(DWORD(width));
	   HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
	   if (hMon)
	   {
		   // only restore the window position if the monitor is valid
		   MoveWindow(app.wMain, LOWORD(DWORD(pos)), HIWORD(DWORD(pos)),
			   LOWORD(DWORD(width)), HIWORD(DWORD(width)), FALSE);
	   }
   }
   if (DWORD(state) == SW_MAXIMIZE)
	   ShowWindow(app.wMain, SW_MAXIMIZE);
   else
	   ShowWindow(app.wMain, nCmdShow);
   UpdateWindow(app.wMain);

   //Create the tooltips

   INITCOMMONCONTROLSEX iccex; 
   app.hwndTT;                 // handle to the ToolTip control
   TOOLINFO ti;
   RECT rect;                  // for client area coordinates
   iccex.dwICC = ICC_WIN95_CLASSES;
   iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   InitCommonControlsEx(&iccex);

   /* CREATE A TOOLTIP WINDOW */
   app.hwndTT = CreateWindowEx(WS_EX_TOPMOST,
	   TOOLTIPS_CLASS,
	   NULL,
	   WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
	   CW_USEDEFAULT,
	   CW_USEDEFAULT,
	   CW_USEDEFAULT,
	   CW_USEDEFAULT,
	   app.wBlame,
	   NULL,
	   app.hResource,
	   NULL
	   );

   SetWindowPos(app.hwndTT,
	   HWND_TOPMOST,
	   0,
	   0,
	   0,
	   0,
	   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

   /* GET COORDINATES OF THE MAIN CLIENT AREA */
   GetClientRect (app.wBlame, &rect);

   /* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
   ti.cbSize = sizeof(TOOLINFO);
   ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;//TTF_SUBCLASS | TTF_PARSELINKS;
   ti.hwnd = app.wBlame;
   ti.hinst = app.hResource;
   ti.uId = 0;
   ti.lpszText = LPSTR_TEXTCALLBACK;
   // ToolTip control will cover the whole window
   ti.rect.left = rect.left;    
   ti.rect.top = rect.top;
   ti.rect.right = rect.right;
   ti.rect.bottom = rect.bottom;

   /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
   SendMessage(app.hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
   SendMessage(app.hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
   //SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELONG(50000, 0));
   //SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_RESHOW, MAKELONG(1000, 0));
   
   uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
   
   return TRUE;
}
#endif
void CTortoiseGitBlameView::InitSize()
{
    RECT rc;
    RECT blamerc;
    RECT sourcerc;
    ::GetClientRect(wMain, &rc);
    ::SetWindowPos(wHeader, 0, rc.left, rc.top, rc.right-rc.left, HEADER_HEIGHT, 0);
    rc.top += HEADER_HEIGHT;
    blamerc.left = rc.left;
    blamerc.top = rc.top;
	LONG w = GetBlameWidth();
    blamerc.right = w > abs(rc.right - rc.left) ? rc.right : w + rc.left;
    blamerc.bottom = rc.bottom;
    sourcerc.left = blamerc.right;
    sourcerc.top = rc.top;
    sourcerc.bottom = rc.bottom;
    sourcerc.right = rc.right;
	if (m_colorage)
	{
		::OffsetRect(&blamerc, LOCATOR_WIDTH, 0);
		::OffsetRect(&sourcerc, LOCATOR_WIDTH, 0);
		sourcerc.right -= LOCATOR_WIDTH;
	}
	::InvalidateRect(wMain, NULL, FALSE);
    ::SetWindowPos(m_wEditor, 0, sourcerc.left, sourcerc.top, sourcerc.right - sourcerc.left, sourcerc.bottom - sourcerc.top, 0);
	::SetWindowPos(wBlame, 0, blamerc.left, blamerc.top, blamerc.right - blamerc.left, blamerc.bottom - blamerc.top, 0);
	if (m_colorage)
		::SetWindowPos(wLocator, 0, 0, blamerc.top, LOCATOR_WIDTH, blamerc.bottom - blamerc.top, SWP_SHOWWINDOW);
	else
		::ShowWindow(wLocator, SW_HIDE);
}

#if 0
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == uFindReplaceMsg)
	{
		LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;

		// If the FR_DIALOGTERM flag is set, 
		// invalidate the handle identifying the dialog box. 
		if (lpfr->Flags & FR_DIALOGTERM)
		{
			app.currentDialog = NULL; 
			return 0; 
		} 
		if (lpfr->Flags & FR_FINDNEXT)
		{
			app.DoSearch(lpfr->lpstrFindWhat, lpfr->Flags);
		}
		return 0; 
	}
	switch (message) 
	{
	case WM_CREATE:
		app.m_wEditor = ::CreateWindow(
			"Scintilla",
			"Source",
			WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
			0, 0,
			100, 100,
			hWnd,
			0,
			app.hResource,
			0);
		app.InitialiseEditor();
		::ShowWindow(app.m_wEditor, SW_SHOW);
		::SetFocus(app.m_wEditor);
		app.wBlame = ::CreateWindow(
			_T("TortoiseGitBlameViewBlame"), 
			_T("blame"), 
			WS_CHILD | WS_CLIPCHILDREN,
			CW_USEDEFAULT, 0, 
			CW_USEDEFAULT, 0, 
			hWnd, 
			NULL, 
			app.hResource, 
			NULL);
		::ShowWindow(app.wBlame, SW_SHOW);
		app.wHeader = ::CreateWindow(
			_T("TortoiseGitBlameViewHeader"), 
			_T("header"), 
			WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
			CW_USEDEFAULT, 0, 
			CW_USEDEFAULT, 0, 
			hWnd, 
			NULL, 
			app.hResource, 
			NULL);
		::ShowWindow(app.wHeader, SW_SHOW);
		app.wLocator = ::CreateWindow(
			_T("TortoiseGitBlameViewLocator"), 
			_T("locator"), 
			WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
			CW_USEDEFAULT, 0, 
			CW_USEDEFAULT, 0, 
			hWnd, 
			NULL, 
			app.hResource, 
			NULL);
		::ShowWindow(app.wLocator, SW_SHOW);
		return 0;

	case WM_SIZE:
		if (wParam != 1) 
		{
            app.InitSize();
		}
		return 0;

	case WM_COMMAND:
		app.Command(LOWORD(wParam));
		break;
	case WM_NOTIFY:
		app.Notify(reinterpret_cast<SCNotification *>(lParam));
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		{
			CRegStdWORD pos(_T("Software\\TortoiseGit\\TBlamePos"), 0);
			CRegStdWORD width(_T("Software\\TortoiseGit\\TBlameSize"), 0);
			CRegStdWORD state(_T("Software\\TortoiseGit\\TBlameState"), 0);
			RECT rc;
			GetWindowRect(app.wMain, &rc);
			if ((rc.left >= 0)&&(rc.top >= 0))
			{
				pos = MAKELONG(rc.left, rc.top);
				width = MAKELONG(rc.right-rc.left, rc.bottom-rc.top);
			}
			WINDOWPLACEMENT wp = {0};
			wp.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(app.wMain, &wp);
			state = wp.showCmd;
			::DestroyWindow(app.m_wEditor);
			::PostQuitMessage(0);
		}
		return 0;
	case WM_SETFOCUS:
		::SetFocus(app.wBlame);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndBlameProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	TRACKMOUSEEVENT mevt;
	HDC hDC;
	switch (message) 
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(app.wBlame, &ps);
		app.DrawBlame(hDC);
		EndPaint(app.wBlame, &ps);
		break;
	case WM_COMMAND:
		app.Command(LOWORD(wParam));
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case TTN_GETDISPINFO:
			{
				LPNMHDR pNMHDR = (LPNMHDR)lParam;
				NMTTDISPINFOA* pTTTA = (NMTTDISPINFOA*)pNMHDR;
				NMTTDISPINFOW* pTTTW = (NMTTDISPINFOW*)pNMHDR;
				POINT point;
				DWORD ptW = GetMessagePos();
				point.x = GET_X_LPARAM(ptW);
				point.y = GET_Y_LPARAM(ptW);
				::ScreenToClient(app.wBlame, &point);
				LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
				LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
				line = line + (point.y/height);
				if (line >= (LONG)app.revs.size())
					break;
				if (line < 0)
					break;
				LONG rev = app.revs[line];
				if (line >= (LONG)app.revs.size())
					break;

				SecureZeroMemory(app.m_szTip, sizeof(app.m_szTip));
				SecureZeroMemory(app.m_wszTip, sizeof(app.m_wszTip));
				std::map<LONG, CString>::iterator iter;
				if ((iter = app.logmessages.find(rev)) != app.logmessages.end())
				{
					CString msg;
					if (!ShowAuthor)
					{
						msg += app.authors[line];
					}
					if (!ShowDate)
					{
						if (!ShowAuthor) msg += "  ";
						msg += app.dates[line];
					}
					if (!ShowAuthor || !ShowDate)
						msg += '\n';
					msg += iter->second;
					// an empty tooltip string will deactivate the tooltips,
					// which means we must make sure that the tooltip won't
					// be empty.
					if (msg.empty())
						msg = _T(" ");
					if (pNMHDR->code == TTN_NEEDTEXTA)
					{
						lstrcpyn(app.m_szTip, msg.c_str(), MAX_LOG_LENGTH*2);
						app.StringExpand(app.m_szTip);
						pTTTA->lpszText = app.m_szTip;
					}
					else
					{
						pTTTW->lpszText = app.m_wszTip;
						::MultiByteToWideChar( CP_ACP , 0, msg.c_str(), min(msg.size(), MAX_LOG_LENGTH*2), app.m_wszTip, MAX_LOG_LENGTH*2);
						app.StringExpand(app.m_wszTip);
					}
				}
			}
			break;
		}
		return 0;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		return 0;
	case WM_MOUSELEAVE:
		app.m_mouserev = -2;
		app.m_mouseauthor.clear();
		app.ttVisible = FALSE;
		SendMessage(app.hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
		::InvalidateRect(app.wBlame, NULL, FALSE);
		break;
	case WM_MOUSEMOVE:
		{
			mevt.cbSize = sizeof(TRACKMOUSEEVENT);
			mevt.dwFlags = TME_LEAVE;
			mevt.dwHoverTime = HOVER_DEFAULT;
			mevt.hwndTrack = app.wBlame;
			::TrackMouseEvent(&mevt);
			POINT pt = {((int)(short)LOWORD(lParam)), ((int)(short)HIWORD(lParam))};
			ClientToScreen(app.wBlame, &pt);
			pt.x += 15;
			pt.y += 15;
			SendMessage(app.hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
			if (!app.ttVisible)
			{
				TOOLINFO ti;
				ti.cbSize = sizeof(TOOLINFO);
				ti.hwnd = app.wBlame;
				ti.uId = 0;
				SendMessage(app.hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
			}
			int y = ((int)(short)HIWORD(lParam));
			LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
			LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
			line = line + (y/height);
			app.ttVisible = (line < (LONG)app.revs.size());
			if ( app.ttVisible )
			{
				if (app.authors[line].compare(app.m_mouseauthor) != 0)
				{
					app.m_mouseauthor = app.authors[line];
				}
				if (app.revs[line] != app.m_mouserev)
				{
					app.m_mouserev = app.revs[line];
					::InvalidateRect(app.wBlame, NULL, FALSE);
					SendMessage(app.hwndTT, TTM_UPDATE, 0, 0);
				}
			}
		}
		break;
	case WM_RBUTTONDOWN:
		// fall through
	case WM_LBUTTONDOWN:
		{
			int y = ((int)(short)HIWORD(lParam));
			LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
			LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
			line = line + (y/height);
			if (line < (LONG)app.revs.size())
			{
				app.SetSelectedLine(line);
				if (app.revs[line] != app.m_selectedrev)
				{
					app.m_selectedrev = app.revs[line];
					app.m_selectedorigrev = app.origrevs[line];
					app.m_selectedauthor = app.authors[line];
					app.m_selecteddate = app.dates[line];
				}
				else
				{
					app.m_selectedauthor.clear();
					app.m_selecteddate.clear();
					app.m_selectedrev = -2;
					app.m_selectedorigrev = -2;
				}
				::InvalidateRect(app.wBlame, NULL, FALSE);
			}
			else
			{
				app.SetSelectedLine(-1);
			}
		}
		break;
	case WM_SETFOCUS:
		::SetFocus(app.wBlame);
		app.SendEditor(SCI_GRABFOCUS);
		break;
	case WM_CONTEXTMENU:
		{
			if (app.m_selectedrev <= 0)
				break;;
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			if ((xPos < 0)||(yPos < 0))
			{
				// requested from keyboard, not mouse pointer
				// use the center of the window
				RECT rect;
				GetClientRect(app.wBlame, &rect);
				xPos = rect.right-rect.left;
				yPos = rect.bottom-rect.top;
			}
			HMENU hMenu = LoadMenu(app.hResource, MAKEINTRESOURCE(IDR_BLAMEPOPUP));
			HMENU hPopMenu = GetSubMenu(hMenu, 0);

			if ( _tcslen(szOrigPath)==0 )
			{
				// Without knowing the original path we cannot blame the previous revision
				// because we don't know which filename to pass to tortoiseproc.
				EnableMenuItem(hPopMenu,ID_BLAME_PREVIOUS_REVISION, MF_DISABLED|MF_GRAYED);
			}
			
			TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, xPos, yPos, 0, app.wBlame, NULL); 
			DestroyMenu(hMenu);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndHeaderProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	switch (message) 
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(app.wHeader, &ps);
		app.DrawHeader(hDC);
		EndPaint(app.wHeader, &ps);
		break;
	case WM_COMMAND:
		break;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndLocatorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	switch (message) 
	{
	case WM_PAINT:
		hDC = BeginPaint(app.wLocator, &ps);
		app.DrawLocatorBar(hDC);
		EndPaint(app.wLocator, &ps);
		break;
	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
		if (wParam & MK_LBUTTON)
		{
			RECT rect;
			::GetClientRect(hWnd, &rect); 
			int nLine = HIWORD(lParam)*app.revs.size()/(rect.bottom-rect.top);

			if (nLine < 0)
				nLine = 0;
			app.ScrollToLine(nLine);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#endif

void CTortoiseGitBlameView::SetupLexer(CString filename)
{

	TCHAR *line;
	//const char * lineptr = _tcsrchr(filename, '.');
	int start=filename.ReverseFind(_T('.'));
	if (start>0)
	{
		//_tcscpy_s(line, 20, lineptr+1);
		//_tcslwr_s(line, 20);
		CString ext=filename.Right(filename.GetLength()-start-1);
		line=ext.GetBuffer();

		if ((_tcscmp(line, _T("py"))==0)||
			(_tcscmp(line, _T("pyw"))==0)||
			(_tcscmp(line, _T("pyw"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PYTHON);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield")).GetBuffer()));
			SetAStyle(SCE_P_DEFAULT, black);
			SetAStyle(SCE_P_COMMENTLINE, darkGreen);
			SetAStyle(SCE_P_NUMBER, RGB(0, 0x80, 0x80));
			SetAStyle(SCE_P_STRING, RGB(0, 0, 0x80));
			SetAStyle(SCE_P_CHARACTER, RGB(0, 0, 0x80));
			SetAStyle(SCE_P_WORD, RGB(0x80, 0, 0x80));
			SetAStyle(SCE_P_TRIPLE, black);
			SetAStyle(SCE_P_TRIPLEDOUBLE, black);
			SetAStyle(SCE_P_CLASSNAME, darkBlue);
			SetAStyle(SCE_P_DEFNAME, darkBlue);
			SetAStyle(SCE_P_OPERATOR, darkBlue);
			SetAStyle(SCE_P_IDENTIFIER, darkBlue);
			SetAStyle(SCE_P_COMMENTBLOCK, darkGreen);
			SetAStyle(SCE_P_STRINGEOL, red);
		}
		if ((_tcscmp(line, _T("c"))==0)||
			(_tcscmp(line, _T("cc"))==0)||
			(_tcscmp(line, _T("cpp"))==0)||
			(_tcscmp(line, _T("cxx"))==0)||
			(_tcscmp(line, _T("h"))==0)||
			(_tcscmp(line, _T("hh"))==0)||
			(_tcscmp(line, _T("hpp"))==0)||
			(_tcscmp(line, _T("hxx"))==0)||
			(_tcscmp(line, _T("dlg"))==0)||
			(_tcscmp(line, _T("mak"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("and and_eq asm auto bitand bitor bool break \
case catch char class compl const const_cast continue \
default delete do double dynamic_cast else enum explicit export extern false float for \
friend goto if inline int long mutable namespace new not not_eq \
operator or or_eq private protected public \
register reinterpret_cast return short signed sizeof static static_cast struct switch \
template this throw true try typedef typeid typename union unsigned using \
virtual void volatile wchar_t while xor xor_eq")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 3, (LPARAM)(m_TextView.StringForControl(_T("a addindex addtogroup anchor arg attention \
author b brief bug c class code date def defgroup deprecated dontinclude \
e em endcode endhtmlonly endif endlatexonly endlink endverbatim enum example exception \
f$ f[ f] file fn hideinitializer htmlinclude htmlonly \
if image include ingroup internal invariant interface latexonly li line link \
mainpage name namespace nosubgrouping note overload \
p page par param post pre ref relates remarks return retval \
sa section see showinitializer since skip skipline struct subsection \
test throw todo typedef union until \
var verbatim verbinclude version warning weakgroup $ @ \\ & < > # { }")).GetBuffer()));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("cs"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("abstract as base bool break byte case catch char checked class \
const continue decimal default delegate do double else enum \
event explicit extern false finally fixed float for foreach goto if \
implicit in int interface internal is lock long namespace new null \
object operator out override params private protected public \
readonly ref return sbyte sealed short sizeof stackalloc static \
string struct switch this throw true try typeof uint ulong \
unchecked unsafe ushort using virtual void while")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("rc"))==0)||
			(_tcscmp(line, _T("rc2"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("ACCELERATORS ALT AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON \
BEGIN BITMAP BLOCK BUTTON CAPTION CHARACTERISTICS CHECKBOX CLASS \
COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG DIALOGEX DISCARDABLE \
EDITTEXT END EXSTYLE FONT GROUPBOX ICON LANGUAGE LISTBOX LTEXT \
MENU MENUEX MENUITEM MESSAGETABLE POPUP \
PUSHBUTTON RADIOBUTTON RCDATA RTEXT SCROLLBAR SEPARATOR SHIFT STATE3 \
STRINGTABLE STYLE TEXTINCLUDE VALUE VERSION VERSIONINFO VIRTKEY")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("idl"))==0)||
			(_tcscmp(line, _T("odl"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("aggregatable allocate appobject arrays async async_uuid \
auto_handle \
bindable boolean broadcast byte byte_count \
call_as callback char coclass code comm_status \
const context_handle context_handle_noserialize \
context_handle_serialize control cpp_quote custom \
decode default defaultbind defaultcollelem \
defaultvalue defaultvtable dispinterface displaybind dllname \
double dual \
enable_allocate encode endpoint entry enum error_status_t \
explicit_handle \
fault_status first_is float \
handle_t heap helpcontext helpfile helpstring \
helpstringcontext helpstringdll hidden hyper \
id idempotent ignore iid_as iid_is immediatebind implicit_handle \
import importlib in include in_line int __int64 __int3264 interface \
last_is lcid length_is library licensed local long \
max_is maybe message methods midl_pragma \
midl_user_allocate midl_user_free min_is module ms_union \
ncacn_at_dsp ncacn_dnet_nsp ncacn_http ncacn_ip_tcp \
ncacn_nb_ipx ncacn_nb_nb ncacn_nb_tcp ncacn_np \
ncacn_spx ncacn_vns_spp ncadg_ip_udp ncadg_ipx ncadg_mq \
ncalrpc nocode nonbrowsable noncreatable nonextensible notify \
object odl oleautomation optimize optional out out_of_line \
pipe pointer_default pragma properties propget propput propputref \
ptr public \
range readonly ref represent_as requestedit restricted retval \
shape short signed size_is small source strict_context_handle \
string struct switch switch_is switch_type \
transmit_as typedef \
uidefault union unique unsigned user_marshal usesgetlasterror uuid \
v1_enum vararg version void wchar_t wire_marshal")).GetBuffer()));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("java"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("abstract assert boolean break byte case catch char class \
const continue default do double else extends final finally float for future \
generic goto if implements import inner instanceof int interface long \
native new null outer package private protected public rest \
return short static super switch synchronized this throw throws \
transient try var void volatile while")).GetBuffer()));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("js"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("abstract boolean break byte case catch char class \
const continue debugger default delete do double else enum export extends \
final finally float for function goto if implements import in instanceof \
int interface long native new package private protected public \
return short static super switch synchronized this throw throws \
transient try typeof var void volatile while with")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("pas"))==0)||
			(_tcscmp(line, _T("dpr"))==0)||
			(_tcscmp(line, _T("pp"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PASCAL);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("and array as begin case class const constructor \
destructor div do downto else end except file finally \
for function goto if implementation in inherited \
interface is mod not object of on or packed \
procedure program property raise record repeat \
set shl shr then threadvar to try type unit \
until uses var while with xor")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("as"))==0)||
			(_tcscmp(line, _T("asc"))==0)||
			(_tcscmp(line, _T("jsfl"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("add and break case catch class continue default delete do \
dynamic else eq extends false finally for function ge get gt if implements import in \
instanceof interface intrinsic le lt ne new not null or private public return \
set static super switch this throw true try typeof undefined var void while with")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 1, (LPARAM)(m_TextView.StringForControl(_T("Array Arguments Accessibility Boolean Button Camera Color \
ContextMenu ContextMenuItem Date Error Function Key LoadVars LocalConnection Math \
Microphone Mouse MovieClip MovieClipLoader NetConnection NetStream Number Object \
PrintJob Selection SharedObject Sound Stage String StyleSheet System TextField \
TextFormat TextSnapshot Video Void XML XMLNode XMLSocket \
_accProps _focusrect _global _highquality _parent _quality _root _soundbuftime \
arguments asfunction call capabilities chr clearInterval duplicateMovieClip \
escape eval fscommand getProperty getTimer getURL getVersion gotoAndPlay gotoAndStop \
ifFrameLoaded Infinity -Infinity int isFinite isNaN length loadMovie loadMovieNum \
loadVariables loadVariablesNum maxscroll mbchr mblength mbord mbsubstring MMExecute \
NaN newline nextFrame nextScene on onClipEvent onUpdate ord parseFloat parseInt play \
prevFrame prevScene print printAsBitmap printAsBitmapNum printNum random removeMovieClip \
scroll set setInterval setProperty startDrag stop stopAllSounds stopDrag substring \
targetPath tellTarget toggleHighQuality trace unescape unloadMovie unLoadMovieNum updateAfterEvent")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("html"))==0)||
			(_tcscmp(line, _T("htm"))==0)||
			(_tcscmp(line, _T("shtml"))==0)||
			(_tcscmp(line, _T("htt"))==0)||
			(_tcscmp(line, _T("xml"))==0)||
			(_tcscmp(line, _T("asp"))==0)||
			(_tcscmp(line, _T("xsl"))==0)||
			(_tcscmp(line, _T("php"))==0)||
			(_tcscmp(line, _T("xhtml"))==0)||
			(_tcscmp(line, _T("phtml"))==0)||
			(_tcscmp(line, _T("cfm"))==0)||
			(_tcscmp(line, _T("tpl"))==0)||
			(_tcscmp(line, _T("dtd"))==0)||
			(_tcscmp(line, _T("hta"))==0)||
			(_tcscmp(line, _T("htd"))==0)||
			(_tcscmp(line, _T("wxs"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_HTML);
			SendEditor(SCI_SETSTYLEBITS, 7);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("a abbr acronym address applet area b base basefont \
bdo big blockquote body br button caption center \
cite code col colgroup dd del dfn dir div dl dt em \
fieldset font form frame frameset h1 h2 h3 h4 h5 h6 \
head hr html i iframe img input ins isindex kbd label \
legend li link map menu meta noframes noscript \
object ol optgroup option p param pre q s samp \
script select small span strike strong style sub sup \
table tbody td textarea tfoot th thead title tr tt u ul \
var xml xmlns abbr accept-charset accept accesskey action align alink \
alt archive axis background bgcolor border \
cellpadding cellspacing char charoff charset checked cite \
class classid clear codebase codetype color cols colspan \
compact content coords \
data datafld dataformatas datapagesize datasrc datetime \
declare defer dir disabled enctype event \
face for frame frameborder \
headers height href hreflang hspace http-equiv \
id ismap label lang language leftmargin link longdesc \
marginwidth marginheight maxlength media method multiple \
name nohref noresize noshade nowrap \
object onblur onchange onclick ondblclick onfocus \
onkeydown onkeypress onkeyup onload onmousedown \
onmousemove onmouseover onmouseout onmouseup \
onreset onselect onsubmit onunload \
profile prompt readonly rel rev rows rowspan rules \
scheme scope selected shape size span src standby start style \
summary tabindex target text title topmargin type usemap \
valign value valuetype version vlink vspace width \
text password checkbox radio submit reset \
file hidden image")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 1, (LPARAM)(m_TextView.StringForControl(_T("assign audio block break catch choice clear disconnect else elseif \
emphasis enumerate error exit field filled form goto grammar help \
if initial link log menu meta noinput nomatch object option p paragraph \
param phoneme prompt property prosody record reprompt return s say-as \
script sentence subdialog submit throw transfer value var voice vxml")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 2, (LPARAM)(m_TextView.StringForControl(_T("accept age alphabet anchor application base beep bridge category charset \
classid cond connecttimeout content contour count dest destexpr dtmf dtmfterm \
duration enctype event eventexpr expr expritem fetchtimeout finalsilence \
gender http-equiv id level maxage maxstale maxtime message messageexpr \
method mime modal mode name namelist next nextitem ph pitch range rate \
scope size sizeexpr skiplist slot src srcexpr sub time timeexpr timeout \
transferaudio type value variant version volume xml:lang")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 3, (LPARAM)(m_TextView.StringForControl(_T("and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 4, (LPARAM)(m_TextView.StringForControl(_T("and argv as argc break case cfunction class continue declare default do \
die echo else elseif empty enddeclare endfor endforeach endif endswitch \
endwhile e_all e_parse e_error e_warning eval exit extends false for \
foreach function global http_cookie_vars http_get_vars http_post_vars \
http_post_files http_env_vars http_server_vars if include include_once \
list new not null old_function or parent php_os php_self php_version \
print require require_once return static switch stdclass this true var \
xor virtual while __file__ __line__ __sleep __wakeup")).GetBuffer()));

			SetAStyle(SCE_H_TAG, darkBlue);
			SetAStyle(SCE_H_TAGUNKNOWN, red);
			SetAStyle(SCE_H_ATTRIBUTE, darkBlue);
			SetAStyle(SCE_H_ATTRIBUTEUNKNOWN, red);
			SetAStyle(SCE_H_NUMBER, RGB(0x80,0,0x80));
			SetAStyle(SCE_H_DOUBLESTRING, RGB(0,0x80,0));
			SetAStyle(SCE_H_SINGLESTRING, RGB(0,0x80,0));
			SetAStyle(SCE_H_OTHER, RGB(0x80,0,0x80));
			SetAStyle(SCE_H_COMMENT, RGB(0x80,0x80,0));
			SetAStyle(SCE_H_ENTITY, RGB(0x80,0,0x80));

			SetAStyle(SCE_H_TAGEND, darkBlue);
			SetAStyle(SCE_H_XMLSTART, darkBlue);	// <?
			SetAStyle(SCE_H_QUESTION, darkBlue);	// <?
			SetAStyle(SCE_H_XMLEND, darkBlue);		// ?>
			SetAStyle(SCE_H_SCRIPT, darkBlue);		// <script
			SetAStyle(SCE_H_ASP, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <% ... %>
			SetAStyle(SCE_H_ASPAT, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <%@ ... %>

			SetAStyle(SCE_HB_DEFAULT, black);
			SetAStyle(SCE_HB_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HB_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HB_WORD, darkBlue);
			SendEditor(SCI_STYLESETBOLD, SCE_HB_WORD, 1);
			SetAStyle(SCE_HB_STRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HB_IDENTIFIER, black);

			// This light blue is found in the windows system palette so is safe to use even in 256 colour modes.
			// Show the whole section of VBScript with light blue background
			for (int bstyle=SCE_HB_DEFAULT; bstyle<=SCE_HB_STRINGEOL; bstyle++) {
				SendEditor(SCI_STYLESETFONT, bstyle, 
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, bstyle, lightBlue);
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HB_STRINGEOL, RGB(0x7F,0x7F,0xFF));
			SendEditor(SCI_STYLESETFONT, SCE_HB_COMMENTLINE, 
				reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));

			SetAStyle(SCE_HBA_DEFAULT, black);
			SetAStyle(SCE_HBA_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HBA_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HBA_WORD, darkBlue);
			SendEditor(SCI_STYLESETBOLD, SCE_HBA_WORD, 1);
			SetAStyle(SCE_HBA_STRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HBA_IDENTIFIER, black);

			// Show the whole section of ASP VBScript with bright yellow background
			for (int bastyle=SCE_HBA_DEFAULT; bastyle<=SCE_HBA_STRINGEOL; bastyle++) {
				SendEditor(SCI_STYLESETFONT, bastyle, 
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, bastyle, RGB(0xFF, 0xFF, 0));
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bastyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HBA_STRINGEOL, RGB(0xCF,0xCF,0x7F));
			SendEditor(SCI_STYLESETFONT, SCE_HBA_COMMENTLINE, 
				reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));

			// If there is no need to support embedded Javascript, the following code can be dropped.
			// Javascript will still be correctly processed but will be displayed in just the default style.

			SetAStyle(SCE_HJ_START, RGB(0x80,0x80,0));
			SetAStyle(SCE_HJ_DEFAULT, black);
			SetAStyle(SCE_HJ_COMMENT, darkGreen);
			SetAStyle(SCE_HJ_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HJ_COMMENTDOC, darkGreen);
			SetAStyle(SCE_HJ_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HJ_WORD, black);
			SetAStyle(SCE_HJ_KEYWORD, darkBlue);
			SetAStyle(SCE_HJ_DOUBLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJ_SINGLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJ_SYMBOLS, black);

			SetAStyle(SCE_HJA_START, RGB(0x80,0x80,0));
			SetAStyle(SCE_HJA_DEFAULT, black);
			SetAStyle(SCE_HJA_COMMENT, darkGreen);
			SetAStyle(SCE_HJA_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HJA_COMMENTDOC, darkGreen);
			SetAStyle(SCE_HJA_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HJA_WORD, black);
			SetAStyle(SCE_HJA_KEYWORD, darkBlue);
			SetAStyle(SCE_HJA_DOUBLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJA_SINGLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJA_SYMBOLS, black);

			SetAStyle(SCE_HPHP_DEFAULT, black);
			SetAStyle(SCE_HPHP_HSTRING,  RGB(0x80,0,0x80));
			SetAStyle(SCE_HPHP_SIMPLESTRING,  RGB(0x80,0,0x80));
			SetAStyle(SCE_HPHP_WORD, darkBlue);
			SetAStyle(SCE_HPHP_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HPHP_VARIABLE, red);
			SetAStyle(SCE_HPHP_HSTRING_VARIABLE, red);
			SetAStyle(SCE_HPHP_COMPLEX_VARIABLE, red);
			SetAStyle(SCE_HPHP_COMMENT, darkGreen);
			SetAStyle(SCE_HPHP_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HPHP_OPERATOR, darkBlue);

			// Show the whole section of Javascript with off white background
			for (int jstyle=SCE_HJ_DEFAULT; jstyle<=SCE_HJ_SYMBOLS; jstyle++) {
				SendEditor(SCI_STYLESETFONT, jstyle, 
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, jstyle, offWhite);
				SendEditor(SCI_STYLESETEOLFILLED, jstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HJ_STRINGEOL, RGB(0xDF, 0xDF, 0x7F));
			SendEditor(SCI_STYLESETEOLFILLED, SCE_HJ_STRINGEOL, 1);

			// Show the whole section of Javascript with brown background
			for (int jastyle=SCE_HJA_DEFAULT; jastyle<=SCE_HJA_SYMBOLS; jastyle++) {
				SendEditor(SCI_STYLESETFONT, jastyle, 
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, jastyle, RGB(0xDF, 0xDF, 0x7F));
				SendEditor(SCI_STYLESETEOLFILLED, jastyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HJA_STRINGEOL, RGB(0x0,0xAF,0x5F));
			SendEditor(SCI_STYLESETEOLFILLED, SCE_HJA_STRINGEOL, 1);
		}
	}
	else
	{
		SendEditor(SCI_SETLEXER, SCLEX_CPP);
		SetupCppLexer();
	}
	SendEditor(SCI_COLOURISE, 0, -1);

}

void CTortoiseGitBlameView::SetupCppLexer()
{
	SetAStyle(SCE_C_DEFAULT, RGB(0, 0, 0));
	SetAStyle(SCE_C_COMMENT, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTLINE, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOC, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTLINEDOC, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOCKEYWORD, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOCKEYWORDERROR, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_NUMBER, RGB(0, 0x80, 0x80));
	SetAStyle(SCE_C_WORD, RGB(0, 0, 0x80));
	SendEditor(SCE_C_WORD, 1);
	SetAStyle(SCE_C_STRING, RGB(0x80, 0, 0x80));
	SetAStyle(SCE_C_IDENTIFIER, RGB(0, 0, 0));
	SetAStyle(SCE_C_PREPROCESSOR, RGB(0x80, 0, 0));
	SetAStyle(SCE_C_OPERATOR, RGB(0x80, 0x80, 0));
}


void CTortoiseGitBlameView::UpdateInfo()
{
	CString &data = GetDocument()->m_BlameData;
	CString one;
	int pos=0;

	CLogDataVector * pRevs= GetLogData();

	this->m_CommitHash.clear();
	this->m_Authors.clear();
	this->m_ID.clear();
	CString line;

	CreateFont();

	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);

	while( pos>=0 )
	{
		one=data.Tokenize(_T("\n"),pos);
		m_CommitHash.push_back(one.Left(40));
		int start=0;
		start=one.Find(_T(')'),40);
		if(start>0)
		{
			line=one.Right(one.GetLength()-start-2);
			this->m_TextView.InsertText(line,true);
		}
		int id=pRevs->m_HashMap[one.Left(40)];		
		if(id>=0)
		{
			m_ID.push_back(pRevs->size()-id);
			m_Authors.push_back(pRevs->at(id).m_AuthorName);
		}else
		{
			ASSERT(FALSE);
		}
	}

	SetupLexer(GetDocument()->m_CurrentFileName);

	SendEditor(SCI_SETUNDOCOLLECTION, 1);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETSCROLLWIDTHTRACKING, TRUE);
	SendEditor(SCI_SETREADONLY, TRUE);


	this->Invalidate();
}

CLogDataVector * CTortoiseGitBlameView::GetLogData()
{
	return &(GetDocument()->GetMainFrame()->m_wndOutput.m_LogList.m_logEntries);
}

void CTortoiseGitBlameView::OnSciPainted(NMHDR *,LRESULT *)
{
	this->Invalidate();
}