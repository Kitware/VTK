/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardMethod.h
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
// .NAME vtkShepardMethod - sample unstructured points onto structured points using the method of Shepard
// .SECTION Description
// vtkShepardMethod is a filter used to visualize unstructured point data using
// Shepard's method. The method works by resampling the unstructured points 
// onto a structured points set. The influence functions are described as 
// "inverse distance weighted". Once the structured points are computed, the 
// usual visualization techniques (e.g., iso-contouring or volume rendering)
// can be used visualize the structured points.
// .SECTION Caveats
// The input to this filter is any dataset type. This filter can be used 
// to resample any form of data, i.e., the input data need not be 
// unstructured. 
//
// The bounds of the data (i.e., the sample space) is automatically computed
// if not set by the user.
//
// If you use a maximum distance less than 1.0, some output points may
// never receive a contribution. The final value of these points can be 
// specified with the "NullValue" instance variable.

#ifndef __vtkShepardMethod_h
#define __vtkShepardMethod_h

#include "vtkDataSetToStructuredPointsFilter.h"

class VTK_EXPORT vtkShepardMethod : public vtkDataSetToStructuredPointsFilter 
{
public:
  vtkTypeMacro(vtkShepardMethod,vtkDataSetToStructuredPointsFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with sample dimensions=(50,50,50) and so that model bounds are
  // automatically computed from input. Null value for each unvisited output 
  // point is 0.0. Maximum distance is 0.25.
  static vtkShepardMethod *New();
  
  // Description:
  // Compute ModelBounds from input geometry.
  float ComputeModelBounds(float origin[3], float ar[3]);

  // Description:
  // Specify i-j-k dimensions on which to sample input points.
  vtkGetVectorMacro(SampleDimensions,int,3);
  
  // Description:
  // Set the i-j-k dimensions on which to sample the distance function.
  void SetSampleDimensions(int i, int j, int k);

  // Description:
  // Set the i-j-k dimensions on which to sample the distance function.
  void SetSampleDimensions(int dim[3]);

  // Description:
  // Specify influence distance of each input point. This distance is a 
  // fraction of the length of the diagonal of the sample space. Thus, values 
  // of 1.0 will cause each input point to influence all points in the 
  // structured point dataset. Values less than 1.0 can improve performance
  // significantly.
  vtkSetClampMacro(MaximumDistance,float,0.0,1.0);
  vtkGetMacro(MaximumDistance,float);

  // Description:
  // Specify the position in space to perform the sampling.
  vtkSetVector6Macro(ModelBounds,float);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Set the Null value for output points not receiving a contribution from the
  // input points.
  vtkSetMacro(NullValue,float);
  vtkGetMacro(NullValue,float);

protected:
  vtkShepardMethod();
  ~vtkShepardMethod() {};
  vtkShepardMethod(const vtkShepardMethod&);
  void operator=(const vtkShepardMethod&);

  void Execute();

  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
  float NullValue;
};

#endif


