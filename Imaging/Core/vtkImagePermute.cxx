// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImagePermute.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImagePermute);

vtkImagePermute::vtkImagePermute()
{
  this->FilteredAxes[0] = 0;
  this->FilteredAxes[1] = 1;
  this->FilteredAxes[2] = 2;
}

void vtkImagePermute::SetFilteredAxes(int newx, int newy, int newz)
{
  static double axes[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

  this->SetResliceAxesDirectionCosines(axes[newx], axes[newy], axes[newz]);

  this->FilteredAxes[0] = newx;
  this->FilteredAxes[1] = newy;
  this->FilteredAxes[2] = newz;
}

void vtkImagePermute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FilteredAxes: ( " << this->FilteredAxes[0] << ", " << this->FilteredAxes[1]
     << ", " << this->FilteredAxes[2] << " )\n";
}
VTK_ABI_NAMESPACE_END
