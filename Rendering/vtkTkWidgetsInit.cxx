#include <tcl.h>
#include <tk.h>

#include "vtkTkImageWindowWidget.h"

//----------------------------------------------------------------------------
// Vtkrenderingpythontkwidgets_Init
// Called upon system startup to create the widget commands.
extern "C" {VTK_TK_EXPORT int Vtkrenderingpythontkwidgets_Init(Tcl_Interp *interp);}

int vtkTkRenderWidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
			  int argc, char **argv);
int vtkTkImageViewerWidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
			       int argc, char **argv);
int vtkTkImageWindowWidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
			       int argc, char **argv);

int Vtkrenderingpythontkwidgets_Init(Tcl_Interp *interp)
{
  if (Tcl_PkgProvide(interp, (char *) "Vtkrenderingpythontkwidgets", (char *) "1.2") != TCL_OK) 
    {
    return TCL_ERROR;
    }
  
  Tcl_CreateCommand(interp, (char *) "vtkTkRenderWidget", vtkTkRenderWidget_Cmd, 
		    Tk_MainWindow(interp), NULL);
  Tcl_CreateCommand(interp, (char *) "vtkTkImageViewerWidget", 
		    vtkTkImageViewerWidget_Cmd, Tk_MainWindow(interp), NULL);
  Tcl_CreateCommand(interp, (char *) "vtkTkImageWindowWidget", 
		    vtkTkImageWindowWidget_Cmd, Tk_MainWindow(interp), NULL);
  
  return TCL_OK;
}
