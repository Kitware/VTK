/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageXViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageXViewer.h"

//----------------------------------------------------------------------------
vtkImageXViewer::vtkImageXViewer()
{
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)(NULL);
  this->ParentId = (Window)(NULL);
  this->NumberOfColors = 150;
  this->ColorMap = (Colormap)0;
}


//----------------------------------------------------------------------------
vtkImageXViewer::~vtkImageXViewer()
{
}


//----------------------------------------------------------------------------
void vtkImageXViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageViewer::PrintSelf(os, indent);
}

// Description:
// Get this RenderWindow's X window id.
Window vtkImageXViewer::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << (void *)this->WindowId << "\n");
  return this->WindowId;
}

// Description:
// Get this RenderWindow's parent X window id.
Window vtkImageXViewer::GetParentId()
{
  vtkDebugMacro(<< "Returning ParentId of " << (void *)this->ParentId << "\n");
  return this->ParentId;
}

// Description:
// Sets the parent of the window that WILL BE created.
void vtkImageXViewer::SetParentId(Window arg)
{
  if (this->ParentId)
    {
    vtkErrorMacro("ParentId is already set.");
    return;
    }
  
  vtkDebugMacro(<< "Setting ParentId to " << (void *)arg << "\n"); 

  this->ParentId = arg;
}

// Description:
// Get the position in screen coordinates (pixels) of the window.
int *vtkImageXViewer::GetPosition(void)
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
// Move the window to a new position on the display.
void vtkImageXViewer::SetPosition(int x, int y)
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

  XMoveResizeWindow(this->DisplayId,this->WindowId,x,y,
                    this->Size[0], this->Size[1]);
  XSync(this->DisplayId,False);
}

//----------------------------------------------------------------------------
// Description:
// A templated function that handles gray scale images.
template <class T>
static void vtkImageXViewerRenderGrey(vtkImageXViewer *self, 
				      vtkImageRegion *region,
				      T *inPtr, unsigned char *outPtr)
{
  int colorIdx;
  T *inPtr0, *inPtr1;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx0, idx1;
  float shift, scale;
  XColor *colors;
  int colorsMax;
  int visualDepth, visualClass;
  
  
  visualClass = self->GetVisualClass();
  if (visualClass == TrueColor)
    {
    colorsMax = 255;
    }
  else
    {
    colorsMax = self->GetNumberOfColors() - 1;
    }
  
  colors = self->GetColors();
  shift = self->GetColorShift();
  scale = self->GetColorScale();
  visualDepth = self->GetVisualDepth();
  region->GetExtent(inMin0, inMax0, inMin1, inMax1);
  region->GetIncrements(inInc0, inInc1);
  
  // Loop through in regions pixels
  inPtr1 = inPtr;
  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    inPtr0 = inPtr1;
    for (idx0 = inMin0; idx0 <= inMax0; idx0++)
      {

      colorIdx = (int)(((float)(*inPtr0) + shift) * scale);
      if (colorIdx < 0)
	{
	colorIdx = 0;
	}
      if (colorIdx > colorsMax)
	{
	colorIdx = colorsMax;
	}

      if (visualClass == TrueColor)
	{
	*outPtr++ = (unsigned char)(255);
	*outPtr++ = (unsigned char)(colorIdx);
	*outPtr++ = (unsigned char)(colorIdx);
	*outPtr++ = (unsigned char)(colorIdx);
	}
      else if (visualClass == DirectColor)
	{
	*outPtr++ = (unsigned char)(colors[colorIdx].pixel);
	*outPtr++ = (unsigned char)(colors[colorIdx].pixel);
	*outPtr++ = (unsigned char)(colors[colorIdx].pixel);
	*outPtr++ = (unsigned char)(colors[colorIdx].pixel);
	}
      else if (visualClass == PseudoColor)
	{
	*outPtr++ = (unsigned char)(colors[colorIdx].pixel);
	}
      

      inPtr0 += inInc0;
      }
    inPtr1 += inInc1;
    }
}


//----------------------------------------------------------------------------
// Description:
// A templated function that handles color images. (only True Color 24 bit)
template <class T>
static void vtkImageXViewerRenderColor(vtkImageXViewer *self, vtkImageRegion *region,
				T *redPtr, T *greenPtr, T *bluePtr,
				unsigned char *outPtr)
{
  int red, green, blue;
  T *redPtr0, *redPtr1;
  T *bluePtr0, *bluePtr1;
  T *greenPtr0, *greenPtr1;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx0, idx1;
  float shift, scale;
  
  
  shift = self->GetColorShift();
  scale = self->GetColorScale();
  region->GetExtent(inMin0, inMax0, inMin1, inMax1);
  region->GetIncrements(inInc0, inInc1);
  
  // Loop through in regions pixels
  redPtr1 = redPtr;
  greenPtr1 = greenPtr;
  bluePtr1 = bluePtr;
  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    redPtr0 = redPtr1;
    greenPtr0 = greenPtr1;
    bluePtr0 = bluePtr1;
    for (idx0 = inMin0; idx0 <= inMax0; idx0++)
      {

      red = (int)(((float)(*redPtr0) + shift) * scale);
      if (red < 0) red = 0;
      if (red > 255) red = 255;
      green = (int)(((float)(*greenPtr0) + shift) * scale);
      if (green < 0) green = 0;
      if (green > 255) green = 255;
      blue = (int)(((float)(*bluePtr0) + shift) * scale);
      if (blue < 0) blue = 0;
      if (blue > 255) blue = 255;

      *outPtr++ = (unsigned char)(255);
      *outPtr++ = (unsigned char)(blue);
      *outPtr++ = (unsigned char)(green);
      *outPtr++ = (unsigned char)(red);

      redPtr0 += inInc0;
      greenPtr0 += inInc0;
      bluePtr0 += inInc0;
      }
    redPtr1 += inInc1;
    greenPtr1 += inInc1;
    bluePtr1 += inInc1;
    }
}

//----------------------------------------------------------------------------
// Maybe we should cache the dataOut! (MTime)
void vtkImageXViewer::Render(void)
{
  int extent[8];
  int *imageExtent;
  int width, height;
  int size;
  unsigned char *dataOut;
  vtkImageRegion *region;
  void *ptr0, *ptr1, *ptr2;
  
  if ( ! this->Input)
    {
    // open the window anyhow if not yet open
    // use default size if not specified
    if (!this->WindowId)
      {
      if (this->Size[0] == 0 )
	{
	this->Size[0] = width;
	this->Size[1] = height;
	}
      if (this->Size[0] == 0 )
	{
	this->Size[0] = 256;
	this->Size[1] = 256;
	}
      this->SetWindow(this->MakeDefaultWindow(this->Size[0],this->Size[1]));
      }
    vtkDebugMacro(<< "Render: Please Set the input.");
    return;
    }

  this->Input->UpdateImageInformation(&(this->Region));
  imageExtent = this->Region.GetImageExtent();
  
  // determine the Extent of the 2D input region needed
  if (this->WholeImage)
    {
    this->Region.GetImageExtent(2, extent);
    }
  else
    {
    this->Region.GetExtent(2, extent);
    }
  
  if (this->ColorFlag)
    {
    extent[4] = extent[5] = this->Red;
    if (this->Green < extent[4]) extent[4] = this->Green;
    if (this->Green > extent[5]) extent[5] = this->Green;
    if (this->Blue < extent[4]) extent[4] = this->Blue;
    if (this->Blue > extent[5]) extent[5] = this->Blue;
    }
  else
    {
    // Make sure the requested  image is in the range.
    if (this->Coordinate2 < imageExtent[4])
      {
      extent[4] = extent[5] = imageExtent[4];
      }
    else if (this->Coordinate2 > imageExtent[5])
      {
      extent[4] = extent[5] = imageExtent[5];
      }
    else
      {
      extent[4] = extent[5] = this->Coordinate2;
      }
    }

  // Make sure the requested  image is in the range.
  if (this->Coordinate3 < imageExtent[6])
    {
    extent[6] = extent[6] = imageExtent[6];
    }
  else if (this->Coordinate3 > imageExtent[7])
    {
    extent[6] = extent[7] = imageExtent[7];
    }
  else
    {
    extent[6] = extent[7] = this->Coordinate3;
    }
  
  // Get the region form the input
  region = new vtkImageRegion;
  region->SetAxes(this->Region.GetAxes());
  region->SetExtent(4, extent);
  this->Input->UpdateRegion(region);
  if ( ! region->AreScalarsAllocated())
    {
    vtkErrorMacro(<< "View: Could not get region from input.");
    region->Delete();
    return;
    }

  // allocate the display data array.
  width = (extent[1] - extent[0] + 1);
  height = (extent[3] - extent[2] + 1);

  // In case a window has not been set.
  if ( ! this->WindowId)
    {
    // use default size if not specified
    if (this->Size[0] == 0 )
      {
      this->Size[0] = width;
      this->Size[1] = height;
      }
    this->SetWindow(this->MakeDefaultWindow(this->Size[0],this->Size[1]));
    }
  
  // Allocate output data
  size = width * height;
  if (this->VisualDepth == 24)
    {
    size *= 4;
    }
  dataOut = new unsigned char[size];

  if (this->ColorFlag)
    {
    // Handle color display
    // We only support color with 24 bit True Color Visuals
    if (this->VisualDepth != 24 || this->VisualClass != TrueColor)
      {
      vtkErrorMacro(<< "Color is only supported with 24 bit True Color");
      return;
      }
    ptr0 = region->GetScalarPointer(extent[0], extent[2], this->Red);
    ptr1 = region->GetScalarPointer(extent[0], extent[2], this->Green);
    ptr2 = region->GetScalarPointer(extent[0], extent[2], this->Blue);
    // Call the appropriate templated function
    switch (region->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageXViewerRenderColor(this, region, 
			   (float *)(ptr0),(float *)(ptr1),(float *)(ptr2), 
			   dataOut);
	break;
      case VTK_INT:
	vtkImageXViewerRenderColor(this, region, 
			   (int *)(ptr0), (int *)(ptr1), (int *)(ptr2), 
			   dataOut);
	break;
      case VTK_SHORT:
	vtkImageXViewerRenderColor(this, region, 
			   (short *)(ptr0),(short *)(ptr1),(short *)(ptr2), 
			   dataOut);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageXViewerRenderColor(this, region, (unsigned short *)(ptr0),
			   (unsigned short *)(ptr1),(unsigned short *)(ptr2), 
			    dataOut);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageXViewerRenderColor(this, region, (unsigned char *)(ptr0), 
			   (unsigned char *)(ptr1),(unsigned char *)(ptr2), 
			    dataOut);
	break;
      }
    }
  else
    {
    // GreyScale images.
    ptr0 = region->GetScalarPointer();
    // Call the appropriate templated function
    switch (region->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageXViewerRenderGrey(this, region, (float *)(ptr0), dataOut);
	break;
      case VTK_INT:
	vtkImageXViewerRenderGrey(this, region, (int *)(ptr0), dataOut);
	break;
      case VTK_SHORT:
	vtkImageXViewerRenderGrey(this, region, (short *)(ptr0), dataOut);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageXViewerRenderGrey(this, region, (unsigned short *)(ptr0), 
				  dataOut);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageXViewerRenderGrey(this, region, (unsigned char *)(ptr0), 
				  dataOut);
	break;
      }   
    }
  
  // Display the image.
  this->Image = XCreateImage(this->DisplayId, this->VisualId,this->VisualDepth,
			     ZPixmap, 0, (char *)dataOut, width, height, 8,0);
  XPutImage(this->DisplayId, this->WindowId, this->Gc, this->Image, 0, 0,
	    this->XOffset, this->YOffset, width, height);

  XFlush(this->DisplayId);
  XSync(this->DisplayId, False);
  
  delete dataOut;	 
  XFree(this->Image);
  region->Delete();
}





//----------------------------------------------------------------------------
// Support for the templated function.
XColor *vtkImageXViewer::GetColors()
{
  return this->Colors + this->Offset;
}

//----------------------------------------------------------------------------
Window vtkImageXViewer::MakeDefaultWindow(int width, int height) 
{
  char name[80];
  int screen;
  XVisualInfo info;
  XSetWindowAttributes values;
  Window window;
  
  
  strcpy(name,"XViewer");

  // make sure we have a connection to the X server.
  if ( ! this->DisplayId)
    {
    if ( ( this->DisplayId = XOpenDisplay((char *)NULL)) == NULL) 
      {
      cerr <<"cannot connect to X server"<< XDisplayName((char *)NULL)<< endl;
      exit(-1);
      }
    }
  
  
  screen = DefaultScreen(this->DisplayId);
  this->GetDefaultVisualInfo(&info);
  
  // Create a window 
  // If this is a pseudocolor visual, create a color map.
  values.colormap = this->GetDesiredColormap();
  
  values.background_pixel = BlackPixel(this->DisplayId, screen);
  values.border_pixel = None;
  values.event_mask = 0;
  values.override_redirect = False;
  //  if ((w > 0) && (x >= 0) && (!borders))
  //  values.override_redirect = True;
  XFlush(this->DisplayId);

  // get a default parent if one has not been set.
  if (! this->ParentId)
    {
    this->ParentId = RootWindow(this->DisplayId, screen);
    }

  window = XCreateWindow(this->DisplayId, this->ParentId,
			 0, 0, width, height, 0, info.depth, 
			 InputOutput, info.visual,
			 CWEventMask | CWBackPixel | CWBorderPixel | 
			 CWColormap | CWOverrideRedirect, 
			 &values);
  XSetStandardProperties(this->DisplayId, window, name, name, None, 0, 0, 0);
  XSync(this->DisplayId, False);
  
  // Select event types wanted 
  XSelectInput(this->DisplayId, window,
	       ExposureMask | KeyPressMask | ButtonPressMask |
	       PointerMotionMask | StructureNotifyMask | PropertyChangeMask);
  
  // Map Window onto Screen and sysc
  XMapWindow(this->DisplayId, window);
  
  XSync(this->DisplayId,0);
  
  return window;
}


//----------------------------------------------------------------------------
void vtkImageXViewer::GetDefaultVisualInfo(XVisualInfo *info) 
{
  int screen;
  XVisualInfo templ;
  XVisualInfo *visuals, *v;
  XVisualInfo *best = NULL;
  int nvisuals;
  int i, rate, bestRate = 100;
  
  screen = DefaultScreen(this->DisplayId);  
  templ.screen = screen;
  //templ.depth = 24;
  //templ.c_class = DirectColor;

  // Get a list of all the possible visuals for this screen.
  visuals = XGetVisualInfo(this->DisplayId,
			   // VisualScreenMask | VisualClassMask,
			   VisualScreenMask,
			   &templ, &nvisuals);
  
  if (nvisuals == 0)
    {
    vtkErrorMacro(<< "Could not get a visual");
    }
  
  for (v = visuals, i = 0; i < nvisuals; v++, i++)
    {
    // which are available
    if (this->Debug)
      {
      if (v->c_class == TrueColor)
	vtkDebugMacro(<< "Available: " << v->depth << " bit TrueColor");
      if (v->c_class == DirectColor)
	vtkDebugMacro(<< "Available: " << v->depth << " bit DirectColor");
      if (v->c_class == PseudoColor)
	vtkDebugMacro(<< "Available: " << v->depth << " bit PseudoColor");
      }
  
    // We only handle three types of visuals now.
    // Rate the visual
    if (v->depth == 24 && v->c_class == TrueColor)
      {
      rate = 1;
      }
    else if (v->depth == 24 && v->c_class == DirectColor)
      {
      rate = 2;
      }
    else if (v->depth == 8 && v->c_class == PseudoColor)
      {
      rate = 3;
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
  
  if (this->Debug)
    {
    if (best->c_class == TrueColor)
      vtkDebugMacro(<< "Chose: " << best->depth << " bit TrueColor");
    if (best->c_class == DirectColor)
      vtkDebugMacro(<< "Chose: " << best->depth << " bit DirectColor");
    if (best->c_class == PseudoColor)
      vtkDebugMacro(<< "Chose: " << best->depth << " bit PseudoColor");
    }
  
  // Copy visual
  *info = *best;
  
  XFree(visuals);
}

int vtkImageXViewer::GetDesiredDepth()
{
  XVisualInfo v;

  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  return v.depth;  
}

// Description:
// Get a visual from the windowing system.
Visual *vtkImageXViewer::GetDesiredVisual ()
{
  XVisualInfo v;

  // get the default visual to use 
  this->GetDefaultVisualInfo(&v);

  return v.visual;  
}


// Description:
// Get a colormap from the windowing system.
Colormap vtkImageXViewer::GetDesiredColormap ()
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

//----------------------------------------------------------------------------
int vtkImageXViewer::GetWindow() 
{
  int extent[8];
  int width, height;
  
  if ( this->WindowId)
    {
    return (int)(this->WindowId);
    }
  
  // In case a window has not been set.
  if ( ! this->Input)
    {
    return (int)(NULL);
    }

  // determine the Extent of the input region to be displayed
  if (this->WholeImage)
    {
    this->Input->UpdateImageInformation(&(this->Region));
    this->Region.GetImageExtent(2, extent);
    }
  else
    {
    this->Region.GetExtent(2, extent);
    }
  
  // allocate the display data array.
  width = (extent[1] - extent[0] + 1);
  height = (extent[3] - extent[2] + 1);

  this->SetWindow(this->MakeDefaultWindow(width + this->XOffset,
					  height + this->YOffset));
  this->Modified();

  return (int)(this->WindowId);
}

  


//----------------------------------------------------------------------------
// Description:
// An arbitrary window can be used for the window.
void vtkImageXViewer::SetWindow(int win) 
{
  XVisualInfo templ;
  XVisualInfo *visuals;
  int nvisuals;
  XWindowAttributes attributes;
  
  this->WindowId = (Window)(win);
  
  // Now we must get the right visual, Gc, and DisplayId ...
  if ( ! this->DisplayId)
    {
    if ((this->DisplayId = XOpenDisplay((char *)NULL)) == NULL) 
      {
      cerr <<"cannot connect to X server"<< XDisplayName((char *)NULL)<< endl;
      exit(-1);
      }
    }
  
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
  
  // Get the visual class
  templ.visualid = this->VisualId->visualid;
  visuals = XGetVisualInfo(this->DisplayId,
			   VisualIDMask,
			   &templ, &nvisuals);  
  if (nvisuals == 0)
    {
    vtkErrorMacro(<< "Could not get visual class");
    }
  this->VisualClass = visuals->c_class;
  if (this->Debug)
    {
    if (this->VisualClass == TrueColor)
      vtkDebugMacro(<< "Window: " << this->VisualDepth << " bit TrueColor");
    if (this->VisualClass == DirectColor)
      vtkDebugMacro(<< "Window: " << this->VisualDepth << " bit DirectColor");
    if (this->VisualClass == PseudoColor)
      vtkDebugMacro(<< "Window: " << this->VisualDepth << " bit PseudoColor");
    }
  
  // Make sure the color map is set up properly.
  if (this->VisualClass == DirectColor)
    {
    this->AllocateDirectColorMap();
    }
}


//----------------------------------------------------------------------------
// Description:
// Set this ImageXViewer's X window id to a pre-existing window.
void vtkImageXViewer::SetWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << (void *)arg << "\n"); 

  this->SetWindow((int)(arg));
}
void vtkImageXViewer::SetWindowId(void *arg)
{
  this->SetWindowId((Window)arg);
}

// Description:
// Set the X display id for this ImageXViewer to use to a pre-existing 
// X display id.
void vtkImageXViewer::SetDisplayId(Display  *arg)
{
  vtkDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
}
void vtkImageXViewer::SetDisplayId(void *arg)
{
  this->SetDisplayId((Display *)arg);
}
Display *vtkImageXViewer::GetDisplayId()
{
  return(this->DisplayId);
}
GC vtkImageXViewer::GetGC()
{
  return(this->Gc);
}
//----------------------------------------------------------------------------
Colormap vtkImageXViewer::MakeColorMap(Visual *visual) 
{
  int idx;
  int value;
  unsigned long planeMask, pval[256];
  int screen;
  Colormap  defaultMap, newMap;
  XColor    defccells[256];
  
  this->Offset = 0;

  screen = DefaultScreen(this->DisplayId);
  defaultMap = DefaultColormap(this->DisplayId, screen);
  
  if ( !XAllocColorCells(this->DisplayId, defaultMap, 0, &planeMask, 0, 
			 pval, (unsigned int) this->NumberOfColors))
    {
    // can't allocate NUM_COLORS from Def ColorMap
    // create new ColorMap ... but first cp some def ColorMap
    
    newMap = XCreateColormap(this->DisplayId, 
			     RootWindow(this->DisplayId, screen),
			     visual, AllocNone);
    this->Offset = 100;
    if (! XAllocColorCells(this->DisplayId, newMap, 1, &planeMask, 0, pval,
			   (unsigned int)256))
      {
      vtkErrorMacro(<< "Sorry cann't allocate any more Colors");
      return (Colormap)(NULL);
      }
    
    for ( idx = 0 ; idx < 256; idx++) 
      {
      defccells[idx].pixel = idx; 
      }
    XQueryColors(this->DisplayId, defaultMap, defccells, 256);
    
    for (idx = 0 ; idx < 256; idx++)
      {
      // Value should range between ? and ?
      value = 1000 + (int)(60000.0 * (float)(idx - this->Offset) / (float)(this->NumberOfColors));
    
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
	this->Colors[idx].pixel = pval[idx];
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
  else
    {
    for (idx = 0 ; idx < this->NumberOfColors ; idx++)
      {
      if (idx) 
	{
	value = (((192 * idx)/(this->NumberOfColors -1)) << 8)  + 16000;
	}
      else 
	{
	value = 0;
	}
      this->Colors[idx].pixel = pval[idx];
      this->Colors[idx].red   = value ;
      this->Colors[idx].green = value ;
      this->Colors[idx].blue  = value ;
      this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
      XStoreColor(this->DisplayId, defaultMap, &(this->Colors[idx]));
      }

    return defaultMap;
    } 
}



//----------------------------------------------------------------------------
void vtkImageXViewer::AllocateDirectColorMap() 
{
  int idx;
  int value;
  unsigned long planeMask, pval[256];
  Colormap newMap;
  
  this->Offset = 100;
  vtkDebugMacro(<< "AllocateDirectColorMap: " << this->NumberOfColors 
                << " colors");

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


//----------------------------------------------------------------------------
// Support for the templated function.
float vtkImageXViewer::GetColorShift()
{
  return this->ColorWindow / 2.0 - this->ColorLevel;
}

//----------------------------------------------------------------------------
// Support for the templated function.
float vtkImageXViewer::GetColorScale()
{
  return (float)(this->NumberOfColors - 1) / this->ColorWindow;
}

















