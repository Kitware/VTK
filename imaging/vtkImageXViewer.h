/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageXViewer.h
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
// .NAME vtkImageXViewer - Display a 2d image.
// .SECTION Description
// vtkImageXViewer displays a 2d image in an X window.

#ifndef __vtkImageXViewer_h
#define __vtkImageXViewer_h

#include 	<X11/Xlib.h>
#include 	<X11/Xutil.h>
#include 	<X11/cursorfont.h>
#include 	<X11/X.h>
#include 	<X11/keysym.h>
#include    	<fstream.h>
#include 	<stdlib.h>
#include 	<iostream.h>

#include 	"vtkObject.hh"
#include 	"vtkImageSource.hh"
#include 	"vtkImageRegion.hh"

class vtkImageXViewer : public vtkObject {
public:
  vtkImageXViewer();
  ~vtkImageXViewer();
  char *GetClassName() {return "vtkImageXViewer";};

  // Description:
  // Messages that get forwarded to this viewers "Region".
  void SetBounds(int *bounds)
  {this->Region.SetBounds2d(bounds); this->Modified();};
  void SetBounds(int min0, int max0, int min1, int max1)
  {this->Region.SetBounds2d(min0,max0, min1,max1); this->Modified();};
  int *GetBounds(){return this->Region.GetBounds2d();};
  void GetBounds(int *bounds){this->Region.GetBounds2d(bounds);};
  void GetBounds(int &min0,int &max0,int &min1,int &max1)
  {this->Region.GetBounds2d(min0,max0,min1,max1);};
  void SetDefaultCoordinate2(int v)
  {this->Region.SetDefaultCoordinate2(v); this->Modified();};
  void SetDefaultCoordinate3(int v)
  {this->Region.SetDefaultCoordinate3(v); this->Modified();};
  
  // Description:
  // Sets The coordinate system of the displayed region.  The first
  // Two dimensions are the ones displayed.  The others are provided
  // to set default values (i.e. slice of a volume.
  void SetAxes(int axis0, int axis1)
  {this->Region.SetAxes2d(axis0,axis1); this->Modified();};
  void SetAxes(int axis0, int axis1, int axis2)
  {this->Region.SetAxes3d(axis0,axis1,axis2); this->Modified();};
  void SetAxes(int axis0, int axis1, int axis2, int axis3)
  {this->Region.SetAxes4d(axis0,axis1,axis2,axis3); this->Modified();};

  // Description:
  // Sets window/level for mapping pixels to colors.
  vtkSetMacro(ColorWindow,float);
  vtkGetMacro(ColorWindow,float);
  vtkSetMacro(ColorLevel,float);
  vtkGetMacro(ColorLevel,float);

  // Description:
  // Window/level information used by templated function.
  float GetColorShift();
  float GetColorScale();
  XColor *GetColors();
  vtkGetMacro(NumberColors,int);

  // Description:
  // Set/Get the input that will be supplying the image.
  vtkSetObjectMacro(Input,vtkImageSource);
  vtkGetObjectMacro(Input,vtkImageSource);
  
  void Render(void);
  
  void SetWindow(Window win);
  
  // Description:
  // Gets the windows depth. For the templated function.
  vtkGetMacro(VisualDepth,int);
  
  // Description:
  // Turn color interpretation on/off.
  vtkSetMacro(ColorFlag, int);
  vtkGetMacro(ColorFlag, int);
  vtkBooleanMacro(ColorFlag, int);
  // Description:
  // Which components should be used for RGB.
  vtkSetMacro(Red, int);
  vtkGetMacro(Red, int);
  vtkSetMacro(Green, int);
  vtkGetMacro(Green, int);
  vtkSetMacro(Blue, int);
  vtkGetMacro(Blue, int);
  
  
protected:
  // X stuff
  Window	       WindowId;
  Display             *DisplayId;
  Visual              *VisualId;
  int                  VisualDepth;
  Colormap             ColorMap;
  GC                   Gc;
  Pixmap               IconPixmap;
  XImage              *Image;
  XEvent               report;
  int	               Offset;
  XColor               Colors[256];
  int                  NumberColors;
  
  
  vtkImageSource *Input;
  // Contains the bounds of the region to be displayed.
  vtkImageRegion Region;
  // For converting image pixels to X pixels.
  float ColorWindow;
  float ColorLevel;
  // Stuff for mapping color (i.e. Components to RGB)
  int ColorFlag;
  int Red;
  int Green;
  int Blue;
  
  
  Window MakeDefaultWindow(int width, int height);
  void GetDefaultVisualInfo(XVisualInfo *info);
  Colormap MakeColorMap(Visual *visual);
  Display *GetDisplayId();
  Visual *GetVisualId();
};

#endif


