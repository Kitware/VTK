/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoordinate.h
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
// .NAME vtkCoordinate - perform coordinate transformation, and represent position, in a variety of vtk coordinate systems
// .SECTION Description
// vtkCoordinate represents position in a variety of coordinate systems, and
// converts position to other coordinate systems. It also supports relative
// positioning, so you can create a cascade of vtkCoordinate objects (no loops
// please!) that refer to each other. The typical usage of this object is
// to set the coordinate system in which to represent a position (e.g., 
// SetCoordinateSystemToNormalizedDisplay()), set the value of the coordinate
// (e.g., SetValue()), and then invoke the appropriate method to convert to 
// another coordinate system (e.g., GetComputedWorldValue()).
// 
// The coordinate systems in vtk are as follows:
//<PRE>
//  DISPLAY -             x-y pixel values in window
//  NORMALIZED DISPLAY -  x-y (0,1) normalized values
//  VIEWPORT -            x-y pixel values in viewport
//  NORMALIZED VIEWPORT - x-y (0,1) normalized value in viewport
//  VIEW -                x-y-z (-1,1) values in camera coordinates. (z is depth)
//  WORLD -               x-y-z global coordinate values
//  USERDEFINED -         x-y-z in User defined space
//</PRE>
//
// If you cascade vtkCoordinate objects, you refer to another vtkCoordinate
// object which in turn can refer to others, and so on. This allows you to
// create composite groups of things like vtkActor2D that are positioned
// relative to one another. Note that in cascaded sequences, each
// vtkCoordinate object may be specified in different coordinate systems!

// .SECTION See Also
// vtkProp2D vtkActor2D vtkScalarBarActor

#ifndef __vtkCoordinate_h
#define __vtkCoordinate_h

#include "vtkObject.h"
class vtkViewport;

#define VTK_DISPLAY             0
#define VTK_NORMALIZED_DISPLAY  1
#define VTK_VIEWPORT            2
#define VTK_NORMALIZED_VIEWPORT 3
#define VTK_VIEW                4
#define VTK_WORLD               5
#define VTK_USERDEFINED         6

class VTK_COMMON_EXPORT vtkCoordinate : public vtkObject
{
public:
  vtkTypeMacro(vtkCoordinate,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates an instance of this class with the following defaults: 
  // value of  (0,0,0) in world  coordinates.
  static vtkCoordinate* New();

  // Description:
  // Set/get the coordinate system which this coordinate
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
    
  const char *GetCoordinateSystemAsString ();

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
  vtkSetObjectMacro(ReferenceCoordinate,vtkCoordinate);
  vtkGetObjectMacro(ReferenceCoordinate,vtkCoordinate);

  // Description:
  // If you want this coordinate to be relative to a specific
  // vtkViewport (vtkRenderer, vtkImager) then you can specify
  // that here.
  void SetViewport(vtkViewport *viewport);
  vtkGetObjectMacro(Viewport,vtkViewport);

  // Description:
  // Return the computed value in a specified coordinate system.
  float *GetComputedWorldValue(vtkViewport *);
  int *GetComputedViewportValue(vtkViewport *);
  int *GetComputedDisplayValue(vtkViewport *);
  int *GetComputedLocalDisplayValue(vtkViewport *);

  float *GetComputedFloatViewportValue(vtkViewport *);
  float *GetComputedFloatDisplayValue(vtkViewport *);

  // Description:
  // GetComputedValue() will return either World, Viewport or
  // Display based on what has been set as the coordinate system.
  // This is good for objects like vtkLineSource, where the
  // user might want to use them as World or Viewport coordinates
  float *GetComputedValue(vtkViewport *);

  // Description:
  // GetComputedUserDefinedValue() is to be used only when
  // the coordinate system is VTK_USERDEFINED. The user
  // must subclass vtkCoordinate and override this function,
  // when set as the TransformCoordinate in 2D-Mappers, the user
  // can customize display of 2D polygons
  virtual float *GetComputedUserDefinedValue(vtkViewport *)
    { return this->Value; }

protected:
  vtkCoordinate();
  ~vtkCoordinate();
  vtkCoordinate(const vtkCoordinate&);
  void operator=(const vtkCoordinate&);

  float Value[3];
  int   CoordinateSystem;
  vtkCoordinate *ReferenceCoordinate;
  vtkViewport *Viewport;
  float ComputedWorldValue[3];
  int   ComputedDisplayValue[2];
  int   ComputedViewportValue[2];
  int   Computing;

  float ComputedFloatDisplayValue[2];
  float ComputedFloatViewportValue[2];
  float ComputedUserDefinedValue[3];

};

#endif


