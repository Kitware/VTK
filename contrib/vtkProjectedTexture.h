/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTexture.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkProjectedTexture - assign texture coordinates for a projected texture
// .SECTION Description
// vtkProjectedTexture assigns texture coordinates to a dataset as if
// the texture was projected from a slide projected located somewhere in the
// scene.  Methods are provided to position the projector and aim it at a 
// location, to set the width of the projector's frustum, and to set the
// range of texture coordinates assigned to the dataset.  
//
// Objects in the scene that appear behind the projector are also assigned
// texture coordinates; the projected image is left-right and top-bottom 
// flipped, much as a lens' focus flips the rays of light that pass through
// it.  A warning is issued if a point in the dataset falls at the focus
// of the projector.

#ifndef __vtkProjectedTexture_h
#define __vtkProjectedTexture_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkProjectedTexture : public vtkDataSetToDataSetFilter 
{
public:
  static vtkProjectedTexture *New();
  vtkTypeMacro(vtkProjectedTexture,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the position of the focus of the projector.
  vtkSetVector3Macro(Position,float);
  vtkGetVectorMacro(Position,float,3);

  // Description:
  // Set/Get the focal point of the projector (a point that lies along
  // the center axis of the projector's frustum).
  void SetFocalPoint(float focalPoint[3]);
  void SetFocalPoint(float x, float y, float z);
  vtkGetVectorMacro(FocalPoint,float,3);

  // Description:
  // Get the normalized orientation vector of the projector.
  vtkGetVectorMacro(Orientation,float,3);
  
  // Set/Get the up vector of the projector.
  vtkSetVector3Macro(Up,float);
  vtkGetVectorMacro(Up,float,3);

  // Set/Get the aspect ratio of a perpendicular cross-section of the
  // the projector's frustum.  The aspect ratio consists of three 
  // numbers:  (x, y, z), where x is the width of the 
  // frustum, y is the height, and z is the perpendicular
  // distance from the focus of the projector.
  vtkSetVector3Macro(AspectRatio,float);
  vtkGetVectorMacro(AspectRatio,float,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);
  
protected:
  vtkProjectedTexture();
  ~vtkProjectedTexture() {};
  vtkProjectedTexture(const vtkProjectedTexture&);
  void operator=(const vtkProjectedTexture&);

  void Execute();
  void ComputeNormal();

  float Position[3];
  float Orientation[3];
  float FocalPoint[3];
  float Up[3];
  float AspectRatio[3];
  float SRange[2];
  float TRange[2];
};

#endif

