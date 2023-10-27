// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridFeatureEdges.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"
#include "vtkInformation.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
// X = 0, Y = 1, Z = 2
constexpr unsigned int ORIENTATION_AXES_2D[3][2] = { { 1, 2 }, { 0, 2 }, { 0, 1 } };

constexpr unsigned int VON_NEUMANN_NEIGH_ID_2D[4] = { 0, 1, 3, 4 };

constexpr unsigned int EDGE_PT_IDS_2D[4][2] = { { 0, 1 }, { 0, 3 }, { 1, 2 }, { 2, 3 } };

constexpr unsigned int EDGE_PTS_IDS_3D[12][2] = { { 0, 1 }, { 2, 3 }, { 0, 2 }, { 1, 3 }, { 4, 5 },
  { 6, 7 }, { 4, 6 }, { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };

// Indices of the 3 Moore neighbors sharing one given edge for a 3D cell
constexpr unsigned int MOORE_NEIGH_IDS_3D[12][3] = { { 4, 1, 10 }, { 4, 7, 16 }, { 4, 3, 12 },
  { 4, 5, 14 }, { 22, 19, 10 }, { 22, 25, 16 }, { 22, 21, 12 }, { 22, 23, 14 }, { 12, 9, 10 },
  { 14, 11, 10 }, { 12, 15, 16 }, { 14, 17, 16 } };
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridFeatureEdges);

//------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MergePoints: " << this->MergePoints << endl;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridFeatureEdges::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridFeatureEdges::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  vtkPolyData* output = vtkPolyData::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  this->InData = input->GetCellData();
  this->OutData = output->GetCellData();
  this->OutData->CopyAllocate(this->InData);

  vtkNew<vtkPoints> outPoints;
  vtkNew<vtkCellArray> outCells;

  if (this->MergePoints)
  {
    this->Locator = vtkSmartPointer<vtkMergePoints>::New();
    this->Locator->InitPointInsertion(outPoints, input->GetBounds());
  }
  else
  {
    this->Locator = nullptr;
  }

  // Create a custom internal class depending on the dimension of the input HTG.
  switch (input->GetDimension())
  {
    case 1:
      this->OrientationAxe1D = input->GetOrientation();
      this->Process1DHTG(input, outPoints, outCells);
      break;
    case 2:
      this->OrientationAxes2D = ORIENTATION_AXES_2D[input->GetOrientation()];
      this->Process2DHTG(input, outPoints, outCells);
      break;
    case 3:
      this->Process3DHTG(input, outPoints, outCells);
      break;
    default:
      vtkErrorMacro("Incorrect dimension of input HTG: " << input->GetDimension());
      return 0;
  }

  output->SetPoints(outPoints);
  output->SetLines(outCells);

  return 1;
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::Process1DHTG(
  vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells)
{
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);

  vtkIdType hyperTreeId;
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;

  while (it.GetNextTree(hyperTreeId))
  {
    input->InitializeNonOrientedGeometryCursor(cursor, hyperTreeId);
    this->RecursivelyProcess1DHTGTree(input, outPoints, outCells, cursor);
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::Process2DHTG(
  vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells)
{
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);

  vtkIdType hyperTreeId;
  vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursor> cursor;

  while (it.GetNextTree(hyperTreeId))
  {
    input->InitializeNonOrientedVonNeumannSuperCursor(cursor, hyperTreeId);
    this->RecursivelyProcess2DHTGTree(input, outPoints, outCells, cursor);
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::Process3DHTG(
  vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells)
{
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);

  vtkIdType hyperTreeId;
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> cursor;

  while (it.GetNextTree(hyperTreeId))
  {
    input->InitializeNonOrientedMooreSuperCursor(cursor, hyperTreeId);
    this->RecursivelyProcess3DHTGTree(input, outPoints, outCells, cursor);
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::RecursivelyProcess1DHTGTree(vtkHyperTreeGrid* input,
  vtkPoints* outPoints, vtkCellArray* outCells, vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (cursor->IsLeaf() && !cursor->IsMasked())
  {
    auto points = this->Build1DCellPoints(cursor);

    // Add edge
    double pt1[3] = { 0.0 };
    double pt2[3] = { 0.0 };
    points->GetPoint(0, pt1);
    points->GetPoint(1, pt2);
    this->InsertNewEdge(pt1, pt2, outPoints, outCells, cursor->GetGlobalNodeIndex());

    return;
  }

  for (unsigned int ichild = 0; ichild < cursor->GetNumberOfChildren(); ++ichild)
  {
    if (!cursor->IsMasked())
    {
      cursor->ToChild(ichild);
      this->RecursivelyProcess1DHTGTree(input, outPoints, outCells, cursor);
      cursor->ToParent();
    }
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::RecursivelyProcess2DHTGTree(vtkHyperTreeGrid* input,
  vtkPoints* outPoints, vtkCellArray* outCells,
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor)
{
  if (cursor->IsLeaf())
  {
    auto points = this->Build2DCellPoints(cursor);

    for (unsigned int edgeId = 0; edgeId < 4; ++edgeId)
    {
      if (this->ShouldAddEdge2D(cursor, edgeId))
      {
        // Add edge
        double pt1[3] = { 0.0 };
        double pt2[3] = { 0.0 };
        points->GetPoint(EDGE_PT_IDS_2D[edgeId][0], pt1);
        points->GetPoint(EDGE_PT_IDS_2D[edgeId][1], pt2);
        this->InsertNewEdge(pt1, pt2, outPoints, outCells, cursor->GetGlobalNodeIndex());
      }
    }

    return;
  }

  for (unsigned int ichild = 0; ichild < cursor->GetNumberOfChildren(); ++ichild)
  {
    if (!cursor->IsMasked())
    {
      cursor->ToChild(ichild);
      this->RecursivelyProcess2DHTGTree(input, outPoints, outCells, cursor);
      cursor->ToParent();
    }
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::RecursivelyProcess3DHTGTree(vtkHyperTreeGrid* input,
  vtkPoints* outPoints, vtkCellArray* outCells, vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor)
{
  if (cursor->IsLeaf())
  {
    auto points = this->Build3DCellPoints(cursor);
    for (unsigned int edgeId = 0; edgeId < 12; ++edgeId)
    {
      if (this->ShouldAddEdge3D(cursor, edgeId))
      {
        // Add edge
        double pt1[3] = { 0.0 };
        double pt2[3] = { 0.0 };
        points->GetPoint(EDGE_PTS_IDS_3D[edgeId][0], pt1);
        points->GetPoint(EDGE_PTS_IDS_3D[edgeId][1], pt2);
        this->InsertNewEdge(pt1, pt2, outPoints, outCells, cursor->GetGlobalNodeIndex());
      }
    }

    return;
  }

  for (unsigned int ichild = 0; ichild < cursor->GetNumberOfChildren(); ++ichild)
  {
    if (!cursor->IsMasked())
    {
      cursor->ToChild(ichild);
      this->RecursivelyProcess3DHTGTree(input, outPoints, outCells, cursor);
      cursor->ToParent();
    }
  }
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridFeatureEdges::ShouldAddEdge2D(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor, unsigned int edgeId)
{
  auto neighborVisible = [&cursor](unsigned int neighborId) {
    return cursor->HasTree(neighborId) && !cursor->IsMasked(neighborId);
  };

  const unsigned int neighborId = VON_NEUMANN_NEIGH_ID_2D[edgeId];

  // A leaf cell generate an edge if:
  // - The cell is visible and there is no visible neighbor (of same level) sharing the edge
  // - The cell is masked and there is a visible neigbor of inferior level (leaf) sharing the edge
  return (!cursor->IsMasked() && !neighborVisible(neighborId)) ||
    (cursor->IsMasked() && neighborVisible(neighborId) &&
      cursor->GetLevel(neighborId) < cursor->GetLevel());
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridFeatureEdges::ShouldAddEdge3D(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, unsigned int edgeId)
{
  auto neighborVisible = [&cursor](unsigned int neighborId) {
    return cursor->HasTree(neighborId) && !cursor->IsMasked(neighborId);
  };

  auto neighborVisibleAndInf = [&cursor, &neighborVisible](unsigned int neighborId) {
    return neighborVisible(neighborId) && cursor->GetLevel(neighborId) < cursor->GetLevel();
  };

  auto neighborVisibleAndLeaf = [&cursor, &neighborVisible](unsigned int neighborId) {
    return neighborVisible(neighborId) && cursor->IsLeaf(neighborId);
  };

  const unsigned int* neighborIds = MOORE_NEIGH_IDS_3D[edgeId];

  // For a given level, visible cells will generate edges only shared with other visible
  // neighboring leaf cells (by definition, such cell is necessarily a cell of same level).
  // Current visible cell will generate an edge if:
  // - no other cell sharing this edge is visible (we have a "corner" edge),
  // - only the "diagonal" cell sharing the edge is visible (the edge represents the
  //   intersection between the two cells),
  // - only 2 neighbouring cells sharing are visible (we have 3 cells sharing the edge,
  //   forming a 90 degrees angle).
  bool noneVisible = !neighborVisible(neighborIds[0]) && !neighborVisible(neighborIds[1]) &&
    !neighborVisible(neighborIds[2]);
  bool onlyDiag = !neighborVisible(neighborIds[0]) && neighborVisibleAndLeaf(neighborIds[1]) &&
    !neighborVisible(neighborIds[2]);
  bool onlyTwo = (!neighborVisible(neighborIds[0]) && neighborVisibleAndLeaf(neighborIds[1]) &&
                   neighborVisibleAndLeaf(neighborIds[2])) ||
    (neighborVisibleAndLeaf(neighborIds[0]) && !neighborVisible(neighborIds[1]) &&
      neighborVisibleAndLeaf(neighborIds[2])) ||
    (neighborVisibleAndLeaf(neighborIds[0]) && neighborVisibleAndLeaf(neighborIds[1]) &&
      !neighborVisible(neighborIds[2]));
  bool visibleShouldAdd = !cursor->IsMasked() && (noneVisible || onlyDiag || onlyTwo);

  // For a given level, masked cells will generate edges only shared with other visible
  // neighboring cells of inferior level (by definition, such cell is necessarily a leaf).
  // Current masked cell will generate an edge if:
  // - only one of the neighboring cells sharing the edge is visible (we have a "corner edge"),
  // - the 2 neighboring cells sharing the edge except the "diagonal" are visible (the edge
  //   represents the intersection between the 2 neighboring cells),
  // - all neighboring cells sharing the edge are visible (we have 3 cells sharing the edge,
  //   forming a 90 degrees angle).
  bool onlyOne = (neighborVisibleAndInf(neighborIds[0]) && !neighborVisible(neighborIds[1]) &&
                   !neighborVisible(neighborIds[2])) ||
    (!neighborVisible(neighborIds[0]) && neighborVisibleAndInf(neighborIds[1]) &&
      !neighborVisible(neighborIds[2])) ||
    (!neighborVisible(neighborIds[0]) && !neighborVisible(neighborIds[1]) &&
      neighborVisibleAndInf(neighborIds[2]));
  bool twoExceptDiag = neighborVisibleAndInf(neighborIds[0]) && !neighborVisible(neighborIds[1]) &&
    neighborVisibleAndInf(neighborIds[2]);
  bool allVisible = neighborVisibleAndInf(neighborIds[0]) &&
    neighborVisibleAndInf(neighborIds[1]) && neighborVisibleAndInf(neighborIds[2]);
  bool maskedShouldAdd = cursor->IsMasked() && (onlyOne || twoExceptDiag || allVisible);

  return visibleShouldAdd || maskedShouldAdd;
}

//----------------------------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> vtkHyperTreeGridFeatureEdges::Build1DCellPoints(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(2);

  // Retrieve intercept tuple and type
  double* cellOrigin = cursor->GetOrigin();
  double* cellSize = cursor->GetSize();

  // Compute points
  std::vector<double> xyz(cellOrigin, cellOrigin + 3);
  points->SetPoint(0, xyz.data());
  xyz[this->OrientationAxe1D] += cellSize[this->OrientationAxe1D];
  points->SetPoint(1, xyz.data());

  return points;
}

//----------------------------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> vtkHyperTreeGridFeatureEdges::Build2DCellPoints(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(4);

  // Retrieve intercept tuple and type
  double* cellOrigin = cursor->GetOrigin();
  double* cellSize = cursor->GetSize();

  // Compute points
  std::vector<double> xyz(cellOrigin, cellOrigin + 3);
  points->SetPoint(0, xyz.data());
  xyz[this->OrientationAxes2D[0]] += cellSize[this->OrientationAxes2D[0]];
  points->SetPoint(1, xyz.data());
  xyz[this->OrientationAxes2D[1]] += cellSize[this->OrientationAxes2D[1]];
  points->SetPoint(2, xyz.data());
  xyz[this->OrientationAxes2D[0]] = cellOrigin[this->OrientationAxes2D[0]];
  points->SetPoint(3, xyz.data());

  return points;
}

//----------------------------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> vtkHyperTreeGridFeatureEdges::Build3DCellPoints(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints(8);

  // Retrieve intercept tuple and type
  double* cellOrigin = cursor->GetOrigin();
  double* cellSize = cursor->GetSize();

  for (int ptId = 0; ptId < 8; ptId++)
  {
    double pt[3] = { cellOrigin[0] + (ptId & 1) * cellSize[0],
      cellOrigin[1] + ((ptId >> 1) & 1) * cellSize[1],
      cellOrigin[2] + ((ptId >> 2) & 1) * cellSize[2] };
    points->SetPoint(ptId, pt);
  }

  return points;
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridFeatureEdges::InsertNewEdge(
  double* edgePt1, double* edgePt2, vtkPoints* outPoints, vtkCellArray* outCells, vtkIdType cellId)
{
  vtkIdType ptId1 = 0;
  vtkIdType ptId2 = 0;

  if (this->Locator)
  {
    this->Locator->InsertUniquePoint(edgePt1, ptId1);
    this->Locator->InsertUniquePoint(edgePt2, ptId2);
  }
  else
  {
    ptId1 = outPoints->InsertNextPoint(edgePt1);
    ptId2 = outPoints->InsertNextPoint(edgePt2);
  }

  std::vector<vtkIdType> outPointIds = { ptId1, ptId2 };

  vtkIdType outputCellIndex = outCells->InsertNextCell(outPointIds.size(), outPointIds.data());

  this->OutData->CopyData(this->InData, cellId, outputCellIndex);
}

VTK_ABI_NAMESPACE_END
