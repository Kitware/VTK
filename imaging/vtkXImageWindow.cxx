/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXImageWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkXImageWindow.h"
#include "vtkObjectFactory.h"


#ifndef VTK_REMOVE_LEGACY_CODE
//mark this class for future legacy-related changes
#endif

//--------------------------------------------------------------------------
vtkXImageWindow* vtkXImageWindow::New()
{
  vtkGenericWarningMacro(<<"Obsolete native imaging class: " 
                         <<"use OpenGL version instead");

  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXImageWindow");
  if(ret)
    {
    return (vtkXImageWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXImageWindow;
}

void vtkXImageWindow::GetShiftsScalesAndMasks(int &rshift, int &gshift, 
					      int &bshift,
					      int &rscale, int &gscale, 
					      int &bscale, 
					      unsigned long &rmask,
					      unsigned long &gmask,
					      unsigned long &bmask)
{
  XWindowAttributes winAttribs;
  XVisualInfo temp1;

  if ( !this->WindowId )
    {
    vtkErrorMacro ( << "Attempt to use a NULL WindowId" );
    return;
    }
	
  XGetWindowAttributes(this->DisplayId, this->WindowId, &winAttribs);
  temp1.visualid = winAttribs.visual->visualid;
  temp1.screen = DefaultScreen(this->DisplayId);
  int nvisuals = 0;
  XVisualInfo* visuals = 
    XGetVisualInfo(this->DisplayId, VisualIDMask | VisualScreenMask, &temp1, 
		   &nvisuals);   
  if (nvisuals == 0)  vtkErrorMacro(<<"Could not get color masks");
  
  rmask = visuals->red_mask;
  gmask = visuals->green_mask;
  bmask = visuals->blue_mask;
  
  XFree(visuals);

  // Compute the shifts needed to align the color masks with the 
  // pixels
  rshift = 0;
  gshift = 0;
  bshift = 0;
  unsigned long tmp;

  tmp = rmask;
  while (tmp != 0)
    {
    tmp = tmp >> 1;
    rshift++;
    }
  tmp = gmask;
  while (tmp != 0)
    {
    tmp = tmp >> 1;
    gshift++;
    }
  tmp = bmask;
  while (tmp != 0)
    {
    tmp = tmp >> 1;
    bshift++;
    }

  // determine the "cumulative" shifts and relative scales
  int t_rshift, t_gshift, t_bshift;
  t_rshift = 0;
  t_gshift = 0;
  t_bshift = 0;
  rscale = 8;
  gscale = 8;
  bscale = 8;
  if (rshift > gshift)
    {
    // r > g
    if (gshift > bshift)
      {
      // r > g > b
      t_rshift = gshift;
      t_gshift = bshift;
      rscale = rshift - gshift;
      gscale = gshift - bshift;
      bscale = bshift;
      }
    else
      {
      // r > b > g
      t_rshift = bshift;
      t_bshift = gshift;
      rscale = rshift - gshift;
      bscale = bshift - gshift;
      gscale = gshift;
      }
    }
  else
    {
    // g > r
    if (gshift > bshift)
      {
      // g > r,  g > b
      if (bshift > rshift)
	{
	// g > b > r
	t_gshift = bshift;
	t_bshift = rshift;
	gscale = gshift - bshift;
	bscale = bshift - rshift;
	rscale = rshift;
	}
      else
	{
	// g > r > b
	t_gshift = rshift;
	t_rshift = bshift;
	gscale = gshift - rshift;
	rscale = rshift - bshift;
	bscale = bshift;
	}
      }
    else
      {
      // b > g > r
      t_bshift = gshift;
      t_gshift = rshift;
      bscale = bshift - gshift;
      gscale = gshift - rshift;
      rscale = rshift;
      }
    }

  rshift = t_rshift;
  gshift = t_gshift;
  bshift = t_bshift;
}


unsigned char *vtkXImageWindow::GetPixelData(int x1, int y1, 
					     int x2, int y2, int)
{
  vtkDebugMacro (<< "Getting pixel data...");

  int width  = (abs(x2 - x1)+1);
  int height = (abs(y2 - y1)+1);
  int rshift, gshift, bshift;
  int rscale, gscale, bscale;
  unsigned long rmask, gmask, bmask;

  if ( !this->WindowId )
    {
    vtkErrorMacro ( << "Attempt to use NULL WindowId" );
    return (unsigned char*) NULL;
    }
	
  this->GetShiftsScalesAndMasks(rshift,gshift,bshift,rscale,gscale,bscale,
				rmask,gmask,bmask);

  // Get the XImage
  XImage* image = XGetImage(this->DisplayId, this->WindowId, x1, y1,
			    width, height, AllPlanes, XYPixmap);

  // Allocate space for the data
  unsigned char*  data = new unsigned char[width*height*3];
  if (!data) 
    {
    vtkErrorMacro(<< "Failed to malloc space for pixel data!");
    return (unsigned char*) NULL;
    }

  // Pointer to index through the data
  unsigned char* p_data = data;

  // Set up the loop indices
  int yloop = 0;
  int xloop = 0;
  unsigned long pixel = 0; 
  int y_low = 0;
  int y_hi = 0;
  int x_low = 0;
  int x_hi = 0;

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

  for (yloop = y_hi; yloop >= y_low; yloop--)
    {
    for (xloop = x_low; xloop <= x_hi ; xloop++)
      {
      pixel = XGetPixel(image, xloop, yloop);
      *p_data = ((pixel & rmask) >> rshift) << (8-rscale); p_data++;
      *p_data = ((pixel & gmask) >> gshift) << (8-gscale); p_data++;
      *p_data = ((pixel & bmask) >> bshift) << (8-bscale); p_data++;
      }
    }
 
  XDestroyImage(image);
  
  return data;
}

void vtkXImageWindow::Frame()
{
  this->SwapBuffers();
}


void vtkXImageWindow::SwapBuffers()
{
  static int swapFlag = 0;

  if (swapFlag == 0)
    {
    swapFlag = 1;
    }
  else
    {
    if ( !this->WindowId)
      {
      vtkErrorMacro ( << "Attempt to use NULL WindowId" );
      return;
      }
    if (this->DoubleBuffer)
      {
      XCopyArea(this->DisplayId, this->Drawable, this->WindowId, this->Gc, 
		0, 0, this->Size[0], this->Size[1], 0, 0);
      swapFlag = 0;
      }
    }
    XSync(this->DisplayId, False);
    XFlush(this->DisplayId);
}

void *vtkXImageWindow::GetGenericDrawable()
{
  if (this->DoubleBuffer)
    {
    if ( !this->WindowId )
      {
      vtkErrorMacro ( << "Attempt to use NULL WindowId" );
      return (void*) NULL;
      }
    if (!this->Drawable)
      {
      this->Drawable = XCreatePixmap(this->DisplayId, this->WindowId, 
				     this->Size[0], this->Size[1],
				     this->VisualDepth);
      this->PixmapWidth = this->Size[0];
      this->PixmapHeight = this->Size[1];
      }
    else if ((this->PixmapWidth != this->Size[0]) || 
	     (this->PixmapHeight != this->Size[1]))
      {
      XFreePixmap(this->DisplayId, this->Drawable);
      this->Drawable = XCreatePixmap(this->DisplayId, this->WindowId, 
				      this->Size[0], this->Size[1],
				      this->VisualDepth);       
      this->PixmapWidth = this->Size[0];
      this->PixmapHeight = this->Size[1];
      }
    return (void *) this->Drawable;
    }
  else
    {
    return (void *) this->WindowId;
    }

}


//----------------------------------------------------------------------------
vtkXImageWindow::vtkXImageWindow()
{
  vtkDebugMacro(<< "vtkXImageWindow::vtkXImageWindow");
  this->ParentId = (Window)(NULL);
  this->WindowId = (Window)(NULL);
  this->DisplayId = (Display *)NULL;
  this->VisualId = 0;
  this->VisualDepth = 0;
  this->VisualClass = 0;
  this->ColorMap = (Colormap) NULL;
  this->Gc = NULL;
  this->Offset = 0;
  this->NumberOfColors = 150;
  this->Drawable = (Pixmap) NULL;
  this->OwnDisplay = 0;
  this->PixmapWidth = 0;
  this->PixmapHeight = 0;
  this->WindowCreated = 0;
}


//----------------------------------------------------------------------------
vtkXImageWindow::~vtkXImageWindow()
{
  vtkDebugMacro(<< "vtkXImageWindow::vtkXImageWindow");

  /* free the Xwindow we created no need to free the colormap */
  if (this->DisplayId && this->WindowId && this->WindowCreated)
    {
    if (this->Gc)
      {
      XFreeGC(this->DisplayId, this->Gc);
      }
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  if (this->DisplayId)
    {
    XSync(this->DisplayId,0);
    }
  if (this->OwnDisplay && this->DisplayId)
    {
    XCloseDisplay(this->DisplayId);
    }
}


//----------------------------------------------------------------------------
void vtkXImageWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageWindow::PrintSelf(os, indent);
  os << indent << "Parent Id: " << this->ParentId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
  os << indent << "Display Id: " << this->DisplayId << "\n";
  os << indent << "Visual Id: " << this->VisualId << "\n";
  os << indent << "Visual Depth: " << this->VisualDepth << "\n";
  os << indent << "Visual Class: " << this->VisualClass << "\n";
  os << indent << "ColorMap: " << this->ColorMap << "\n";
  os << indent << "GC: " << this->Gc << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Colors: " << this->Colors << "\n";
  os << indent << "Number Of Colors: " << this->NumberOfColors << "\n";
  os << indent << "Drawable: " << this->Drawable << "\n";

}

void vtkXImageWindow::SetWindowName(char* name)
{
  XTextProperty win_name_text_prop;

  vtkImageWindow::SetWindowName(name);
  
  if (this->Mapped)
    {
    if( XStringListToTextProperty( &name, 1, &win_name_text_prop ) == 0 )
      {
      XFree (win_name_text_prop.value);
      vtkWarningMacro(<< "Can't rename window"); 
      return;
      }
    if ( !this->WindowId )
      {
      vtkErrorMacro ( << "Attempt to use NULL WindowId" );
      return;
      }
   
    XSetWMName( this->DisplayId, this->WindowId, &win_name_text_prop );
    XSetWMIconName( this->DisplayId, this->WindowId, &win_name_text_prop );
    XFree (win_name_text_prop.value);
    }
}


void vtkXImageWindow::SetBackgroundColor(float r, float g, float b)
{
  unsigned long background = 0;
  unsigned long rmask = 0;
  unsigned long gmask = 0;
  unsigned long bmask = 0;
  Window windowID;
  Display* displayID;
  XWindowAttributes winAttribs;
  XVisualInfo temp1;
  int nvisuals = 0;
  XVisualInfo* visuals;
  int rshift = 0;
  int gshift = 0;
  int bshift = 0;
  // I think these need to be unsigned char
  unsigned short test = 1;
  unsigned short red = 0;
  unsigned short green = 0;
  unsigned short blue = 0;

  if ( !this->WindowId )
    {
    vtkErrorMacro ( << "Attempt to use NULL WindowId" );
    return;
    }
	
  // Get color masks from visual
  windowID = (Window) this->GetGenericWindowId();
  displayID = (Display*) this->GetGenericDisplayId();
  XGetWindowAttributes(displayID, windowID, &winAttribs);
  temp1.visualid = winAttribs.visual->visualid;
  visuals = XGetVisualInfo(displayID, VisualIDMask, &temp1, &nvisuals);   
  if (nvisuals == 0)  
    {
    vtkErrorMacro(<<"Could not get color masks");
    }

  rmask = visuals->red_mask;
  gmask = visuals->green_mask;
  bmask = visuals->blue_mask;

  XFree(visuals);

  // Compute the shifts needed to align the color masks with the 
  // pixels
  
  while ( ((rmask & test) == 0) && (rshift < 32))
    {
    rmask = rmask >> 1;
    rshift++;
    }

  while ( ((gmask & test) == 0) && (gshift < 32))
    {
    gmask = gmask >> 1;
    gshift++;
    }

  while ( ((bmask & test) == 0) && (bshift < 32))
    {
    bmask = bmask >> 1;
    bshift++;
    }
  
  // Check if colors are > 1 ???
  red = (unsigned short) (r * 255.0);
  green = (unsigned short) (g * 255.0);
  blue = (unsigned short)  (b * 255.0);

  background = background | (blue << bshift);
  background = background | (green << gshift);
  background = background | (red << rshift);

  vtkDebugMacro(<<"vtkXImageWindow::SetBackgroundColor - value: " << background);

  vtkDebugMacro(<<"vtkXImageWindow::SetBackgroundColor - red: " << red << ", green: " << green <<
  ", blue: " << blue);

  // what should I do if a window has not been created (lawcc dfss)
  if (this->WindowId == (Window)(NULL))
    {
    this->MakeDefaultWindow();
    }
  
  XSetWindowBackground(this->DisplayId, this->WindowId, background);

  XClearWindow(this->DisplayId, this->WindowId);

  XFlush(this->DisplayId);
   
  XSync(this->DisplayId, False); 
}

void vtkXImageWindow::EraseWindow()
{
  
  // what should I do if a window has not been created (lawcc dfss)
  if (this->WindowId == (Window)(NULL))
    {
    this->MakeDefaultWindow();
    }
  
  // If double buffering is on and we don't have a drawable
  // yet, then we better make one
  if (this->DoubleBuffer && !this->Drawable)
    {
    this->GetGenericDrawable();
    }

  // Erase the drawable if double buffering is on
  // and the drawable exists
  if (this->DoubleBuffer && this->Drawable)
    {
vtkWarningMacro ("EraseWindow");
    // Get the old foreground and background
    XGCValues vals;
    XGetGCValues(this->DisplayId, this->Gc, GCForeground, &vals);
    unsigned long oldForeground = vals.foreground;

    // Set the foreground color to the background so the rectangle
    // matches the background color
    XColor aColor;
    aColor.red = 65535;
    aColor.green = 0;
    aColor.blue = 0;
    XAllocColor(this->DisplayId,this->ColorMap,&aColor);
    XSetForeground(this->DisplayId, this->Gc, aColor.pixel);
    XFillRectangle(this->DisplayId, this->Drawable, this->Gc, 0, 0, 
	           this->Size[0], this->Size[1]);

    // Reset the foreground to it's previous color
    XSetForeground (this->DisplayId, this->Gc, oldForeground);
    }
  // otherwise, erase the window
  else
    {
    XClearWindow(this->DisplayId,this->WindowId);
    XFlush(this->DisplayId);
    }
}


// Get this RenderWindow's X window id.
Window vtkXImageWindow::GetWindowId()
{
  return this->WindowId;
}

// Get this RenderWindow's parent X window id.
Window vtkXImageWindow::GetParentId()
{
  return this->ParentId;
}

// Sets the parent of the window that WILL BE created.
void vtkXImageWindow::SetParentId(Window arg)
{
  if (this->ParentId)
    {
    vtkErrorMacro("ParentId is already set.");
    return;
    }
  this->ParentId = arg;
}

void vtkXImageWindow::SetParentId(void* arg)
{
  this->SetParentId((Window)arg);
}


// Get the position in screen coordinates (pixels) of the window.
int *vtkXImageWindow::GetPosition(void)
{
  XWindowAttributes attribs;
  int x,y;
  Window child;
 
  // what should I do if a window has not been created (lawcc dfss)
  if (this->WindowId == (Window)(NULL))
    {
    this->MakeDefaultWindow();
    }
 
  // if we aren't mapped then just return the ivar 
  if ( ! this->Mapped)
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

// Move the window to a new position on the display.
void vtkXImageWindow::SetPosition(int x, int y)
{
  // if we aren't mapped then just set the ivars
  if (!this->Mapped)
    {
    if ((this->Position[0] != x)||(this->Position[1] != y))
      {
      this->Modified();
      }
    this->Position[0] = x;
    this->Position[1] = y;
    return;
    }
	
  if ( !this->WindowId )
    {
    vtkErrorMacro ( << "Attempt to use NULL WindowId" );
    return;
    }

  // This is different from vtkXRenderWindow which calls XMoveResizeWindow.
  // XMoveResizeWindow cannot be called here beacuse, this->Size[] may be zero.
  XMoveWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}


void vtkXImageWindow::SetSize(int x, int y)
{
  // If the values have changed, change the ivars
  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;

    }

  // If the window isn't displayed, return
  if (!this->Mapped)
    {
    return;
    }

  if ( !this->WindowId )
    {
    vtkErrorMacro ( << "Attempt to use NULL WindowId" );
    return;
    }

   // Make the X window call 
   XResizeWindow(this->DisplayId, this->WindowId, this->Size[0], this->Size[1]);


   // Need the XFlush to clear the X queue or else there are X timing problems
   // i.e. The first image may not display correctly
   XFlush(this->DisplayId);
   
   XSync(this->DisplayId, False); 
}


int* vtkXImageWindow::GetSize()
{
  XWindowAttributes attribs;
 
  vtkDebugMacro (<< "vtkXImageWindow::GetSize");
 
  // if we aren't mapped then just return the ivar 
  if ( ! this->Mapped)
    {
    vtkDebugMacro (<< "vtkXImageWindow::GetSize - Window not mapped");
    return(this->Size);
    }
		
  if ( !this->WindowId )
    {
    vtkErrorMacro ( << "Attempt to use NULL WindowId" );
    return(this->Size);
    }

  //  Find the current window size 
  XFlush(this->DisplayId);
  XSync(this->DisplayId, False);

  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);


  // May want to put in call to update Size ivars if different than 
  // what X thinks the size is

  this->Size[0] = attribs.width;
  this->Size[1] = attribs.height;

  return this->Size; 

}

// Set this ImageWindow's X window id to a pre-existing window.
void vtkXImageWindow::SetWindowInfo(char *info)
{
  int tmp;
  
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  sscanf(info,"%i",&tmp);
 
  this->SetWindowId(tmp);
}

// Sets the X window id of the window that WILL BE created.
void vtkXImageWindow::SetParentInfo(char *info)
{
  int tmp;
  
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  sscanf(info,"%i",&tmp);
 
  this->SetParentId(tmp);
}

//----------------------------------------------------------------------------
void vtkXImageWindow::MakeDefaultWindow() 
{
  char name[80];
  int screen;
  XVisualInfo info;
  XSetWindowAttributes values;
  XSizeHints xsh;
  int x, y, width, height;

  vtkDebugMacro (<< "vtkXImageWindow::MakeDefaultWindow" ); 
  
  strcpy(name,"vtk - X Viewer Window");

  // make sure we have a connection to the X server.
  if ( ! this->DisplayId)
    {
    if ( ( this->DisplayId = XOpenDisplay((char *)NULL)) == NULL) 
      {
      vtkErrorMacro(<<"cannot connect to X server"<< XDisplayName((char *)NULL));
      exit(-1);
      }
    this->OwnDisplay = 1;
    }
  
  
  screen = DefaultScreen(this->DisplayId);
  this->GetDefaultVisualInfo(&info);
  
  values.override_redirect = False;

  // Create our own window?
  this->WindowCreated = 0;
  if (!this->WindowId)
    {
    // If this is a pseudocolor visual, create a color map.
    values.colormap = this->GetDesiredColormap();
  
    XColor aColor;
    aColor.red = 0;
    aColor.green = 0;
    aColor.blue = 0;
    XAllocColor(this->DisplayId,values.colormap,&aColor);
    values.background_pixel = aColor.pixel;
    values.border_pixel = None;
    values.event_mask = 0;

    //  if ((w > 0) && (x >= 0) && (!borders))
    //  values.override_redirect = True;
    XFlush(this->DisplayId);

    // get a default parent if one has not been set.
    if (! this->ParentId)
      {
      this->ParentId = RootWindow(this->DisplayId, screen);
      }

    // if size not set use default of 256
    if (this->Size[0] == 0) 
      {
      this->Size[0] = 256;
      this->Size[1] = 256;
      }
    
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

    this->WindowId = XCreateWindow(this->DisplayId, this->ParentId,
			   x, y, width, height, 0, info.depth, 
			   InputOutput, info.visual,
			   CWEventMask | CWBackPixel | CWBorderPixel | 
			   CWColormap | CWOverrideRedirect, 
			   &values);
    XSetStandardProperties(this->DisplayId, this->WindowId,
			   name, name, None, 0, 0, 0);
    XSetNormalHints(this->DisplayId,this->WindowId,&xsh);

    XSync(this->DisplayId, False);
    this->WindowCreated = 1;
    }
  else
    {
    XChangeWindowAttributes(this->DisplayId,this->WindowId,
			    CWOverrideRedirect, &values);
    }
  // Select event types wanted 
  XSelectInput(this->DisplayId, this->WindowId,
	       ExposureMask | KeyPressMask | ButtonPressMask |
	       PointerMotionMask | StructureNotifyMask | PropertyChangeMask);
  
  // Map Window onto Screen and sysc
  // RESIZE THE WINDOW TO THE DESIRED SIZE
  vtkDebugMacro(<< "Resizing the xwindow\n");
  XResizeWindow(this->DisplayId,this->WindowId,
		((this->Size[0] > 0) ? 
		 (int)(this->Size[0]) : 256),
		((this->Size[1] > 0) ? 
		 (int)(this->Size[1]) : 256));
  XSync(this->DisplayId,False);
  XMapWindow(this->DisplayId, this->WindowId);
  
  XSync(this->DisplayId,0);
 
// ####

  XVisualInfo templ;
  XVisualInfo *visuals;
  int nvisuals;
  XWindowAttributes attributes;
 
  // Create a graphics contect for this window
  this->Gc = XCreateGC(this->DisplayId, this->WindowId, 0, NULL);
  XSetForeground(this->DisplayId, this->Gc, 0XFFFFFF);
  XSetBackground(this->DisplayId, this->Gc, 0X000000);

  // Get the visual
  if ( ! XGetWindowAttributes(this->DisplayId, this->WindowId, &attributes))
    {
    vtkErrorMacro(<< "SetWindow: Could not get window attributes.");
    return;
    }
  this->VisualId = attributes.visual;
  this->VisualDepth = attributes.depth;
  this->ColorMap = attributes.colormap;
 
  //####
  if (this->ColorMap == None) 
    {
    vtkDebugMacro(<<"vtkXImageWindow::MakeDefaultWindow - No colormap!");
    }
  
  if (attributes.map_installed == False) 
    {
    vtkDebugMacro(<<"vtkXImageWindow::MakeDefaultWindow - Colormap not installed!");
    }
  
  // Get the visual class
  templ.visualid = this->VisualId->visualid;
  visuals = XGetVisualInfo(this->DisplayId, VisualIDMask,
                           &templ, &nvisuals);
  if (nvisuals == 0)
    {
    vtkErrorMacro(<< "Could not get visual class");
    }
  this->VisualClass = visuals->c_class;

  XFree(visuals);

  // Make sure the color map is set up properly.
  if (this->VisualClass == DirectColor)
    {
    vtkDebugMacro(<< "vtkXImageWindow::MakeDefaultWindow - Allocating direct color map");
    this->AllocateDirectColorMap();
    }

  this->SetBackgroundColor (0.0, 0.0, 0.0);
  this->Mapped = 1;


// ####
 
  return;
}


//----------------------------------------------------------------------------
void vtkXImageWindow::GetDefaultVisualInfo(XVisualInfo *info) 
{
  int screen;
  XVisualInfo templ;
  XVisualInfo *visuals, *v;
  XVisualInfo *best = NULL;
  int nvisuals;
  int i, rate, bestRate = 100;
  
  screen = DefaultScreen(this->DisplayId);  
  templ.screen = screen;

  // Get a list of all the possible visuals for this screen.
  visuals = XGetVisualInfo(this->DisplayId,
			   VisualScreenMask,
			   &templ, &nvisuals);
  
  if (nvisuals == 0)
    {
    vtkErrorMacro(<< "Could not get a visual");
    }
  
  for (v = visuals, i = 0; i < nvisuals; v++, i++)
    {
    // We only handle three types of visuals now.
    // Rate the visual
    if (v->depth == 24 && v->c_class == TrueColor)
      {
      rate = 1;
      }
    else if (v->depth == 32 && v->c_class == TrueColor)
      {
      rate = 2;
      }
    else if (v->depth == 24 && v->c_class == DirectColor)
      {
      rate = 3;
      }
    else if (v->depth == 16 && v->c_class == TrueColor)
      {
      rate = 4;
      }
    else if (v->depth == 8 && v->c_class == PseudoColor)
      {
      rate = 5;
      }
    else
      {
      rate = 50;
      }
    
    if (rate < bestRate)
      {
      bestRate = rate;
      best = v;
      }
    }

  if (bestRate >= 50)
    {
    vtkWarningMacro("Could not find a visual I like");
    }
 
  // Copy visual
  *info = *best;
  
  XFree(visuals);
}

int vtkXImageWindow::GetDesiredDepth()
{
  XVisualInfo v;

  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  return v.depth;  
}

// Get a visual from the windowing system.
Visual *vtkXImageWindow::GetDesiredVisual ()
{
  XVisualInfo v;

  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  return v.visual;  
}


// Get a colormap from the windowing system.
Colormap vtkXImageWindow::GetDesiredColormap ()
{
  XVisualInfo v;

  if (this->ColorMap) return this->ColorMap;
  
  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  if (v.depth == 8)
    {
    this->ColorMap = this->MakeColorMap(v.visual);
    }
  else
    {
    this->ColorMap = 
      XCreateColormap(this->DisplayId, RootWindow(this->DisplayId, v.screen),
		      v.visual, AllocNone);
    }
  
  return this->ColorMap;  
}


void vtkXImageWindow::SetWindowId(void *arg)
{
  this->SetWindowId((Window)arg);
}

void vtkXImageWindow::SetWindowId(Window arg)
{
  this->WindowId = arg;
}

// Set the X display id for this ImageXWindow to use to a pre-existing 
// X display id.
void vtkXImageWindow::SetDisplayId(Display  *arg)
{
  this->DisplayId = arg;
  this->OwnDisplay = 0;
}
void vtkXImageWindow::SetDisplayId(void *arg)
{
  this->SetDisplayId((Display *)arg);
  this->OwnDisplay = 0;
}



Display *vtkXImageWindow::GetDisplayId()
{
  return(this->DisplayId);
}
GC vtkXImageWindow::GetGC()
{
  return(this->Gc);
}

Colormap vtkXImageWindow::MakeColorMap(Visual *visual) 
{
  int idx;
  int value;
  unsigned long planeMask, pval[256];
  int screen;
  Colormap  defaultMap, newMap;
  XColor    defccells[256];
  
  this->Offset = 50;

  screen = DefaultScreen(this->DisplayId);
  defaultMap = DefaultColormap(this->DisplayId, screen);
  // allways use a private colormap
    
  newMap = XCreateColormap(this->DisplayId, 
			   RootWindow(this->DisplayId, screen),
			   visual, AllocNone);
  if (! XAllocColorCells(this->DisplayId, newMap, 1, &planeMask, 0, pval,
			 (unsigned int)this->NumberOfColors+this->Offset))
    {
      vtkErrorMacro(<< "Sorry cann't allocate any more Colors");
      return (Colormap)(NULL);
    }
  
  for ( idx = 0 ; idx < this->Offset; idx++) 
    {
      defccells[idx].pixel = idx; 
    }
  XQueryColors(this->DisplayId, defaultMap, defccells, this->Offset);
  
  for (idx = 0 ; idx < this->Offset+this->NumberOfColors; idx++)
    {
      // Value should range between ? and ?
      value = (int)(65000.0 * (float)(idx - this->Offset) / (float)(this->NumberOfColors-1));
      
      if ( (idx < this->Offset)) 
	{
	  this->Colors[idx].pixel = defccells[idx].pixel;
	  this->Colors[idx].red   = defccells[idx].red ;
	  this->Colors[idx].green = defccells[idx].green ;
	  this->Colors[idx].blue  = defccells[idx].blue ;
	  this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
	  XStoreColor(this->DisplayId, newMap, &(this->Colors[idx]));
	}
      else 
	{
	  this->Colors[idx].pixel = idx;
	  this->Colors[idx].red   = value ;
	  this->Colors[idx].green = value ; 
	  this->Colors[idx].blue  = value ;
	  this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
	  XStoreColor(this->DisplayId, newMap, &(this->Colors[idx]));
	}
    }
  XInstallColormap(this->DisplayId, newMap);
  return newMap;
}



//----------------------------------------------------------------------------
void vtkXImageWindow::AllocateDirectColorMap() 
{
  int idx;
  int value;
  unsigned long planeMask, pval[256];
  Colormap newMap;
  
//  vtkDebugMacro(<< "vtkXImageWindow::AllocateDirectColorMap\n"); 
  if ( !this->WindowId )
    {
    vtkErrorMacro ( << "Attempt to use NULL WindowId" );
    return;
    }

  this->Offset = 100;

  // Get the colors in the current color map.
  for ( idx = 0 ; idx < 256; idx++) 
    {
    this->Colors[idx].pixel = idx; 
    }
  XQueryColors(this->DisplayId, this->ColorMap, this->Colors, 256);
    
  
  newMap = XCreateColormap(this->DisplayId, this->WindowId,
			   this->VisualId, AllocNone);
  if (! XAllocColorCells(this->DisplayId, newMap, 1, &planeMask, 0, pval,
			 (unsigned int)256))
    {
    vtkErrorMacro(<< "Sorry cann't allocate any more Colors");
    return;
    }
  
  // Set up the colors
  for (idx = 0; idx < 100; ++idx)
    {
    this->Colors[idx].pixel = pval[idx];
    this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
    XStoreColor(this->DisplayId, newMap, &(this->Colors[idx]));
    }
  for (idx = 0 ; idx < this->NumberOfColors; ++idx)
    {
    // Value should range between 0 and 65000
    value = 1000 + (int)(60000.0 * (float)(idx)/(float)(this->NumberOfColors));
    this->Colors[idx+100].pixel = pval[idx];
    this->Colors[idx+100].red   = value ;
    this->Colors[idx+100].green = value ; 
    this->Colors[idx+100].blue  = value ;
    this->Colors[idx+100].flags = DoRed | DoGreen | DoBlue ;
    XStoreColor(this->DisplayId, newMap, &(this->Colors[idx+100]));
    }
  XInstallColormap(this->DisplayId, newMap);
  this->ColorMap = newMap;
  XSetWindowColormap(this->DisplayId, this->WindowId, this->ColorMap);
}
















