// vtkSDI.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "vtkSDI.h"
#include "MainFrm.h"

#include "vtkSDIDoc.h"
#include "vtkSDIView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CvtkSDIApp

BEGIN_MESSAGE_MAP(CvtkSDIApp, CWinApp)
  ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
  // Standard file based document commands
  ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
  ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
  // Standard print setup command
  ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


// CvtkSDIApp construction

CvtkSDIApp::CvtkSDIApp()
{
  // TODO: add construction code here,
  // Place all significant initialization in InitInstance
}


// The one and only CvtkSDIApp object

CvtkSDIApp theApp;

// CvtkSDIApp initialization

BOOL CvtkSDIApp::InitInstance()
{
  // InitCommonControls() is required on Windows XP if an application
  // manifest specifies use of ComCtl32.dll version 6 or later to enable
  // visual styles.  Otherwise, any window creation will fail.
  InitCommonControls();

  CWinApp::InitInstance();

  // Initialize OLE libraries
  if (!AfxOleInit())
  {
    AfxMessageBox(IDP_OLE_INIT_FAILED);
    return FALSE;
  }
  AfxEnableControlContainer();
  // Standard initialization
  // If you are not using these features and wish to reduce the size
  // of your final executable, you should remove from the following
  // the specific initialization routines you do not need
  // Change the registry key under which our settings are stored
  // TODO: You should modify this string to be something appropriate
  // such as the name of your company or organization
  SetRegistryKey(_T("Local AppWizard-Generated Applications"));
  LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
  // Register the application's document templates.  Document templates
  //  serve as the connection between documents, frame windows and views
  CSingleDocTemplate* pDocTemplate;
  pDocTemplate = new CSingleDocTemplate(
    IDR_MAINFRAME,
    RUNTIME_CLASS(CvtkSDIDoc),
    RUNTIME_CLASS(CMainFrame),       // main SDI frame window
    RUNTIME_CLASS(CvtkSDIView));
  if (!pDocTemplate)
    return FALSE;
  AddDocTemplate(pDocTemplate);
  // Parse command line for standard shell commands, DDE, file open
  CCommandLineInfo cmdInfo;
  ParseCommandLine(cmdInfo);
  // Dispatch commands specified on the command line.  Will return FALSE if
  // app was launched with /RegServer, /Register, /Unregserver or /Unregister.
  if (!ProcessShellCommand(cmdInfo))
    return FALSE;
  // The one and only window has been initialized, so show and update it
  m_pMainWnd->ShowWindow(SW_SHOW);
  m_pMainWnd->UpdateWindow();
  // call DragAcceptFiles only if there's a suffix
  //  In an SDI app, this should occur after ProcessShellCommand
  return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
  CAboutDlg();

// Dialog Data
  enum { IDD = IDD_ABOUTBOX };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CvtkSDIApp::OnAppAbout()
{
  CAboutDlg aboutDlg;
  aboutDlg.DoModal();
}


// CvtkSDIApp message handlers

