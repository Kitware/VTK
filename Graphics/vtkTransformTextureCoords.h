/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformTextureCoords.h
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
// .NAME vtkTransformTextureCoords - transform (scale, rotate, translate) texture coordinates
// .SECTION Description
// vtkTransformTextureCoords is a filter that operates on texture
// coordinates. It ingests any type of dataset, and outputs a dataset of the
// same type. The filter lets you scale, translate, and rotate texture
// coordinates. For example, by using the the Scale ivar, you can shift
// texture coordinates that range from (0->1) to range from (0->10) (useful
// for repeated patterns).
// 
// The filter operates on texture coordinates of dimension 1->3. The texture 
// coordinates are referred to as r-s-t. If the texture map is two dimensional,
// the t-coordinate (and operations on the t-coordinate) are ignored.

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToBox vtkTextureMapToCylinder 
// vtkTextureMapToSphere vtkThresholdTextureCoords vtkTexture

#ifndef __vtkTransformTextureCoords_h
#define __vtkTransformTextureCoords_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkTransformTextureCoords : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkTransformTextureCoords,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create instance with Origin (0.5,0.5,0.5); Position (0,0,0); and Scale
  // set to (1,1,1). Rotation of the texture coordinates is turned off.
  static vtkTransformTextureCoords *New();

  // Description:
  // Set/Get the position of the texture map. Setting the position translates
  // the texture map by the amount specified. 
  vtkSetVector3Macro(Position,float);
  vtkGetVectorMacro(Position,float,3);

  // Description:
  // Incrementally change the position of the texture map (i.e., does a
  // translate or shift of the texture coordinates).
  void AddPosition(float deltaR, float deltaS, float deltaT);
  void AddPosition(float deltaPosition[3]);
  
  // Description:
  // Set/Get the scale of the texture map. Scaling in performed independently 
  // on the r, s and t axes.
  vtkSetVector3Macro(Scale,float);
  vtkGetVectorMacro(Scale,float,3);

  // Description:
  // Set/Get the origin of the texture map. This is the point about which the
  // texture map is flipped (e.g., rotated). Since a typical texture map ranges
  // from (0,1) in the r-s-t coordinates, the default origin is set at 
  // (0.5,0.5,0.5).
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Boolean indicates whether the texture map should be flipped around the 
  // s-axis. Note that the flips occur around the texture origin.
  vtkSetMacro(FlipR,int);
  vtkGetMacro(FlipR,int);
  vtkBooleanMacro(FlipR,int);

  // Description:
  // Boolean indicates whether the texture map should be flipped around the 
  // s-axis. Note that the flips occur around the texture origin.
  vtkSetMacro(FlipS,int);
  vtkGetMacro(FlipS,int);
  vtkBooleanMacro(FlipS,int);

  // Description:
  // Boolean indicates whether the texture map should be flipped around the 
  // t-axis. Note that the flips occur around the texture origin.
  vtkSetMacro(FlipT,int);
  vtkGetMacro(FlipT,int);
  vtkBooleanMacro(FlipT,int);

protected:
  vtkTransformTextureCoords();
  ~vtkTransformTextureCoords() {};

  void Execute();

  float Origin[3]; //point around which map rotates
  float Position[3]; //controls translation of map
  float Scale[3]; //scales the texture map
  int FlipR; //boolean indicates whether to flip texture around r-axis
  int FlipS; //boolean indicates whether to flip texture around s-axis
  int FlipT; //boolean indicates whether to flip texture around t-axis
private:
  vtkTransformTextureCoords(const vtkTransformTextureCoords&);  // Not implemented.
  void operator=(const vtkTransformTextureCoords&);  // Not implemented.
};

#endif
