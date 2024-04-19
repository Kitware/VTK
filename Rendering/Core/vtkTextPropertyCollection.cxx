// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTextPropertyCollection.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTextPropertyCollection);

//------------------------------------------------------------------------------
void vtkTextPropertyCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkTextPropertyCollection::vtkTextPropertyCollection() = default;

//------------------------------------------------------------------------------
vtkTextPropertyCollection::~vtkTextPropertyCollection() = default;
VTK_ABI_NAMESPACE_END
