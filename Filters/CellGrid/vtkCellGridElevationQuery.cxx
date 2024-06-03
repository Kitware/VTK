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

bool vtkCellGridElevationQuery::Initialize()
{
  bool ok = this->Superclass::Initialize();
  this->Elevation->Initialize(this->Name, "ℝ"_token, 1);
  return ok;
}

bool vtkCellGridElevationQuery::Finalize()
{
  // this->Elevation->SetColormap(…);
  return true;
}

VTK_ABI_NAMESPACE_END
