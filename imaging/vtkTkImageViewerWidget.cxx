/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkImageViewerWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>

#include "vtkTkImageViewerWidget.h"
#include <tcl.h>
#include <tk.h>

#ifdef _WIN32
#include "vtkImageWin32Viewer.h"
#else
#include "vtkImageXViewer.h"
#endif

#define VTK_ALL_EVENTS_MASK \
    KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|	\
    EnterWindowMask|LeaveWindowMask|PointerMotionMask|ExposureMask|	\
    VisibilityChangeMask|FocusChangeMask|PropertyChangeMask|ColormapChangeMask

#ifndef MAX
#define MAX(a,b)	(((a)>(b))?(a):(b))
#endif
    
// These are the options that can be set when the widget is created
// or with the command configure.  The only new one is "-rw" which allows
// the uses to set their own ImageViewer window.
static Tk_ConfigSpec vtkTkImageViewerWidgetConfigSpecs[] = {
    {TK_CONFIG_PIXELS, "-height", "height", "Height",
     "400", Tk_Offset(struct vtkTkImageViewerWidget, Height), 0, NULL},
  
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
     "400", Tk_Offset(struct vtkTkImageViewerWidget, Width), 0, NULL},
  
    {TK_CONFIG_STRING, "-iv", "iv", "IV",
     "", Tk_Offset(struct vtkTkImageViewerWidget, IV), 0, NULL},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
     (char *) NULL, 0, 0, NULL}
};


// Foward prototypes
static void vtkTkImageViewerWidget_EventProc(ClientData clientData, 
					     XEvent *eventPtr);
static int vtkTkImageViewerWidget_MakeImageViewer(struct vtkTkImageViewerWidget *self);
extern int vtkImageViewerCommand(ClientData cd, Tcl_Interp *interp,
				 int argc, char *argv[]);

    
//----------------------------------------------------------------------------
// It's possible to change with this function or in a script some
// options like width, hieght or the ImageViewer widget.
int vtkTkImageViewerWidget_Configure(Tcl_Interp *interp, 
				     struct vtkTkImageViewerWidget *self,
				     int argc, char *argv[], int flags) 
{
  // Let Tk handle generic configure options.
  if (Tk_ConfigureWidget(interp, self->TkWin, 
			 vtkTkImageViewerWidgetConfigSpecs,
			 argc, argv, (char *)self, flags) == TCL_ERROR) 
    {
    return(TCL_ERROR);
    }
  
  // Get the new  width and height of the widget
  Tk_GeometryRequest(self->TkWin, self->Width, self->Height);
  
  // Make sure the ImageViewer window has been set.  If not, create one.
  if (vtkTkImageViewerWidget_MakeImageViewer(self) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  
  return TCL_OK;
}

//----------------------------------------------------------------------------
// This function is called when the ImageViewer widget name is 
// evaluated in a Tcl script.  It will compare string parameters
// to choose the appropriate method to invoke.
int vtkTkImageViewerWidget_Widget(ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[]) 
{
  struct vtkTkImageViewerWidget *self = 
    (struct vtkTkImageViewerWidget *)clientData;
  int result = TCL_OK;
  
  // Check to see if the command has enough arguments.
  if (argc < 2) 
    {
    Tcl_AppendResult(interp, "wrong # args: should be \"",
		     argv[0], " ?options?\"", NULL);
    return TCL_ERROR;
    }
  
  // Make sure the widget is not deleted during this function
  Tk_Preserve((ClientData)self);
  
  // Handle configure method
  if (!strncmp(argv[1], "configure", MAX(1, strlen(argv[1])))) 
    {
    if (argc == 2) 
      {
      /* Return list of all configuration parameters */
      result = Tk_ConfigureInfo(interp, self->TkWin, 
				vtkTkImageViewerWidgetConfigSpecs,
				(char *)self, (char *)NULL, 0);
      }
    else if (argc == 3) 
      {
      /* Return a specific configuration parameter */
      result = Tk_ConfigureInfo(interp, self->TkWin, 
				vtkTkImageViewerWidgetConfigSpecs,
				(char *)self, argv[2], 0);
      }
    else 
      {
      /* Execute a configuration change */
      result = vtkTkImageViewerWidget_Configure(interp, self, argc-2, 
					   argv+2, TK_CONFIG_ARGV_ONLY);
      }
    }
  else if (!strcmp(argv[1], "GetImageViewer"))
    { // Get ImageViewerWindow is my own method
    // Create a ImageViewerWidget if one has not been set yet.
    result = vtkTkImageViewerWidget_MakeImageViewer(self);
    if (result != TCL_ERROR)
      {
      // Return the name (Make Tcl copy the string)
      Tcl_SetResult(interp, self->IV, TCL_VOLATILE);
      }
    }
  else 
    {
    // Unknown method name.
    Tcl_AppendResult(interp, "vtkTkImageViewerWidget: Unknown option: ", argv[1], 
		     "\n", "Try: configure or GetImageViewerWindow\n", NULL);
    result = TCL_ERROR;
    }

  // Unlock the object so it can be deleted.
  Tk_Release((ClientData)self);
  return result;
}

//----------------------------------------------------------------------------
// vtkTkImageViewerWidget_Cmd
// Called when vtkTkImageViewerWidget is executed 
// - creation of a vtkTkImageViewerWidget widget.
//     * Creates a new window
//     * Creates an 'vtkTkImageViewerWidget' data structure
//     * Creates an event handler for this window
//     * Creates a command that handles this object
//     * Configures this vtkTkImageViewerWidget for the given arguments
static int vtkTkImageViewerWidget_Cmd(ClientData clientData, 
				      Tcl_Interp *interp, 
				      int argc, char **argv)
{
  char *name;
  Tk_Window main = (Tk_Window)clientData;
  Tk_Window tkwin;
  struct vtkTkImageViewerWidget *self;
  
  // Make sure we have an instance name.
  if (argc <= 1) 
    {
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, 
		     "wrong # args: should be \"pathName read filename\"", 
		     NULL);
    return(TCL_ERROR);
    }
  
  // Create the window.
  name = argv[1];
  // Possibly X dependent
  tkwin = Tk_CreateWindowFromPath(interp, main, name, (char *) NULL);
  if (tkwin == NULL) 
    {
    return TCL_ERROR;
    }
  
  // Tcl needs this for setting options and matching event bindings.
  Tk_SetClass(tkwin, "vtkTkImageViewerWidget");
  
  // Create vtkTkImageViewerWidget data structure 
  self = (struct vtkTkImageViewerWidget *)
    malloc(sizeof(struct vtkTkImageViewerWidget));
  self->TkWin = tkwin;
  self->Interp = interp;
  self->Width = 0;
  self->Height = 0;
  self->ImageViewer = NULL;
  self->IV = NULL;
  
  // ...
  // Create command event handler
  Tcl_CreateCommand(interp, Tk_PathName(tkwin), vtkTkImageViewerWidget_Widget, 
		    (ClientData)self, (void (*)(ClientData)) NULL);
  Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
			vtkTkImageViewerWidget_EventProc, (ClientData)self);
  
  // Configure vtkTkImageViewerWidget widget
  if (vtkTkImageViewerWidget_Configure(interp, self, argc-2, argv+2, 0) 
      == TCL_ERROR) 
    {
    Tk_DestroyWindow(tkwin);
    Tcl_DeleteCommand(interp, "vtkTkImageViewerWidget");
    // Don't free it, if we do a crash occurs later...
    //free(self);  
    return TCL_ERROR;
    }
  
  Tcl_AppendResult(interp, Tk_PathName(tkwin), NULL);
  return TCL_OK;
}


//----------------------------------------------------------------------------
char *vtkTkImageViewerWidget_IV(const struct vtkTkImageViewerWidget *self)
{
  return self->IV;
}


//----------------------------------------------------------------------------
int vtkTkImageViewerWidget_Width( const struct vtkTkImageViewerWidget *self)
{
   return self->Width;
}


//----------------------------------------------------------------------------
int vtkTkImageViewerWidget_Height( const struct vtkTkImageViewerWidget *self)
{
   return self->Height;
}

//----------------------------------------------------------------------------
// This gets called to handle vtkTkImageViewerWidget wind configuration events
// Possibly X dependent
static void vtkTkImageViewerWidget_EventProc(ClientData clientData, 
					     XEvent *eventPtr) 
{
  struct vtkTkImageViewerWidget *self = 
    (struct vtkTkImageViewerWidget *)clientData;
  
  switch (eventPtr->type) 
    {
    case Expose:
      if ((eventPtr->xexpose.count == 0)
	  /* && !self->UpdatePending*/) 
	{
	self->ImageViewer->Render();
	}
      break;
    case ConfigureNotify:
      if ( 1 /*Tk_IsMapped(self->TkWin)*/ ) 
        {
	self->Width = Tk_Width(self->TkWin);
	self->Height = Tk_Height(self->TkWin);
        Tk_GeometryRequest(self->TkWin,self->Width,self->Height);
	if (self->ImageViewer)
	  {
          self->ImageViewer->SetPosition(Tk_X(self->TkWin),Tk_Y(self->TkWin));
	  self->ImageViewer->SetSize(self->Width, self->Height);
	  }
	//vtkTkImageViewerWidget_PostRedisplay(self);
	}
      break;
    case MapNotify:
      break;
    case DestroyNotify:
      // Tcl_EventuallyFree( (ClientData) self, vtkTkImageViewerWidget_Destroy );
      break;
    default:
      // nothing
      ;
    }
}



//----------------------------------------------------------------------------
// vtkTkImageViewerWidget_Init
// Called upon system startup to create vtkTkImageViewerWidget command.
extern "C" {int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);}
int Vtktkimageviewerwidget_Init(Tcl_Interp *interp)
{
  if (Tcl_PkgProvide(interp, "Vtktkimageviewerwidget", "1.2") != TCL_OK) 
    {
    return TCL_ERROR;
    }
  
  Tcl_CreateCommand(interp, "vtkTkImageViewerWidget", 
		    vtkTkImageViewerWidget_Cmd, 
		    Tk_MainWindow(interp), NULL);
  
  return TCL_OK;
}


// Here is the windows specific code for creating the window
// The Xwindows version follows after this
#ifdef _WIN32

LRESULT APIENTRY vtkTkImageViewerWidgetProc(HWND hWnd, UINT message, 
					    WPARAM wParam, LPARAM lParam)
{
  LRESULT rval;
  struct vtkTkImageViewerWidget *self = 
    (struct vtkTkImageViewerWidget *)GetWindowLong(hWnd,GWL_USERDATA);
  
  // forward message to Tk handler
  SetWindowLong(hWnd,GWL_USERDATA,(LONG)((TkWindow *)self->TkWin)->window);
  if (((TkWindow *)self->TkWin)->parentPtr)
    {
    SetWindowLong(hWnd,GWL_WNDPROC,(LONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd,message,wParam,lParam);
    }
  else
    {
    SetWindowLong(hWnd,GWL_WNDPROC,(LONG)TkWinTopLevelProc);
    rval = TkWinTopLevelProc(hWnd,message,wParam,lParam);
    }

    if (message != WM_PAINT)
      {
      SetWindowLong(hWnd,GWL_USERDATA,(LONG)self->ImageViewer);
      SetWindowLong(hWnd,GWL_WNDPROC,(LONG)self->OldProc);
      self->OldProc(hWnd,message,wParam,lParam);
      }

    // now reset to the original config
    SetWindowLong(hWnd,GWL_USERDATA,(LONG)self);
    SetWindowLong(hWnd,GWL_WNDPROC,(LONG)vtkTkImageViewerWidgetProc);
    return rval;
}

//-----------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int vtkTkImageViewerWidget_MakeImageViewer(struct vtkTkImageViewerWidget *self) 
{
  Display *dpy;
  TkWindow *winPtr = (TkWindow *) self->TkWin;
  TkWindow *winPtr2;
  Tcl_HashEntry *hPtr;
  int new_flag;
  vtkImageWin32Viewer *ImageViewer;
  TkWinDrawable *twdPtr;
  HWND parentWin;

  if (self->ImageViewer)
    {
    return TCL_OK;
    }
  
  if (winPtr->window != None) 
    {
    // XDestroyWindow(dpy, winPtr->window);
    }

  if (self->IV[0] == '\0')
    {
    // Make the ImageViewer window.
    self->ImageViewer = vtkImageViewer::New();
    ImageViewer = (vtkImageWin32Viewer *)(self->ImageViewer);
    vtkTclGetObjectFromPointer(self->Interp, self->ImageViewer,
			       vtkImageViewerCommand);
    self->IV = strdup(self->Interp->result);
    self->Interp->result[0] = '\0';
    }
  else
    {
    self->ImageViewer = (vtkImageViewer *)
      vtkTclGetPointerFromObject(self->IV, "vtkImageViewer", self->Interp);
    ImageViewer = (vtkImageWin32Viewer *)(self->ImageViewer);
    }
  
  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);
  
  // Set the parent correctly
  // Possibly X dependent
  if ((winPtr->parentPtr != NULL) && !(winPtr->flags & TK_TOP_LEVEL)) 
    {
    if (winPtr->parentPtr->window == None) 
      {
      Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
      }

    parentWin = ((TkWinDrawable *)winPtr->parentPtr->window)->window.handle;
    ImageViewer->SetParentId(parentWin);
    }
  
  // Use the same display
  self->ImageViewer->SetDisplayId(dpy);
  
  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  //Tk_SetWindowVisual(self->TkWin, ImageViewer->GetDesiredVisual(), 
  //ImageViewer->GetDesiredDepth(), 
  //ImageViewer->GetDesiredColormap());
  
  self->ImageViewer->Render();  

  twdPtr = (TkWinDrawable*) ckalloc(sizeof(TkWinDrawable));
  twdPtr->type = TWD_WINDOW;
  twdPtr->window.winPtr = winPtr;
  twdPtr->window.handle = ImageViewer->GetWindowId();
  self->OldProc = (WNDPROC)GetWindowLong(twdPtr->window.handle,GWL_WNDPROC);
  SetWindowLong(twdPtr->window.handle,GWL_USERDATA,(LONG)self);
  SetWindowLong(twdPtr->window.handle,GWL_WNDPROC,(LONG)vtkTkImageViewerWidgetProc);

  winPtr->window = (Window)twdPtr;
  
  hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable,
			     (char *) winPtr->window, &new_flag);
  Tcl_SetHashValue(hPtr, winPtr);
  
  winPtr->dirtyAtts = 0;
  winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
  winPtr->inputContext = NULL;
#endif // TK_USE_INPUT_METHODS 

  if (!(winPtr->flags & TK_TOP_LEVEL)) 
    {
    /*
     * If this window has a different colormap than its parent, add
     * the window to the WM_COLORMAP_WINDOWS property for its top-level.
     */
    if ((winPtr->parentPtr != NULL) &&
	      (winPtr->atts.colormap != winPtr->parentPtr->atts.colormap)) 
      {
      TkWmAddToColormapWindows(winPtr);
      }
    } 

  /*
   * Issue a ConfigureNotify event if there were deferred configuration
   * changes (but skip it if the window is being deleted;  the
   * ConfigureNotify event could cause problems if we're being called
   * from Tk_DestroyWindow under some conditions).
   */
  if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
      && !(winPtr->flags & TK_ALREADY_DEAD))
    {
    XEvent event;
    
    winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;
    
    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above) 
      {
      event.xconfigure.above = winPtr->changes.sibling;
      }
    else 
      {
      event.xconfigure.above = None;
      }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
    }

  return TCL_OK;
}

// now the Xwindows version
#else

//----------------------------------------------------------------------------
// Creates a ImageViewer window and forces Tk to use the window.
static int
vtkTkImageViewerWidget_MakeImageViewer(struct vtkTkImageViewerWidget *self) 
{
  Display *dpy;
  TkWindow *winPtr = (TkWindow *) self->TkWin;
  TkWindow *winPtr2;
  Tcl_HashEntry *hPtr;
  int new_flag;
  vtkImageXViewer *ImageViewer;
  
  if (self->ImageViewer)
    {
    return TCL_OK;
    }
  
  dpy = Tk_Display(self->TkWin);
  
  if (winPtr->window != None) 
    {
    XDestroyWindow(dpy, winPtr->window);
    }

  if (self->IV[0] == '\0')
    {
    // Make the ImageViewer window.
    self->ImageViewer = vtkImageViewer::New();
    ImageViewer = (vtkImageXViewer *)(self->ImageViewer);
    vtkTclGetObjectFromPointer(self->Interp, self->ImageViewer,
			       vtkImageViewerCommand);
    self->IV = strdup(self->Interp->result);
    self->Interp->result[0] = '\0';
    }
  else
    {
    ImageViewer = (vtkImageXViewer *)
      vtkTclGetPointerFromObject(self->IV,"vtkImageViewer",self->Interp);
    self->ImageViewer = (vtkImageViewer *)(ImageViewer);
    }
  
  // Set the size
  self->ImageViewer->SetSize(self->Width, self->Height);
  
  // Set the parent correctly
  // Possibly X dependent
  if ((winPtr->parentPtr == NULL) || (winPtr->flags & TK_TOP_LEVEL)) 
    {
    ImageViewer->SetParentId(XRootWindow(winPtr->display, winPtr->screenNum));
    }
  else 
    {
    if (winPtr->parentPtr->window == None) 
      {
      Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
      }
    ImageViewer->SetParentId(winPtr->parentPtr->window);
    }

  // Use the same display
  ImageViewer->SetDisplayId(dpy);
  
  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  Tk_SetWindowVisual(self->TkWin, ImageViewer->GetDesiredVisual(), 
		     ImageViewer->GetDesiredDepth(), 
		     ImageViewer->GetDesiredColormap());
  
  self->ImageViewer->Render();  
  winPtr->window = ImageViewer->GetWindowId();
  XSelectInput(dpy, winPtr->window, VTK_ALL_EVENTS_MASK);
  
  hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable,
			     (char *) winPtr->window, &new_flag);
  Tcl_SetHashValue(hPtr, winPtr);
  
  winPtr->dirtyAtts = 0;
  winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
  winPtr->inputContext = NULL;
#endif // TK_USE_INPUT_METHODS 

  if (!(winPtr->flags & TK_TOP_LEVEL)) 
    {
    /*
     * If any siblings higher up in the stacking order have already
     * been created then move this window to its rightful position
     * in the stacking order.
     *
     * NOTE: this code ignores any changes anyone might have made
     * to the sibling and stack_mode field of the window's attributes,
     * so it really isn't safe for these to be manipulated except
     * by calling Tk_RestackWindow.
     */

    for (winPtr2 = winPtr->nextPtr; winPtr2 != NULL;
	 winPtr2 = winPtr2->nextPtr) 
      {
      if ((winPtr2->window != None) && !(winPtr2->flags & TK_TOP_LEVEL)) 
	{
	XWindowChanges changes;
	changes.sibling = winPtr2->window;
	changes.stack_mode = Below;
	XConfigureWindow(winPtr->display, winPtr->window,
			 CWSibling|CWStackMode, &changes);
	break;
	}
      }
    
    /*
     * If this window has a different colormap than its parent, add
     * the window to the WM_COLORMAP_WINDOWS property for its top-level.
     */
    if ((winPtr->parentPtr != NULL) &&
	(winPtr->atts.colormap != winPtr->parentPtr->atts.colormap)) 
      {
      TkWmAddToColormapWindows(winPtr);
      }
    } 

  /*
   * Issue a ConfigureNotify event if there were deferred configuration
   * changes (but skip it if the window is being deleted;  the
   * ConfigureNotify event could cause problems if we're being called
   * from Tk_DestroyWindow under some conditions).
   */
  if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
      && !(winPtr->flags & TK_ALREADY_DEAD))
    {
    XEvent event;
    
    winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;
    
    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above) 
      {
      event.xconfigure.above = winPtr->changes.sibling;
      }
    else 
      {
      event.xconfigure.above = None;
      }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
    }

  return TCL_OK;
}
#endif
