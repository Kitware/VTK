

// .NAME vtkTkImageWindowWidget - a Tk Widget for viewing vtk images

// .SECTION Description
// vtkTkImageWindowWidget is a Tk widget that you can render into. It has a 
// GetImageWindow method that returns a vtkImageWindow. You can also 
// specify a vtkImageWindow to be used when creating the widget by using
// the -iv option. It also takes -width and -height options.


// .SECTION Event Bindings
// Events can be bound on this widget just liek any other Tk widget.

// .SECTION See Also
// vtkImageWindow


#ifndef __vtkTkImageWindowWidget_h
#define __vtkTkImageWindowWidget_h

#include "vtkImageWindow.h"
#include "vtkTclUtil.h"

struct vtkTkImageWindowWidget
{
  Tk_Window  TkWin;		/* Tk window structure */
  Tcl_Interp *Interp;		/* Tcl interpreter */
  int Width;
  int Height;
  vtkImageWindow *ImageWindow;
  char *IV;
#ifdef _WIN32
  WNDPROC OldProc;
#endif
};

// This widget requires access to structures that are normally 
// not visible to Tcl/Tk applications. For this reason you must
// have access to tkInt.h
#include "tkInt.h"

#ifdef _WIN32
extern "C" {
#include "tkWinInt.h" 
}
#endif

#endif








