// pcmakerDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPcmakerDlg dialog

class CPcmakerDlg : public CDialog
{
// Construction
public:
	CPcmakerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPcmakerDlg)
	enum { IDD = IDD_PCMAKER_DIALOG };
	CString	m_WhereVTK;
	CString	m_WhereBuild;
	CString	m_WhereJDK;
	BOOL	m_BorlandComp;
	BOOL	m_MSComp;
	BOOL	m_Contrib;
	BOOL	m_Graphics;
	BOOL	m_Imaging;
	CString	m_WhereCompiler;
	BOOL	m_Debug;
	BOOL	m_GEMSIO;
	BOOL	m_GEMSIP;
	BOOL	m_GEMSVOLUME;
	BOOL	m_Patented;
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
