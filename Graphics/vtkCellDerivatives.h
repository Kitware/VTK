/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDerivatives.h
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
// .NAME vtkCellDerivatives - compute derivatives of scalars and vectors
// .SECTION Description
// vtkCellDerivatives is a filter that computes derivatives of scalars
// and vectors at the center of cells. You can choose to generate
// different output including the scalar gradient (a vector), computed
// tensor vorticity (a vector), gradient of input vectors (a tensor),
// and strain matrix of the input vectors (a tensor); or you may
// choose to pass data through to the output.
//
// Note that it is assumed that on input scalars and vector point data
// is available, which are then used to generate cell vectors and tensors.
// (The interpolation functions of the cells are used to compute the
// derivatives which is why point data is required.)

// .SECTION Caveats
// The computed derivatives are cell attribute data; you can convert them to
// point attribute data by using the vtkCellDataToPointData filter.
// Note that, due to the interpolation function used (obtained using 
// 1/r**2 normalized sum), the derivatives calculated for polygons
// with more than 4 vertices are inaccurate in most cases.
//
// The point data is passed through the filter to the output.

// .SECTION See Also
// vtkVectorNorm

#ifndef __vtkCellDerivatives_h
#define __vtkCellDerivatives_h

#include "vtkDataSetToDataSetFilter.h"

#define VTK_VECTOR_MODE_PASS_VECTORS      0
#define VTK_VECTOR_MODE_COMPUTE_GRADIENT  1
#define VTK_VECTOR_MODE_COMPUTE_VORTICITY 2

#define VTK_TENSOR_MODE_PASS_TENSORS     0
#define VTK_TENSOR_MODE_COMPUTE_GRADIENT 1
#define VTK_TENSOR_MODE_COMPUTE_STRAIN   2

class VTK_GRAPHICS_EXPORT vtkCellDerivatives : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkCellDerivatives,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct to compute the gradient of the scalars and vectors.
  static vtkCellDerivatives *New();

  // Description:
  // Control how the filter works to generate vector cell data. You
  // can choose to pass the input cell vectors, compute the gradient
  // of the input scalars, or extract the vorticity of the computed
  // vector gradient tensor. By default (VectorModeToComputeGradient),
  // the filter will take the gradient of the input scalar data.
  vtkSetMacro(VectorMode,int);
  vtkGetMacro(VectorMode,int);
  void SetVectorModeToPassVectors() 
    {this->SetVectorMode(VTK_VECTOR_MODE_PASS_VECTORS);};
  void SetVectorModeToComputeGradient() 
    {this->SetVectorMode(VTK_VECTOR_MODE_COMPUTE_GRADIENT);};
  void SetVectorModeToComputeVorticity() 
    {this->SetVectorMode(VTK_VECTOR_MODE_COMPUTE_VORTICITY);};
  const char *GetVectorModeAsString();

  // Description:
  // Control how the filter works to generate tensor cell data. You can
  // choose to pass the input cell tensors, compute the gradient of
  // the input vectors, or compute the strain tensor of the vector gradient
  // tensor. By default (TensorModeToComputeGradient), the filter will
  // take the gradient of the vector data to construct a tensor.
  vtkSetMacro(TensorMode,int);
  vtkGetMacro(TensorMode,int);
  void SetTensorModeToPassTensors() 
    {this->SetTensorMode(VTK_TENSOR_MODE_PASS_TENSORS);};
  void SetTensorModeToComputeGradient() 
    {this->SetTensorMode(VTK_TENSOR_MODE_COMPUTE_GRADIENT);};
  void SetTensorModeToComputeStrain() 
    {this->SetTensorMode(VTK_TENSOR_MODE_COMPUTE_STRAIN);};
  const char *GetTensorModeAsString();

protected:
  vtkCellDerivatives();
  ~vtkCellDerivatives() {};
  void Execute();

  int VectorMode;
  int TensorMode;
private:
  vtkCellDerivatives(const vtkCellDerivatives&);  // Not implemented.
  void operator=(const vtkCellDerivatives&);  // Not implemented.
};

#endif


