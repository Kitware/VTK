// help.cpp : implementation file
//

#include "stdafx.h"
#include "pcmaker.h"
#include "help.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// help dialog


help::help(CWnd* pParent /*=NULL*/)
	: CDialog(help::IDD, pParent)
{
	//{{AFX_DATA_INIT(help)
	//}}AFX_DATA_INIT
	m_helptext = 
    "Help for pcmaker.exe\r\n"
    "====================\r\n\r\n"
    "pcmaker.exe is a program that generates Makefiles "
    "for vtk.  This is the first step in the process of "
    "compiling vtk on a PC. There are a few pieces of "
    "information that are required in order to build vtk. "
    "The first is the directory location of your vtk "
    "source code. This should not have any spaces in it. "
    "C:\\vtk is an example of what someone might type. "
    "Next you need to tell pcmaker where to place the "
    "resulting object files and libraries. C:\\vtkbin is "
    "one example. The directory you enter does not have "
    "to exist, pcmaker will ask you if you want it created.\r\n\r\n"
    "Next you'll need to provide the directory where your "
    "C++ compiler is installed. Some examples are:\r\n"
    "  C:\\msdev\r\n"
    "  C:\\program Files\\DevStudio\\vc\r\n"
    "  D:\\msvc40\r\n"
    "This path can have a space in it.\r\n\r\n"
    "If you plan to build Java libraries then you need to "
    "specify the path to your copy of the Java JDK. If "
    "you do not plan on building java libraries then you "
    "should leave this blank. Otherwise pcmaker will "
    "complain that it cannot find the required header files.\r\n\r\n"
    "Next you need to specify what compiler you are using "
    "from the options in the What Compiler box. You must"
    "select one and only one compiler. Next you need to decide "
    "what to include in your vtk build. Just select the "
    "different options from the remaining toggle buttons. "
    "They are relatively self explanatory. The Build Lean VTK "
    "option builds vtk without any debug macros. This means that "
    "DebugOn will have no effect. This option is most useful "
    "for developing final release versions of end users applications "
    "since the resulting dlls will be smaller and a little faster\r\n\r\n"
    "Once everything is set you can click on the OK button "
    "and pcmaker will start to generate the Makefiles. "
    "This will take a few minutes and a progress bar will "
    "indicate how far along the process is. While this is "
    "happening the remaining buttons will be inactive. "
    "Once this is complete pcmaker will exit.\r\n\r\n"
	"pcmaker generates makefiles for both debug and optimized "
	"versions of vtk. It also generates makefiles that support "
	"incremental linking. Here's how to use the different "
	"options. You need to open a MS-DOS shell from windows "
	"NT or 95. In that shell cd to your vtkbin directory. "
	"If you nmake in this directory you will get an incremental "
	"linking optimized build. If you cd down into the Debug "
	"directory and build there you will get an incremental "
	"linking debug build. To generate the non incremental "
	"libraries you need to issue nmake noninc (or build in the vtkdll "
        "subdirectory under vtkbin (for optimized) or vtkbin/Debug " 
        "(for debug), same goes for the Java, Python, or Tcl libs).\r\n\r\n"
	"nmake is the command to build vtk using Microsoft Visual "
	"C++. If you receive an error trying to run nmake then you "
	"probably need to set it up using the vcvars32 script "
	"that comes with Microsoft Visual C++. One example of "
	"how to run this is\r\n    "
    "  C:\\program files\\devstudio\\vc\\bin\\vcvars32\r\n\r\n"
	"Once vtk has been built you need tell windows how to "
	"find the dlls. One way is to copy the resulting dlls "
	"into your windows system directory. For incremental "
	"builds it is probably easier to add vtkbin/lib or "
	"vtkbin/Debug/lib to your PATH environment variable. "
	"This can be done using sysedit on Windows95 and using "
	"My Computer/Properties under windows NT."
    ;
}


void help::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(help)
	DDX_Text(pDX, IDC_EDIT1, m_helptext);
	DDV_MaxChars(pDX, m_helptext, 10000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(help, CDialog)
	//{{AFX_MSG_MAP(help)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// help message handlers
