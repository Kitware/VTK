/*=========================================================================

  Program:   Visualization Library
  Module:    SbrRenW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <iostream.h>
#include "SbrRenW.hh"
#include "SbrRen.hh"
#include "SbrProp.hh"
#include "SbrCam.hh"
#include "SbrLgt.hh"

#define MAX_LIGHTS 16

static char *lights[MAX_LIGHTS] =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

vlActor *vlSbrRenderWindow::MakeActor()
{
  vlActor *actor;
  vlSbrProperty *prop;

  actor = new vlActor;
  prop  = new vlSbrProperty;

  actor->Property = (vlProperty *)prop;
  return (vlActor *)actor;
}

vlLight *vlSbrRenderWindow::MakeLight()
{
  vlSbrLight *light;

  light = new vlSbrLight;
  return (vlLight *)light;
}

vlRenderer *vlSbrRenderWindow::MakeRenderer()
{
  vlSbrRenderer *ren;

  ren = new vlSbrRenderer;

  // by default we are its parent
  ren->SetRenderWindow((vlRenderWindow*)this);
  
  return (vlRenderer *)ren;
}

vlCamera *vlSbrRenderWindow::MakeCamera()
{
  vlSbrCamera *camera;

  camera = new vlSbrCamera;
  return (vlCamera *)camera;
}

vlSbrRenderWindow::vlSbrRenderWindow()
{
  this->Fd = -1;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;

  strcpy(this->Name,"Visualization Library - Starbase");
}

void vlSbrRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (this->Fd == -1)
    this->Initialize();

  flush_matrices(this->Fd);
}

void vlSbrRenderWindow::Frame(void)
{
  // flush and display the buffer
  if (this->DoubleBuffer) 
    {
    dbuffer_switch(this->Fd, this->Buffer = !(this->Buffer));
    }
}

/*
 * get the visual type which matches the depth argument
 */
static Visual * xlib_getvisual(Display *display,int screen,int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	templ.screen = screen;
	templ.depth = depth;

	vis = DefaultVisual(display, screen);

	visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == vis->c_class) {
			vis = v->visual;
			break;						
		}

	return(vis);
}

/*
 * get a PseudoColor visual
 */
static Visual * xlib_getpseudocolorvisual(Display *display,int screen,
					  int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	  templ.screen = screen;
	  templ.depth = depth;

	  visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	  for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == PseudoColor) {
			vis = v->visual;
			break;						
		}

	return(vis);
}

/*
 * get a TrueColor visual
 */
static Visual * xlib_gettruecolorvisual(Display *display,int screen,int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	  templ.screen = screen;
	  templ.depth = depth;

	  visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	  for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == TrueColor) {
			vis = v->visual;
			break;						
		}

	return(vis);
}

/*
 * get a DirectColor visual
 */
static Visual * xlib_getdirectcolorvisual(Display *display,
					  int screen,int depth)
{
    XVisualInfo templ;
    XVisualInfo *visuals, *v;
    Visual	*vis = NULL;
    int		nvisuals;
    int		i;

	  templ.screen = screen;
	  templ.depth = depth;

	  visuals = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
			&templ, &nvisuals);

	  for (v = visuals, i = 0; i < nvisuals; v++, i++)
		if (v->c_class == DirectColor) {
			vis = v->visual;
			break;						
		}

	return(vis);
}



/*
 * get the best visual for XGL accelerated colors
 * This should be called instead of xglut_argprocess if you
 * don't want your user to be able to select the color type
 * and visual of xglut.
 */
static int xlib_get_best_depth(Display *display)
{
  int depth;
  Visual *vis;

  vis = xlib_gettruecolorvisual(display, DefaultScreen(display), 24);
  if (vis == NULL) 
    {
    vis = xlib_getdirectcolorvisual(display, DefaultScreen(display), 24);
    if (vis == NULL) 
      {
      vis = xlib_getpseudocolorvisual(display, DefaultScreen(display), 8); 
      if (vis == NULL) 
	{
	fprintf(stderr,"can't get visual info\n");
	exit(1);
	}
      else 
	{
	depth = 8;
	}
      }
    else 
      {
      depth = 24;
      }
    }
  else 
    {
    depth = 24;
    }

  return(depth);
}

/*
 * get the best visual for XGL accelerated colors
 * This should be called instead of xglut_argprocess if you
 * don't want your user to be able to select the color type
 * and visual of xglut.
 */
static Visual *xlib_get_best_visual(Display *display)
{
  int depth;
  Visual *vis;

    vis = xlib_getdirectcolorvisual(display, DefaultScreen(display), 24);
    if (vis == NULL) 
      {
      vis = xlib_getpseudocolorvisual(display, DefaultScreen(display), 8); 
      if (vis == NULL) 
	{
	fprintf(stderr,"can't get visual info\n");
	exit(1);
	}
      else 
	{
	depth = 8;
	}
      }
    else 
      {
      depth = 24;
      }

  return (vis);
}


int vlSbrRenderWindow::GetDesiredDepth ()
{
  int depth;

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vlErrorMacro(<< "bad X server connection.\n");
      }
    }

  /* get the default depth to use */
  depth = xlib_get_best_depth(this->DisplayId);

  return depth;  
}

Colormap vlSbrRenderWindow::GetDesiredColormap ()
{
  XVisualInfo *pVisInfo, visInfo;
  Colormap cmapID;
  int depth;
  unsigned int mask;
  int retVal;
  Display *dpy;

  /* get the default depth to use */
  depth = xlib_get_best_depth(this->DisplayId);
  dpy = this->DisplayId;

  visInfo.screen = 0;
  visInfo.depth = depth;

  if(depth == 4)
    visInfo.c_class = PseudoColor;
  if(depth == 8)
    visInfo.c_class = PseudoColor;
  if(depth == 12)
    visInfo.c_class = PseudoColor;
  if(depth == 16)
    visInfo.c_class = PseudoColor;
  
  /* DirectColor visual is used needed for CMAP_FULL */
  if(depth == 24)
    visInfo.c_class = DirectColor;
  
  vlDebugMacro(<< "Starbase: The depth is " << depth << "\n");
  /*
   * First, ask for the desired visual
   */
  mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
  
  pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
  
  if (!retVal) 
    {
    if (depth == 24) 
      {
      /*
       * try again with 16 bits
       */
      visInfo.depth = 16;
      visInfo.c_class = PseudoColor;
      pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
      if (!retVal) 
	{
	fprintf(stderr, "Could not get visual info\n");
	return 0;
	}
      } 
    else 
      {
      fprintf(stderr,"Could not get visual info\n");
      return 0;
      }
    }
  
  if (retVal != 1) 
    {
    fprintf(stderr,"Too many visuals match display+depth+class\n");
    return 0;
    }
  

  /*
   * a ColorMap MUST be created
   */
  if (!this->ColorMap)
    {
    cmapID = 
      XCreateColormap(this->DisplayId,
		      RootWindowOfScreen(ScreenOfDisplay(dpy,0)),
		      pVisInfo->visual, AllocNone);
    if (!cmapID) 
      {
      fprintf(stderr,"Could not create color map\n");
      return 0;
      }
    this->ColorMap = cmapID;
    }

  return this->ColorMap;  
}


Visual *vlSbrRenderWindow::GetDesiredVisual ()
{
  Visual *vis;

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vlErrorMacro(<< "bad X server connection.\n");
      }
    }

  /* get the default visual to use */
  vis = xlib_get_best_visual(this->DisplayId);

  return vis;  
}


// create a window for starbase output
int vlSbrRenderWindow::CreateXWindow(Display *dpy,int xpos,int ypos, 
				     int width,int height,int depth, 
				     char name[80])
{
  Window win;
  XEvent event;
  XVisualInfo *pVisInfo,visInfo;
  Colormap cmapID;
  XColor c0, c1; 
  XSetWindowAttributes winattr;
  char *window_name;
  Pixmap icon_pixmap;
  int screen;
  unsigned int mask;
  int retVal;
  XSizeHints xsh;
  
  visInfo.screen = 0;
  visInfo.depth = depth;
  icon_pixmap = 0;
  
  /*
   * PseudoColor visual is used needed for CMAP_NORMAL
   */
  if(depth == 4)
    visInfo.c_class = PseudoColor;
  
  if(depth == 8)
    visInfo.c_class = PseudoColor;
  
  if(depth == 12)
    visInfo.c_class = PseudoColor;
  
  if(depth == 16)
    visInfo.c_class = PseudoColor;
  
  /*
   * DirectColor visual is used needed for CMAP_FULL
   */
  if(depth == 24)
    visInfo.c_class = DirectColor;
  
  vlDebugMacro(<< "Starbase: The depth is " << depth << "\n");
  /*
   * First, ask for the desired visual
   */
  mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
  
  pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
  
  if (!retVal) {
    if (depth == 24) {
      /*
       * try again with 16 bits
       */
      visInfo.depth = 16;
      visInfo.c_class = PseudoColor;
      pVisInfo = XGetVisualInfo(dpy, mask, &visInfo, &retVal);
      if (!retVal) {
	fprintf(stderr, "Could not get visual info\n");
	return 0;
      }
    } else {
      fprintf(stderr,"Could not get visual info\n");
        return 0;
    }
  }
  
  if (retVal != 1) {
    fprintf(stderr,"Too many visuals match display+depth+class\n");
    return 0;
  }
  
  // a ColorMap MUST be created
  if (!this->ColorMap)
    {
    cmapID = XCreateColormap(dpy,
			     RootWindowOfScreen(ScreenOfDisplay(dpy,0)),
			     pVisInfo->visual, AllocNone);
    if (!cmapID) 
      {
      fprintf(stderr,"Could not create color map\n");
      return 0;
      }
    this->ColorMap = cmapID;
    }

  
  // Border and background info MUST be passed in also
  winattr.event_mask = 0;
  winattr.border_pixel = 1;
  winattr.background_pixel = 0;
  winattr.colormap = this->ColorMap;
  if ((xpos >= 0)&&(ypos >= 0))
    {
    xsh.flags = USPosition | USSize;
    }
  else
    {
    xsh.flags = PPosition | PSize;
    }
  xsh.x = ((xpos >= 0) ? xpos : 5);
  xsh.y = ((ypos >= 0) ? ypos : 5);
  xsh.width = WidthOfScreen(ScreenOfDisplay(dpy,0));
  xsh.height = HeightOfScreen(ScreenOfDisplay(dpy,0));
  
  // if both the position and size have been set, override the window
  // manager
  winattr.override_redirect = False;
  if ((width > 0) && (xpos >= 0) && (!this->Borders))
    winattr.override_redirect = True;

  XFlush(dpy);
  
  /*
   * create the parent X11 Window
   */
  
  win = XCreateWindow(dpy, RootWindowOfScreen(ScreenOfDisplay(dpy,0)),
                      xsh.x, xsh.y, xsh.width, xsh.height, 0, depth,
                      InputOutput, pVisInfo->visual,
                      CWColormap | CWBorderPixel | CWBackPixel |
			CWEventMask | CWOverrideRedirect , &winattr);
  if(! win) 
    {
    fprintf(stderr,"Could not create window\n");
    return 0;
    }

  /*
   * Give the window a name
   */
  XSetStandardProperties(dpy, win, name, name, icon_pixmap, NULL, 0, &xsh);
  XSelectInput(dpy, win, KeyPressMask|ExposureMask|StructureNotifyMask);

  /*
   * set the default window
   */
  this->WindowId = win;  
  this->DisplayId = dpy;
  XSync(dpy, False );

  return 1;
}
 
void vlSbrRenderWindow::WindowInitialize (void)
{
  char *device, *driver, *getenv(), *str;
  int planes, depth, mode;
  int create_xwindow();
  int cmap_size;
  XSizeHints *size_hints;
  XClassHint *class_hint;
  XWMHints *wm_hints;
  XTextProperty window_name, icon_name;
  char *list[1];
  XEvent event;
  XWindowAttributes winattr;

  // get the default depth to use
  depth = this->GetDesiredDepth();

  mode = OUTDEV;
  if (this->WindowId == 0) 
    {
    if (! 
	this->CreateXWindow(this->DisplayId, this->Position[0],
			    this->Position[1],
			    this->Size[0],
			    this->Size[1],
			    depth, this->Name)) 
      {
      vlErrorMacro(<< "Couldn't create window\n");
      return;
      }
    this->OwnWindow = 1;
    }
  else
    {
    this->OwnWindow = 0;
    /* make sure the window is unmapped */
    XUnmapWindow(this->DisplayId, this->WindowId);
    XSync(this->DisplayId,False);
    vlDebugMacro(<< "Unmapping the xwindow\n");
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    while (winattr.map_state != IsUnmapped)
      {
      XNextEvent(this->DisplayId, &event);
      XGetWindowAttributes(this->DisplayId,
			   this->WindowId,&winattr);
      }; 
    
    /* make sure the window is full screen */
    vlDebugMacro( << "Resizing the xwindow\n");
    XResizeWindow(this->DisplayId,this->WindowId,
		  WidthOfScreen(ScreenOfDisplay(this->DisplayId,0)),
		  HeightOfScreen(ScreenOfDisplay(this->DisplayId,0)));
    XSync(this->DisplayId,False);
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    while (winattr.width != 
	   WidthOfScreen(ScreenOfDisplay(this->DisplayId,0)))
      {
      XNextEvent(this->DisplayId, &event);
      XGetWindowAttributes(this->DisplayId,
			   this->WindowId,&winattr);
      } 
    }

  // convert window id to something Starbase can open
  device = make_X11_gopen_string(this->DisplayId, (Window)this->WindowId);
  if (!device) 
    {
    vlErrorMacro(<< "Could not create device file for window.\n");
    device = "/dev/crt";
    }
  
  driver = getenv("SB_OUTDRIVER");
  if ((this->Fd = 
       gopen(device, mode, driver, 
	     RESET_DEVICE | INIT | THREE_D | MODEL_XFORM)) == -1) 
    {
    vlErrorMacro(<< "cannot open starbase driver error number= " 
    << errno << "\n");
    return;
    }

  // RESIZE THE WINDOW TO THE DESIRED SIZE
  vlDebugMacro(<< "Resizing the xwindow\n");
  XResizeWindow(this->DisplayId,this->WindowId,
		((this->Size[0] > 0) ? 
		 (int)(this->Size[0]) : 256),
		((this->Size[1] > 0) ? 
		 (int)(this->Size[1]) : 256));
  XSync(this->DisplayId,False);

  list[0] = this->Name;
  XStringListToTextProperty( list, 1, &window_name );
  list[0] = this->Name;
  XStringListToTextProperty( list, 1, &icon_name );
    
  size_hints = XAllocSizeHints();
  size_hints->flags = USSize;
  if ((this->Position[0] >= 0)&&(this->Position[1] >= 0))
    {
    size_hints->flags |= USPosition;
    size_hints->x =  (int)(this->Position[0]);
    size_hints->y =  (int)(this->Position[1]);
    }
  
  size_hints->width  = 
    ((this->Size[0] > 0) ? (int)(this->Size[0]) : 256);
  size_hints->height = 
    ((this->Size[1] > 0) ?  (int)(this->Size[1]) : 256);
  
  wm_hints = XAllocWMHints();

  class_hint = XAllocClassHint();
  class_hint->res_name = this->Name;
  class_hint->res_class = this->Name;
  
  XSetWMProperties(this->DisplayId, 
		   this->WindowId, &window_name, &icon_name,
		   NULL, 0, size_hints, wm_hints, class_hint );

  /* Finally -- we can map the window!  We won't actually render anything
     to the window until the expose event happens later. */
  vlDebugMacro(<< "Mapping the xwindow\n");
  XMapWindow(this->DisplayId, this->WindowId);
  XSync(this->DisplayId,False);
  XGetWindowAttributes(this->DisplayId,
		       this->WindowId,&winattr);
  while (winattr.map_state == IsUnmapped)
    {
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    };
  
  // free up the memory allocated above 
  free(device);

  set_p1_p2(this->Fd, FRACTIONAL, 0.0, 
	    0.0,
	    0.0, 1.0, 1.0, 1.0);

  mapping_mode(this->Fd, DISTORT);
  vlDebugMacro(<< "SB_mapping_mode: DISTORT\n");
  
  // set clipping
  clip_rectangle(this->Fd, 0.0, 1.0, 0.0, 1.0);
  clip_depth(this->Fd, 0.0, 1.0);
  clip_indicator(this->Fd, CLIP_TO_VIEWPORT);
  depth_indicator(this->Fd, TRUE, TRUE);
  
  // use the full color map, initialize it and turn shading on 
  shade_mode(this->Fd, CMAP_FULL | INIT, TRUE); 
  
  // set Fd update state - reset viewport and buffer commands
  if (this->DoubleBuffer > 0.0) 
    {
    if ((planes = double_buffer(this->Fd, 
				TRUE | INIT | SUPPRESS_CLEAR,
				depth)) != depth)
      {
      vlDebugMacro(<< "Only " << planes <<
      " planes available for double buffering\n");
      }
    dbuffer_switch(this->Fd,this->Buffer); 
    buffer_mode(this->Fd, TRUE);
    }


  // turn on z buffering and disable backface culling 
  hidden_surface(this->Fd, TRUE, FALSE);
  clear_control(this->Fd, CLEAR_DISPLAY_SURFACE | CLEAR_ZBUFFER);
  
  // set back faces of polygons to be rendered same as front 
  bf_control(this->Fd, FALSE, FALSE);
  // make default polymarker a dot (pixel) 
  marker_type(this->Fd, 0);

  // clear the display 
  clear_view_surface(this->Fd);

  clear_control(this->Fd, CLEAR_VIEWPORT | CLEAR_ZBUFFER);

  // ignore errors 
  gerr_print_control(NO_ERROR_PRINTING);
  this->Mapped = 1;
}

void vlSbrRenderWindow::Initialize (void)
{
  // make sure we haven't already been initialiozed 
  if (this->Fd != -1) return;

  // now initialize the window 
  this->WindowInitialize();
}


void vlSbrRenderWindow::SetFullScreen(int arg)
{
  int *temp;

  if (this->FullScreen == arg) return;
  
  if (!this->Mapped)
    {
    this->PrefFullScreen();
    return;
    }

  // set the mode 
  this->FullScreen = arg;
  if (this->FullScreen <= 0)
    {
    this->Position[0] = this->OldScreen[0];
    this->Position[1] = this->OldScreen[1];
    this->Size[0] = this->OldScreen[2]; 
    this->Size[1] = this->OldScreen[3];
    this->Borders = this->OldScreen[4];
    }
  else
    {
    // if window already up get its values 
    if (this->WindowId)
      {
      XWindowAttributes attribs;
      
      //  Find the current window size 
      XGetWindowAttributes(this->DisplayId, 
			   this->WindowId, &attribs);
      
      this->OldScreen[2] = attribs.width;
      this->OldScreen[3] = attribs.height;;

      temp = this->GetPosition();      
      this->OldScreen[0] = temp[0];
      this->OldScreen[1] = temp[1];

      this->OldScreen[4] = this->Borders;
      this->PrefFullScreen();
      }
    }
  
  // remap the window 
  this->WindowRemap();

  // if full screen then grab the keyboard 
  if (this->FullScreen)
    {
    XGrabKeyboard(this->DisplayId,this->WindowId,
		  False,GrabModeAsync,GrabModeAsync,CurrentTime);
    }
  this->Modified();
}

int *vlSbrRenderWindow::GetScreenSize()
{
  this->ScreenSize[0] = 
    DisplayWidth(this->DisplayId, DefaultScreen(this->DisplayId));
  this->ScreenSize[1] = 
    DisplayHeight(this->DisplayId, DefaultScreen(this->DisplayId));

  return this->ScreenSize;
}

void vlSbrRenderWindow::PrefFullScreen()
{
  int *size;

  size = this->GetScreenSize();

  // use full screen 
  this->Position[0] = 0;
  this->Position[1] = 0;
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  // don't show borders 
  this->Borders = 0;
}


void vlSbrRenderWindow::WindowRemap()
{
  // close the starbase window 
  if (this->Fd)
    {
    gclose(this->Fd);
    }
  this->Fd = -1;
  
  /* free the Xwindow we created no need to free the colormap */
  if (this->OwnWindow)
    {
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  XSync(this->DisplayId,0);
  this->WindowId = this->NextWindowId;
  this->NextWindowId = 0;

  // configure the window 
  this->WindowInitialize();
}


int *vlSbrRenderWindow::GetSize(void)
{
  XWindowAttributes attribs;

  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Size);
    }

  //  Find the current window size 
  XGetWindowAttributes(this->DisplayId, 
		       this->WindowId, &attribs);

  this->Size[0] = attribs.width;
  this->Size[1] = attribs.height;
  
  return this->Size;
}

int *vlSbrRenderWindow::GetPosition(void)
{
  XWindowAttributes attribs;
  int x,y;
  Window child;
  
  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window size 
  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);
  x = attribs.x;
  y = attribs.y;

  XTranslateCoordinates(this->DisplayId,this->WindowId,
		 RootWindowOfScreen(ScreenOfDisplay(this->DisplayId,0)),
			x,y,&this->Position[0],&this->Position[1],&child);

  return this->Position;
}

void vlSbrRenderWindow::SetSize(int x,int y)
{
  // if we arent mappen then just set the ivars 
  if (!this->Mapped)
    {
    if ((this->Size[0] != x)||(this->Size[1] != y))
      {
      this->Modified();
      }
    this->Size[0] = x;
    this->Size[1] = y;
    return;
    }
  
  if ((this->Size[0] != x)||(this->Size[1] != y))
    {
    this->Modified();
    }
  this->Size[0] = x;
  this->Size[1] = y;
  
  XResizeWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}


Display *vlSbrRenderWindow::GetDisplayId()
{
  vlDebugMacro(<< "Returning DisplayId of " << (void *)this->DisplayId << "\n"); 

  return this->DisplayId;
}

Window vlSbrRenderWindow::GetWindowId()
{
  vlDebugMacro(<< "Returning WindowId of " << (void *)this->WindowId << "\n"); 

  return this->WindowId;
}

void vlSbrRenderWindow::SetWindowId(Window arg)
{
  vlDebugMacro(<< "Setting WindowId to " << (void *)arg << "\n"); 

  this->WindowId = arg;
}

void vlSbrRenderWindow::SetDisplayId(Display  *arg)
{
  vlDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
}


void vlSbrRenderWindow::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlSbrRenderWindow::GetClassName()))
    {
    this->vlRenderWindow::PrintSelf(os,indent);

    os << indent << "Color Map: " << this->ColorMap << "\n";
    os << indent << "Display Id: " << this->GetDisplayId() << "\n";
    os << indent << "Fd: " << this->Fd << "\n";
    os << indent << "Next Window Id: " << this->NextWindowId << "\n";
    os << indent << "Window Id: " << this->GetWindowId() << "\n";
    }
}
