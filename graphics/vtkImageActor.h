/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageActor - represents an image (data & properties) in a rendered scene
//
// .SECTION Description
// vtkImageActor is used to represent an imge entity in a rendering scene.
// It inherits functions related to the image's position, orientation and
// origin from vtkProp3D. 

// .SECTION see also
// vtkImageData vtkProp3D

#ifndef __vtkImageActor_h
#define __vtkImageActor_h

#include "vtkProp3D.h"
#include "vtkImageData.h"

class vtkPropCollection;
class vtkRenderer;

class VTK_EXPORT vtkImageActor : public vtkProp3D
{
public:
  vtkTypeMacro(vtkImageActor,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a ImageActor with the following defaults: origin(0,0,0) 
  // position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
  // orientation=(0,0,0).
  static vtkImageActor *New();

  // Description:
  // Set/Get the input for the image mapper.  
  vtkSetObjectMacro(Input, vtkImageData);
  vtkGetObjectMacro(Input,vtkImageData);

  // Description:
  // Turn on/off linear interpolation of the image when rendering.
  vtkGetMacro(Interpolate,int);
  vtkSetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);

  // Description:
  // The image extent of the output has to be set explicitely.
  void SetDisplayExtent(int extent[6]);
  void SetDisplayExtent(int minX, int maxX, int minY, int maxY, 
			    int minZ, int maxZ);
  void GetDisplayExtent(int extent[6]);
  int *GetDisplayExtent() {return this->DisplayExtent;}

  // Description:
  // Get the bounds - either all six at once 
  // (xmin, xmax, ymin, ymax, zmin, zmax) or one at a time.
  float *GetBounds();
  void GetBounds(float bounds[6]) { this->vtkProp3D::GetBounds( bounds ); };

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support the standard render methods.
  // int RenderTranslucentGeometry(vtkViewport *viewport);
  int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual void Load(vtkRenderer *) {};
//ETX

  // Description:
  // Return a slice number computed from the display extent
  int GetSliceNumber();
  
protected:
  vtkImageActor();
  ~vtkImageActor();
  vtkImageActor(const vtkImageActor&) {};
  void operator=(const vtkImageActor&) {};

  int   Interpolate;
  vtkImageData* Input;
  int DisplayExtent[6];
};

#endif

