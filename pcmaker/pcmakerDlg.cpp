// pcmakerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcmaker.h"
#include "pcmakerDlg.h"
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
	m_MSComp = FALSE;
	m_Contrib = TRUE;
	m_Graphics = TRUE;
	m_Imaging = TRUE;
	m_WhereCompiler = _T("");
	m_Debug = FALSE;
	m_GEMSIO = FALSE;
	m_GEMSIP = FALSE;
	m_GEMSVOLUME = FALSE;
	m_Patented = FALSE;
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
	DDV_MaxChars(pDX, m_WhereVTK, 80);
	DDX_Text(pDX, IDC_WHEREBUILD, m_WhereBuild);
	DDV_MaxChars(pDX, m_WhereBuild, 80);
	DDX_Text(pDX, IDC_WHEREJDK, m_WhereJDK);
	DDV_MaxChars(pDX, m_WhereJDK, 40);
	DDX_Check(pDX, IDC_BORLANDCOMP, m_BorlandComp);
	DDX_Check(pDX, IDC_MSCOMP, m_MSComp);
	DDX_Check(pDX, IDC_Contrib, m_Contrib);
	DDX_Check(pDX, IDC_Graphics, m_Graphics);
	DDX_Check(pDX, IDC_Imaging, m_Imaging);
	DDX_Text(pDX, IDC_WHERECOMPILER, m_WhereCompiler);
	DDV_MaxChars(pDX, m_WhereCompiler, 40);
	DDX_Check(pDX, IDC_Debug, m_Debug);
	DDX_Check(pDX, IDC_GEMSIO, m_GEMSIO);
	DDX_Check(pDX, IDC_GEMSIP, m_GEMSIP);
	DDX_Check(pDX, IDC_GEMSVOLUME, m_GEMSVOLUME);
	DDX_Check(pDX, IDC_PATENTED, m_Patented);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPcmakerDlg, CDialog)
	//{{AFX_MSG_MAP(CPcmakerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
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

extern void makeMakefile(CPcmakerDlg *vals);

void CPcmakerDlg::OnOK() 
{
  this->DoOKStuff();
}

void CPcmakerDlg::DoOKStuff()
{
  FILE *fp;
  char fname[128];
  char msg[256];
  struct stat statBuff;

  // TODO: Add extra validation here
  CWnd::UpdateData();

  // make sure we can find vtk
  sprintf(fname,"%s\\targets.c",this->m_WhereVTK);
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
  sprintf(fname,"%s\\vtkdll",this->m_WhereBuild);
  if (stat(fname,&statBuff) == -1) mkdir(fname);
  sprintf(fname,"%s\\vtktcl",this->m_WhereBuild);
  if (stat(fname,&statBuff) == -1) mkdir(fname);
  sprintf(fname,"%s\\vtktcl\\src",this->m_WhereBuild);
  if (stat(fname,&statBuff) == -1) mkdir(fname);
  sprintf(fname,"%s\\vtkjava",this->m_WhereBuild);
  if (stat(fname,&statBuff) == -1) mkdir(fname);
  sprintf(fname,"%s\\vtkjava\\src",this->m_WhereBuild);
  if (stat(fname,&statBuff) == -1) mkdir(fname);
  sprintf(fname,"%s\\vtkjava\\vtk",this->m_WhereBuild);
  if (stat(fname,&statBuff) == -1) mkdir(fname);

  makeMakefile(this);
  CDialog::OnOK();
}