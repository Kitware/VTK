// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPassThroughLayoutStrategy.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPassThroughLayoutStrategy);

//------------------------------------------------------------------------------

vtkPassThroughLayoutStrategy::vtkPassThroughLayoutStrategy() = default;

//------------------------------------------------------------------------------

vtkPassThroughLayoutStrategy::~vtkPassThroughLayoutStrategy() = default;

//------------------------------------------------------------------------------
// Set the graph that will be laid out
void vtkPassThroughLayoutStrategy::Initialize() {}

//------------------------------------------------------------------------------

// Simple graph layout method
void vtkPassThroughLayoutStrategy::Layout() {}

void vtkPassThroughLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
