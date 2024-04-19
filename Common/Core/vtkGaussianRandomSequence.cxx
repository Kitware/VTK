// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGaussianRandomSequence.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkGaussianRandomSequence::vtkGaussianRandomSequence() = default;

//------------------------------------------------------------------------------
vtkGaussianRandomSequence::~vtkGaussianRandomSequence() = default;

//------------------------------------------------------------------------------
double vtkGaussianRandomSequence::GetScaledValue(double mean, double standardDeviation)
{
  return mean + standardDeviation * this->GetValue();
}

//------------------------------------------------------------------------------
double vtkGaussianRandomSequence::GetNextScaledValue(double mean, double standardDeviation)
{
  this->Next();
  return this->GetScaledValue(mean, standardDeviation);
}

//------------------------------------------------------------------------------
void vtkGaussianRandomSequence::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
