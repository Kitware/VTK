/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageXViewer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
  this->ColorWindow = 255.0;
  this->ColorLevel = 127.0;
  this->NumberColors = 150;
  this->ColorFlag = 0;
  this->Red = 0;
  this->Green = 0;
  this->Blue = 0;
  
  this->WindowId = (Window)(NULL);
}


//----------------------------------------------------------------------------
vtkImageXViewer::~vtkImageXViewer()
{
}

//----------------------------------------------------------------------------
// Description:
// Set the input to the viewer.
// Set the default region to display as the whole image.  The input should
// have already been connected to its final source to get this information,
// otherwise an error will occur.
void vtkImageXViewer::SetInput(vtkImageSource *input)
{
  int bounds[8];
  
  vtkDebugMacro(<< "SetInput: (" << input << ")");
  this->Modified();
  this->Input = input;

  // Get the default region to display
  input->UpdateImageInformation(&(this->Region));
  this->Region.GetImageBounds4d(bounds);
  this->SetBounds(bounds);
  this->SetDefaultCoordinate2(bounds[4]);
  this->SetDefaultCoordinate3(bounds[6]);
}


//----------------------------------------------------------------------------
// Description:
// A templated function that handles gray scale images.
template <class T>
void vtkImageXViewerRenderGrey(vtkImageXViewer *self, vtkImageRegion *region,
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
  int visualDepth;
  
  
  colorsMax = self->GetNumberColors() - 1;
  colors = self->GetColors();
  shift = self->GetColorShift();
  scale = self->GetColorScale();
  visualDepth = self->GetVisualDepth();
  region->GetBounds2d(inMin0, inMax0, inMin1, inMax1);
  region->GetIncrements2d(inInc0, inInc1);
  
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

      if (visualDepth == 8)
	{
	*outPtr++ = (unsigned char)(colors[colorIdx].pixel);
	}
      else
	{
	*outPtr++ = (unsigned char)(255);
	*outPtr++ = (unsigned char)(colorIdx);
	*outPtr++ = (unsigned char)(colorIdx);
	*outPtr++ = (unsigned char)(colorIdx);
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
void vtkImageXViewerRenderColor(vtkImageXViewer *self, vtkImageRegion *region,
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
  region->GetBounds2d(inMin0, inMax0, inMin1, inMax1);
  region->GetIncrements2d(inInc0, inInc1);
  
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
  int bounds[8];
  int width, height;
  int size;
  unsigned char *dataOut;
  vtkImageRegion *region;
  void *ptr0, *ptr1, *ptr2;
  
  // determine the Bounds of the input region needed
  this->Region.GetBounds4d(bounds);
  if (this->ColorFlag)
    {
    bounds[4] = bounds[5] = this->Red;
    if (this->Green < bounds[4]) bounds[4] = this->Green;
    if (this->Green > bounds[5]) bounds[5] = this->Green;
    if (this->Blue < bounds[4]) bounds[4] = this->Blue;
    if (this->Blue > bounds[5]) bounds[5] = this->Blue;
    }
  else
    {
    bounds[4] = bounds[5] = this->Region.GetDefaultCoordinate2();
    }
  bounds[6] = bounds[7] = this->Region.GetDefaultCoordinate3();

  // Get the region form the input
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "View: Please Set the input.");
    return;
    }
  region = new vtkImageRegion;
  region->SetAxes(this->Region.GetAxes());
  region->SetBounds4d(bounds);
  this->Input->UpdateRegion(region);
  if ( ! region->IsAllocated())
    {
    vtkErrorMacro(<< "View: Could not get region from input.");
    region->Delete();
    return;
    }

  // allocate the display data array.
  width = (bounds[1] - bounds[0] + 1);
  height = (bounds[3] - bounds[2] + 1);

  // In case a window has not been set.
  if ( ! this->WindowId)
    {
    this->SetWindow(this->MakeDefaultWindow(width, height));
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
    if (this->VisualDepth != 24)
      {
      vtkErrorMacro(<< "Color is only supported with 24 bit True Color");
      return;
      }
    ptr0 = region->GetVoidPointer3d(bounds[0], bounds[2], this->Red);
    ptr1 = region->GetVoidPointer3d(bounds[0], bounds[2], this->Green);
    ptr2 = region->GetVoidPointer3d(bounds[0], bounds[2], this->Blue);
    // Call the appropriate templated function
    switch (region->GetDataType())
      {
      case VTK_IMAGE_FLOAT:
	vtkImageXViewerRenderColor(this, region, 
			   (float *)(ptr0),(float *)(ptr1),(float *)(ptr2), 
			   dataOut);
	break;
      case VTK_IMAGE_INT:
	vtkImageXViewerRenderColor(this, region, 
			   (int *)(ptr0), (int *)(ptr1), (int *)(ptr2), 
			   dataOut);
	break;
      case VTK_IMAGE_SHORT:
	vtkImageXViewerRenderColor(this, region, 
			   (short *)(ptr0),(short *)(ptr1),(short *)(ptr2), 
			   dataOut);
	break;
      case VTK_IMAGE_UNSIGNED_SHORT:
	vtkImageXViewerRenderColor(this, region, (unsigned short *)(ptr0),
			   (unsigned short *)(ptr1),(unsigned short *)(ptr2), 
			    dataOut);
	break;
      case VTK_IMAGE_UNSIGNED_CHAR:
	vtkImageXViewerRenderColor(this, region, (unsigned char *)(ptr0), 
			   (unsigned char *)(ptr1),(unsigned char *)(ptr2), 
			    dataOut);
	break;
      }
    }
  else
    {
    // GreyScale images.
    ptr0 = region->GetVoidPointer2d();
    // Call the appropriate templated function
    switch (region->GetDataType())
      {
      case VTK_IMAGE_FLOAT:
	vtkImageXViewerRenderGrey(this, region, (float *)(ptr0), dataOut);
	break;
      case VTK_IMAGE_INT:
	vtkImageXViewerRenderGrey(this, region, (int *)(ptr0), dataOut);
	break;
      case VTK_IMAGE_SHORT:
	vtkImageXViewerRenderGrey(this, region, (short *)(ptr0), dataOut);
	break;
      case VTK_IMAGE_UNSIGNED_SHORT:
	vtkImageXViewerRenderGrey(this, region, (unsigned short *)(ptr0), 
				  dataOut);
	break;
      case VTK_IMAGE_UNSIGNED_CHAR:
	vtkImageXViewerRenderGrey(this, region, (unsigned char *)(ptr0), 
				  dataOut);
	break;
      }   
    }
  
  // Display the image.
  this->Image = XCreateImage(this->DisplayId, this->VisualId,this->VisualDepth,
			     ZPixmap, 0, (char *)dataOut, width, height, 8,0);
  XPutImage(this->DisplayId, this->WindowId, this->Gc, this->Image, 0, 0,
	    0, 0, width, height);

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
// Support for the templated function.
float vtkImageXViewer::GetColorShift()
{
  return this->ColorWindow / 2.0 - this->ColorLevel;
}

//----------------------------------------------------------------------------
// Support for the templated function.
float vtkImageXViewer::GetColorScale()
{
  return (float)(this->NumberColors - 1) / this->ColorWindow;
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
  if (info.depth == 8)
    {
    values.colormap = this->MakeColorMap(info.visual);
    }
  else
    {
    values.colormap = 
      XCreateColormap(this->DisplayId, RootWindow(this->DisplayId, screen),
		      info.visual, AllocNone);
    }
  values.background_pixel = BlackPixel(this->DisplayId, screen);
  values.border_pixel = None;
  values.event_mask = 0;
  values.override_redirect = False;
  //  if ((w > 0) && (x >= 0) && (!borders))
  //  values.override_redirect = True;
  XFlush(this->DisplayId);
  window = XCreateWindow(this->DisplayId, RootWindow(this->DisplayId,screen),
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
  int i;
  
  screen = DefaultScreen(this->DisplayId);  
  templ.screen = screen;
  //templ.depth = 24;
  //templ.c_class = TrueColor;

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
    // set the defualt as the first visual encountered
    if (best == NULL)
      {
      best = v;
      }
    // deeper visuals are always better
    if (v->depth > best->depth)
      {
      best = v;
      }
    // true color is better than direct color which is beter than pseudo color
    if (v->c_class == TrueColor && v->depth == best->depth)
      {
      best = v;
      }
    else if (v->c_class == DirectColor && v->depth == best->depth)
      {
      best = v;
      }
    }

  if (this->Debug)
    {
    if (best->c_class == TrueColor)
      vtkDebugMacro(<< "DefaultVisual: " << best->depth << " bit TrueColor");
    if (best->c_class == DirectColor)
      vtkDebugMacro(<< "DefaultVisual: " << best->depth << " bit DirectColor");
    if (best->c_class == PseudoColor)
      vtkDebugMacro(<< "DefaultVisual: " << best->depth << " bit PseudoColor");
    }
  
  // Copy visual
  *info = *best;
  
  XFree(visuals);
}



//----------------------------------------------------------------------------
// Description:
// An arbitrary window can be used for the window.
void vtkImageXViewer::SetWindow(Window win) 
{
  XWindowAttributes attributes;
  
  this->WindowId = win;
  
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
			 pval, (unsigned int) this->NumberColors))
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
      value = 1000 + (int)(60000.0 * (float)(idx - this->Offset) / (float)(this->NumberColors));
    
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
    for (idx = 0 ; idx < this->NumberColors ; idx++)
      {
      if (idx) 
	{
	value = (((192 * idx)/(this->NumberColors -1)) << 8)  + 16000;
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



