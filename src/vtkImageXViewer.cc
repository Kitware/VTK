/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageXViewer.cc
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
#include "vtkImageXViewer.hh"

//----------------------------------------------------------------------------
vtkImageXViewer::vtkImageXViewer()
{
  this->WinInfo[0] = 512;
  this->WinInfo[1] = 512;
  this->WinInfo[2] = 0;
  this->WinInfo[3] = 0;
  this->ViewerOn      = 0;
  this->ColorWindow = 255.0;
  this->ColorLevel = 127.0;
  this->NumberColors = 150;
}

//----------------------------------------------------------------------------
vtkImageXViewer::~vtkImageXViewer()
{
}



//----------------------------------------------------------------------------
template <class T>
void vtkImageXViewerView(vtkImageXViewer *self, vtkImageRegion *region,
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
  
  colorsMax = self->GetNumberColors() - 1;
  colors = self->GetColors();
  shift = self->GetColorShift();
  scale = self->GetColorScale();
  region->GetBounds2d(inMin0, inMax0, inMin1, inMax1);
  region->GetIncrements2d(inInc0, inInc1);
  
  // Loop through in regions pixels
  inPtr1 = inPtr;
  for (idx0 = inMin0; idx0 <= inMax0; idx0++)
    {
    inPtr0 = inPtr1;
    for (idx1 = inMin1; idx1 <= inMax1; idx1++)
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
      *outPtr = (unsigned char)(colors[colorIdx].pixel);
      
      ++outPtr;
      inPtr0 += inInc0;
      }
    inPtr1 += inInc1;
    }
}


//----------------------------------------------------------------------------
// Maybe we should cache the dataOut! (MTime)
void vtkImageXViewer::View(void)
{
  int bounds[8];
  int width, height;
  int size;
  unsigned char *dataOut;
  vtkImageRegion *region;
  void *ptr;
  
  // Get the region form the input
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "View: Please Set the input.");
    return;
    }
  region = new vtkImageRegion;
  region->SetAxes(this->Region.GetAxes());
  this->Region.GetBounds4d(bounds);
  bounds[4] = bounds[5] = this->Region.GetDefaultCoordinate2();
  bounds[6] = bounds[7] = this->Region.GetDefaultCoordinate3();
  region->SetBounds4d(bounds);
  this->Input->UpdateRegion(region);
  if ( ! region->IsAllocated())
    {
    vtkErrorMacro(<< "View: Could not get region from input.");
    region->Delete();
    return;
    }

  // Initialize the window  
  this->InitializeWindow();
  
  // allocate the display data array.
  width = (bounds[1] - bounds[0] + 1);
  height = (bounds[3] - bounds[2] + 1);
  size = width * height;
  dataOut = new unsigned char[size];

  // Call the appropriate templated function
  ptr = region->GetVoidPointer2d();
  switch (region->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImageXViewerView(this, region, (float *)(ptr), dataOut);
      break;
    case VTK_IMAGE_INT:
      vtkImageXViewerView(this, region, (int *)(ptr), dataOut);
      break;
    case VTK_IMAGE_SHORT:
      vtkImageXViewerView(this, region, (short *)(ptr), dataOut);
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageXViewerView(this, region, (unsigned short *)(ptr), dataOut);
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageXViewerView(this, region, (unsigned char *)(ptr), dataOut);
      break;
    }   
    
  // Display the image.
  this->Image = XCreateImage(this->DisplayId, this->VisualInfo.visual, 8, 
			     ZPixmap, 0, (char *)dataOut, width, height, 8, 0);
  XPutImage(this->DisplayId, this->WindowId, this->Gc, this->Image, 0, 0,
	    this->WinInfo[2], this->WinInfo[3],  
	    width, height);

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
void vtkImageXViewer::InitializeWindow() 
{
  char windowName[80];
  char iconName[80];
  
  if (this->ViewerOn == 1) return;
  
  // looks like setting default size of the window
  if ( (this->WinInfo[0] == 0) || (this->WinInfo[1] == 0) )
    this->WinInfo[0] = this->WinInfo[1] = 512;
  
  strcpy(windowName,"Viewer");
  strcpy(iconName,"DIP");
  
  if ( ( this->DisplayId = XOpenDisplay((char *)NULL)) == NULL) 
    {
    cerr <<"cannot connect to X server"<< XDisplayName((char *)NULL)<< endl;
    exit(-1);
    }
  
  this->Screen = DefaultScreen(this->DisplayId);
  this->Gc     = DefaultGC(this->DisplayId,this->Screen) ;

  if (!XMatchVisualInfo(this->DisplayId,this->Screen,8,PseudoColor,
			&(this->VisualInfo)))
    {
    cerr <<  "cannot find PseudoColor visual" << endl;
    exit(-1);
    }
  
  this->ColorMap = DefaultColormap(this->DisplayId,this->Screen);
  this->Attributes.colormap = this->ColorMap;
  this->Attributes.background_pixel = BlackPixel(this->DisplayId,this->Screen);
  this->Attributes.border_pixel = None;
  this->Attributes.event_mask = 0;
  this->Attributes.backing_store = Always;
  
  // Create an opaque Window 
  this->WindowId = XCreateWindow(this->DisplayId, 
    RootWindow(this->DisplayId,this->Screen),
    0, 0, this->WinInfo[0], this->WinInfo[1], 4,
    this->VisualInfo.depth, InputOutput, this->VisualInfo.visual, 
    CWEventMask | CWBackPixel | CWBorderPixel | CWColormap | CWBackingStore,
    &(this->Attributes));
  
  // initilize size hint property for Window manager
  this->SizeHints.flags = PPosition | PSize | PMinSize;
  
  // set property of Window manager before "always before mapping"
  XSetStandardProperties(this->DisplayId, this->WindowId, windowName, iconName,
			 this->IconPixmap,(char **)NULL,1,&(this->SizeHints));
  
  // Create and Define a cursor... 
  MyStdCursor = XCreateFontCursor(this->DisplayId,XC_hand2);
  XDefineCursor(this->DisplayId,this->WindowId,MyStdCursor);
  
  // Select event types wanted 
  XSelectInput(this->DisplayId,this->WindowId,
	       ExposureMask | KeyPressMask | ButtonPressMask 
	       | PointerMotionMask | StructureNotifyMask | PropertyChangeMask);
  
  // Map Window onto Screen and sysc
  XMapWindow(this->DisplayId,this->WindowId);
  XSync(this->DisplayId,0);
  this->InitializeColor();
  this->ViewerOn = 1;
}


//----------------------------------------------------------------------------
void vtkImageXViewer::InitializeColor() 
{
  int idx;
  int value;
  unsigned long planeMask, pval[256];
  Colormap  defColorMap, ncolormap;
  XColor    defccells[256];
  
  this->Offset = 0;
  
  this->ColorMap = DefaultColormap(this->DisplayId,this->Screen);
  
  if ( !XAllocColorCells(this->DisplayId, this->ColorMap, 0, &planeMask, 0, 
			 pval, (unsigned int) this->NumberColors))
    {
    // can't allocate NUM_COLORS from Def ColorMap
    // create new ColorMap ... but first cp some def ColorMap
    
    ncolormap = XCreateColormap(this->DisplayId, RootWindow(this->DisplayId,this->Screen),
				this->VisualInfo.visual, AllocNone);
    this->Offset = 100;
    if (! XAllocColorCells(this->DisplayId, ncolormap, 1, &planeMask, 0, pval,
			   (unsigned int)256))
      {
      vtkErrorMacro(<< "Sorry cann't allocate any more Colors");
      return;
      }
    
    defColorMap = DefaultColormap(this->DisplayId, this->Screen);
    for ( idx = 0 ; idx < 256; idx++) 
      {
      defccells[idx].pixel = idx; 
      }
    XQueryColors(this->DisplayId, defColorMap, defccells, 256);
    
    
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
	XStoreColor(this->DisplayId, ncolormap, &(this->Colors[idx]));
	}
      else 
	{
	this->Colors[idx].pixel = pval[idx];
	this->Colors[idx].red   = value ;
	this->Colors[idx].green = value ; 
	this->Colors[idx].blue  = value ;
	this->Colors[idx].flags = DoRed | DoGreen | DoBlue ;
	XStoreColor(this->DisplayId, ncolormap, &(this->Colors[idx]));
	}
      }
    
    
    this->Attributes.colormap = ncolormap;
    XChangeWindowAttributes(this->DisplayId, this->WindowId, CWColormap, 
			    &(this->Attributes));
    XInstallColormap(this->DisplayId, ncolormap);
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
      XStoreColor(this->DisplayId, this->ColorMap, &(this->Colors[idx]));
      }
    } 
}



