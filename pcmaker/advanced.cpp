// advanced.cpp : implementation file
//

#include "stdafx.h"
#include "pcmaker.h"
#include "advanced.h"
#include <shlobj.h> // for browseinfo 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// advanced dialog


advanced::advanced(CWnd* pParent /*=NULL*/)
	: CDialog(advanced::IDD, pParent)
{
	//{{AFX_DATA_INIT(advanced)
	m_EXTRA_CFLAGS = _T("");
	m_EXTRA_LINK_FLAGS = _T("");
	m_WhereTcl = _T("");
	m_WhereTk = _T("");
	m_LibPrefix = _T("vtk");
	m_WhereMPIInclude = _T("");
	m_WhereMPILibrary = _T("");
	m_UseMPI = FALSE;
	//}}AFX_DATA_INIT
}


void advanced::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(advanced)
	DDX_Text(pDX, IDC_EXTRA_CFLAGS, m_EXTRA_CFLAGS);
	DDV_MaxChars(pDX, m_EXTRA_CFLAGS, 512);
	DDX_Text(pDX, IDC_EXTRA_LINK_FLAGS, m_EXTRA_LINK_FLAGS);
	DDV_MaxChars(pDX, m_EXTRA_LINK_FLAGS, 512);
	DDX_Text(pDX, IDC_WHERETCL, m_WhereTcl);
	DDV_MaxChars(pDX, m_WhereTcl, 512);
	DDX_Text(pDX, IDC_WHERETK, m_WhereTk);
	DDV_MaxChars(pDX, m_WhereTk, 512);
	DDX_Text(pDX, IDC_LIBPREFIX, m_LibPrefix);
	DDV_MaxChars(pDX, m_LibPrefix, 40);
	DDX_Text(pDX, IDC_WHEREMPIINCLUDE, m_WhereMPIInclude);
	DDV_MaxChars(pDX, m_WhereMPIInclude, 512);
	DDX_Text(pDX, IDC_WHEREMPILIB, m_WhereMPILibrary);
	DDV_MaxChars(pDX, m_WhereMPILibrary, 512);
	DDX_Check(pDX, IDC_USEMPI, m_UseMPI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(advanced, CDialog)
	//{{AFX_MSG_MAP(advanced)
	ON_BN_CLICKED(IDC_WhereLibTCL, OnWhereLibTCL)
	ON_BN_CLICKED(IDC_WHERE_LIBTK, OnWhereLibtk)
	ON_BN_CLICKED(IDC_BROWSEMPIINCLUDE, OnBrowsempiinclude)
	ON_BN_CLICKED(IDC_BROWSEMPILIB, OnBrowsempilib)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// advanced message handlers

BOOL advanced::Browse(CString& result, const char* title)
{
 
  CFileDialog dialog(TRUE, NULL, NULL, 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
		"Library Files (*.lib)|*.lib||" );
  dialog.m_ofn.lpstrTitle = title;
  if(dialog.DoModal() == IDOK)
  {
	  result = dialog.GetPathName();
	  return true;
  }
  return false;
}

BOOL advanced::BrowseFolder(CString& result, const char* title)
{
  char path[MAX_PATH];
  
  BROWSEINFO bi;
  bi.hwndOwner = NULL;
  bi.pidlRoot = NULL;
  bi.pszDisplayName = path; 
  bi.lpszTitle = "Select Folder"; 
  bi.ulFlags = 0; 
  bi.lpfn = NULL; 
  bi.lParam = 0; 
  
  LPITEMIDLIST foo = SHBrowseForFolder(&bi);
  if (foo)
    {
    SHGetPathFromIDList(foo,path);
    result = path;
    return true;
    }
  return false;
}

void advanced::OnWhereLibTCL() 
{
	// TODO: Add your control notification handler code here
  this->UpdateData();
  Browse(m_WhereTcl, "Select TCL library");
  this->UpdateData(FALSE);	
}

void advanced::OnWhereLibtk() 
{
	// TODO: Add your control notification handler code here
  this->UpdateData();
  Browse(m_WhereTk, "Select TK library");
  this->UpdateData(FALSE);		
}

void advanced::OnBrowsempiinclude() 
{
  // TODO: Add your control notification handler code here
  this->UpdateData();
  BrowseFolder(m_WhereMPIInclude, "Select MPI Include Directory");
  this->UpdateData(FALSE);	
	
}

void advanced::OnBrowsempilib() 
{
  // TODO: Add your control notification handler code here
  this->UpdateData();
  BrowseFolder(m_WhereMPILibrary, "Select MPI Library library");
  this->UpdateData(FALSE);	
	
}
