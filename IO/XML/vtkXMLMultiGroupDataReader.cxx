// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_5_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkXMLMultiGroupDataReader.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLMultiGroupDataReader);
//------------------------------------------------------------------------------
vtkXMLMultiGroupDataReader::vtkXMLMultiGroupDataReader() = default;

//------------------------------------------------------------------------------
vtkXMLMultiGroupDataReader::~vtkXMLMultiGroupDataReader() = default;

//------------------------------------------------------------------------------
void vtkXMLMultiGroupDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
