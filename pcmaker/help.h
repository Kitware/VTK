// help.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// help dialog

class help : public CDialog
{
// Construction
public:
	help(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(help)
	enum { IDD = IDD_HELPDIALOG };
	CString	m_helptext;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(help)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(help)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
