/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty2D.h
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
// .NAME vtkProperty2D
// .SECTION Description
// vtkProperty2D contains properties used to render two dimensional images
// and annotations.

// .SECTION See Also
// vtkActor2D 

#ifndef __vtkProperty2D_h
#define __vtkProperty2D_h

#include "vtkObject.h"

class vtkViewport;

#define VTK_BACKGROUND_LOCATION 0
#define VTK_FOREGROUND_LOCATION 1

class VTK_EXPORT vtkProperty2D : public vtkObject
{
public:
  vtkTypeMacro(vtkProperty2D,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a vtkProperty2D with the following default values:
  // Opacity 1, Color (1,1,1)
  static vtkProperty2D *New();

  // Description:
  // Assign one property to another. 
  void DeepCopy(vtkProperty2D *p);

  // Description:
  // Set/Get the RGB color of this property.
  vtkSetVector3Macro(Color, float);
  vtkGetVectorMacro(Color, float, 3);

  // Description:
  // Set/Get the Opacity of this property.
  vtkGetMacro(Opacity, float);
  vtkSetMacro(Opacity, float);

  // Description:
  // Set/Get the diameter of a Point. The size is expressed in screen units.
  // This is only implemented for OpenGL. The default is 1.0.
  vtkSetClampMacro(PointSize,float,0,VTK_LARGE_FLOAT);
  vtkGetMacro(PointSize,float);

  // Description:
  // Set/Get the width of a Line. The width is expressed in screen units.
  // This is only implemented for OpenGL. The default is 1.0.
  vtkSetClampMacro(LineWidth,float,0,VTK_LARGE_FLOAT);
  vtkGetMacro(LineWidth,float);

  // Description:
  // Set/Get the stippling pattern of a Line, as a 16-bit binary pattern 
  // (1 = pixel on, 0 = pixel off).
  // This is only implemented for OpenGL. The default is 0xFFFF.
  vtkSetMacro(LineStipplePattern,int);
  vtkGetMacro(LineStipplePattern,int);

  // Description:
  // Set/Get the stippling repeat factor of a Line, which specifies how
  // many times each bit in the pattern is to be repeated.
  // This is only implemented for OpenGL. The default is 1.
  vtkSetClampMacro(LineStippleRepeatFactor,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(LineStippleRepeatFactor,int);

  // Description:
  // The DisplayLocation is either background or foreground.
  // If it is background, then this 2D actor will be drawn
  // behind all 3D props or foreground 2D actors. If it is
  // background, then this 2D actor will be drawn in front of
  // all 3D props and background 2D actors. Within 2D actors
  // of the same DisplayLocation type, order is determined by
  // the order in which the 2D actors were added to the viewport.
  vtkSetClampMacro( DisplayLocation, int, 
                    VTK_BACKGROUND_LOCATION, VTK_FOREGROUND_LOCATION );
  vtkGetMacro( DisplayLocation, int );
  void SetDisplayLocationToBackground() 
    {this->DisplayLocation = VTK_BACKGROUND_LOCATION;};
  void SetDisplayLocationToForeground() 
    {this->DisplayLocation = VTK_FOREGROUND_LOCATION;};
  
  
  // Description:
  // Have the device specific subclass render this property.
  virtual void Render (vtkViewport* vtkNotUsed(viewport))  {}
  
protected:
  vtkProperty2D();
  ~vtkProperty2D();
  vtkProperty2D(const vtkProperty2D&);
  void operator=(const vtkProperty2D&);

  float Color[3];
  float Opacity;
  float PointSize;
  float LineWidth;
  int   LineStipplePattern;
  int   LineStippleRepeatFactor;
  int   DisplayLocation;
};
  
  
#endif


  
