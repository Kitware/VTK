// pcmakerDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPcmakerDlg dialog
#include "advanced.h"

class CPcmakerDlg : public CDialog
{
// Construction
public:
	CPcmakerDlg(CWnd* pParent = NULL);	// standard constructor
  void DoOKStuff();
  char TclRoot[512];
  char TkRoot[512];
  advanced adlg;

// Dialog Data
	//{{AFX_DATA(CPcmakerDlg)
	enum { IDD = IDD_PCMAKER_DIALOG };
	CProgressCtrl	m_Progress;
	CString	m_WhereVTK;
	CString	m_WhereBuild;
	CString	m_WhereJDK;
	BOOL	m_BorlandComp;
	BOOL	m_MSComp;
	BOOL	m_Contrib;
	BOOL	m_Graphics;
	BOOL	m_Imaging;
	CString	m_WhereCompiler;
	BOOL	m_Patented;
	BOOL	m_Lean;
	BOOL	m_BuildTcl;
	BOOL	m_BuildJava;
	BOOL	m_BuildPython;
	CString	m_WherePy;
	BOOL	m_Local;
	BOOL	m_AnsiCpp;
	BOOL	m_Parallel;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPcmakerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CPcmakerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnHelp1();
	afx_msg void OnAdvanced();
	afx_msg void OnVtkInstallBrowse();
	afx_msg void OnVtkLibBrowse();
	afx_msg void OnCompilerPathBrowse();
	afx_msg BOOL Browse(CString&, const char*);
	afx_msg void OnJdkWhere();
	afx_msg void OnPythonWhere();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
