// advanced.cpp : implementation file
//

#include "stdafx.h"
#include "pcmaker.h"
#include "advanced.h"

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
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(advanced, CDialog)
	//{{AFX_MSG_MAP(advanced)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// advanced message handlers
