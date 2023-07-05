// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHandleSource.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkHandleSource::vtkHandleSource()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
void vtkHandleSource::GetPosition(double pos[3])
{
  for (int i = 0; i < 3; i++)
  {
    pos[i] = this->GetPosition()[i];
  }
}

//------------------------------------------------------------------------------
void vtkHandleSource::GetDirection(double dir[3])
{
  for (int i = 0; i < 3; i++)
  {
    dir[i] = this->GetDirection()[i];
  }
}

//------------------------------------------------------------------------------
void vtkHandleSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Directional: " << this->Directional << "\n";
  os << indent << "Position: (" << this->GetPosition()[0] << ", " << this->GetPosition()[1] << ", "
     << this->GetPosition()[2] << ")\n";

  if (this->Directional)
  {
    os << indent << "Direction: (" << this->GetDirection()[0] << ", " << this->GetDirection()[1]
       << ", " << this->GetDirection()[2] << ")\n";
  }
  else
  {
    os << indent << "Direction: (none)\n";
  }

  os << indent << "Size: " << this->GetSize() << "\n";
}
VTK_ABI_NAMESPACE_END
