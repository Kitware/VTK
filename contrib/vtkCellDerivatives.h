/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDerivatives.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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

class VTK_EXPORT vtkCellDerivatives : public vtkDataSetToDataSetFilter 
{
public:
  vtkCellDerivatives();
  const char *GetClassName() {return "vtkCellDerivatives";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct to compute the gradient of the scalars and vectors.
  static vtkCellDerivatives *New() {return new vtkCellDerivatives;};

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
  char *GetVectorModeAsString();

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
  char *GetTensorModeAsString();

protected:
  void Execute();

  int VectorMode;
  int TensorMode;
};

#endif


