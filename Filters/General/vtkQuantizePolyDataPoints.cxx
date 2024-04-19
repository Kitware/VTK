// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkQuantizePolyDataPoints.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQuantizePolyDataPoints);

//------------------------------------------------------------------------------
// Construct object with initial QFactor of 0.25
vtkQuantizePolyDataPoints::vtkQuantizePolyDataPoints()
{
  this->QFactor = 0.25;
  this->Tolerance = 0.0;
}

//------------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::OperateOnPoint(double in[3], double out[3])
{
  out[0] = floor(in[0] / this->QFactor + 0.5) * this->QFactor;
  out[1] = floor(in[1] / this->QFactor + 0.5) * this->QFactor;
  out[2] = floor(in[2] / this->QFactor + 0.5) * this->QFactor;
}

//------------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::OperateOnBounds(double in[6], double out[6])
{
  out[0] = floor(in[0] / this->QFactor + 0.5) * this->QFactor;
  out[1] = floor(in[1] / this->QFactor + 0.5) * this->QFactor;
  out[2] = floor(in[2] / this->QFactor + 0.5) * this->QFactor;
  out[3] = floor(in[3] / this->QFactor + 0.5) * this->QFactor;
  out[4] = floor(in[4] / this->QFactor + 0.5) * this->QFactor;
  out[5] = floor(in[5] / this->QFactor + 0.5) * this->QFactor;
}

//------------------------------------------------------------------------------
void vtkQuantizePolyDataPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "QFactor: " << this->QFactor << "\n";
}
VTK_ABI_NAMESPACE_END
