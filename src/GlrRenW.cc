/*=========================================================================

  Program:   Visualization Library
  Module:    GlrRenW.cc
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
#include "GlrRenW.hh"
#include "GlrRen.hh"
#include "GlrProp.hh"
#include "GlrCam.hh"
#include "GlrLgt.hh"
#include "gl/glws.h"

#define MAX_LIGHTS 8

#ifndef GD_MULTISAMPLE
#define GD_MULTISAMPLE 1000
#define GC_MS_SAMPLES 1000
void mssize() {return;}
long getgconfig() {return 1;}
void zbsize() {return;}
#endif

/* Declare the data structure for the GL rendering configuration needed */
static GLXconfig the_config[] = 
{
  { GLX_NORMAL,	GLX_RGB,	True} ,
  { GLX_NORMAL,	GLX_DOUBLE,	True} ,
  { GLX_NORMAL,	GLX_ZSIZE,	GLX_NOCONFIG} ,
  { GLX_NORMAL,	GLX_MSSAMPLE,	0} ,
  { GLX_NORMAL,	GLX_MSZSIZE,	0} ,
  { 0,		0,		0}
  };

static float tevprops[]
  = {
  TV_MODULATE, TV_NULL
    };

static unsigned long extract_config_value(int buffer,int mode,
					  GLXconfig *conf)
{
    int	i;
    for (i = 0; conf[i].buffer; i++)
	if (conf[i].buffer == buffer && conf[i].mode == mode)
	    return conf[i].arg;
    return 0;
}

static void set_config_value(int buffer,int mode,
			     GLXconfig *conf,unsigned long value)
{
  int	i;
  for (i = 0; conf[i].buffer; i++)
    {
    if (conf[i].buffer == buffer && conf[i].mode == mode)
      {
      conf[i].arg = (int)value;
      return;
      }
    }
}

/* Extract X visual information */
static XVisualInfo *extract_visual(int buffer,GLXconfig *conf,
				   Display *D,int S)
{
  XVisualInfo	templ;
  int n;
  
  templ.screen = S;
  templ.visualid = extract_config_value(buffer, GLX_VISUAL, conf);
  return XGetVisualInfo (D, VisualScreenMask|VisualIDMask, &templ, &n);
}

/* Fill the configuration structure with the appropriately */
/* created window */
static void set_window(int buffer,Window W,GLXconfig *conf)
{
    int	i;

    for (i = 0; conf[i].buffer; i++)
	if (conf[i].buffer == buffer && conf[i].mode == GLX_WINDOW)
	    conf[i].arg = (int)W;
}

vlGlrRenderWindow::vlGlrRenderWindow()
{
  this->Gid = -2;
  this->MultiSamples = 8;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;

  strcpy(this->Name,"Visualization Library - GL");
}

// Description:
// Create a gl-specific actor.
vlActor *vlGlrRenderWindow::MakeActor()
{
  vlActor *actor;
  vlGlrProperty *prop;

  actor = new vlActor;
  prop  = new vlGlrProperty;

  actor->SetProperty((vlProperty *)prop);
  return (vlActor *)actor;
}

// Description:
// Create a gl specific light.
vlLight *vlGlrRenderWindow::MakeLight()
{
  vlGlrLight *light;

  light = new vlGlrLight;
  return (vlLight *)light;
}

// Description:
// Create a gl specific renderer.
vlRenderer *vlGlrRenderWindow::MakeRenderer()
{
  vlGlrRenderer *ren;

  ren = new vlGlrRenderer;

  // by default we are its parent
  ren->SetRenderWindow((vlRenderWindow*)this);
  
  return (vlRenderer *)ren;
}

// Description:
// Create a gl specific camera.
vlCamera *vlGlrRenderWindow::MakeCamera()
{
  vlGlrCamera *camera;

  camera = new vlGlrCamera;
  return (vlCamera *)camera;
}

// Description:
// Create a gl specific property.
vlProperty *vlGlrRenderWindow::MakeProperty()
{
  vlGlrProperty *property;

  property = new vlGlrProperty;
  return (vlProperty *)property;
}

// Description:
// Begin the rendering process.
void vlGlrRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (this->Gid < 0)
    this->Initialize();

  winset(this->Gid);
}

// Description:
// End the rendering process and display the image.
void vlGlrRenderWindow::Frame(void)
{
  if (this->DoubleBuffer)
    {
    swapbuffers();
    vlDebugMacro(<< " GL swapbuffers\n");
    }
}
 
// Description:
// Specify various window parameters.
void vlGlrRenderWindow::WindowConfigure()
{
  if (this->DoubleBuffer)
    {
    set_config_value(GLX_NORMAL,GLX_DOUBLE,the_config,True);
    }
  else
    {
    set_config_value(GLX_NORMAL,GLX_DOUBLE,the_config,False);
    }
  
  if (this->MultiSamples > 1.0)
    {
    set_config_value(GLX_NORMAL,GLX_ZSIZE,the_config,0);
    set_config_value(GLX_NORMAL,GLX_MSSAMPLE,the_config,
		     (unsigned long)this->MultiSamples);
    set_config_value(GLX_NORMAL,GLX_MSZSIZE,the_config,32);
    if (extract_config_value(GLX_NORMAL,GLX_MSSAMPLE,the_config)
	< this->MultiSamples) 
      {
      vlDebugMacro(<< " Only got " << 
      extract_config_value(GLX_NORMAL,GLX_MSSAMPLE,the_config) 
      << " multisamples\n");
      this->MultiSamples =
	(int)extract_config_value(GLX_NORMAL,GLX_MSSAMPLE,the_config);
      }
    }
  if (this->MultiSamples <= 1.0)
    {
    set_config_value(GLX_NORMAL,GLX_ZSIZE,the_config,GLX_NOCONFIG);
    set_config_value(GLX_NORMAL,GLX_MSSAMPLE,the_config,0);
    set_config_value(GLX_NORMAL,GLX_MSZSIZE,the_config,0);
    }
}


// Description:
// Initialize the window for rendering.
void vlGlrRenderWindow::WindowInitialize (void)
{
  GLXconfig  *conf;
  XVisualInfo  *v;
  XSetWindowAttributes	attr;
  Window  wins[2];
  int x,y,width,height;
  XWindowAttributes winattr;
  XSizeHints xsh;

  xsh.flags = USSize;
  if ((this->Position[0] >= 0)&&(this->Position[1] >= 0))
    {
    xsh.flags |= USPosition;
    xsh.x =  (int)(this->Position[0]);
    xsh.y =  (int)(this->Position[1]);
    }
  
  x = ((this->Position[0] >= 0) ? this->Position[0] : 5);
  y = ((this->Position[1] >= 0) ? this->Position[1] : 5);
  width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  height = ((this->Size[1] > 0) ? this->Size[1] : 300);

  xsh.width  = width;
  xsh.height = height;

  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vlErrorMacro(<< "bad X server connection.\n");
      }
    }

  // test for overlay planes 
  if ((conf = GLXgetconfig(this->DisplayId, 
			   DefaultScreen(this->DisplayId),
			   the_config)) == 0) 
    {
    vlErrorMacro(<< "GL: getconfig failed\n");
    exit(1);
    }

  /*
   * if both the position and size have been set, override the window
   * manager
   */
  attr.override_redirect = False;
  if (this->Borders == 0.0)
    attr.override_redirect = True;

  v = extract_visual(GLX_NORMAL,conf,this->DisplayId,
		     DefaultScreen(this->DisplayId));

  attr.colormap = extract_config_value(GLX_NORMAL, GLX_COLORMAP, conf);
  this->ColorMap = attr.colormap;

  attr.border_pixel = 0;
  this->WindowId = 
    XCreateWindow(this->DisplayId,
		  RootWindow(this->DisplayId,
			     DefaultScreen(this->DisplayId)), 
		  x, y, width, height, 0, v->depth, InputOutput, v->visual,
		  CWBorderPixel|CWColormap|CWOverrideRedirect, &attr);
  XSetNormalHints(this->DisplayId,this->WindowId,&xsh);
  XStoreName(this->DisplayId, this->WindowId, this->Name);
  set_window(GLX_NORMAL, this->WindowId, conf);
  
  // Bind the GL to the created windows 
  if (GLXlink(this->DisplayId, conf) < 0) 
    {
    vlErrorMacro("GL: Bind failed\n");
    exit(1);
    }

  XSelectInput(this->DisplayId, this->WindowId, 
	       KeyPressMask|ExposureMask|StructureNotifyMask);
  vlDebugMacro(" Mapping the xwindow\n");
  XMapWindow(this->DisplayId, this->WindowId);
  XSync(this->DisplayId,False);
  XGetWindowAttributes(this->DisplayId,
		       this->WindowId,&winattr);
  while (winattr.map_state == IsUnmapped)
    {
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    };
  
  if (GLXwinset(this->DisplayId,this->WindowId) < 0)
    {
    vlErrorMacro(<< "GL: winset failed\n");
    exit(1);
    }

  vlDebugMacro(" mmode(MVIEWING)\n");
  mmode(MVIEWING);

  vlDebugMacro(" zbuff stuff\n");
  zbuffer(TRUE);
 
  vlDebugMacro(" texture stuff\n");
  if (getgdesc(GD_TEXTURE))
    {
    tevdef(1,0,tevprops);
    tevbind(TV_ENV0,1);
    }

  vlDebugMacro("% alpha stuff\n");
  if (getgdesc(GD_AFUNCTION))
    {
    afunction(0,AF_NOTEQUAL);
    }

  /*
   * initialize blending for transparency
   */
  vlDebugMacro(" blend func stuff\n");
  blendfunction(BF_SA, BF_MSA);

  this->Mapped = 1;
}

// Description:
// Initialize the rendering window.
void vlGlrRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->Gid >= 0)
    return;

  this->Connect();

  // now initialize the window 
  this->WindowConfigure();
  this->WindowInitialize();

  this->Gid = 1;
}

// Description:
// Make the connection to the window manager.
void vlGlrRenderWindow::Connect()
{
  int status = -1;

  // make sure we haven't already opened 
  if (this->Gid > -2)
    {
    return;
    }

  /* get the default display connection */
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vlErrorMacro(<< "bad X server connection.\n");
      }
    }
  else
    {
    if ((status = (int)dglopen (DisplayString(this->DisplayId), DGLLOCAL)) < 0)
      {
      /* try local host */
      if ((status = (int)dglopen ("localhost:0.0", DGLLOCAL)) < 0)
	{
	vlErrorMacro(<< " error from glopen : " << status << endl);
	exit(-1);
	}
      else
	{
	/* this is recoverable */
	vlErrorMacro(<< " error2 from glopen : " << status << endl);
	exit(-1);
	}
      }
    }

  this->Gid = -1;
}

// Description:
// Change the window to fill the entire screen.
void vlGlrRenderWindow::SetFullScreen(int arg)
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
void vlGlrRenderWindow::PrefFullScreen()
{
  this->Connect();

  // use full screen 
  this->Position[0] = 0;
  this->Position[1] = 0;
  this->Size[0] = (int)getgdesc(GD_XPMAX);
  this->Size[1] = (int)getgdesc(GD_YPMAX);

  // don't show borders 
  this->Borders = 0;
}

// Description:
// Resize the window.
void vlGlrRenderWindow::WindowRemap()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = LIGHT0; cur_light < LIGHT0+MAX_LIGHTS; cur_light++)
    {
    lmbind(cur_light,0);
    }

  // then close the old window 
  XDestroyWindow(this->DisplayId,this->WindowId);
  GLXunlink(this->DisplayId,this->WindowId);
  // set the default windowid 
  this->WindowId = this->NextWindowId;
  this->NextWindowId = NULL;

  /* configure the window */
  this->WindowConfigure();
  this->WindowInitialize();
}

// Description:
// Get the current size of the window.
int *vlGlrRenderWindow::GetSize(void)
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

// Description:
// Get the position in screen coordinates of the window.
int *vlGlrRenderWindow::GetPosition(void)
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

// Description:
// Specify the size of the rendering window.
void vlGlrRenderWindow::SetSize(int x,int y)
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

  XResizeWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}


// Description:
// Get the window display id.
Display *vlGlrRenderWindow::GetDisplayId()
{
  vlDebugMacro(<< "Returning DisplayId of " << (void *)this->DisplayId << "\n"); 

  return this->DisplayId;
}

// Description:
// Get the window id.
Window vlGlrRenderWindow::GetWindowId()
{
  vlDebugMacro(<< "Returning WindowId of " << (void *)this->WindowId << "\n"); 

  return this->WindowId;
}

// Description:
// Set the window id to a pre-existing window.
void vlGlrRenderWindow::SetWindowId(Window arg)
{
  vlDebugMacro(<< "Setting WindowId to " << (void *)arg << "\n"); 

  this->WindowId = arg;
}

// Description:
// Set the display id of the window to a pre-exisiting display id.
void vlGlrRenderWindow::SetDisplayId(Display  *arg)
{
  vlDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
}


void vlGlrRenderWindow::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlGlrRenderWindow::GetClassName()))
    {
    this->vlRenderWindow::PrintSelf(os,indent);

    os << indent << "Color Map: " << this->ColorMap << "\n";
    os << indent << "Display Id: " << this->GetDisplayId() << "\n";
    os << indent << "Gid: " << this->Gid << "\n";
    os << indent << "MultiSamples: " << this->MultiSamples << "\n";
    os << indent << "Next Window Id: " << this->NextWindowId << "\n";
    os << indent << "Window Id: " << this->GetWindowId() << "\n";
    }
}
