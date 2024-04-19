// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkCircularLayoutStrategy.h"

#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCircularLayoutStrategy);

vtkCircularLayoutStrategy::vtkCircularLayoutStrategy() = default;

vtkCircularLayoutStrategy::~vtkCircularLayoutStrategy() = default;

void vtkCircularLayoutStrategy::Layout()
{
  vtkPoints* points = vtkPoints::New();
  vtkIdType numVerts = this->Graph->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for (vtkIdType i = 0; i < numVerts; i++)
  {
    double x = cos(2.0 * vtkMath::Pi() * i / numVerts);
    double y = sin(2.0 * vtkMath::Pi() * i / numVerts);
    points->SetPoint(i, x, y, 0);
  }
  this->Graph->SetPoints(points);
  points->Delete();
}

void vtkCircularLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
