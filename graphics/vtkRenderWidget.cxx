#include <stdlib.h>
#include <tcl.h>
#include <tk.h>

#include "vtkRenderWidget.h"
#include "vtkRenderMaster.h"
   
   static vtkRenderMaster vtkRenderWidgetMaster;

static Tk_ConfigSpec vtkRenderWidgetConfigSpecs[] = {
    {TK_CONFIG_PIXELS, "-height", "height", "Height",
     "400", Tk_Offset(struct Vtkrenderwidget, Height), 0, NULL},
  
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
     "400", Tk_Offset(struct Vtkrenderwidget, Width), 0, NULL},
  
    {TK_CONFIG_STRING, "-rw", "rw", "RW",
     "", Tk_Offset(struct Vtkrenderwidget, RW), 0, NULL},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
     (char *) NULL, 0, 0, NULL}
};



static void Vtkrenderwidget_EventProc(ClientData clientData, XEvent *eventPtr);
static int Vtkrenderwidget_MakeRenderWindow(struct Vtkrenderwidget *vtkrenderwidget);
extern int vtkXRenderWindowCommand(ClientData cd, Tcl_Interp *interp,
                                   int argc, char *argv[]);



    
//----------------------------------------------------------------------------
// It's possible to change with this function or in a script some
// options like width and hieght
int Vtkrenderwidget_Configure(Tcl_Interp *interp, 
			      struct Vtkrenderwidget *vtkrenderwidget, 
			      int argc, char *argv[], int flags) 
{
  if (Tk_ConfigureWidget(interp, vtkrenderwidget->TkWin, 
			 vtkRenderWidgetConfigSpecs,
			 argc, argv, (char *)vtkrenderwidget, flags) 
      == TCL_ERROR) 
    {
    return(TCL_ERROR);
    }

  Tk_GeometryRequest(vtkrenderwidget->TkWin, 
		     vtkrenderwidget->Width, vtkrenderwidget->Height);
		     
		     
  if (Vtkrenderwidget_MakeRenderWindow(vtkrenderwidget)==TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  
  return(TCL_OK);
}

//----------------------------------------------------------------------------
// Instance function
int Vtkrenderwidget_Widget(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[]) 
{
   struct Vtkrenderwidget *vtkrenderwidget = 
     (struct Vtkrenderwidget *)clientData;
   int result = TCL_OK;
   //Tcl_HashEntry *entry;
   // Tcl_HashSearch search;
   
   if (argc < 2) 
     {
     Tcl_AppendResult(interp, "wrong # args: should be \"",
		      argv[0], " ?options?\"", NULL);
     return TCL_ERROR;
     }

   Tk_Preserve((ClientData)vtkrenderwidget);
   
   if (!strncmp(argv[1], "configure", MAX(1, strlen(argv[1])))) 
     {
     if (argc == 2) 
       {
       /* Return list of all configuration parameters */
       result = Tk_ConfigureInfo(interp, vtkrenderwidget->TkWin, 
				 vtkRenderWidgetConfigSpecs,
				 (char *)vtkrenderwidget, (char *)NULL, 0);
       }
     else if (argc == 3) 
       {
       /* Return a specific configuration parameter */
       result = Tk_ConfigureInfo(interp, vtkrenderwidget->TkWin, 
				 vtkRenderWidgetConfigSpecs,
				 (char *)vtkrenderwidget, argv[2], 0);
       }
     else 
       {
       /* Execute a configuration change */
       result = Vtkrenderwidget_Configure(interp, vtkrenderwidget, argc-2, argv+2, 
					  TK_CONFIG_ARGV_ONLY);
       }
     }
   else if (!strcmp(argv[1], "GetRenderWindow"))
     {
     // Just incase this can be called before configure
     result = Vtkrenderwidget_MakeRenderWindow(vtkrenderwidget);
     if (result != TCL_ERROR)
       {
       Tcl_SetResult(interp, vtkrenderwidget->RW, TCL_VOLATILE);
       }
     }
   else 
     {
     Tcl_AppendResult(interp, "vtkRenderWidget: Unknown option: ", argv[1], "\n", 
		      "Try: configure or GetRenderWindow\n",
		      NULL);
     /*
     entry = Tcl_FirstHashEntry(&CommandTable, &search);
     while (entry) 
       {
       Tcl_AppendResult(interp, "  ",
			Tcl_GetHashKey(&CommandTable, entry),
			"\n", NULL);
       entry = Tcl_NextHashEntry(&search);
       }
       */
     result = TCL_ERROR;
     }
   
   Tk_Release((ClientData)vtkrenderwidget);
   return result;
}

//----------------------------------------------------------------------------
// Vtkrenderwidget_Cmd
// Called when vtkRenderWidget is executed 
// - creation of a Vtkrenderwidget widget.
//     * Creates a new window
//     * Creates an 'Vtkrenderwidget' data structure
//     * Creates an event handler for this window
//     * Creates a command that handles this object
//     * Configures this Vtkrenderwidget for the given arguments
static int Vtkrenderwidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
                               int argc, char **argv)
{
   char *name;
   Tk_Window main = (Tk_Window)clientData;
   Tk_Window tkwin;
   struct Vtkrenderwidget *vtkrenderwidget;
    
   if (argc <= 1) 
     {
     Tcl_ResetResult(interp);
     Tcl_AppendResult(interp, 
	      "wrong # args: should be \"pathName read filename\"", NULL);
     return(TCL_ERROR);
     }

   // Create the window.
   name = argv[1];
   tkwin = Tk_CreateWindowFromPath(interp, main, name, (char *) NULL);
   if (tkwin == NULL) 
     {
     return TCL_ERROR;
     }
    
   Tk_SetClass(tkwin, "Vtkrenderwidget");

   // Create Vtkrenderwidget data structure 
   vtkrenderwidget = (struct Vtkrenderwidget *)malloc(sizeof(struct Vtkrenderwidget));
   vtkrenderwidget->TkWin = tkwin;
   vtkrenderwidget->Interp = interp;
   vtkrenderwidget->Width = 0;
   vtkrenderwidget->Height = 0;
   vtkrenderwidget->RenderWindow = NULL;
   vtkrenderwidget->RW = NULL;

   // Create command event handler
   Tcl_CreateCommand(interp, Tk_PathName(tkwin), Vtkrenderwidget_Widget, 
                     (ClientData)vtkrenderwidget, (void (*)(ClientData)) NULL);
   Tk_CreateEventHandler(tkwin, 
                         ExposureMask | StructureNotifyMask,
                         Vtkrenderwidget_EventProc, 
                         (ClientData)vtkrenderwidget);
   
   // Configure Vtkrenderwidget widget
   if (Vtkrenderwidget_Configure(interp, vtkrenderwidget, argc-2, argv+2, 0) 
       == TCL_ERROR) 
     {
     Tk_DestroyWindow(tkwin);
     Tcl_DeleteCommand(interp, "vtkrenderwidget");
     // Don't free it, if we do a crash occurs later...
     //free(vtkrenderwidget);  
     return TCL_ERROR;
     }
   
   Tcl_AppendResult(interp, Tk_PathName(tkwin), NULL);
   return TCL_OK;
}


//----------------------------------------------------------------------------
char *
Vtkrenderwidget_RW(const struct Vtkrenderwidget *vtkrenderwidget )
{
  return vtkrenderwidget->RW;
}


//----------------------------------------------------------------------------
int Vtkrenderwidget_Width( const struct Vtkrenderwidget *vtkrenderwidget )
{
   return vtkrenderwidget->Width;
}


//----------------------------------------------------------------------------
int Vtkrenderwidget_Height( const struct Vtkrenderwidget *vtkrenderwidget )
{
   return vtkrenderwidget->Height;
}

//----------------------------------------------------------------------------
// This gets called to handle Vtkrenderwidget window configuration events
static void Vtkrenderwidget_EventProc(ClientData clientData, XEvent *eventPtr) 
{
   struct Vtkrenderwidget *vtkrenderwidget = 
     (struct Vtkrenderwidget *)clientData;

   switch (eventPtr->type) 
     {
     case Expose:
       if ((eventPtr->xexpose.count == 0)
	   /* && !vtkrenderwidget->UpdatePending*/) 
	 {
	 vtkrenderwidget->RenderWindow->Render();
         }
       break;
     case ConfigureNotify:
       if ( 1 /*Tk_IsMapped(vtkrenderwidget->TkWin)*/ ) 
	 {
	 vtkrenderwidget->Width = Tk_Width(vtkrenderwidget->TkWin);
	 vtkrenderwidget->Height = Tk_Height(vtkrenderwidget->TkWin);
	 XResizeWindow(Tk_Display(vtkrenderwidget->TkWin), 
		       Tk_WindowId(vtkrenderwidget->TkWin), 
		       vtkrenderwidget->Width, vtkrenderwidget->Height);
	 if (vtkrenderwidget->RenderWindow)
	   {
	   vtkrenderwidget->RenderWindow->SetSize(vtkrenderwidget->Width,
						  vtkrenderwidget->Height);
	   }
	 //Vtkrenderwidget_PostRedisplay(vtkrenderwidget);
         }
       break;
     case MapNotify:
       break;
     case DestroyNotify:
       // Tcl_EventuallyFree( (ClientData) vtkrenderwidget, Vtkrenderwidget_Destroy );
       break;
     default:
       // nothing
       ;
     }
}



//----------------------------------------------------------------------------
// Vtkrenderwidget_Init
// Called upon system startup to create Vtkrenderwidget command.
extern "C" {int Vtkrenderwidget_Init(Tcl_Interp *interp);}
int Vtkrenderwidget_Init(Tcl_Interp *interp)
{
  if (Tcl_PkgProvide(interp, "Vtkrenderwidget", "1.2") != TCL_OK) 
    {
    return TCL_ERROR;
    }
  
  Tcl_CreateCommand(interp, "vtkRenderWidget", Vtkrenderwidget_Cmd, 
		    Tk_MainWindow(interp), NULL);
  
  return TCL_OK;
}



//-----------------------------------------------------------------------------
// Creates a render window and forces Tk to use the window.
static int
Vtkrenderwidget_MakeRenderWindow(struct Vtkrenderwidget *vtkrenderwidget) 
{
  Display *dpy;
  TkWindow *winPtr = (TkWindow *) vtkrenderwidget->TkWin;
  TkWindow *winPtr2;
  Tcl_HashEntry *hPtr;
  int new_flag;
  
  if (vtkrenderwidget->RenderWindow)
    {
    return TCL_OK;
    }
  
  dpy = Tk_Display(vtkrenderwidget->TkWin);
  
  if (winPtr->window != None) 
    {
    XDestroyWindow(dpy, winPtr->window);
    }

  if (vtkrenderwidget->RW[0] == '\0')
    {
    // Make the Render window.
    vtkrenderwidget->RenderWindow = 
      (vtkXRenderWindow *)vtkRenderWidgetMaster.MakeRenderWindow();
    vtkTclGetObjectFromPointer(vtkrenderwidget->Interp,
			       vtkrenderwidget->RenderWindow,
			       vtkXRenderWindowCommand);
    vtkrenderwidget->RW = strdup(vtkrenderwidget->Interp->result);
    vtkrenderwidget->Interp->result[0] = '\0';
    }
  else
    {
    vtkrenderwidget->RenderWindow = 
      (vtkXRenderWindow *)vtkTclGetPointerFromObject(vtkrenderwidget->RW,
						     "vtkRenderWindow");
    }
  
  // Set the size
  vtkrenderwidget->RenderWindow->SetSize(vtkrenderwidget->Width,
					 vtkrenderwidget->Height);
  
  // Set the parent correctly
  if ((winPtr->parentPtr == NULL) || (winPtr->flags & TK_TOP_LEVEL)) 
    {
    vtkrenderwidget->RenderWindow->SetParentId(XRootWindow(winPtr->display, 
							   winPtr->screenNum));
    }
  else 
    {
    if (winPtr->parentPtr->window == None) 
      {
      Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
      }
    vtkrenderwidget->RenderWindow->SetParentId(winPtr->parentPtr->window);
    }

  // Use the same display
  vtkrenderwidget->RenderWindow->SetDisplayId(dpy);
  
  /* Make sure Tk knows to switch to the new colormap when the cursor
   * is over this window when running in color index mode.
   */
  Tk_SetWindowVisual(vtkrenderwidget->TkWin, 
		     vtkrenderwidget->RenderWindow->GetDesiredVisual(), 
		     vtkrenderwidget->RenderWindow->GetDesiredDepth(), 
		     vtkrenderwidget->RenderWindow->GetDesiredColormap());
  
  vtkrenderwidget->RenderWindow->Render();  
  winPtr->window = vtkrenderwidget->RenderWindow->GetWindowId();
  XSelectInput(dpy, winPtr->window, ALL_EVENTS_MASK);
  
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

