/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlrRenderWindow.cc
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
#include <math.h>
#include <iostream.h>
#include "vtkGlrRenderWindow.hh"
#include "vtkGlrRenderer.hh"
#include "vtkGlrProperty.hh"
#include "vtkGlrTexture.hh"
#include "vtkGlrCamera.hh"
#include "vtkGlrLight.hh"
#include "vtkGlrActor.hh"
#include "vtkGlrPolyMapper.hh"
#include "gl/glws.h"
#include "gl/get.h"

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

vtkGlrRenderWindow::vtkGlrRenderWindow()
{
  this->Gid = -2;
  this->MultiSamples = 8;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;

  strcpy(this->Name,"Visualization Toolkit - GL");
}

// Description:
// Resize the window.
vtkGlrRenderWindow::~vtkGlrRenderWindow()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = LIGHT0; cur_light < LIGHT0+MAX_LIGHTS; cur_light++)
    {
    lmbind(cur_light,0);
    }

  // then close the old window 
  if (this->OwnWindow)
    {
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  GLXunlink(this->DisplayId,this->WindowId);
  XSync(this->DisplayId,0);
}

// Description:
// Create a gl specific light.
vtkLightDevice *vtkGlrRenderWindow::MakeLight()
{
  vtkGlrLight *light;

  light = new vtkGlrLight;
  return (vtkLightDevice *)light;
}

// Description:
// Create a gl specific actor.
vtkActorDevice *vtkGlrRenderWindow::MakeActor()
{
  vtkGlrActor *actor;

  actor = new vtkGlrActor;
  return (vtkActorDevice *)actor;
}

// Description:
// Create a gl specific renderer.
vtkRenderer *vtkGlrRenderWindow::MakeRenderer()
{
  vtkGlrRenderer *ren;

  ren = new vtkGlrRenderer;
  this->AddRenderers(ren);

  // by default we are its parent
  ren->SetRenderWindow((vtkRenderWindow*)this);
  
  return (vtkRenderer *)ren;
}

// Description:
// Create a gl specific camera.
vtkCameraDevice *vtkGlrRenderWindow::MakeCamera()
{
  vtkGlrCamera *camera;

  camera = new vtkGlrCamera;
  return (vtkCameraDevice *)camera;
}

// Description:
// Create a gl specific property.
vtkPropertyDevice *vtkGlrRenderWindow::MakeProperty()
{
  vtkGlrProperty *property;

  property = new vtkGlrProperty;
  return (vtkPropertyDevice *)property;
}

// Description:
// Create a gl specific texture.
vtkTextureDevice *vtkGlrRenderWindow::MakeTexture()
{
  vtkGlrTexture *texture;

  texture = new vtkGlrTexture;
  return (vtkTextureDevice *)texture;
}

// Description:
// Create a XGL specific PolyMapper.
vtkPolyMapperDevice *vtkGlrRenderWindow::MakePolyMapper()
{
  vtkGlrPolyMapper *polyMapper;

  polyMapper = new vtkGlrPolyMapper;
  return (vtkPolyMapperDevice *)polyMapper;
}

// Description:
// Begin the rendering process.
void vtkGlrRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (this->Gid < 0)
    this->Initialize();

  // set the current window 
  GLXwinset(this->DisplayId,this->WindowId);
}

// Description:
// End the rendering process and display the image.
void vtkGlrRenderWindow::Frame(void)
{
  if (this->DoubleBuffer&&this->SwapBuffers)
    {
    swapbuffers();
    vtkDebugMacro(<< " GL swapbuffers\n");
    }
}
 

// Description:
// Update system if needed due to stereo rendering.
void vtkGlrRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	this->OldMonitorSetting = getmonitor();
	gflush();
	setmonitor(STR_RECT);
	gflush();
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
	/* restore the monitor */
	gflush();
	setmonitor(this->OldMonitorSetting);
	gflush();
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
// Specify various window parameters.
void vtkGlrRenderWindow::WindowConfigure()
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
    set_config_value(GLX_NORMAL,GLX_ZSIZE,the_config,GLX_NOCONFIG);
    set_config_value(GLX_NORMAL,GLX_MSSAMPLE,the_config,
		     (unsigned long)this->MultiSamples);
    set_config_value(GLX_NORMAL,GLX_MSZSIZE,the_config,32);
    if (extract_config_value(GLX_NORMAL,GLX_MSSAMPLE,the_config)
	< (unsigned int)this->MultiSamples) 
      {
      vtkDebugMacro(<< " Only got " << 
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
void vtkGlrRenderWindow::WindowInitialize (void)
{
  GLXconfig  *conf;
  XVisualInfo  *v;
  XSetWindowAttributes	attr;
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
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    }

  // test for overlay planes 
  if ((conf = GLXgetconfig(this->DisplayId, 
			   DefaultScreen(this->DisplayId),
			   the_config)) == 0) 
    {
    vtkErrorMacro(<< "GL: getconfig failed\n");
    exit(1);
    }


  attr.override_redirect = False;
  if (this->Borders == 0.0)
    attr.override_redirect = True;
  
  /* create our own window ? */
  this->OwnWindow = 0;
  if (!this->WindowId)
    {
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
    XStoreName(this->DisplayId, this->WindowId, this->Name);
    XSetNormalHints(this->DisplayId,this->WindowId,&xsh);
    this->OwnWindow = 1;
    }
  else
    {
    XChangeWindowAttributes(this->DisplayId,this->WindowId,
			    CWOverrideRedirect, &attr);
    }

  // RESIZE THE WINDOW TO THE DESIRED SIZE
  vtkDebugMacro(<< "Resizing the xwindow\n");
  XResizeWindow(this->DisplayId,this->WindowId,
		((this->Size[0] > 0) ? 
		 (int)(this->Size[0]) : 256),
		((this->Size[1] > 0) ? 
		 (int)(this->Size[1]) : 256));
  XSync(this->DisplayId,False);

  set_window(GLX_NORMAL, this->WindowId, conf);
  
  // Bind the GL to the created windows 
  if (GLXlink(this->DisplayId, conf) < 0) 
    {
    vtkErrorMacro("GL: Bind failed\n");
    exit(1);
    }

  vtkDebugMacro(" Mapping the xwindow\n");
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
    vtkErrorMacro(<< "GL: winset failed\n");
    exit(1);
    }

  vtkDebugMacro(" mmode(MVIEWING)\n");
  mmode(MVIEWING);

  vtkDebugMacro(" zbuff stuff\n");
  zbuffer(TRUE);

  vtkDebugMacro(" subpixel stuff\n");
  subpixel(TRUE);
 
  vtkDebugMacro(" texture stuff\n");
  if (getgdesc(GD_TEXTURE))
    {
    tevdef(1,0,tevprops);
    tevbind(TV_ENV0,1);
    }

  vtkDebugMacro("% alpha stuff\n");
  if (getgdesc(GD_AFUNCTION))
    {
    afunction(0,AF_NOTEQUAL);
    }

  /*
   * initialize blending for transparency
   */
  vtkDebugMacro(" blend func stuff\n");
  blendfunction(BF_SA, BF_MSA);

  this->Mapped = 1;
}

// Description:
// Initialize the rendering window.
void vtkGlrRenderWindow::Initialize (void)
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
void vtkGlrRenderWindow::Connect()
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
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    }
  else
    {
    if ((status = (int)dglopen (DisplayString(this->DisplayId), DGLLOCAL)) < 0)
      {
      /* try local host */
      if ((status = (int)dglopen ("localhost:0.0", DGLLOCAL)) < 0)
	{
	vtkErrorMacro(<< " error from glopen : " << status << endl);
	exit(-1);
	}
      else
	{
	/* this is recoverable */
	vtkErrorMacro(<< " error2 from glopen : " << status << endl);
	exit(-1);
	}
      }
    }

  this->Gid = -1;
}

// Description:
// Change the window to fill the entire screen.
void vtkGlrRenderWindow::SetFullScreen(int arg)
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
void vtkGlrRenderWindow::PrefFullScreen()
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
void vtkGlrRenderWindow::WindowRemap()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = LIGHT0; cur_light < LIGHT0+MAX_LIGHTS; cur_light++)
    {
    lmbind(cur_light,0);
    }

  // then close the old window 
  if (this->OwnWindow)
    {
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  GLXunlink(this->DisplayId,this->WindowId);
  // set the default windowid 
  this->WindowId = this->NextWindowId;
  this->NextWindowId = (Window)NULL;

  /* configure the window */
  this->WindowConfigure();
  this->WindowInitialize();
}

// Description:
// Specify the size of the rendering window.
void vtkGlrRenderWindow::SetSize(int x,int y)
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



int vtkGlrRenderWindow::GetDesiredDepth()
{
  GLXconfig *conf;
  XVisualInfo *v;

  this->Connect();

  if ((conf = GLXgetconfig(this->DisplayId, 
			   DefaultScreen(this->DisplayId),
			   the_config)) == 0) 
    {
    vtkErrorMacro(<< "GL: getconfig failed\n");
    exit(1);
    }

  /* get the default visual to use */
  v = extract_visual(GLX_NORMAL,conf,this->DisplayId,
		     DefaultScreen(this->DisplayId));

  return v->depth;  
}

// Description:
// Get a visual from the windowing system.
Visual *vtkGlrRenderWindow::GetDesiredVisual ()
{
  XVisualInfo  *v;
  GLXconfig *conf;

  this->Connect();

  if ((conf = GLXgetconfig(this->DisplayId, 
			   DefaultScreen(this->DisplayId),
			   the_config)) == 0) 
    {
    vtkErrorMacro(<< "GL: getconfig failed\n");
    exit(1);
    }

  /* get the default visual to use */
  v = extract_visual(GLX_NORMAL,conf,this->DisplayId,
		     DefaultScreen(this->DisplayId));

  return v->visual;  
}


// Description:
// Get a colormap from the windowing system.
Colormap vtkGlrRenderWindow::GetDesiredColormap ()
{
  GLXconfig *conf;

  if (this->ColorMap) return this->ColorMap;

  this->Connect();

  if ((conf = GLXgetconfig(this->DisplayId, 
			   DefaultScreen(this->DisplayId),
			   the_config)) == 0) 
    {
    vtkErrorMacro(<< "GL: getconfig failed\n");
    exit(1);
    }

  /* get the default colormap to use */
  this->ColorMap = extract_config_value(GLX_NORMAL, GLX_COLORMAP, conf);

  return this->ColorMap;  
}

void vtkGlrRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkXRenderWindow::PrintSelf(os,indent);

  os << indent << "Gid: " << this->Gid << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}

unsigned char *vtkGlrRenderWindow::GetPixelData(int x1, int y1, int x2, int y2,
						int front)
{
  long     xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned long   *buffer;
  unsigned char   *data = NULL;
  unsigned char   *p_data = NULL;

  /* set the current window */
  GLXwinset(this->DisplayId,this->WindowId);

  buffer = new unsigned long[abs(x2 - x1)+1];
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

  if (front)
    {
    readsource(SRC_FRONT);
    }
  else
    { 
    readsource(SRC_BACK);
    }
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    // read in a row of pixels 
    lrectread(x_low,yloop,x_hi,yloop,buffer);
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      *p_data = buffer[xloop] & (0x000000ff); p_data++;
      *p_data = (buffer[xloop] & (0x0000ff00)) >> 8; p_data++;
      *p_data = (buffer[xloop] & (0x00ff0000)) >> 16; p_data++;
      }
    }
  
  delete [] buffer;

  return data;
}

void vtkGlrRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
				     unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     xloop,yloop;
  unsigned long   *buffer;
  unsigned char   *p_data = NULL;
  long lastBuffer = 0;
  
  // set the current window 
  GLXwinset(this->DisplayId,this->WindowId);

  if (this->DoubleBuffer)
    {
    lastBuffer = getbuffer();
    if (front)
      {
      frontbuffer(TRUE);
      }
    else
      {
      backbuffer(TRUE);
      }
    }
  dither(DT_OFF);

  buffer = new unsigned long[4*(abs(x2 - x1)+1)];

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
  
  viewport(x_low,x_hi,y_low,y_hi);

  /* now write the binary info one row at a time */
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      buffer[xloop] = 0xff000000 + *p_data; p_data++; 
      buffer[xloop] += (*p_data) << 8; p_data++;
      buffer[xloop] += (*p_data) << 16; p_data++;
      }
    /* write out a row of pixels */
    lrectwrite(x_low,yloop,x_hi,yloop,buffer);
    }
  
  delete [] buffer;

  dither(DT_ON);
  if (this->DoubleBuffer)
    {
    if (lastBuffer == FRNTBUFFER)
      {
      frontbuffer(TRUE);
      }
    else
      {
      backbuffer(TRUE);
      }
    }
}


unsigned char *vtkGlrRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
						int front)
{
  long     xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned long   *buffer;
  unsigned char   *data = NULL;
  unsigned char   *p_data = NULL;

  /* set the current window */
  GLXwinset(this->DisplayId,this->WindowId);

  buffer = new unsigned long[abs(x2 - x1)+1];
  data = new unsigned char[(abs(x2 - x1) + 1)*(abs(y2 - y1) + 1)*4];

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

  if (front)
    {
    readsource(SRC_FRONT);
    }
  else
    { 
    readsource(SRC_BACK);
    }
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    // read in a row of pixels 
    lrectread(x_low,yloop,x_hi,yloop,buffer);
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      *p_data = buffer[xloop] & (0x000000ff); p_data++;
      *p_data = (buffer[xloop] & (0x0000ff00)) >> 8; p_data++;
      *p_data = (buffer[xloop] & (0x00ff0000)) >> 16; p_data++;
      *p_data = (buffer[xloop] & (0xff000000)) >> 24; p_data++;
      }
    }
  
  delete [] buffer;

  return data;
}

void vtkGlrRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
				     unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     xloop,yloop;
  unsigned long   *buffer;
  unsigned char   *p_data = NULL;
  long lastBuffer = 0;
  
  // set the current window 
  GLXwinset(this->DisplayId,this->WindowId);

  if (this->DoubleBuffer)
    {
    lastBuffer = getbuffer();
    if (front)
      {
      frontbuffer(TRUE);
      }
    else
      {
      backbuffer(TRUE);
      }
    }
  dither(DT_OFF);

  buffer = new unsigned long[4*(abs(x2 - x1)+1)];

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
  
  viewport(x_low,x_hi,y_low,y_hi);

  /* now write the binary info one row at a time */
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      buffer[xloop] =  *p_data; p_data++; 
      buffer[xloop] += (*p_data) << 8; p_data++;
      buffer[xloop] += (*p_data) << 16; p_data++;
      buffer[xloop] += (*p_data) << 24; p_data++;
      }
    /* write out a row of pixels */
    lrectwrite(x_low,yloop,x_hi,yloop,buffer);
    }
  
  delete [] buffer;

  dither(DT_ON);
  if (this->DoubleBuffer)
    {
    if (lastBuffer == FRNTBUFFER)
      {
      frontbuffer(TRUE);
      }
    else
      {
      backbuffer(TRUE);
      }
    }
}

void vtkGlrRenderWindow::SetZbufferData(int x1, int y1, int x2, int y2, 
					float *f_z_data )
{
  int    y_low, y_hi;
  int    x_low, x_hi;
  int    width, height;
  int    i;

  long   z_min;
  long   z_max;
  long   z_range;
  int    z_bits;

  long   value;

  long   *l_z_data = NULL;
  long   *l_z_ptr;
  float  *f_z_ptr;

  // set the current window 
  GLXwinset(this->DisplayId,this->WindowId);

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

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  /* This assumes that no one has changed the zbuffer max and min */
  /* using lsetdepth() */
  z_min = getgdesc(GD_ZMIN);
  z_max = getgdesc(GD_ZMAX);
  z_range = z_max - z_min;
  z_bits = getgdesc(GD_BITS_NORM_ZBUFFER);

  l_z_data = new long[width*height];
  l_z_ptr = l_z_data;
  f_z_ptr = f_z_data;

  // Convert float zbuffer values into integers
  for( i=0; i<(width*height); i++ )
  {
    value = (long)(*(f_z_ptr++) * (double)z_range) + z_min;

    // Sign bits might need to be cut off??

    *(l_z_ptr++) = value;
  }

  // Write the converted data into the zbuffer
  pixmode( PM_ZDATA, 1 );

  lrectwrite( x1, y1, x2, y2, (unsigned long *)l_z_data );

  pixmode( PM_ZDATA, 0 );

  free( l_z_data );
}

float *vtkGlrRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2 )
{
  int    y_low, y_hi;
  int    x_low, x_hi;
  int    width, height;
  float  *f_z_data = NULL;
  long   *l_z_data = NULL;
  float  *f_z_ptr;

  long   value;
  int    i;

  double z_min;
  double z_max;
  double z_range;
  int    z_bits;

  // Set the current window 
  GLXwinset(this->DisplayId,this->WindowId);

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

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  f_z_data = new float[width*height];
  l_z_data = (long *)f_z_data;

  readsource( SRC_ZBUFFER );

  lrectread( x_low, y_low, x_hi, y_hi, (unsigned long *)l_z_data );

  /* This assumes that no one has changed the zbuffer max and min */
  /* using lsetdepth() */
  z_min = (double)getgdesc(GD_ZMIN);
  z_max = (double)getgdesc(GD_ZMAX);
  z_range = z_max - z_min;
  z_bits = getgdesc(GD_BITS_NORM_ZBUFFER);

  f_z_ptr = f_z_data;

  for( i=0; i<(width*height);  i++ )
  {
    value = *(l_z_data++);

    if( z_bits == 23 || z_bits == 24 )
    {
      /* Extend the sign bit if necessary */
      if( value & 0x00800000 )
        value = 0xff800000 | value;
      else
        value = 0x007fffff & value;
    }

    *(f_z_ptr++) = ((double)value - z_min)/z_range;
  }

  return ( f_z_data );
}

