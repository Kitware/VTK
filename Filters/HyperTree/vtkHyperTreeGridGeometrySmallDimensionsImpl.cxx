// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometrySmallDimensionsImpl.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkPoints.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometrySmallDimensionsImpl::vtkHyperTreeGridGeometrySmallDimensionsImpl(
  vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells,
  vtkDataSetAttributes* inCellDataAttributes, vtkDataSetAttributes* outCellDataAttributes,
  bool passThroughCellIds, const std::string& originalCellIdArrayName)
  : vtkHyperTreeGridGeometryImpl(input, outPoints, outCells, inCellDataAttributes,
      outCellDataAttributes, passThroughCellIds, originalCellIdArrayName)
{
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometrySmallDimensionsImpl::GenerateGeometry()
{
  // initialize iterator on HypterTrees (HT) of an HyperTreeGrid (HTG)
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  this->Input->InitializeTreeIterator(it);
  // index of current HT
  vtkIdType hyperTreeId;
  // non oriented geometry cursor describe one cell on HT
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;

  // traversal on HTG for describe a current HT
  while (it.GetNextTree(hyperTreeId))
  {
    // initialize cursor on first cell (root)  of current HT
    this->Input->InitializeNonOrientedGeometryCursor(cursor, hyperTreeId);

    // traversal recursively
    this->RecursivelyProcessTree(cursor);
  } // it
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometrySmallDimensionsImpl::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (this->IsMaskedOrGhost(cursor->GetGlobalNodeIndex()))
  {
    return;
  }
  // case leaf cell
  if (cursor->IsLeaf())
  {
    if (this->HasInterface)
    {
      this->ProcessLeafCellWithInterface(cursor);
    }
    else
    {
      this->ProcessLeafCellWithoutInterface(cursor);
    }
    return;
  }
  // case coarse cell
  for (unsigned int ichild = 0; ichild < cursor->GetNumberOfChildren(); ++ichild)
  {
    cursor->ToChild(ichild);
    this->RecursivelyProcessTree(cursor);
    cursor->ToParent();
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometrySmallDimensionsImpl::ProcessLeafCellWithoutInterface(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // As no interface cross the cell, we simply build it and add it as-it to the output surface.
  this->BuildCellPoints(cursor);

  std::vector<vtkIdType> outputIndexPoints;
  for (int ptId = 0; ptId < this->CellPoints->GetNumberOfPoints(); ptId++)
  {
    auto outPtId = this->OutPoints->InsertNextPoint(this->CellPoints->GetPoint(ptId));
    outputIndexPoints.emplace_back(outPtId);
  }

  this->CreateNewCellAndCopyData(outputIndexPoints, cursor->GetGlobalNodeIndex());
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometrySmallDimensionsImpl::ProcessLeafCellWithInterface(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (!this->ProbeForCellInterface(cursor->GetGlobalNodeIndex(), false))
  { // case type >= 2, pure cell
    this->ProcessLeafCellWithoutInterface(cursor);
    return;
  }

  this->BuildCellPoints(cursor);
  unsigned int nbPts = this->CellPoints->GetNumberOfPoints();

  // compute distance point to interface
  std::vector<double> scalarsInterfaceA(nbPts);
  std::vector<double> scalarsInterfaceB(nbPts);
  for (unsigned int iPt = 0; iPt < nbPts; ++iPt)
  {
    // Retrieve vertex coordinates
    double* xyz = this->CellPoints->GetPoint(iPt);

    // Set face scalars
    if (this->CellInterfaceType != 1.)
    {
      scalarsInterfaceA[iPt] = this->ComputeDistanceToInterfaceA(xyz);
    }
    if (this->CellInterfaceType != -1.)
    {
      scalarsInterfaceB[iPt] = this->ComputeDistanceToInterfaceB(xyz);
    }
  }

  if (!this->CellInterfaceType)
  { // case intermediate interface with A and B
    this->ProcessLeafCellWithDoubleInterface(cursor, scalarsInterfaceA, scalarsInterfaceB);
  }
  else
  {
    // case type == 1 just "right" interface with B
    // case type == -1, case just "left" interface with A
    double dist = (this->CellInterfaceType == 1 ? -1.0 : 1.0);
    auto scalarsInterface = (this->CellInterfaceType == 1 ? scalarsInterfaceB : scalarsInterfaceA);
    this->ProcessLeafCellWithOneInterface(cursor, dist, scalarsInterface);
  }
}

VTK_ABI_NAMESPACE_END
