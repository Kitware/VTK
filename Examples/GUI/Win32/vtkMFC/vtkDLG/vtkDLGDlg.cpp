// vtkDLGDlg.cpp : implementation file
//

#include "stdafx.h"
#include "vtkDLG.h"
#include "vtkDLGDlg.h"

#include "vtkCallbackCommand.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CvtkDLGDlg dialog

CvtkDLGDlg::CvtkDLGDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CvtkDLGDlg::IDD, pParent)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  this->pvtkMFCWindow     = NULL;

  // set data reader to null
  this->pvtkDataSetReader = NULL;

  // Create the the renderer, window and interactor objects.
  this->pvtkRenderer    = vtkRenderer::New();

  // Create the the objects used to form the visualisation.
  this->pvtkDataSetMapper  = vtkDataSetMapper::New();
  this->pvtkActor      = vtkActor::New();
  this->pvtkActor2D    = vtkActor2D::New();
  this->pvtkTextMapper  = vtkTextMapper::New();

  this->ptBorder = CPoint(0,0);
}

void CvtkDLGDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CvtkDLGDlg, CDialog)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  //}}AFX_MSG_MAP
  ON_WM_DESTROY()
  ON_WM_SIZE()
  ON_BN_CLICKED(ID_LOADFILE, OnBtnLoadFile)
  ON_BN_CLICKED(ID_RESETSCENE, OnBtnResetScene)
END_MESSAGE_MAP()


// CvtkDLGDlg message handlers

void CvtkDLGDlg::ExecutePipeline()
{
  if (pvtkDataSetReader)  // have file
  {
    this->pvtkDataSetMapper->SetInput(pvtkDataSetReader->GetOutput());
    this->pvtkActor->SetMapper(this->pvtkDataSetMapper);

    this->pvtkTextMapper->SetInput(pvtkDataSetReader->GetFileName());
    this->pvtkTextMapper->GetTextProperty()->SetFontSize(12);
    this->pvtkActor2D->SetMapper(this->pvtkTextMapper);

    this->pvtkRenderer->SetBackground(0.0,0.0,0.4);
    this->pvtkRenderer->AddActor(this->pvtkActor);
    this->pvtkRenderer->AddActor(this->pvtkActor2D);
  }
  else  // have no file
  {
    this->pvtkTextMapper->SetInput("Hello World");
    this->pvtkTextMapper->GetTextProperty()->SetFontSize(24);
    this->pvtkActor2D->SetMapper(this->pvtkTextMapper);

    this->pvtkRenderer->SetBackground(0.0,0.0,0.4);
    this->pvtkRenderer->AddActor(this->pvtkActor2D);
  }
  this->pvtkRenderer->ResetCamera();
}

static void handle_double_click(vtkObject* obj, unsigned long,
                                void*, void*)
{
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  if(iren && iren->GetRepeatCount())
    {
    AfxMessageBox("Double Click");
    }
}


BOOL CvtkDLGDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    CString strAboutMenu;
    strAboutMenu.LoadString(IDS_ABOUTBOX);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);      // Set big icon
  SetIcon(m_hIcon, FALSE);    // Set small icon

  // adjust dialog & window size
  this->pvtkMFCWindow = new vtkMFCWindow(this->GetDlgItem(IDC_MAIN_WND));
  // get double click events
  vtkCallbackCommand* callback = vtkCallbackCommand::New();
  callback->SetCallback(handle_double_click);
  this->pvtkMFCWindow->GetInteractor()->AddObserver(vtkCommand::LeftButtonPressEvent, callback, 1.0);
  

  CRect cRectVTK;
  this->pvtkMFCWindow->GetClientRect(&cRectVTK);

  CRect cRectClient;
  GetClientRect(&cRectClient);

  this->ptBorder.x = cRectClient.Width()  - cRectVTK.Width();
  this->ptBorder.y = cRectClient.Height() - cRectVTK.Height();

  // set the vtk renderer, windows, etc
  this->pvtkMFCWindow->GetRenderWindow()->AddRenderer(this->pvtkRenderer);

  // execute object pipeline
  ExecutePipeline();
  
  return TRUE;  // return TRUE  unless you set the focus to a control
}

void CvtkDLGDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CDialog::OnSysCommand(nID, lParam);
  }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CvtkDLGDlg::OnPaint() 
{
  if (IsIconic())
  {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CvtkDLGDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

void CvtkDLGDlg::OnDestroy()
{
  // delete data
  if (this->pvtkDataSetReader)  this->pvtkDataSetReader->Delete();

  // Delete the the renderer, window and interactor objects.
  if (this->pvtkRenderer)      this->pvtkRenderer->Delete();

  // Delete the the objects used to form the visualisation.
  if (this->pvtkDataSetMapper)  this->pvtkDataSetMapper->Delete();
  if (this->pvtkActor)      this->pvtkActor->Delete();
  if (this->pvtkActor2D)      this->pvtkActor2D->Delete();
  if (this->pvtkTextMapper)    this->pvtkTextMapper->Delete();

  if (this->pvtkMFCWindow)
    {
    delete this->pvtkMFCWindow;
    }

  CDialog::OnDestroy();
}

void CvtkDLGDlg::OnSize(UINT nType, int cx, int cy)
{
  CDialog::OnSize(nType, cx, cy);

  if (::IsWindow(this->GetSafeHwnd()))
  {
    if (this->pvtkMFCWindow)
    {
      cx -= ptBorder.x;
      cy -= ptBorder.y;
      this->GetDlgItem(IDC_MAIN_WND)->SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
      this->pvtkMFCWindow->SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
    }
  }
}

void CvtkDLGDlg::OnBtnLoadFile()
{
  static char BASED_CODE szFilter[] = "VTK Files (*.vtk)|*.vtk|All Files (*.*)|*.*||";
  CFileDialog cFileDialog(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);

  if (cFileDialog.DoModal() == IDOK)
  {
    // remove old actors
    this->pvtkRenderer->RemoveActor(this->pvtkActor);
    this->pvtkRenderer->RemoveActor(this->pvtkActor2D);
  
    // read new data
    if (!this->pvtkDataSetReader)
      this->pvtkDataSetReader = vtkDataSetReader::New();
    this->pvtkDataSetReader->SetFileName(cFileDialog.GetPathName());
  
    // execute object pipeline
    ExecutePipeline();

    // update window
    if (this->pvtkMFCWindow)
      this->pvtkMFCWindow->RedrawWindow();
  }
}

void CvtkDLGDlg::OnBtnResetScene()
{
  // remove old actors
  this->pvtkRenderer->RemoveActor(this->pvtkActor);
  this->pvtkRenderer->RemoveActor(this->pvtkActor2D);

  // delete data
  if (this->pvtkDataSetReader)
  {
    this->pvtkDataSetReader->Delete();
    this->pvtkDataSetReader = NULL;
  }

  // execute object pipeline
  ExecutePipeline();

  // update window
  if (this->pvtkMFCWindow)
    this->pvtkMFCWindow->RedrawWindow();
}
