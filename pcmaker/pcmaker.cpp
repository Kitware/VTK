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


// here is a simple helper function to help parse command line args
char *GetNextArg(char *in)
{
  static int pos = 0;
  static char result[1024];
  int len = strlen(in);
  int pos2;

  // skip any white space
  while ((pos <= len)&&(in[pos] == ' ')) pos++;

  if (pos > len) return NULL;

  // if we are at a quote then return quoted string
  if (in[pos] == '"')
    {
    pos++;
    pos2 = 0;
    while ((pos <= len)&&(in[pos] != '"'))
      {
      result[pos2] = in[pos];
      pos++;
      pos2++;
      }
    pos++;
    }
  else
    {
    pos2 = 0;
    while ((pos <= len)&&(in[pos] != ' '))
      {
      result[pos2] = in[pos];
      pos++;
      pos2++;
      }
    }

  result[pos2] = '\0';
  return result;
}

void ReadAValue(HKEY hKey,CString *val,char *key, char *adefault)
  {
  DWORD dwType, dwSize;
  char *pb;

  dwType = REG_SZ;
  pb = val->GetBuffer(MAX_PATH);
  dwSize = MAX_PATH;
  if(RegQueryValueEx(hKey,_T(key), NULL, &dwType, 
		       (BYTE *)pb, &dwSize) != ERROR_SUCCESS)
    {
    val->ReleaseBuffer();
    *val = _T(adefault);
    }
  else
    {
    val->ReleaseBuffer();
    }
  }

// ReadRegistry
// Read values for dialog from keys in the registry
int ReadRegistry(CPcmakerDlg &dlg)
{
  HKEY hKey;

  if(RegOpenKeyEx(HKEY_CURRENT_USER, 
		  _T("Software\\Kitware\\VTK PCMaker\\Settings"), 
		  0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
    return 0;
    }
  else
    {
    DWORD dwType, dwSize, dwData;
    
    // save some values
    ReadAValue(hKey, &(dlg.m_WhereVTK),"WhereVTK","C:\\vtk");
    ReadAValue(hKey, &(dlg.m_WhereBuild),"WhereBuild", "C:\\vtkbin");
    ReadAValue(hKey, &(dlg.m_WhereJDK),"WhereJDK","");
    ReadAValue(hKey, &(dlg.m_WherePy),"WherePy","");
    ReadAValue(hKey, &(dlg.m_WhereCompiler),"WhereCompiler",
      "C:\\Program Files\\DevStudio\\vc");

    // read the advanced options
    ReadAValue(hKey, &(dlg.adlg.m_EXTRA_CFLAGS),"EXTRA_CFLAGS","");
    ReadAValue(hKey, &(dlg.adlg.m_EXTRA_LINK_FLAGS),"EXTRA_LINK_FLAGS","");
    ReadAValue(hKey, &(dlg.adlg.m_WhereTcl),"WhereTcl","");
    ReadAValue(hKey, &(dlg.adlg.m_WhereTk),"WhereTk","");
    ReadAValue(hKey, &(dlg.adlg.m_LibPrefix),"LibPrefix","vtk");
    // if the value is an empty string then set it to vtk
    if (!dlg.adlg.m_LibPrefix || strlen(dlg.adlg.m_LibPrefix) < 1)
      {
      dlg.adlg.m_LibPrefix = _T("vtk");
      }
    ReadAValue(hKey, &(dlg.adlg.m_WhereMPIInclude),"WhereMPIInclude","");
    ReadAValue(hKey, &(dlg.adlg.m_WhereMPILibrary),"WhereMPILibrary","");

    // save which compiler
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    if(RegQueryValueEx(hKey, _T("Compiler"), NULL, &dwType, 
		       (BYTE *) &dwData, &dwSize) != ERROR_SUCCESS)
      {
      dlg.m_MSComp = TRUE;
      dlg.m_BorlandComp = FALSE;
      }
    else
      {
      dlg.m_MSComp = dwData&0x1;
      dlg.m_BorlandComp = (dwData&0x2)?TRUE:FALSE;
      }

    // save other flags
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    if(RegQueryValueEx(hKey, _T("Flags"), NULL, &dwType, 
		       (BYTE *)&dwData, &dwSize) != ERROR_SUCCESS)
      {
      dlg.m_Parallel = FALSE;
      dlg.m_Local = FALSE;
      dlg.m_Contrib = TRUE;
      dlg.m_Graphics = TRUE;
      dlg.m_Imaging = TRUE;
      dlg.m_Patented = FALSE;
      dlg.m_Lean = TRUE;
      dlg.m_BuildJava = FALSE;
      dlg.m_BuildPython = FALSE;
      dlg.m_BuildTcl = FALSE;
      dlg.adlg.m_UseMPI = FALSE;
      }
    else
      {
      dlg.m_Contrib = dwData&0x1;
      dlg.m_Graphics = (dwData&0x2)?TRUE:FALSE;
      dlg.m_Imaging = (dwData&0x4)?TRUE:FALSE;
      dlg.m_Patented = (dwData&0x8)?TRUE:FALSE;
      dlg.m_Lean = (dwData&0x10)?TRUE:FALSE;
      dlg.m_BuildJava = (dwData&0x20)?TRUE:FALSE;
      dlg.m_BuildPython = (dwData&0x40)?TRUE:FALSE;
      dlg.m_BuildTcl = (dwData&0x80)?TRUE:FALSE;
      dlg.m_Local = (dwData&0x100)?TRUE:FALSE;
      dlg.adlg.m_UseMPI = (dwData&0x200)?TRUE:FALSE;
      dlg.m_AnsiCpp = (dwData&0x400)?TRUE:FALSE;
      dlg.m_Parallel = (dwData&0x800)?TRUE:FALSE;
      }
    }

  RegCloseKey(hKey);
  return 1;
}

// WriteRegistry:
// writes the values from the dialog into the registry
void WriteRegistry(CPcmakerDlg &dlg)
{
  HKEY hKey;
  DWORD dwDummy;

  if(RegCreateKeyEx(HKEY_CURRENT_USER, 
		    _T("Software\\Kitware\\VTK PCMaker\\Settings"),
		    0, "", REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, 
		    NULL, &hKey, &dwDummy) != ERROR_SUCCESS) 
    {
    return;
    }
  else
    {
    DWORD dwData;
		
    RegSetValueEx(hKey, _T("WhereVTK"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.m_WhereVTK, 
		  dlg.m_WhereVTK.GetLength());
    RegSetValueEx(hKey, _T("WhereBuild"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.m_WhereBuild, 
		  dlg.m_WhereBuild.GetLength());
    RegSetValueEx(hKey, _T("WhereJDK"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.m_WhereJDK, 
		  dlg.m_WhereJDK.GetLength());
    RegSetValueEx(hKey, _T("WherePy"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.m_WherePy, 
		  dlg.m_WherePy.GetLength());
    RegSetValueEx(hKey, _T("WhereCompiler"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.m_WhereCompiler, 
		  dlg.m_WhereCompiler.GetLength());

    // save the advanced settings
    RegSetValueEx(hKey, _T("EXTRA_CFLAGS"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.adlg.m_EXTRA_CFLAGS, 
		  dlg.adlg.m_EXTRA_CFLAGS.GetLength());
    RegSetValueEx(hKey, _T("EXTRA_LINK_FLAGS"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.adlg.m_EXTRA_LINK_FLAGS, 
		  dlg.adlg.m_EXTRA_LINK_FLAGS.GetLength());
    RegSetValueEx(hKey, _T("WhereTcl"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.adlg.m_WhereTcl, 
		  dlg.adlg.m_WhereTcl.GetLength());
    RegSetValueEx(hKey, _T("WhereTk"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.adlg.m_WhereTk, 
		  dlg.adlg.m_WhereTk.GetLength());
    RegSetValueEx(hKey, _T("LibPrefix"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.adlg.m_LibPrefix, 
		  dlg.adlg.m_LibPrefix.GetLength());
    RegSetValueEx(hKey, _T("WhereMPIInclude"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.adlg.m_WhereMPIInclude, 
		  dlg.adlg.m_WhereMPIInclude.GetLength());
    RegSetValueEx(hKey, _T("WhereMPILibrary"), 0, REG_SZ, 
		  (CONST BYTE *)(const char *)dlg.adlg.m_WhereMPILibrary, 
		  dlg.adlg.m_WhereMPILibrary.GetLength());

    dwData = 0;
    dwData |= (dlg.m_MSComp)?1:0;
    dwData |= (dlg.m_BorlandComp)?2:0;
    RegSetValueEx(hKey, _T("Compiler"), 0, REG_DWORD, 
		  (CONST BYTE *)&dwData, sizeof(DWORD));
    dwData = 0;
    dwData |= (dlg.m_Contrib)?1:0;
    dwData |= (dlg.m_Graphics)?2:0;
    dwData |= (dlg.m_Imaging)?4:0;
    dwData |= (dlg.m_Patented)?8:0;
    dwData |= (dlg.m_Lean)?0x10:0;
    dwData |= (dlg.m_BuildJava)?0x20:0;
    dwData |= (dlg.m_BuildPython)?0x40:0;
    dwData |= (dlg.m_BuildTcl)?0x80:0;
    dwData |= (dlg.m_Local)?0x100:0;
    dwData |= (dlg.adlg.m_UseMPI)?0x200:0;
    dwData |= (dlg.m_AnsiCpp)?0x400:0;
    dwData |= (dlg.m_Parallel)?0x800:0;
    RegSetValueEx(hKey, _T("Flags"), 0, REG_DWORD, 
		  (CONST BYTE *)&dwData, sizeof(DWORD));
    }

  RegCloseKey(hKey);
}

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

   if(!ReadRegistry(dlg))
    { 
    // no registry, use default values
    dlg.m_WhereVTK = "C:\\vtk";
    dlg.m_WhereBuild = "C:\\vtkbin";
    dlg.m_WhereCompiler = "C:\\msdev";
    dlg.m_WhereJDK = "";
    dlg.m_WherePy = "";
    dlg.adlg.m_WhereTcl = "";
    dlg.adlg.m_WhereTk  = "";
    dlg.adlg.m_LibPrefix  = "vtk";
    dlg.adlg.m_WhereMPIInclude = "";
    dlg.adlg.m_WhereMPILibrary  = "";
    }
   if (strncmp(m_lpCmdLine, "nightly", 7) == 0)  // skip any trailing characters
    {
    // use the quality testing effort defaults
    dlg.m_WhereVTK = "d:\\nightly\\vtk";
    dlg.m_WhereBuild = "d:\\nightly\\vtkbin";
    dlg.m_WhereJDK = "D:\\jdk1.3";
    dlg.m_WherePy = "d:\\Python";
    dlg.adlg.m_WhereTcl = "";
    dlg.adlg.m_WhereTk  = "";
    dlg.adlg.m_EXTRA_LINK_FLAGS = "";
    dlg.adlg.m_LibPrefix  = "vtk";
    dlg.m_BuildJava = TRUE;
    dlg.m_BuildPython = TRUE;
    dlg.m_BuildTcl = TRUE;
    dlg.Create(IDD_PCMAKER_DIALOG,NULL);
    dlg.DoOKStuff();
    }
   else if (strncmp(m_lpCmdLine, "qualityNT", 9) == 0)  // skip any trailing characters
    {
    // use the quality testing effort defaults
    dlg.m_WhereVTK = "d:\\production\\vtk";
    dlg.m_WhereBuild = "d:\\production\\vtkbin";
    dlg.m_WhereJDK = "c:\\progra~1\\jdk1.2.1";
    dlg.m_WherePy = "";
    dlg.adlg.m_WhereTcl = "";
    dlg.adlg.m_WhereTk  = "";
    dlg.adlg.m_LibPrefix  = "vtk";
    dlg.Create(IDD_PCMAKER_DIALOG,NULL);
    dlg.DoOKStuff();
    }
   else if (strncmp(m_lpCmdLine, "quality98", 9) == 0)  // skip any trailing characters
    {
    // use the quality testing effort defaults
    dlg.m_WhereVTK = "c:\\production\\vtk";
    dlg.m_WhereBuild = "c:\\production\\vtkbin";
    dlg.m_WhereJDK = "";
    dlg.m_WherePy = "";
    dlg.adlg.m_WhereTcl = "";
    dlg.adlg.m_WhereTk  = "";
    dlg.Create(IDD_PCMAKER_DIALOG,NULL);
    dlg.DoOKStuff();
    dlg.adlg.m_LibPrefix  = "vtk";
    }
   else if (m_lpCmdLine[0] != '\0')
    {
    // use the registry alues
    dlg.Create(IDD_PCMAKER_DIALOG,NULL);
    dlg.DoOKStuff();
    }
   else
    {
    if(dlg.DoModal() == IDOK)
      {
      WriteRegistry(dlg);
      }
    }

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
