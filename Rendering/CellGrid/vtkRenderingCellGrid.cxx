// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRenderingCellGrid.h"

#include "vtkCellGridRenderRequest.h"
#include "vtkDGRenderResponder.h"
#include "vtkFiltersCellGrid.h"

// Cell types
#include "vtkDGEdge.h"
#include "vtkDGHex.h"
#include "vtkDGPyr.h"
#include "vtkDGQuad.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDGVert.h"
#include "vtkDGWdg.h"

void vtkRenderingCellGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkRenderingCellGrid::RegisterCellsAndResponders()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();

  static bool once = false;
  if (!once)
  {
    once = true;

    vtkNew<vtkDGRenderResponder> dgRenderer;
    dgRenderer->ResetModsToDefault();
    auto* responders = vtkCellMetadata::GetResponders();
    // clang-format off
    responders->RegisterQueryResponder<vtkDGEdge, vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    responders->RegisterQueryResponder<vtkDGHex,  vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    responders->RegisterQueryResponder<vtkDGPyr,  vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    responders->RegisterQueryResponder<vtkDGQuad, vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    responders->RegisterQueryResponder<vtkDGTet,  vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    responders->RegisterQueryResponder<vtkDGTri,  vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    responders->RegisterQueryResponder<vtkDGVert, vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    responders->RegisterQueryResponder<vtkDGWdg,  vtkCellGridRenderRequest>(dgRenderer.GetPointer());
    // clang-format on
  }

  return true;
}
