/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrRenW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <stdlib.h>
#include <iostream.h>
#include "XglrRenW.hh"
#include "XglrRen.hh"
#include "XglrProp.hh"
#include "XglrText.hh"
#include "XglrCam.hh"
#include "XglrLgt.hh"

// some globals left over from some Sun code
Xgl_sys_state xglr_sys_state = 0;  // XGLR System State object 
static Xgl_bounds_d3d xglr_vdc_window = 
{-1.0, 1.0, -1.0, 1.0, -1.0, 0.0};

vtkXglrRenderWindow::vtkXglrRenderWindow()
{
  this->Context = NULL;
  this->WindowRaster = NULL;
  this->StereoType = VTK_STEREO_CRYSTAL_EYES;
  strcpy(this->Name,"Visualization Toolkit - XGL");
}

// Description:
// Create a XGL specific light.
vtkLightDevice *vtkXglrRenderWindow::MakeLight()
{
  vtkXglrLight *light;

  light = new vtkXglrLight;
  return (vtkLightDevice *)light;
}

// Description:
// Create a XGL specific renderer.
vtkRenderer *vtkXglrRenderWindow::MakeRenderer()
{
  vtkXglrRenderer *ren;

  ren = new vtkXglrRenderer;
  this->AddRenderers(ren);

  // by default we are its parent
  ren->SetRenderWindow((vtkRenderWindow*)this);
  
  return (vtkRenderer *)ren;
}

// Description:
// Create a XGL specific camera.
vtkCameraDevice *vtkXglrRenderWindow::MakeCamera()
{
  vtkXglrCamera *camera;

  camera = new vtkXglrCamera;
  return (vtkCameraDevice *)camera;
}

// Description:
// Create a XGL specific property.
vtkPropertyDevice *vtkXglrRenderWindow::MakeProperty()
{
  vtkXglrProperty *property;

  property = new vtkXglrProperty;
  return (vtkPropertyDevice *)property;
}

// Description:
// Create a XGL specific texture.
vtkTextureDevice *vtkXglrRenderWindow::MakeTexture()
{
  vtkXglrTexture *texture;

  texture = new vtkXglrTexture;
  return (vtkTextureDevice *)texture;
}

// Description:
// Begin the rendering process.
void vtkXglrRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->Context) this->Initialize();
}

// Description:
// Update system if needed due to stereo rendering.
void vtkXglrRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 1;
	}
	break;
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 0;
	}
	break;
      }
    }
}

// Description:
// End the rendering process and display the image.
void vtkXglrRenderWindow::Frame(void)
{
  // flush and display the buffer
  if (this->DoubleBuffer) 
    {
    xgl_object_set(this->Context,
		   XGL_CTX_NEW_FRAME_ACTION,
		   XGL_CTX_NEW_FRAME_SWITCH_BUFFER,
		   0);
    
    // clear canvas area 
    xgl_context_new_frame (this->Context);
    xgl_object_set(this->Context,
		   XGL_CTX_NEW_FRAME_ACTION,
		   XGL_CTX_NEW_FRAME_HLHSR_ACTION | XGL_CTX_NEW_FRAME_CLEAR,
		   0);
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


int vtkXglrRenderWindow::GetDesiredDepth ()
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
Colormap vtkXglrRenderWindow::GetDesiredColormap ()
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

  if (this->ColorMap) 
    {
    return this->ColorMap;
    }

  // get the default visual to use 
  vis = xlib_get_best_visual(this->DisplayId);

  this->ColorMap = 
    XCreateColormap(this->DisplayId,
		    RootWindowOfScreen(ScreenOfDisplay(this->DisplayId,0)),
		    vis, AllocNone);

  if (!this->ColorMap) 
    {
    fprintf(stderr,"Could not create color map\n");
    return 0;
    }
  
  return this->ColorMap;  
}


// Description:
// Get a visual from the windowing system.
Visual *vtkXglrRenderWindow::GetDesiredVisual ()
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


/*
 * XlibWindowCreate
 *
 * use Xlib functions to create a window
 */
static Window
XlibWindowCreate(Display *display, Visual *vis, int depth, char *name, 
		 int w, int h, int x, int y, int borders)
{
  Window	win;
  XEvent	event;
  XSetWindowAttributes values;
  
  values.colormap = 
    XCreateColormap(display, DefaultRootWindow(display), vis, AllocNone);
  values.background_pixel = None;
  values.border_pixel = None;
  
  values.event_mask = 0;
  
  /*
   * if both the position and size have been set, override the window
   * manager
   */
  values.override_redirect = False;
  if ((w > 0) && (x >= 0) && (!borders))
    values.override_redirect = True;
  
  XFlush(display);
  
  win = XCreateWindow(display, DefaultRootWindow(display),
		      x, y, w, h, 0, depth, InputOutput, vis,
		      CWEventMask | CWBackPixel | CWBorderPixel | CWColormap |
		      CWOverrideRedirect, 
		      &values);
  
  XSetStandardProperties(display, win, name, name, None, 0, 0, 0);
  
  XSync(display,False);
  return win;
}
 
// Description:
// Initialize the rendering window.
void vtkXglrRenderWindow::WindowInitialize (void)
{
  Visual *vis;
  Xgl_X_window        xglr_x_win;      /* XGLR-X data structure */
  Xgl_obj_desc        win_desc;       /* XGLR window raster structure */
  XSetWindowAttributes attrs;	      /* X create window attributes */
  XEvent		event;	      /* X event */
  int                   temp_int;
  XWindowAttributes     winattr;
  XSizeHints           *size_hints;
  XClassHint           *class_hint;
  XWMHints             *wm_hints;
  XTextProperty window_name, icon_name;
  char *list[1];

  if (this->Size[0] <=0)
    {
    this->Size[0] = 300;
    this->Size[1] = 300;
    }

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    }

  // get the default visual to use 
  vis = xlib_get_best_visual(this->DisplayId);
  
  if (!this->WindowId) 
    {
    this->WindowId = XlibWindowCreate(this->DisplayId, vis, 
				      this->GetDesiredDepth(), 
				      "Visualization Toolkit - XGL",
				      this->Size[0], 
				      this->Size[1],
				      this->Position[0],
				      this->Position[1],
				      this->Borders);
    this->OwnWindow = 1;
    }
  else
    {
    this->OwnWindow = 0;
    }
  
  list[0] = this->Name;
  XStringListToTextProperty( list, 1, &window_name );
  list[0] = this->Name;
  XStringListToTextProperty( list, 1, &icon_name );

  size_hints = XAllocSizeHints();
  size_hints->flags = USSize;
  if ((this->Position[0] >= 0)&&(this->Position[1] >= 0))
    {
    size_hints->flags |= USPosition;
    size_hints->x =  this->Position[0];
    size_hints->y =  this->Position[1];
    }
  
  size_hints->width  = 
    ((this->Size[0] > 0) ? this->Size[0] : 256);
  size_hints->height = 
    ((this->Size[1] > 0) ? this->Size[1] : 256);
  
  wm_hints = XAllocWMHints();

  class_hint = XAllocClassHint();
  class_hint->res_name = this->Name;
  class_hint->res_class = this->Name;
  
  XSetWMProperties(this->DisplayId, 
		   this->WindowId, &window_name, &icon_name,
		   NULL, 0, size_hints, wm_hints, class_hint );

  if (!xglr_sys_state)
    {
    xglr_sys_state = xgl_open (XGL_UNUSED);
    }
  
  // copy X information into XGLR data structure 
  xglr_x_win.X_display = this->DisplayId;
  xglr_x_win.X_window = this->WindowId;
  xglr_x_win.X_screen = DefaultScreen(this->DisplayId);
  
  // create Window Raster Device using XView canvas 
  win_desc.win_ras.type = XGL_WIN_X | XGL_WIN_X_PROTO_DEFAULT;
  win_desc.win_ras.desc = &xglr_x_win;
  
  this->WindowRaster = xgl_object_create (xglr_sys_state, 
					  XGL_WIN_RAS, &win_desc,
					  XGL_DEV_COLOR_TYPE, 
					  XGL_COLOR_RGB, 0);

  // create XGLR graphics Context object using the Window Raster object
  this->Context = 
    xgl_object_create(xglr_sys_state, XGL_3D_CTX, 0,
		      XGL_CTX_DEVICE, this->WindowRaster, 
		      XGL_CTX_VDC_ORIENTATION, XGL_Y_UP_Z_TOWARD,
		      XGL_CTX_NEW_FRAME_ACTION, 
		      XGL_CTX_NEW_FRAME_HLHSR_ACTION | XGL_CTX_NEW_FRAME_CLEAR,
		      XGL_3D_CTX_HLHSR_MODE, XGL_HLHSR_Z_BUFFER,
		      XGL_CTX_VDC_MAP, XGL_VDC_MAP_ALL, 
		      XGL_CTX_VDC_WINDOW, &xglr_vdc_window,
		      XGL_CTX_VIEW_CLIP_BOUNDS, &xglr_vdc_window,
		      XGL_CTX_CLIP_PLANES,
		      XGL_CLIP_XMIN | XGL_CLIP_XMAX | XGL_CLIP_YMIN |
		      XGL_CLIP_YMAX | XGL_CLIP_ZMIN | XGL_CLIP_ZMAX,
		      XGL_3D_CTX_SURF_FRONT_LIGHT_COMPONENT, 
		      XGL_LIGHT_ENABLE_COMP_AMBIENT | 
		      XGL_LIGHT_ENABLE_COMP_DIFFUSE |
		      XGL_LIGHT_ENABLE_COMP_SPECULAR,
		      XGL_3D_CTX_SURF_BACK_LIGHT_COMPONENT, 
		      XGL_LIGHT_ENABLE_COMP_AMBIENT | 
		      XGL_LIGHT_ENABLE_COMP_DIFFUSE |
		      XGL_LIGHT_ENABLE_COMP_SPECULAR,
		      XGL_3D_CTX_SURF_FACE_DISTINGUISH, TRUE,
		      XGL_3D_CTX_SURF_FACE_CULL, XGL_CULL_OFF,
		      XGL_CTX_DEFERRAL_MODE, XGL_DEFER_ASAP,
		      XGL_3D_CTX_LIGHT_NUM, MAX_LIGHTS,
		      XGL_3D_CTX_SURF_TRANSP_METHOD, XGL_TRANSP_SCREEN_DOOR,
/*		      XGL_3D_CTX_SURF_TRANSP_METHOD, XGL_TRANSP_BLENDED,
		      XGL_3D_CTX_SURF_TRANSP_BLEND_EQ, XGL_BLEND_ARBITRARY_BG,
*/		      0);
  
  // clear canvas area to default background color of black 
  xgl_context_new_frame (this->Context);
  
  // request double buffering from window raster 
  xgl_object_set(this->WindowRaster, XGL_WIN_RAS_BUFFERS_REQUESTED, 2, 0);
  
  // get number of buffers available in hardware underlying window raster 
  xgl_object_get(this->WindowRaster,XGL_WIN_RAS_BUFFERS_ALLOCATED, 
		 &temp_int);
  if (temp_int >= 2)
    {
    this->DoubleBuffer = 1;
    xgl_object_set (this->WindowRaster, XGL_WIN_RAS_BUFFERS_REQUESTED, 2, 0);
    xgl_object_set(this->WindowRaster,
		   XGL_WIN_RAS_BUF_DISPLAY, 0,
		   XGL_WIN_RAS_BUF_DRAW, 1,
		   0);
    }
  else
    {
    this->DoubleBuffer = 0;
    }

  XMapWindow(this->DisplayId, this->WindowId);
  XSync(this->DisplayId,False);
  XGetWindowAttributes(this->DisplayId, this->WindowId,&winattr);
  while (winattr.map_state == IsUnmapped)
    {
    XGetWindowAttributes(this->DisplayId,this->WindowId,&winattr);
    };

  this->Mapped = 1;
}

// Description:
// Initialize the rendering window.
void vtkXglrRenderWindow::Initialize (void)
{
  // make sure we haven't already been initialized 
  if (this->Context) return;

  // now initialize the window 
  this->WindowInitialize();
}

// Description:
// Change the window to fill the entire screen.
void vtkXglrRenderWindow::SetFullScreen(int arg)
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
void vtkXglrRenderWindow::PrefFullScreen()
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
void vtkXglrRenderWindow::WindowRemap()
{
  // close the XGL window 
  if (this->Context)
    {
    xgl_object_destroy(this->WindowRaster);
    xgl_object_destroy(this->Context);
    this->Context = NULL;
    this->WindowRaster = NULL;
    }

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


// Description:
// Specify the size of the rendering window.
void vtkXglrRenderWindow::SetSize(int x,int y)
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
  xgl_window_raster_resize(this->WindowRaster);
}

void vtkXglrRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkXRenderWindow::PrintSelf(os,indent);
}

unsigned char *vtkXglrRenderWindow::GetPixelData(int x1, int y1, int x2, int y2)
{
  long     xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  Xgl_color col;
  Xgl_pt_i2d pos;
  unsigned char   *data = NULL;
  unsigned char   *p_data = NULL;

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

  /* now write the binary info one row at a time */
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    pos.y = yloop;
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      pos.x = xloop;
      xgl_context_get_pixel(this->Context,&pos,&col);
      *p_data = (unsigned char)(col.rgb.r*255.0); p_data++;
      *p_data = (unsigned char)(col.rgb.g*255.0); p_data++;
      *p_data = (unsigned char)(col.rgb.b*255.0); p_data++;
      }
    }
  
  return data;
}

void vtkXglrRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
				     unsigned char *data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     xloop,yloop;
  unsigned char   *p_data = NULL;
  Xgl_color col;
  Xgl_pt_i2d pos;

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
  
  col.rgb.r = 0;
  col.rgb.g = 0;
  col.rgb.b = 0;
  xgl_object_set(this->Context,XGL_CTX_BACKGROUND_COLOR,
		 &col,0);
  xgl_context_new_frame(this->Context);

  // now write the binary info one row at a time 
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    pos.y = yloop;
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      pos.x = xloop;
      col.rgb.r = *p_data/255.0; p_data++; 
      col.rgb.g = *p_data/255.0; p_data++; 
      col.rgb.b = *p_data/255.0; p_data++; 
      xgl_context_set_pixel(this->Context,&pos,&col);
      }
    }
}

// Description:
// Handles work required at end of render cycle
void vtkXglrRenderWindow::CopyResultFrame(void)
{
  if (this->ResultFrame)
    {
    int *size;

    // get the size
    size = this->GetSize();

    this->SetPixelData(0,0,size[0]-1,size[1]-1,this->ResultFrame);
    delete this->ResultFrame;
    this->ResultFrame = NULL;
    }

  this->Frame();
}

