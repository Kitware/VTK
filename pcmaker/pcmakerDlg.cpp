// pcmakerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcmaker.h"
#include "pcmakerDlg.h"
#include "help.h"
#include <afxinet.h>
#include <shlobj.h> // for browseinfo 
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPcmakerDlg dialog

CPcmakerDlg::CPcmakerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPcmakerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPcmakerDlg)
  	m_WhereVTK = _T("");
	m_WhereBuild = _T("");
	m_WhereJDK = _T("");
	m_BorlandComp = FALSE;
	m_MSComp = TRUE;
	m_Contrib = TRUE;
	m_Graphics = TRUE;
	m_Imaging = TRUE;
	m_WhereCompiler = _T("");
	m_Patented = FALSE;
	m_Lean = FALSE;
	m_BuildTcl = FALSE;
	m_BuildJava = FALSE;
	m_BuildPython = FALSE;
	m_WherePy = _T("");
	m_Local = FALSE;
	m_AnsiCpp = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPcmakerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPcmakerDlg)
	DDX_Control(pDX, IDC_PROGRESS1, m_Progress);
	DDX_Text(pDX, IDC_WHEREVTK, m_WhereVTK);
	DDV_MaxChars(pDX, m_WhereVTK, 512);
	DDX_Text(pDX, IDC_WHEREBUILD, m_WhereBuild);
	DDV_MaxChars(pDX, m_WhereBuild, 512);
	DDX_Text(pDX, IDC_WHEREJDK, m_WhereJDK);
	DDV_MaxChars(pDX, m_WhereJDK, 512);
	DDX_Check(pDX, IDC_BORLANDCOMP, m_BorlandComp);
	DDX_Check(pDX, IDC_MSCOMP, m_MSComp);
	DDX_Check(pDX, IDC_Contrib, m_Contrib);
	DDX_Check(pDX, IDC_Graphics, m_Graphics);
	DDX_Check(pDX, IDC_Imaging, m_Imaging);
	DDX_Text(pDX, IDC_WHERECOMPILER, m_WhereCompiler);
	DDV_MaxChars(pDX, m_WhereCompiler, 512);
	DDX_Check(pDX, IDC_PATENTED, m_Patented);
	DDX_Check(pDX, IDC_LEAN, m_Lean);
	DDX_Check(pDX, IDC_BUILDTCL, m_BuildTcl);
	DDX_Check(pDX, IDC_BUILDJAVA, m_BuildJava);
	DDX_Check(pDX, IDC_BUILDPYTHON, m_BuildPython);
	DDX_Text(pDX, IDC_WHEREPYTHON, m_WherePy);
	DDV_MaxChars(pDX, m_WherePy, 512);
	DDX_Check(pDX, IDC_LOCAL, m_Local);
	DDX_Check(pDX, IDC_ANSICPP, m_AnsiCpp);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPcmakerDlg, CDialog)
	//{{AFX_MSG_MAP(CPcmakerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_HELP1, OnHelp1)
	ON_BN_CLICKED(IDC_ADVANCED, OnAdvanced)
	ON_BN_CLICKED(IDC_VTK_INSTALL_BROWSE, OnVtkInstallBrowse)
	ON_BN_CLICKED(IDC_VTK_LIB_BROWSE, OnVtkLibBrowse)
	ON_BN_CLICKED(IDC_COMPILER_PATH_BROWSE, OnCompilerPathBrowse)
	ON_BN_CLICKED(IDC_JDK_WHERE, OnJdkWhere)
	ON_BN_CLICKED(IDC_PYTHON_WHERE, OnPythonWhere)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPcmakerDlg message handlers

BOOL CPcmakerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPcmakerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPcmakerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

extern void makeMakefiles(CPcmakerDlg *vals);

void CPcmakerDlg::OnOK() 
{
  this->DoOKStuff();
}

void CPcmakerDlg::DoOKStuff()
{
  FILE *fp;
  char fname[128], where[128];
  char msg[256];
  struct stat statBuff;

  // TODO: Add extra validation here
  CWnd::UpdateData();

  // make sure we can find vtk
  sprintf(fname,"%s\\common\\vtkObject.h",this->m_WhereVTK);
  fp = fopen(fname,"r");
  if (!fp)
    {
    sprintf(msg, "Unable to find vtk at: %s",this->m_WhereVTK);
    AfxMessageBox(msg);
    return;
    }
  fclose(fp);


  // make sure we can find the include files
  sprintf(fname,"%s\\include\\stdio.h",this->m_WhereCompiler);
  fp = fopen(fname,"r");
  if (!fp)
    {
    sprintf(msg, "Unable to find %s !!!\nMake sure you correctly specified the location of your compiler.",
      fname);
    AfxMessageBox(msg);
    return;
    }
  fclose(fp);

  if (!this->m_BorlandComp)
  {
  // make sure we can find opengl.lib files
  sprintf(fname,"%s\\lib\\opengl32.lib",this->m_WhereCompiler);
  fp = fopen(fname,"r");
  if (!fp)
    {
    sprintf(msg, "Unable to find %s !!!\nMake sure you correctly specified the location of your compiler.\nIf your compiler location is correct, make sure you have a copy\n of opengl32.lib and glaux.lib in the lib subdirectory.", fname);
    AfxMessageBox(msg);
    return;
    }
  fclose(fp);
   }

  // make sure we can find JDK
  if (this->m_BuildJava)
	  {
	  if (strlen(this->m_WhereJDK) > 1)
		  {
      sprintf(fname,"%s\\include\\jni.h",this->m_WhereJDK);
      fp = fopen(fname,"r");
      if (!fp)
        {
        sprintf(msg, "Unable to find JDK1.1 at: %s",this->m_WhereJDK);
        AfxMessageBox(msg);
        return;
        }
      fclose(fp);
      }
    }

  // make sure we can find Python
  if (this->m_BuildPython)
	  {
    sprintf(fname,"%s\\include\\Python.h",this->m_WherePy);
    fp = fopen(fname,"r");
    if (!fp)
      {
      sprintf(msg, "Unable to find Python at: %s",this->m_WherePy);
      AfxMessageBox(msg);
      return;
      }
    fclose(fp);
    }

  // Tcl/Tk development libs come in two flavors: sources or pre-built binaries
  
  // Sources =>
  //      libs :          TCL_ROOT/win/Release/tcl**.lib
  //                      TK_ROOT/win/Release/tk**.lib
  //      includes dirs : TCL_ROOT/win
  //                      TCL_ROOT/generic 
  //                      TK_ROOT/win
  //                      TK_ROOT/generic
  //                      TK_ROOT/xlib
  
  // Pre-built binaries =>
  //      libs :          TCLTK_ROOT/lib/tcl**.lib
  //                      TCLTK_ROOT/lib/tcl**.lib
  //      includes dirs : TCLTK_ROOT/include
  //
  // WARNING : as of tcl/tk 8.2, some include files are still missing :(
  // Extract them from the source package, and put them in TCLTK_ROOT/include :
  //       TK_ROOT/generic/tkInt.h
  //       TK_ROOT/generic/tkIntDecls.h
  //       TK_ROOT/generic/tkPlatDecls.h
  //       TK_ROOT/generic/tkIntPlatDecls.h
  //       TK_ROOT/win/tkWin.h
  //       TK_ROOT/win/tkWinInt.h
  //       TK_ROOT/win/tkPort.h
  //       TCL_ROOT/generic/tclInt.h
  //       TCL_ROOT/generic/tclIntDecls.h

  // Let's update TclRoot and TkRoot so that they match TCL_ROOT / TK_ROOT or
  // both TCLTK_ROOT.
  // Then use them to add the following INCLUDE path :
  //       /I TkRoot/win /I TkRoot/xblib /I TkRoot/generic 
  //       /I TclRoot/win /I TclRoot/generic
  //       /I TclRoot/include

  // make sure we can find tcl
  if (this->m_BuildTcl && strlen(this->adlg.m_WhereTcl) > 1)
    {
    int i;
    sprintf(fname,"%s",this->adlg.m_WhereTcl);
    fp = fopen(fname,"r");
    if (!fp)
      {
      sprintf(msg, "Unable to find libtcl at: %s",this->adlg.m_WhereTcl);
      AfxMessageBox(msg);
      return;
      }
    fclose(fp);

    i = strlen(this->adlg.m_WhereTcl) - 4;
    while (i)
    {
        if ((_strnicmp((const char *)this->adlg.m_WhereTcl + i,
                       "win",3) == 0) ||
            (_strnicmp((const char *)this->adlg.m_WhereTcl + i,
                       "lib",3) == 0))
        {
            int j;
            for (j = 0; j < i - 1; j++)
            {
                this->TclRoot[j] = this->adlg.m_WhereTcl[j];
            }
            this->TclRoot[j] = '\0';
            break;
        }
        i--;
    }
    }

  // make sure we can find tk
  if (this->m_BuildTcl && strlen(this->adlg.m_WhereTk) > 1)
    {
    int i;
    sprintf(fname,"%s",this->adlg.m_WhereTk);
    fp = fopen(fname,"r");
    if (!fp)
      {
      sprintf(msg, "Unable to find libtk at: %s",this->adlg.m_WhereTk);
      AfxMessageBox(msg);
      return;
      }
    fclose(fp);

    i = strlen(this->adlg.m_WhereTk) - 4;
    while (i)
    {
        if ((_strnicmp((const char *)this->adlg.m_WhereTk + i,
                      "win",3) == 0) ||
            (_strnicmp((const char *)this->adlg.m_WhereTk + i,
                       "lib",3) == 0))
        {
            int j;
            for (j = 0; j < i - 1; j++)
            {
                this->TkRoot[j] = this->adlg.m_WhereTk[j];
            }
            this->TkRoot[j] = '\0';
            break;
        }
        i--;
    }
    }

  // make sure only one compile is specified
  if ((this->m_MSComp + this->m_BorlandComp) > 1)
    {
    AfxMessageBox("Please specify only one compiler.");
    return;
    }
  if (!this->m_MSComp && !this->m_BorlandComp)
    {
    AfxMessageBox("Please specify a compiler.");
    return;
    }
  
  // make sure we can get to build directory
  //does it already exists?
  if (stat(this->m_WhereBuild,&statBuff) == -1)
    {
    // it didn't exist should we create it
    sprintf(msg,"The build directory %s does not exist.\nWould you like me to create it ?",
            this->m_WhereBuild);
    if (AfxMessageBox(msg,MB_YESNO) == IDNO) return;
    if (mkdir(this->m_WhereBuild) == -1)
      {
      sprintf(msg,"There was an error trying to create the directory %s.",this->m_WhereBuild);
      AfxMessageBox(msg);
      return;
      }
    }

  // make the subdirectories if they don't exist
  for (int i = 0; i < 2; i++)
    {
    if (i == 0)   // Debug subDirecteries
      {
      sprintf(where,"%s\\Debug",this->m_WhereBuild);
  	  if (stat(where,&statBuff) == -1) mkdir(where);
      }
    else
      {
      sprintf(where,"%s",this->m_WhereBuild);
      }

  	sprintf(fname,"%s\\vtkdll",where);
	  if (stat(fname,&statBuff) == -1) mkdir(fname);
  	sprintf(fname,"%s\\vtkdll\\obj",where);
	  if (stat(fname,&statBuff) == -1) mkdir(fname);
  	sprintf(fname,"%s\\vtkdll\\src",where);
	  if (stat(fname,&statBuff) == -1) mkdir(fname);
    if (this->m_BuildTcl)
      {
      sprintf(fname,"%s\\vtktcl",where);
	    if (stat(fname,&statBuff) == -1) mkdir(fname);
	    sprintf(fname,"%s\\vtktcl\\src",where);
	    if (stat(fname,&statBuff) == -1) mkdir(fname);
      }
    if (this->m_BuildJava)
      {
	    sprintf(fname,"%s\\vtkjava",where);
	    if (stat(fname,&statBuff) == -1) mkdir(fname);
	    sprintf(fname,"%s\\vtkjava\\src",where);
	    if (stat(fname,&statBuff) == -1) mkdir(fname);
	    sprintf(fname,"%s\\vtkjava\\vtk",where);
	    if (stat(fname,&statBuff) == -1) mkdir(fname);
      }
    if (this->m_BuildPython)
      {
      sprintf(fname,"%s\\vtkpython",where);
	    if (stat(fname,&statBuff) == -1) mkdir(fname);
	    sprintf(fname,"%s\\vtkpython\\src",where);
	    if (stat(fname,&statBuff) == -1) mkdir(fname);
      }
    sprintf(fname,"%s\\lib",where);
	  if (stat(fname,&statBuff) == -1) mkdir(fname);
		}

  makeMakefiles(this);
  CDialog::OnOK();
}

void CPcmakerDlg::OnHelp1() 
{
	// TODO: Add your control notification handler code here
  help dlg;
  dlg.DoModal();	
}

void CPcmakerDlg::OnAdvanced() 
{
	// TODO: Add your control notification handler code here
  this->adlg.DoModal();
}

BOOL CPcmakerDlg::Browse(CString& result, const char * title)
{
	// don't know what to do with initial right now...
  char szPathName[4096];
  BROWSEINFO bi;
 
  bi.hwndOwner = m_hWnd;
  bi.pidlRoot = NULL;
  bi.pszDisplayName = (LPTSTR)szPathName;
  bi.lpszTitle = title;
  bi.ulFlags = BIF_BROWSEINCLUDEFILES  ;
  bi.lpfn = NULL;

  LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

  BOOL bSuccess = SHGetPathFromIDList(pidl, szPathName);
  if(bSuccess)
    {
    result = szPathName;
    }
  
  return bSuccess;
}

void CPcmakerDlg::OnVtkInstallBrowse() 
{
	// TODO: Add your control notification handler code here
  this->UpdateData();
  Browse(m_WhereVTK, "Select VTK source installation");
  this->UpdateData(FALSE);
}

void CPcmakerDlg::OnVtkLibBrowse() 
{
  this->UpdateData();
  Browse(m_WhereBuild, "Select VTK build directory");
  this->UpdateData(FALSE);
}

void CPcmakerDlg::OnCompilerPathBrowse() 
{
  this->UpdateData();
  Browse(m_WhereCompiler, "Select path to compiler installation");
  this->UpdateData(FALSE);
}

void CPcmakerDlg::OnJdkWhere() 
{
	// TODO: Add your control notification handler code here
  this->UpdateData();
  Browse(m_WhereJDK, "Select path to JDK");
  this->UpdateData(FALSE);
}

void CPcmakerDlg::OnPythonWhere() 
{
	// TODO: Add your control notification handler code here
  this->UpdateData();
  Browse(m_WherePy, "Select path to Python");
  this->UpdateData(FALSE);
}
