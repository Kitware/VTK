/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.h
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
// .NAME vtkImageViewer - Display a 2d image.
// .SECTION Description
// vtkImageViewer  is a generic viewer superclass.  The "New" method
// creates a viewer of the correct type.  Viewer have a coordinate system
// measured in pixels, and does not consider "Spacing" when displaying 
// an image.  The viewer can display color stored as RGB in the scalars
// components.  The components used for the RGB values can be specified,
// but default to 0, 1, 2.

#ifndef __vtkImageViewer_h
#define __vtkImageViewer_h

#include <fstream.h>
#include <stdlib.h>
#include <iostream.h>

#include "vtkObject.h"
#include "vtkImageCache.h"
#include "vtkImageRegion.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"

// For placement of origin in the viewer.
#define VTK_IMAGE_VIEWER_UPPER_LEFT 0
#define VTK_IMAGE_VIEWER_LOWER_LEFT 1

class VTK_EXPORT vtkImageViewer : public vtkObject {
public:
  vtkImageViewer();
  ~vtkImageViewer();
  static vtkImageViewer *New();
  const char *GetClassName() {return "vtkImageViewer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get name of rendering window
  vtkGetStringMacro(WindowName);

  void Render(void);
  // Description:
  // Subclass must define these methods.
  virtual void RenderRegion(vtkImageRegion *region) {region = region;};
  
  // Description:
  // Set/Get the position of the origin in the viewer.
  vtkSetMacro(OriginLocation, int);
  vtkGetMacro(OriginLocation, int);
  void SetOriginLocationToUpperLeft()
    {this->SetOriginLocation(VTK_IMAGE_VIEWER_UPPER_LEFT);}
  void SetOriginLocationToLowerLeft()
    {this->SetOriginLocation(VTK_IMAGE_VIEWER_LOWER_LEFT);}
  
  // Description:
  // Set/Get the input to the viewer.
  vtkSetObjectMacro(Input,vtkImageCache);
  vtkGetObjectMacro(Input,vtkImageCache);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}
  
  // Description:
  // Methods used to specify the region to be displayed.
  // The actual extent displayed is clipped by the "WholeExtent".
  vtkSetVector4Macro(DisplayExtent, int);
  vtkGetVector4Macro(DisplayExtent, int);  
  vtkSetMacro(ZSlice, int);
  vtkGetMacro(ZSlice, int);
  vtkSetMacro(TimeSlice, int);
  vtkGetMacro(TimeSlice, int);
  
  // Description:
  // Legacy compatability
  void SetCoordinate2(int c) {this->SetZSlice(c);}
  void SetCoordinate3(int c) {this->SetTimeSlice(c);}

  // Description:
  // Sets window/level for mapping pixels to colors.
  vtkSetMacro(ColorWindow, float);
  vtkGetMacro(ColorWindow, float);
  vtkSetMacro(ColorLevel, float);
  vtkGetMacro(ColorLevel, float);

  // Description:
  // Turn color interpretation on/off.
  vtkSetMacro(ColorFlag, int);
  vtkGetMacro(ColorFlag, int);
  vtkBooleanMacro(ColorFlag, int);
  // Description:
  // Which components should be used for RGB.
  vtkSetMacro(RedComponent, int);
  vtkGetMacro(RedComponent, int);
  vtkSetMacro(GreenComponent, int);
  vtkGetMacro(GreenComponent, int);
  vtkSetMacro(BlueComponent, int);
  vtkGetMacro(BlueComponent, int);
  
  // Description:
  // Set/Get the upper left location of the viewer in the window.
  vtkSetMacro(XOffset, int);
  vtkGetMacro(XOffset, int);
  vtkSetMacro(YOffset, int);
  vtkGetMacro(YOffset, int);

  // Description:
  // These are here for using a tk window.
  virtual void SetDisplayId(void *) {};
  virtual void SetWindowId(void *) {};
  virtual void SetParentId(void *) {};
  
  // Description:
  // Keep track of whether the rendering window has been mapped to screen.
  vtkSetMacro(Mapped,int);
  vtkGetMacro(Mapped,int);
  vtkBooleanMacro(Mapped,int);

  // Description:
  // By default this is a color viewer. 
  // GrayScaleHintOn will improve the appearance
  // of gray scale images on some systems.
  vtkSetMacro(GrayScaleHint,int);
  vtkGetMacro(GrayScaleHint,int);
  vtkBooleanMacro(GrayScaleHint,int);

  // Description:
  // Set/Get the position in screen coordinates of the rendering window.
  virtual int *GetPosition() {return (int *)NULL;};
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]);

  // Description:
  // Set/Get the size of the window in screen coordinates.
  virtual int *GetSize() {return (int *)NULL;};
  virtual void SetSize(int,int) {};
  virtual void SetSize(int a[2]);
  
  // Description:
  // The viewer implements a permutation before the image
  // is displayed.  (Do we really want this?  Is the speed up worth it?)
  vtkSetVector4Macro(PermutationAxes, int);
  vtkGetVector4Macro(PermutationAxes, int);

protected:
  // Placement of origin that determines orientation of image in viewer.
  int OriginLocation;
  // location of upper left corner in window.
  int XOffset;
  int YOffset;
  
  int Mapped;
  vtkImageCache *Input;
  // Contains the extent of the region to be displayed (X and Y axes).
  int DisplayExtent[4];
  int ZSlice;
  int TimeSlice;
  // The viewer can do a permutation before the image is rendered.
  int PermutationAxes[4];
  
  // For converting image pixels to X pixels.
  float ColorWindow;
  float ColorLevel;
  
  // Stuff for mapping color (i.e. Components to RGB)
  int ColorFlag;
  int RedComponent;
  int GreenComponent;
  int BlueComponent;
  char *WindowName;
  int Size[2];
  int Position[2];
  int GrayScaleHint;
  int OwnWindow;
};

#endif


