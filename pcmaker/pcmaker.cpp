// pcmaker.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "pcmaker.h"
#include "pcmakerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPcmakerApp

BEGIN_MESSAGE_MAP(CPcmakerApp, CWinApp)
	//{{AFX_MSG_MAP(CPcmakerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPcmakerApp construction

CPcmakerApp::CPcmakerApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPcmakerApp object

CPcmakerApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPcmakerApp initialization

BOOL CPcmakerApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CPcmakerDlg dlg;
	m_pMainWnd = &dlg;

  if (m_lpCmdLine[0] != '\0')
    {
    char temps[256];
    int tempi;
    int count = 0;

    if (sscanf(strtok(m_lpCmdLine," "),"%s",temps) == 1)
      dlg.m_WhereVTK = strdup(temps);
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%s",temps) == 1)
      dlg.m_WhereBuild = strdup(temps);
    else
      AfxMessageBox("Incorrect command line arguments!");
    
    if (sscanf(strtok(NULL," "),"%s",temps) == 1)
      dlg.m_WhereCompiler = strdup(temps);
    else
      AfxMessageBox("Incorrect command line arguments!");
    
    if (sscanf(strtok(NULL," "),"%s",temps) == 1)
      dlg.m_WhereJDK = strdup(temps);
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_MSComp = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_BorlandComp = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_Debug = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_Patented = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_Graphics = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_Imaging = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_Contrib = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_GEMSIO = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_GEMSIP = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    if (sscanf(strtok(NULL," "),"%i",&tempi) == 1)
      dlg.m_GEMSVOLUME = tempi;
    else
      AfxMessageBox("Incorrect command line arguments!");

    dlg.Create(IDD_PCMAKER_DIALOG,NULL);
    dlg.DoOKStuff();
    }
  else
    {
    dlg.m_WhereVTK = "C:\\vtk";
    dlg.m_WhereBuild = "C:\\vtkbin";
	  dlg.m_WhereCompiler = "C:\\msdev";
    dlg.m_WhereJDK = "";
  	dlg.DoModal();
    }

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
