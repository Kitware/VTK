// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_5_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkXMLPHierarchicalBoxDataWriter.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPHierarchicalBoxDataWriter);

//------------------------------------------------------------------------------
vtkXMLPHierarchicalBoxDataWriter::vtkXMLPHierarchicalBoxDataWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPHierarchicalBoxDataWriter::~vtkXMLPHierarchicalBoxDataWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPHierarchicalBoxDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
