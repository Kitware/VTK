/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoordinate.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkCoordinate
// .SECTION Description
// vtkCoordinate represents a location or position. It supports multiple
// coordinate systems and relative positioning.

#ifndef __vtkCoordinate_h
#define __vtkCoordinate_h

#include "vtkReferenceCount.h"
class vtkViewport;

#define VTK_DISPLAY             0
#define VTK_NORMALIZED_DISPLAY  1
#define VTK_VIEWPORT            2
#define VTK_NORMALIZED_VIEWPORT 3
#define VTK_VIEW                4
#define VTK_WORLD               5

class VTK_EXPORT vtkCoordinate : public vtkReferenceCount
{
public:

  vtkCoordinate();
  ~vtkCoordinate();
  static vtkCoordinate* New() {return new vtkCoordinate;};
  void PrintSelf(ostream& os, vtkIndent indent);
  const char *GetClassName() {return "vtkCoordinate";};

  // Description:
  // Set/get the coordinate system which this corrdinate
  // is defined in. The options are Display, Normalized Display,
  // Viewport, Normalized Viewport, View, and World.
  vtkSetMacro(CoordinateSystem, int);
  vtkGetMacro(CoordinateSystem, int);
  void SetCoordinateSystemToDisplay() {this->SetCoordinateSystem(VTK_DISPLAY);}
  void SetCoordinateSystemToNormalizedDisplay() 
	{this->SetCoordinateSystem(VTK_NORMALIZED_DISPLAY);}
  void SetCoordinateSystemToViewport() 
	{this->SetCoordinateSystem(VTK_VIEWPORT);}
  void SetCoordinateSystemToNormalizedViewport() 
	{this->SetCoordinateSystem(VTK_NORMALIZED_VIEWPORT);}
  void SetCoordinateSystemToView() {this->SetCoordinateSystem(VTK_VIEW);}
  void SetCoordinateSystemToWorld() {this->SetCoordinateSystem(VTK_WORLD);}
    
  // Description:
  // Set/get the value of this coordinate. This can be thought of as
  // the position of this coordinate in its coordinate system.
  vtkSetVector3Macro(Value,float);
  vtkGetVector3Macro(Value,float);
  
  void SetValue(float a, float b) { this->SetValue(a,b,0.0);}
  
  // Description:
  // If this coordinate is relative to another coordinate,
  // then specify that coordinate as the ReferenceCoordinate.
  // If this is NULL the coordinate is assumed to be absolute.
  vtkSetReferenceCountedObjectMacro(ReferenceCoordinate,vtkCoordinate);
  vtkGetObjectMacro(ReferenceCoordinate,vtkCoordinate);

  // Description:
  // If you want this coordinate to be relative to a specific
  // vtkViewport (vtkRenderer, vtkImager) then you can specify
  // that here.
  vtkSetObjectMacro(Viewport,vtkViewport);
  vtkGetObjectMacro(Viewport,vtkViewport);

  float *GetComputedWorldValue(vtkViewport *);
  int *GetComputedViewportValue(vtkViewport *);
  int *GetComputedDisplayValue(vtkViewport *);
  int *GetComputedLocalDisplayValue(vtkViewport *);

  // GetComputed Value will return either World, Viewport or 
  // Display based on what has been set as the coordinate system
  // This is good for objects like vtkLineSource, where the
  // user might want to use them as World or Viewport coordinates
  float *GetComputedValue(vtkViewport *);
  
protected:
  float Value[3];
  int   CoordinateSystem;
  vtkCoordinate *ReferenceCoordinate;
  vtkViewport *Viewport;
  float ComputedWorldValue[3];
  int   ComputedDisplayValue[2];
  int   ComputedViewportValue[2];
  int   Computing;
};

#endif


