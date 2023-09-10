// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridElevationQuery.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridElevationQuery);

void vtkCellGridElevationQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCellGridElevationQuery::Initialize()
{
  this->Elevation->Initialize(this->Name, "DG HGRAD C0 I1"_token, "ℝ¹"_token, 1);
}

void vtkCellGridElevationQuery::Finalize()
{
  // this->Elevation->SetColormap(…);
}

VTK_ABI_NAMESPACE_END
