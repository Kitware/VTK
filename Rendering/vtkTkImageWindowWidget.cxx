

#include <stdlib.h>

#include "vtkTkImageWindowWidget.h"
#include <tcl.h>
#include <tk.h>

#ifdef _WIN32
#pragma warning ( disable : 4273 )
#else
#include "vtkXImageWindow.h"
#endif

#define VTK_ALL_EVENTS_MASK \
    KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|	\
    EnterWindowMask|LeaveWindowMask|PointerMotionMask|ExposureMask|	\
    VisibilityChangeMask|FocusChangeMask|PropertyChangeMask|ColormapChangeMask

#define VTK_MAX(a,b)	(((a)>(b))?(a):(b))
    
// These are the options that can be set when the widget is created
// or with the command configure.  The only new one is "-rw" which allows
// the uses to set their own ImageWindow window.
static Tk_ConfigSpec vtkTkImageWindowWidgetConfigSpecs[] = {
    {TK_CONFIG_PIXELS, (char *) "-height", (char *) "height", (char *) "Height",
     (char *) "400", Tk_Offset(struct vtkTkImageWindowWidget, Height), 0, NULL},
  
    {TK_CONFIG_PIXELS, (char *) "-width", (char *) "width", (char *) "Width",
     (char *) "400", Tk_Offset(struct vtkTkImageWindowWidget, Width), 0, NULL},
  
    {TK_CONFIG_STRING, (char *) "-iw", (char *) "iw", (char *) "IW",
     (char *) "", Tk_Offset(struct vtkTkImageWindowWidget, IW), 0, NULL},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
     (char *) NULL, 0, 0, NULL}
};


// Foward prototypes
static void vtkTkImageWindowWidget_EventProc(ClientData clientData, 
					     XEvent *eventPtr);
static int vtkTkImageWindowWidget_MakeImageWindow(struct vtkTkImageWindowWidget *self);
extern int vtkImageWindowCommand(ClientData cd, Tcl_Interp *interp,
				 int argc, char *argv[]);

    
//----------------------------------------------------------------------------
// It's possible to change with this function or in a script some
// options like width, hieght or the ImageWindow widget.
int vtkTkImageWindowWidget_Configure(Tcl_Interp *interp, 
				     struct vtkTkImageWindowWidget *self,
				     int argc, char *argv[], int flags) 
{
  // Let Tk handle generic configure options.
  if (Tk_ConfigureWidget(interp, self->TkWin, 
			 vtkTkImageWindowWidgetConfigSpecs,
			 argc, argv, (char *)self, flags) == TCL_ERROR) 
    {
    return(TCL_ERROR);
    }
  
  // Get the new  width and height of the widget
  Tk_GeometryRequest(self->TkWin, self->Width, self->Height);
  
  // Make sure the ImageWindow window has been set.  If not, create one.
  if (vtkTkImageWindowWidget_MakeImageWindow(self) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  

  return TCL_OK;
}

//----------------------------------------------------------------------------
// This function is called when the ImageWindow widget name is 
// evaluated in a Tcl script.  It will compare string parameters
// to choose the appropriate method to invoke.
int vtkTkImageWindowWidget_Widget(ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[]) 
{
  struct vtkTkImageWindowWidget *self = 
    (struct vtkTkImageWindowWidget *)clientData;
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
  
  // Handle render call to the widget
  if (strncmp(argv[1], "render", VTK_MAX(1, strlen(argv[1]))) == 0 || 
      strncmp(argv[1], "Render", VTK_MAX(1, strlen(argv[1]))) == 0) 
    {
    // make sure we have a window
    if (self->ImageWindow == NULL)
      {
      vtkTkImageWindowWidget_MakeImageWindow(self);
      }
    self->ImageWindow->Render();
    }
  // Handle configure method
  else if (!strncmp(argv[1], "configure", VTK_MAX(1, strlen(argv[1])))) 
    {
    if (argc == 2) 
      {
      /* Return list of all configuration parameters */
      result = Tk_ConfigureInfo(interp, self->TkWin, 
				vtkTkImageWindowWidgetConfigSpecs,
				(char *)self, (char *)NULL, 0);
      }
    else if (argc == 3) 
      {
      /* Return a specific configuration parameter */
      result = Tk_ConfigureInfo(interp, self->TkWin, 
				vtkTkImageWindowWidgetConfigSpecs,
				(char *)self, argv[2], 0);
      }
    else 
      {
      /* Execute a configuration change */
      result = vtkTkImageWindowWidget_Configure(interp, self, argc-2, 
					   argv+2, TK_CONFIG_ARGV_ONLY);
      }
    }
  else if (!strcmp(argv[1], "GetImageWindow"))
    { // Get ImageWindowWindow is my own method
    // Create a ImageWindowWidget if one has not been set yet.
    result = vtkTkImageWindowWidget_MakeImageWindow(self);
    if (result != TCL_ERROR)
      {
      // Return the name (Make Tcl copy the string)
      Tcl_SetResult(interp, self->IW, TCL_VOLATILE);
      }
    }
  else 
    {
    // Unknown method name.
    Tcl_AppendResult(interp, "vtkTkImageWindowWidget: Unknown option: ", argv[1], 
		     "\n", "Try: configure or GetImageWindow\n", NULL);
    result = TCL_ERROR;
    }

  // Unlock the object so it can be deleted.
  Tk_Release((ClientData)self);
  return result;
}

//----------------------------------------------------------------------------
// vtkTkImageWindowWidget_Cmd
// Called when vtkTkImageWindowWidget is executed 
// - creation of a vtkTkImageWindowWidget widget.
//     * Creates a new window
//     * Creates an 'vtkTkImageWindowWidget' data structure
//     * Creates an event handler for this window
//     * Creates a command that handles this object
//     * Configures this vtkTkImageWindowWidget for the given arguments
int vtkTkImageWindowWidget_Cmd(ClientData clientData, 
			       Tcl_Interp *interp, 
			       int argc, char **argv)
{
  char *name;
  Tk_Window main = (Tk_Window)clientData;
  Tk_Window tkwin;
  struct vtkTkImageWindowWidget *self;
  
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
  Tk_SetClass(tkwin, (char *) "vtkTkImageWindowWidget");
  
  // Create vtkTkImageWindowWidget data structure 
  self = (struct vtkTkImageWindowWidget *)
    malloc(sizeof(struct vtkTkImageWindowWidget));
  self->TkWin = tkwin;
  self->Interp = interp;
  self->Width = 0;
  self->Height = 0;
  self->ImageWindow = NULL;
  self->IW = NULL;
  
  // ...
  // Create command event handler
  Tcl_CreateCommand(interp, Tk_PathName(tkwin), vtkTkImageWindowWidget_Widget, 
		    (ClientData)self, (void (*)(ClientData)) NULL);
  Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
			vtkTkImageWindowWidget_EventProc, (ClientData)self);
  
  // Configure vtkTkImageWindowWidget widget
  if (vtkTkImageWindowWidget_Configure(interp, self, argc-2, argv+2, 0) 
      == TCL_ERROR) 
    {
    Tk_DestroyWindow(tkwin);
    Tcl_DeleteCommand(interp, (char *) "vtkTkImageWindowWidget");
    // Don't free it, if we do a crash occurs later...
    //free(self);  
    return TCL_ERROR;
    }
  
  Tcl_AppendResult(interp, Tk_PathName(tkwin), NULL);
  return TCL_OK;
}


//----------------------------------------------------------------------------
char *vtkTkImageWindowWidget_IW(const struct vtkTkImageWindowWidget *self)
{
  return self->IW;
}


//----------------------------------------------------------------------------
int vtkTkImageWindowWidget_Width( const struct vtkTkImageWindowWidget *self)
{
   return self->Width;
}


//----------------------------------------------------------------------------
int vtkTkImageWindowWidget_Height( const struct vtkTkImageWindowWidget *self)
{
   return self->Height;
}

static void vtkTkImageWindowWidget_Destroy(char *memPtr)
{
  struct vtkTkImageWindowWidget *self = (struct vtkTkImageWindowWidget *)memPtr;

  if (self->ImageWindow)
    {
		// Squash the ImageWindow's WindowID
	  self->ImageWindow->SetWindowId ( (void*)NULL );
    self->ImageWindow->UnRegister(NULL);
    self->ImageWindow = NULL;
    ckfree (self->IW);
    }
  ckfree((char *) memPtr);
}

//----------------------------------------------------------------------------
// This gets called to handle vtkTkImageWindowWidget wind configuration events
// Possibly X dependent
static void vtkTkImageWindowWidget_EventProc(ClientData clientData, 
					     XEvent *eventPtr) 
{
  struct vtkTkImageWindowWidget *self = 
    (struct vtkTkImageWindowWidget *)clientData;
  
  switch (eventPtr->type) 
    {
    case Expose:
      if ((eventPtr->xexpose.count == 0)
	  /* && !self->UpdatePending*/) 
	{
	// bind this explicitely in tcl now
	//self->ImageWindow->Render();
	}
      break;
    case ConfigureNotify:
      if ( 1 /*Tk_IsMapped(self->TkWin)*/ ) 
        {
	self->Width = Tk_Width(self->TkWin);
	self->Height = Tk_Height(self->TkWin);
        //Tk_GeometryRequest(self->TkWin,self->Width,self->Height);
	if (self->ImageWindow)
	  {
          self->ImageWindow->SetPosition(Tk_X(self->TkWin),Tk_Y(self->TkWin));
	  self->ImageWindow->SetSize(self->Width, self->Height);
	  }
	//vtkTkImageWindowWidget_PostRedisplay(self);
	}
      break;
    case MapNotify:
      break;
    case DestroyNotify:
      Tcl_EventuallyFree( (ClientData) self, vtkTkImageWindowWidget_Destroy );
      break;
    default:
      // nothing
      ;
    }
}



//----------------------------------------------------------------------------
// vtkTkImageWindowWidget_Init
// Called upon system startup to create vtkTkImageWindowWidget command.
extern "C" {VTK_TK_EXPORT int Vtktkimagewindowwidget_Init(Tcl_Interp *interp);}
int Vtktkimagewindowwidget_Init(Tcl_Interp *interp)
{
  if (Tcl_PkgProvide(interp, (char *) "Vtktkimagewindowwidget", (char *) "1.2") != TCL_OK) 
    {
    return TCL_ERROR;
    }
  
  Tcl_CreateCommand(interp, (char *) "vtkTkImageWindowWidget", 
		    vtkTkImageWindowWidget_Cmd, 
		    Tk_MainWindow(interp), NULL);
  
  return TCL_OK;
}


// Here is the windows specific code for creating the window
// The Xwindows version follows after this
#ifdef _WIN32

LRESULT APIENTRY vtkTkImageWindowWidgetProc(HWND hWnd, UINT message, 
					    WPARAM wParam, LPARAM lParam)
{
  LRESULT rval;
  struct vtkTkImageWindowWidget *self = 
    (struct vtkTkImageWindowWidget *)GetWindowLong(hWnd,GWL_USERDATA);
  
  // forward message to Tk handler
  SetWindowLong(hWnd,GWL_USERDATA,(LONG)((TkWindow *)self->TkWin)->window);
  if (((TkWindow *)self->TkWin)->parentPtr)
    {
    SetWindowLong(hWnd,GWL_WNDPROC,(LONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd,message,wParam,lParam);
    }
  else
    {
//
// TkWinTopLevelProc has been deprecated in Tcl/Tk8.0.  Not sure how 
// well this will actually work in 8.0.
//
#if (TK_MAJOR_VERSION < 8)
    SetWindowLong(hWnd,GWL_WNDPROC,(LONG)TkWinTopLevelProc);
    rval = TkWinTopLevelProc(hWnd,message,wParam,lParam);
#else
    if (message == WM_WINDOWPOSCHANGED) 
      {
      XEvent event;
	    WINDOWPOS *pos = (WINDOWPOS *) lParam;
	    TkWindow *winPtr = (TkWindow *) Tk_HWNDToWindow(pos->hwnd);
    
	    if (winPtr == NULL) {
	      return 0;
	      }

	    /*
	     * Update the shape of the contained window.
	     */
	    if (!(pos->flags & SWP_NOSIZE)) {
	      winPtr->changes.width = pos->cx;
	      winPtr->changes.height = pos->cy;
	      }
	    if (!(pos->flags & SWP_NOMOVE)) {
	      winPtr->changes.x = pos->x;
	      winPtr->changes.y = pos->y;
	      }


      /*
       *  Generate a ConfigureNotify event.
       */
      event.type = ConfigureNotify;
      event.xconfigure.serial = winPtr->display->request;
      event.xconfigure.send_event = False;
      event.xconfigure.display = winPtr->display;
      event.xconfigure.event = winPtr->window;
      event.xconfigure.window = winPtr->window;
      event.xconfigure.border_width = winPtr->changes.border_width;
      event.xconfigure.override_redirect = winPtr->atts.override_redirect;
      event.xconfigure.x = winPtr->changes.x;
      event.xconfigure.y = winPtr->changes.y;
      event.xconfigure.width = winPtr->changes.width;
      event.xconfigure.height = winPtr->changes.height;
      event.xconfigure.above = None;
      Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

	    Tcl_ServiceAll();
	    return 0;
      }
    SetWindowLong(hWnd,GWL_WNDPROC,(LONG)TkWinChildProc);
    rval = TkWinChildProc(hWnd,message,wParam,lParam);
#endif
    }

    if (message != WM_PAINT)
      {
      SetWindowLong(hWnd,GWL_USERDATA,(LONG)self->ImageWindow);
      SetWindowLong(hWnd,GWL_WNDPROC,(LONG)self->OldProc);
      CallWindowProc(self->OldProc,hWnd,message,wParam,lParam);
      }

    // now reset to the original config
    SetWindowLong(hWnd,GWL_USERDATA,(LONG)self);
    SetWindowLong(hWnd,GWL_WNDPROC,(LONG)vtkTkImageWindowWidgetProc);
    return rval;
}

//-----------------------------------------------------------------------------
// Creates a ImageWindow window and forces Tk to use the window.
static int vtkTkImageWindowWidget_MakeImageWindow(struct vtkTkImageWindowWidget *self) 
{
  Display *dpy;
  TkWindow *winPtr = (TkWindow *) self->TkWin;
  Tcl_HashEntry *hPtr;
  int new_flag;
  vtkImageWindow *ImageWindow;
  TkWinDrawable *twdPtr;
  HWND parentWin;

  if (self->ImageWindow)
    {
    return TCL_OK;
    }
  
  dpy = Tk_Display(self->TkWin);

  if (winPtr->window != None) 
    {
    // XDestroyWindow(dpy, winPtr->window);
    }

  if (self->IW[0] == '\0')
    {
    // Make the ImageWindow window.
    self->ImageWindow = vtkImageWindow::New();
    ImageWindow = self->ImageWindow;
#ifndef VTK_PYTHON_BUILD
    vtkTclGetObjectFromPointer(self->Interp, self->ImageWindow,
			       vtkImageWindowCommand);
#endif
    self->IW = strdup(self->Interp->result);
    self->Interp->result[0] = '\0';
    }
  else
    {
    // is IW an address ? big ole python hack here
    if (self->IW[0] == 'A' && self->IW[1] == 'd' && 
	self->IW[2] == 'd' && self->IW[3] == 'r')
      {
      void *tmp;
      sscanf(self->IW+5,"%p",&tmp);
      ImageWindow = (vtkImageWindow *)tmp;
      }
    else
      {
#ifndef VTK_PYTHON_BUILD
      ImageWindow = (vtkImageWindow *)
	vtkTclGetPointerFromObject(self->IW, "vtkImageWindow", self->Interp,
				   new_flag);
#endif
      }
    if (ImageWindow != self->ImageWindow)
      {
      if (self->ImageWindow != NULL)
	{
	self->ImageWindow->UnRegister(NULL);
	}
      self->ImageWindow = (vtkImageWindow *)(ImageWindow);
      if (self->ImageWindow != NULL)
	{
	self->ImageWindow->Register(NULL);
	}
      }
    }
  
  // Set the size
  self->ImageWindow->SetSize(self->Width, self->Height);
  
  // Set the parent correctly
  // Possibly X dependent
  if ((winPtr->parentPtr != NULL) && !(winPtr->flags & TK_TOP_LEVEL)) 
    {
    if (winPtr->parentPtr->window == None) 
      {
      Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
      }

    parentWin = ((TkWinDrawable *)winPtr->parentPtr->window)->window.handle;
    ImageWindow->SetParentId(parentWin);
    }
  
  // Use the same display
  self->ImageWindow->SetDisplayId(dpy);
  
  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  //Tk_SetWindowVisual(self->TkWin, ImageWindow->GetDesiredVisual(), 
  //ImageWindow->GetDesiredDepth(), 
  //ImageWindow->GetDesiredColormap());
  
  self->ImageWindow->Render();  

#if(TK_MAJOR_VERSION >=  8)
  twdPtr = (TkWinDrawable*)Tk_AttachHWND(self->TkWin, (HWND)ImageWindow->GetGenericWindowId());
#else
  twdPtr = (TkWinDrawable*) ckalloc(sizeof(TkWinDrawable));
  twdPtr->type = TWD_WINDOW;
  twdPtr->window.winPtr = winPtr;
  twdPtr->window.handle = (HWND)ImageWindow->GetGenericWindowId();
#endif
  
  self->OldProc = (WNDPROC)GetWindowLong(twdPtr->window.handle,GWL_WNDPROC);
  SetWindowLong(twdPtr->window.handle,GWL_USERDATA,(LONG)self);
  SetWindowLong(twdPtr->window.handle,GWL_WNDPROC,(LONG)vtkTkImageWindowWidgetProc);

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
// Creates a ImageWindow window and forces Tk to use the window.
static int
vtkTkImageWindowWidget_MakeImageWindow(struct vtkTkImageWindowWidget *self) 
{
  Display *dpy;
  vtkXImageWindow *ImageWindow;
  
  if (self->ImageWindow)
    {
    return TCL_OK;
    }
  
  dpy = Tk_Display(self->TkWin);
  
  if (Tk_WindowId(self->TkWin) != None) 
    {
    XDestroyWindow(dpy, Tk_WindowId(self->TkWin));
    }

  if (self->IW[0] == '\0')
    {
    // Make the ImageWindow window.
    self->ImageWindow = vtkImageWindow::New();
    ImageWindow = (vtkXImageWindow *)(self->ImageWindow);
#ifndef VTK_PYTHON_BUILD
    vtkTclGetObjectFromPointer(self->Interp, self->ImageWindow,
			       vtkImageWindowCommand);
#endif
    self->IW = strdup(self->Interp->result);
    self->Interp->result[0] = '\0';
    }
  else
    {
    // is IW an address ? big ole python hack here
    if (self->IW[0] == 'A' && self->IW[1] == 'd' && 
	self->IW[2] == 'd' && self->IW[3] == 'r')
      {
      void *tmp;
      sscanf(self->IW+5,"%p",&tmp);
      ImageWindow = (vtkXImageWindow *)tmp;
      }
    else
      {
#ifndef VTK_PYTHON_BUILD
			int new_flag;
      ImageWindow = (vtkXImageWindow *)
	vtkTclGetPointerFromObject(self->IW, "vtkImageWindow", self->Interp,
				   new_flag);
#endif
      }
    if (ImageWindow != self->ImageWindow)
      {
      if (self->ImageWindow != NULL)
	{
	self->ImageWindow->UnRegister(NULL);
	}
      self->ImageWindow = (vtkImageWindow *)(ImageWindow);
      if (self->ImageWindow != NULL)
	{
	self->ImageWindow->Register(NULL);
	}
      }
    }
  
  // If the imageviewer has already created it's window, throw up our hands and quit...
  if ( ImageWindow->GetWindowId() != (Window)NULL )
    {
    return TCL_ERROR;
    }
	
  // Use the same display
  ImageWindow->SetDisplayId(dpy);
  // The visual MUST BE SET BEFORE the window is created.
  Tk_SetWindowVisual(self->TkWin, ImageWindow->GetDesiredVisual(), 
		     ImageWindow->GetDesiredDepth(), 
		     ImageWindow->GetDesiredColormap());

  // Make this window exist, then use that information to make the vtkImageViewer in sync
  Tk_MakeWindowExist ( self->TkWin );
  ImageWindow->SetWindowId ( (void*)Tk_WindowId ( self->TkWin ) );

  // Set the size
  self->ImageWindow->SetSize(self->Width, self->Height);
  
  // Set the parent correctly
  // Possibly X dependent
  if ((Tk_Parent(self->TkWin) == NULL) || (Tk_IsTopLevel(self->TkWin))) 
    {
    ImageWindow->SetParentId(XRootWindow(Tk_Display(self->TkWin), Tk_ScreenNumber(self->TkWin)));
    }
  else 
    {
    ImageWindow->SetParentId(Tk_WindowId(Tk_Parent(self->TkWin) ));
    }

  self->ImageWindow->Render();  
  return TCL_OK;
}
#endif
