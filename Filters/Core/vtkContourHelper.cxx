// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkContourHelper.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkIdListCollection.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkPointData.h"
#include "vtkPolygonBuilder.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkContourHelper::vtkContourHelper(vtkIncrementalPointLocator* locator, vtkCellArray* outVerts,
  vtkCellArray* outLines, vtkCellArray* outPolys, vtkPointData* inPd, vtkCellData* inCd,
  vtkPointData* outPd, vtkCellData* outCd, bool outputTriangles)
  : Locator(locator)
  , OutVerts(outVerts)
  , OutLines(outLines)
  , OutPolys(outPolys)
  , InPd(inPd)
  , InCd(inCd)
  , OutPd(outPd)
  , OutCd(outCd)
  , OutputTriangles(outputTriangles)
{
}

//------------------------------------------------------------------------------
void vtkContourHelper::Contour(
  vtkCell* cell, double value, vtkDataArray* cellScalars, vtkIdType cellId)
{
  if (!this->OutputTriangles && cell->GetCellDimension() == 3)
  {

    // Retrieve the output triangles of the contour in temporary structures.
    this->InitializeTempContainers();
    this->TempTris->Reset();
    this->TempTriData->Reset();

    cell->Contour(value, cellScalars, this->Locator, this->OutVerts, this->OutLines, this->TempTris,
      this->InPd, this->OutPd, this->InCd, cellId, this->TempTriData);

    // Add output triangles to the PolygonBuilder in order to merge them into polygons.
    vtkPolygonBuilder polyBuilder;
    polyBuilder.Reset();

    vtkIdType cellSize = 0;
    const vtkIdType* cellVerts = nullptr;

    this->TempTris->InitTraversal();
    while (this->TempTris->GetNextCell(cellSize, cellVerts))
    {
      if (cellSize == 3)
      {
        polyBuilder.InsertTriangle(cellVerts);
      }
      else
      {
        // If for whatever reason, the cell contouring is already outputting polys.
        // Add them directly to the output.
        vtkIdType outCellId = this->OutPolys->InsertNextCell(cellSize, cellVerts);
        this->OutCd->CopyData(this->InCd, cellId,
          outCellId + this->OutVerts->GetNumberOfCells() + this->OutLines->GetNumberOfCells());
      }
    }

    // Add constructed polygons to the output.
    vtkNew<vtkIdListCollection> polyCollection;
    polyBuilder.GetPolygons(polyCollection);
    for (int polyId = 0; polyId < polyCollection->GetNumberOfItems(); ++polyId)
    {
      vtkIdList* poly = polyCollection->GetItem(polyId);
      if (poly->GetNumberOfIds() != 0)
      {
        vtkIdType outCellId = this->OutPolys->InsertNextCell(poly);
        this->OutCd->CopyData(this->InCd, cellId,
          outCellId + this->OutVerts->GetNumberOfCells() + this->OutLines->GetNumberOfCells());
      }
      poly->Delete();
    }
    polyCollection->RemoveAllItems();
  }
  else
  {
    // We do not need to merge output triangles, so we call the contour method directly.
    cell->Contour(value, cellScalars, this->Locator, this->OutVerts, this->OutLines, this->OutPolys,
      this->InPd, this->OutPd, this->InCd, cellId, this->OutCd);
  }
}

//------------------------------------------------------------------------------
void vtkContourHelper::InitializeTempContainers()
{
  if (this->TempContainersInitialized)
  {
    return;
  }
  // Per-cell contouring produces a small number of triangles in practice.
  // Using a small fixed upper bound avoids frequent reallocations while
  // keeping memory overhead negligible.
  this->TempTris->AllocateEstimate(64, 3);
  this->TempTriData->Initialize();
  this->TempContainersInitialized = true;
}
VTK_ABI_NAMESPACE_END
