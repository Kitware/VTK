/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdTextureCoords.h
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
// .NAME vtkThresholdTextureCoords - compute 1D, 2D, or 3D texture coordinates based on scalar threshold
// .SECTION Description
// vtkThresholdTextureCoords is a filter that generates texture coordinates for
// any input dataset type given a threshold criterion. The criterion can take 
// three forms: 1) greater than a particular value (ThresholdByUpper()); 
// 2) less than a particular value (ThresholdByLower(); or 3) between two 
// values (ThresholdBetween(). If the threshold criterion is satisfied, 
// the "in" texture coordinate will be set (this can be specified by the
// user). If the threshold criterion is not satisfied the "out" is set.

// .SECTION Caveats
// There is a texture map - texThres.vtk - that can be used in conjunction
// with this filter. This map defines a "transparent" region for texture 
// coordinates 0<=r<0.5, and an opaque full intensity map for texture 
// coordinates 0.5<r<=1.0. There is a small transition region for r=0.5.

// .SECTION See Also
// vtkThreshold vtkThresholdPoints vtkTextureMapToPlane vtkTextureMapToSphere
// vtkTextureMapToCylinder vtkTextureMapToBox

#ifndef __vtkThresholdTextureCoords_h
#define __vtkThresholdTextureCoords_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkThresholdTextureCoords : public vtkDataSetToDataSetFilter
{
public:
  static vtkThresholdTextureCoords *New();
  vtkTypeMacro(vtkThresholdTextureCoords,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Criterion is cells whose scalars are less than lower threshold.
  void ThresholdByLower(float lower);

  // Description:
  // Criterion is cells whose scalars are less than upper threshold.
  void ThresholdByUpper(float upper);

  // Description:
  // Criterion is cells whose scalars are between lower and upper thresholds.
  void ThresholdBetween(float lower, float upper);

  // Description:
  // Return the upper and lower thresholds.
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

  // Description:
  // Set the desired dimension of the texture map.
  vtkSetClampMacro(TextureDimension,int,1,3);
  vtkGetMacro(TextureDimension,int);

  // Description:
  // Set the texture coordinate value for point satisfying threshold criterion.
  vtkSetVector3Macro(InTextureCoord,float);
  vtkGetVectorMacro(InTextureCoord,float,3);

  // Description:
  // Set the texture coordinate value for point NOT satisfying threshold
  //  criterion.
  vtkSetVector3Macro(OutTextureCoord,float);
  vtkGetVectorMacro(OutTextureCoord,float,3);

protected:
  vtkThresholdTextureCoords();
  ~vtkThresholdTextureCoords() {};
  vtkThresholdTextureCoords(const vtkThresholdTextureCoords&) {};
  void operator=(const vtkThresholdTextureCoords&) {};

  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  int TextureDimension;

  float InTextureCoord[3];
  float OutTextureCoord[3];

  //BTX
  int (vtkThresholdTextureCoords::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif
