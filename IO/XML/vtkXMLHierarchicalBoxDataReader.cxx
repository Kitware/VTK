// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLHierarchicalBoxDataReader.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLHierarchicalBoxDataReader);

//------------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataReader::vtkXMLHierarchicalBoxDataReader() = default;

//------------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataReader::~vtkXMLHierarchicalBoxDataReader() = default;

//------------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
