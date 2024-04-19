// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION See Also
// vtkOBJReader vtkPLYReader vtkSTLReader
#include "vtkAbstractPolyDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
vtkAbstractPolyDataReader::vtkAbstractPolyDataReader()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
}

vtkAbstractPolyDataReader::~vtkAbstractPolyDataReader()
{
  this->SetFileName(nullptr);
}

void vtkAbstractPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NONE") << endl;
}
VTK_ABI_NAMESPACE_END
