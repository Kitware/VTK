/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSbrRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include <iostream.h>
#include "vtkSbrRenderWindow.h"
#include "vtkSbrRenderer.h"
#include "vtkSbrProperty.h"
#include "vtkSbrTexture.h"
#include "vtkSbrCamera.h"
#include "vtkSbrLight.h"
#include "vtkSbrActor.h"
#include "vtkSbrPolyMapper.h"

#define MAX_LIGHTS 16

vtkSbrRenderWindow::vtkSbrRenderWindow()
{
  this->Fd = -1;
  if ( this->WindowName )
    delete [] this->WindowName;
  this->WindowName = strdup("Visualization Toolkit - Starbase");
  this->Buffer = 0;
}

vtkSbrRenderWindow::~vtkSbrRenderWindow()
{
  // close the starbase window 
  if (this->Fd)
    {
    gclose(this->Fd);
    }
  this->Fd = -1;
  
  /* free the Xwindow we created no need to free the colormap */
  if (this->OwnWindow && this->DisplayId && this->WindowId)
    {
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  XSync(this->DisplayId,0);
}


// Description:
// Create a starbase specific light.
vtkLightDevice *vtkSbrRenderWindow::MakeLight()
{
  vtkSbrLight *light;

  light = new vtkSbrLight;
  return (vtkLightDevice *)light;
}

// Description:
// Create a starbase specific renderer.
vtkRenderer *vtkSbrRenderWindow::MakeRenderer()
{
  vtkSbrRenderer *ren;

  ren = new vtkSbrRenderer;
  this->AddRenderers(ren);

  // by default we are its parent
  ren->SetRenderWindow((vtkRenderWindow*)this);
  
  return (vtkRenderer *)ren;
}

// Description:
// Create a starbase specific camera.
vtkCameraDevice *vtkSbrRenderWindow::MakeCamera()
{
  vtkSbrCamera *camera;

  camera = new vtkSbrCamera;
  return (vtkCameraDevice *)camera;
}

// Description:
// Create a starbase specific actor.
vtkActorDevice *vtkSbrRenderWindow::MakeActor()
{
  vtkSbrActor *actor;

  actor = new vtkSbrActor;
  return (vtkActorDevice *)actor;
}

// Description:
// Create a starbase specific property.
vtkPropertyDevice *vtkSbrRenderWindow::MakeProperty()
{
  vtkSbrProperty *property;

  property = new vtkSbrProperty;
  return (vtkPropertyDevice *)property;
}

// Description:
// Create a starbase specific texture.
vtkTextureDevice *vtkSbrRenderWindow::MakeTexture()
{
  vtkSbrTexture *texture;

  texture = new vtkSbrTexture;
  return (vtkTextureDevice *)texture;
}

// Description:
// Create a XGL specific PolyMapper.
vtkPolyMapperDevice *vtkSbrRenderWindow::MakePolyMapper()
{
  vtkSbrPolyMapper *polyMapper;

  polyMapper = new vtkSbrPolyMapper;
  return (vtkPolyMapperDevice *)polyMapper;
}

// Description:
// Begin the rendering process.
void vtkSbrRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (this->Fd == -1)
    this->Initialize();

  flush_matrices(this->Fd);
}

// Description:
// Update system if needed due to stereo rendering.
void vtkSbrRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	gescape_arg arg1,arg2;
	arg1.i[0] = 1;
	gescape(this->Fd,STEREO,&arg1,&arg2);
	// make sure we are in full screen
        this->StereoStatus = 1;
	this->FullScreenOn();
	}
	break;
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 1;
	}
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	gescape_arg arg1,arg2;
	arg1.i[0] = 0;
	gescape(this->Fd,STEREO,&arg1,&arg2);
	// make sure we are in full screen
        this->StereoStatus = 0;
	this->FullScreenOff();
	}
	break;
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 0;
	}
      }
    }
}

// Description:
// End the rendering process and display the image.
void vtkSbrRenderWindow::Frame(void)
{
  // flush and display the buffer
  if (this->DoubleBuffer&&this->SwapBuffers) 
    {
    dbuffer_switch(this->Fd, this->Buffer = !(this->Buffer));
    }
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


int vtkSbrRenderWindow::GetDesiredDepth ()
{
  int depth;

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    }

  /* get the default depth to use */
  depth = xlib_get_best_depth(this->DisplayId);

  return depth;  
}

// Description:
// Obtain a colormap from windowing system.
Colormap vtkSbrRenderWindow::GetDesiredColormap ()
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
  
  vtkDebugMacro(<< "Starbase: The depth is " << depth << "\n");
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


// Description:
// Get a visual from the windowing system.
Visual *vtkSbrRenderWindow::GetDesiredVisual ()
{
  Visual *vis;

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    }

  /* get the default visual to use */
  vis = xlib_get_best_visual(this->DisplayId);

  return vis;  
}


// Description:
// Create a window for starbase output.
int vtkSbrRenderWindow::CreateXWindow(Display *dpy,int xpos,int ypos, 
				      int width,int vtkNotUsed(height),
				      int depth, 
				      char name[80])
{
  Window win;
  XVisualInfo *pVisInfo,visInfo;
  Colormap cmapID;
  XSetWindowAttributes winattr;
  Pixmap icon_pixmap;
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
  
  vtkDebugMacro(<< "Starbase: The depth is " << depth << "\n");
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
  // get a default parent if one has not been set.
  if (! this->ParentId)
    {
    this->ParentId = RootWindowOfScreen(ScreenOfDisplay(dpy,0));
    }
  
  win = XCreateWindow(dpy, this->ParentId,
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
 
// Description:
// Initialize the rendering window.
void vtkSbrRenderWindow::WindowInitialize (void)
{
  char *device, *driver;
  int planes, depth, mode;
  int create_xwindow();
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
			    depth, this->WindowName)) 
      {
      vtkErrorMacro(<< "Couldn't create window\n");
      return;
      }
    this->OwnWindow = 1;
    }
  else
    {
    XSetWindowAttributes xswattr;

    this->OwnWindow = 0;
    /* make sure the window is unmapped */
    XUnmapWindow(this->DisplayId, this->WindowId);
    XSync(this->DisplayId,False);
    vtkDebugMacro(<< "Unmapping the xwindow\n");
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    while (winattr.map_state != IsUnmapped)
      {
      XNextEvent(this->DisplayId, &event);
      XGetWindowAttributes(this->DisplayId,
			   this->WindowId,&winattr);
      }; 
    
    /* make sure the window is full screen */
    vtkDebugMacro( << "Resizing the xwindow\n");
    XSelectInput(this->DisplayId, this->WindowId, 
		 KeyPressMask|ExposureMask);

    xswattr.override_redirect = False;
    if ((!this->Borders))
      xswattr.override_redirect = True;

    XChangeWindowAttributes(this->DisplayId,this->WindowId,
			    CWOverrideRedirect, &xswattr);

    XResizeWindow(this->DisplayId,this->WindowId,
		  WidthOfScreen(ScreenOfDisplay(this->DisplayId,0)),
		  HeightOfScreen(ScreenOfDisplay(this->DisplayId,0)));
    XSync(this->DisplayId,False);
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    while (winattr.width != 
	   WidthOfScreen(ScreenOfDisplay(this->DisplayId,0)))
      {
/*      XNextEvent(this->DisplayId, &event); */
      XGetWindowAttributes(this->DisplayId,
			   this->WindowId,&winattr);
      } 
    }

  // convert window id to something Starbase can open
  device = make_X11_gopen_string(this->DisplayId, (Window)this->WindowId);
  if (!device) 
    {
    vtkErrorMacro(<< "Could not create device file for window.\n");
    device = "/dev/crt";
    }
  
  driver = getenv("SB_OUTDRIVER");
  if ((this->Fd = 
       gopen(device, mode, driver, 
	     RESET_DEVICE | INIT | THREE_D | MODEL_XFORM)) == -1) 
    {
    vtkErrorMacro(<< "cannot open starbase driver error number= " 
    << errno << "\n");
    return;
    }

  // RESIZE THE WINDOW TO THE DESIRED SIZE
  vtkDebugMacro(<< "Resizing the xwindow\n");
  XResizeWindow(this->DisplayId,this->WindowId,
		((this->Size[0] > 0) ? 
		 (int)(this->Size[0]) : 256),
		((this->Size[1] > 0) ? 
		 (int)(this->Size[1]) : 256));
  XSync(this->DisplayId,False);

  list[0] = this->WindowName;
  XStringListToTextProperty( list, 1, &window_name );
  list[0] = this->WindowName;
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
  class_hint->res_name = this->WindowName;
  class_hint->res_class = this->WindowName;
  
  XSetWMProperties(this->DisplayId, 
		   this->WindowId, &window_name, &icon_name,
		   NULL, 0, size_hints, wm_hints, class_hint );

  /* Finally -- we can map the window!  We won't actually render anything
     to the window until the expose event happens later. */
  vtkDebugMacro(<< "Mapping the xwindow\n");
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
  vtkDebugMacro(<< "SB_mapping_mode: DISTORT\n");
  
  // set clipping
  clip_rectangle(this->Fd, 0.0, 1.0, 0.0, 1.0);
  clip_depth(this->Fd, 0.0, 1.0);
  clip_indicator(this->Fd, CLIP_TO_VIEWPORT);
  depth_indicator(this->Fd, TRUE, TRUE);
  
  // use the full color map, initialize it and turn shading on 
  shade_mode(this->Fd, CMAP_FULL | INIT, TRUE); 
  
  // set Fd update state - reset viewport and buffer commands
  this->NumPlanes = depth;
  if (this->DoubleBuffer > 0.0) 
    {
    if ((planes = double_buffer(this->Fd, 
				TRUE | INIT | SUPPRESS_CLEAR,
				depth)) != depth)
      {
      vtkDebugMacro(<< "Only " << planes <<
      " planes available for double buffering\n");
      this->NumPlanes = planes;
      }
    dbuffer_switch(this->Fd,this->Buffer); 
    buffer_mode(this->Fd, TRUE);
    }

  clear_control(this->Fd, CLEAR_DISPLAY_SURFACE | CLEAR_ZBUFFER);
  
  // make default polymarker a dot (pixel) 
  marker_type(this->Fd, 0);

  // clear the display 
  clear_view_surface(this->Fd);

  clear_control(this->Fd, CLEAR_VIEWPORT | CLEAR_ZBUFFER);

  // ignore errors 
  gerr_print_control(NO_ERROR_PRINTING);
  this->Mapped = 1;
}

// Description:
// Initialize the rendering window.
void vtkSbrRenderWindow::Initialize (void)
{
  // make sure we haven't already been initialized 
  if (this->Fd != -1) return;

  // now initialize the window 
  this->WindowInitialize();
}


// Description:
// Change the window to fill the entire screen.
void vtkSbrRenderWindow::SetFullScreen(int arg)
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

// Description:
// Set the preferred window size to full screen.
void vtkSbrRenderWindow::PrefFullScreen()
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


// Description:
// Resize the window.
void vtkSbrRenderWindow::WindowRemap()
{
  // close the starbase window 
  if (this->Fd)
    {
    gclose(this->Fd);
    }
  this->Fd = -1;
  
  /* free the Xwindow we created no need to free the colormap */
  if (this->OwnWindow && this->DisplayId && this->WindowId)
    {
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  XSync(this->DisplayId,0);
  this->WindowId = this->NextWindowId;
  this->NextWindowId = 0;

  // configure the window 
  this->WindowInitialize();
}


// Description:
// Specify the size of the rendering window.
void vtkSbrRenderWindow::SetSize(int x,int y)
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
  
  if ((this->Size[0] == x)&&(this->Size[1] == y))
    {
    return;
    }
  this->Modified();
  this->Size[0] = x;
  this->Size[1] = y;
  
  XResizeWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}

void vtkSbrRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkXRenderWindow::PrintSelf(os,indent);

  os << indent << "Fd: " << this->Fd << "\n";
}

/* There are two possible 8-bit formats, commonly known as 3:3:2 and
6|6|6.  If the SB_X_SHARED_CMAP environment variable is set, we
will use the 6|6|6 format.  Otherwise, we use the 3:3:2 format. 
We'll define some macros to make this easier. */

#define RGB_TO_332( r, g, b ) ( (  r       & 0xe0 ) + \
			      ( (g >> 3) & 0x1c ) + \
			      ( (b >> 6) & 0x03 ) )

/* RGB_TO_666_FACTOR is 5.0/255.0, which is needed to quantize a value in 
the range 0..255 into a range of 0..5 */

#define RGB_TO_666_FACTOR 5.0/255.0
#define RGB_FROM_666_FACTOR 255.0/5.0

#define RGB_TO_666( r, g, b ) ( 40 + \
    (unsigned char)(r * RGB_TO_666_FACTOR) * 36 + \
    (unsigned char)(g * RGB_TO_666_FACTOR) * 6 + \
    (unsigned char)(b * RGB_TO_666_FACTOR) )

#define RED_FROM_666(c) ((unsigned char)(((c-40)/36)*RGB_FROM_666_FACTOR))
#define GREEN_FROM_666(c) ((unsigned char)((((c-40)/6)%6)*RGB_FROM_666_FACTOR))
#define BLUE_FROM_666(c) ((unsigned char)(((c-40)%6)*RGB_FROM_666_FACTOR))
#define RED_FROM_332(c) (c & 0xe0)
#define GREEN_FROM_332(c) ((c & 0x1c) << 3)
#define BLUE_FROM_332(c) ((c & 0x03) << 6)

unsigned char *vtkSbrRenderWindow::GetPixelData(int x1, int y1, 
						int x2, int y2,
						int front)
{
  int     xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned char   *buff1;
  unsigned char   *buff2;
  unsigned char   *buff3;
  unsigned char   *data = NULL;
  unsigned char   *p_data = NULL;
  int cmap_mode, dbuffer_mode, Dfront, SuppressClear;
  
  // make sure values are up to date
  inquire_display_mode(this->Fd, &cmap_mode, &dbuffer_mode, 
		       &this->NumPlanes, &this->Buffer);
  this->DoubleBuffer = (dbuffer_mode & TRUE) ? 1 : 0;
  Dfront = (dbuffer_mode & DFRONT) ? 1 : 0;
  SuppressClear = (dbuffer_mode & SUPPRESS_CLEAR) ? 1 : 0;
  this->Buffer &= 1;

  buff1 = new unsigned char[abs(x2 - x1)+1];
  buff2 = new unsigned char[abs(x2 - x1)+1];
  buff3 = new unsigned char[abs(x2 - x1)+1];
  data = new unsigned char[(abs(x2 - x1) + 1)*(abs(y2 - y1) + 1)*3];

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  /* We'll turn off clipping so that we can do the block write anywhere */
  clip_indicator(this->Fd, CLIP_OFF );

  if (this->DoubleBuffer)
    {
    if (front)
      {
      double_buffer(this->Fd, TRUE | DFRONT, this->NumPlanes);
      }
    else
      {
      double_buffer(this->Fd, TRUE, this->NumPlanes);
      }
    }


  /* now read the binary info one row at a time */
  p_data = data;
  for (yloop = (this->Size[1] - y_low - 1); 
       yloop >= (this->Size[1] - y_hi -1); yloop--)
    {
    if (this->NumPlanes == 24)
      {
      /* No conversion is needed if we're working with a 24-bit frame
	 buffer.  However, the bank_switch() calls are needed to
	 let Starbase know which of the 3 frame buffer banks (red, green,
	 or blue) we wish to write to.  Bank 2 is red, bank 1 is green,
	 and bank 0 is blue. */
      bank_switch( this->Fd, 2, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );
      for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	{
	*p_data = buff1[xloop]; p_data++; 
	*p_data = buff2[xloop]; p_data++; 
	*p_data = buff3[xloop]; p_data++; 
	}
      }
    if (this->NumPlanes == 12)
      {
      /*
	If the frame buffer depth is 12, we still have the red, green, and blue
	banks.  Again, each bank can be considered an array of 8-bit data.  In
	this case, however, there are only 4 bits of meaningful information per
	pixel. Whether this information is contained in the most significant or
	least significant 4 bits of the 8-bit data depends upon a variety of
	factors.  So, when preparing an 8-bit value to be written it is best to
	ensure that the most significant 4 bits are duplicated in the least
	significant 4 bits. When the read occurs Starbase will ensure that 
	the appropriate half of the actual frame buffer data will be modified.
	*/
      
      /* In this case, we will duplicate the most significant nibble
	 (4 bits) of each red, green, and blue value into the least
	 significant nibble. */
      bank_switch( this->Fd, 2, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );

      if ((this->Buffer && (!front)) || ((!this->Buffer) && front))
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = (buff1[xloop] & 0xf0) | ((buff1[xloop] & 0xf0) >> 4); 
	  p_data++; 
	  *p_data = (buff2[xloop] & 0xf0) | ((buff2[xloop] & 0xf0) >> 4); 
	  p_data++; 
	  *p_data = (buff3[xloop] & 0xf0) | ((buff3[xloop] & 0xf0) >> 4); 
	  p_data++; 
	  }
	}
      else
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = ((buff1[xloop] & 0x0f) << 4) | (buff1[xloop] & 0x0f); 
	  p_data++; 
	  *p_data = ((buff2[xloop] & 0x0f) << 4) | (buff2[xloop] & 0x0f); 
	  p_data++; 
	  *p_data = ((buff3[xloop] & 0x0f) << 4) | (buff3[xloop] & 0x0f); 
	  p_data++; 
	  }
	}
      }
    if (this->NumPlanes == 8)
      {
      /*
	If the frame buffer depth is 8, we have a single bank of 8-bit data.
	This case is a bit more complicated because, for each pixel, we need to
	"pack" the 24 bits of red, green, and blue data into a single 8-bit
	value.  If the SB_X_SHARED_CMAP environment variable is set, we pack
	the data using a 6|6|6 scheme. Otherwise, we must pack the data using a
	3:3:2 scheme.  See the "CRX Family of Device Drivers" chapter of the
	Starbase Device Drivers manual for detailed information about the 6|6|6
	and 3:3:2 schemes. In our example, we will define a couple of macros to
	simplify conversion to 6|6|6 and 3:3:2.
	*/
      /* There are two possible 8-bit formats, commonly known as 3:3:2 and
	 6|6|6.  If the SB_X_SHARED_CMAP environment variable is set, we
	 will use the 6|6|6 format.  Otherwise, we use the 3:3:2 format.
	 We'll define some macros to make this easier. */

      dcblock_read( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      if (getenv("SB_X_SHARED_CMAP"))
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = RED_FROM_666(buff1[xloop]); p_data++;
	  *p_data = GREEN_FROM_666(buff1[xloop]); p_data++;
	  *p_data = BLUE_FROM_666(buff1[xloop]); p_data++;
	  }
	}
      else
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  *p_data = RED_FROM_332(buff1[xloop]); p_data++;
	  *p_data = GREEN_FROM_332(buff1[xloop]); p_data++;
	  *p_data = BLUE_FROM_332(buff1[xloop]); p_data++;
	  }
	}
      }
    }
  
  /* Restore the clip_indicator() back to its default value */
  clip_indicator( this->Fd, CLIP_TO_VIEWPORT);

  if (this->DoubleBuffer)
    {
    double_buffer(this->Fd,
		  TRUE
		  | (SuppressClear ? SUPPRESS_CLEAR : 0),
		  this->NumPlanes);
    }

  delete [] buff1;
  delete [] buff2;
  delete [] buff3;

  return data;
}

void vtkSbrRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
				      unsigned char *data,int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     xloop,yloop;
  unsigned char   *buff1;
  unsigned char   *buff2;
  unsigned char   *buff3;
  unsigned char   *p_data = NULL;
  int cmap_mode, dbuffer_mode, Dfront, SuppressClear;
  
  // make sure values are up to date
  inquire_display_mode(this->Fd, &cmap_mode, &dbuffer_mode, 
		       &this->NumPlanes, &this->Buffer);
  this->DoubleBuffer = (dbuffer_mode & TRUE) ? 1 : 0;
  Dfront = (dbuffer_mode & DFRONT) ? 1 : 0;
  SuppressClear = (dbuffer_mode & SUPPRESS_CLEAR) ? 1 : 0;
  this->Buffer &= 1;
  
  /* We'll turn off clipping so that we can do the block write anywhere */
  clip_indicator(this->Fd, CLIP_OFF );

 
  if (this->DoubleBuffer)
    {
    double_buffer(this->Fd, TRUE | (front?DFRONT:0), this->NumPlanes);
    }

  buff1 = new unsigned char[abs(x2 - x1)+1];
  buff2 = new unsigned char[abs(x2 - x1)+1];
  buff3 = new unsigned char[abs(x2 - x1)+1];

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  /* now write the binary info one row at a time */
  p_data = data;
  for (yloop = (this->Size[1] - y_low - 1); 
       yloop >= (this->Size[1] - y_hi -1); yloop--)
    {
    if (this->NumPlanes == 24)
      {
      for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	{
	buff1[xloop] = *p_data; p_data++; 
	buff2[xloop] = *p_data; p_data++;
	buff3[xloop] = *p_data; p_data++;
	}
      /* No conversion is needed if we're working with a 24-bit frame
	 buffer.  However, the bank_switch() calls are needed to
	 let Starbase know which of the 3 frame buffer banks (red, green,
	 or blue) we wish to write to.  Bank 2 is red, bank 1 is green,
	 and bank 0 is blue. */
      bank_switch( this->Fd, 2, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );
      }
    if (this->NumPlanes == 12)
      {
      /*
	If the frame buffer depth is 12, we still have the red, green, and blue
	banks.  Again, each bank can be considered an array of 8-bit data.  In
	this case, however, there are only 4 bits of meaningful information per
	pixel. Whether this information is contained in the most significant or
	least significant 4 bits of the 8-bit data depends upon a variety of
	factors.  So, when preparing an 8-bit value to be written it is best to
	ensure that the most significant 4 bits are duplicated in the least
	significant 4 bits. When the write occurs Starbase will ensure that 
	the appropriate half of the actual frame buffer data will be modified.
	*/
      
      /* In this case, we will duplicate the most significant nibble
	 (4 bits) of each red, green, and blue value into the least
	 significant nibble. */
      for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	{
	buff1[xloop] = (*p_data & 0xf0) | (*p_data >> 4); p_data++; 
	buff2[xloop] = (*p_data & 0xf0) | (*p_data >> 4); p_data++; 
	buff3[xloop] = (*p_data & 0xf0) | (*p_data >> 4); p_data++; 
	}
      bank_switch( this->Fd, 2, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      bank_switch( this->Fd, 1, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff2,FALSE );
      bank_switch( this->Fd, 0, 0 );
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff3,FALSE );
      }
    if (this->NumPlanes == 8)
      {
      /*
	If the frame buffer depth is 8, we have a single bank of 8-bit data.
	This case is a bit more complicated because, for each pixel, we need to
	"pack" the 24 bits of red, green, and blue data into a single 8-bit
	value.  If the SB_X_SHARED_CMAP environment variable is set, we pack
	the data using a 6|6|6 scheme. Otherwise, we must pack the data using a
	3:3:2 scheme.  See the "CRX Family of Device Drivers" chapter of the
	Starbase Device Drivers manual for detailed information about the 6|6|6
	and 3:3:2 schemes. In our example, we will define a couple of macros to
	simplify conversion to 6|6|6 and 3:3:2.
	*/
      /* There are two possible 8-bit formats, commonly known as 3:3:2 and
	 6|6|6.  If the SB_X_SHARED_CMAP environment variable is set, we
	 will use the 6|6|6 format.  Otherwise, we use the 3:3:2 format.
	 We'll define some macros to make this easier. */

      if (getenv("SB_X_SHARED_CMAP"))
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  buff1[xloop] = RGB_TO_666(p_data[0], p_data[1], p_data[2]);
          p_data += 3; 
	  }
	}
      else
	{
	for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
	  {
	  buff1[xloop] = RGB_TO_332(p_data[0], p_data[1], p_data[2]);
          p_data += 3; 
	  }
	}
      /* Now that the data has been converted, we will write the 8-bit
	 values into the window.  There is no need for a bank_switch()
	 since we know that the appropriate 8-bit bank is already
	 enabled for writing. */
      dcblock_write( this->Fd, x_low, yloop,(x_hi - x_low + 1),1,buff1,FALSE );
      }
    }
  
  delete [] buff1;
  delete [] buff2;
  delete [] buff3;

  if (this->DoubleBuffer)
    {
    double_buffer(this->Fd,
		  TRUE
		  | (SuppressClear ? SUPPRESS_CLEAR : 0),
		  this->NumPlanes);
    }

  /* Restore the clip_indicator() back to its default value */
  clip_indicator( this->Fd, CLIP_TO_VIEWPORT);
}
 
float *vtkSbrRenderWindow::GetRGBAPixelData(int x1, int y1, 
						int x2, int y2,
						int front)
{
  vtkErrorMacro(<< "GetRGBAPixelData() not implemented yet for SB.\n");

  return NULL ;
}

void vtkSbrRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
				      float *data, int front)
{
  vtkErrorMacro(<< "SetRGBAPixelData() not implemented yet for SB.\n");

}

float *vtkSbrRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2  )
{
  vtkErrorMacro(<< "GetZbufferData() not implemented yet for SB.\n");

  return NULL ;
}

void vtkSbrRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
                                          float *buffer )
{
  vtkErrorMacro(<< "SetZbufferData() not implemented yet for SB.\n");

}








