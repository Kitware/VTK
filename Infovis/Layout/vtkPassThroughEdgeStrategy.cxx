// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPassThroughEdgeStrategy.h"

#include "vtkCellArray.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"

#include <map>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPassThroughEdgeStrategy);

vtkPassThroughEdgeStrategy::vtkPassThroughEdgeStrategy() = default;

vtkPassThroughEdgeStrategy::~vtkPassThroughEdgeStrategy() = default;

void vtkPassThroughEdgeStrategy::Layout() {}

void vtkPassThroughEdgeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
