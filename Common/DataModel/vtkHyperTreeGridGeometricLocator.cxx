/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeoemtricLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometricLocator.h"

#include "vtkGenericCell.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridGeometricLocator);

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometricLocator::SetHTG(vtkHyperTreeGrid* cand)
{
  this->Superclass::SetHTG(cand);
  const unsigned int bf = this->HTG->GetBranchFactor();
  this->Bins1D.resize(bf - 1);
  for (unsigned int b = 0; b < bf - 1; b++)
  {
    this->Bins1D[b] = static_cast<double>(b + 1) / static_cast<double>(bf);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::Search(const double point[3])
{
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  return this->Search(point, cursor);
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::Search(
  const double point[3], vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Check bounds
  std::array<unsigned int, 3> bin{};
  const int dim = this->HTG->GetDimension();
  bin[0] =
    this->HTG->FindDichotomicX(point[0]); // This and subsequent calls expected to be thread safe
  if (dim > 1)
  {
    bin[1] = this->HTG->FindDichotomicY(point[1]);
  }
  if (dim > 2)
  {
    bin[2] = this->HTG->FindDichotomicZ(point[2]);
  }
  const unsigned int* cellDims = this->HTG->GetCellDims();
  for (int i = 0; i < dim; i++)
  {
    if (bin[i] >= cellDims[i])
    {
      return -1;
    }
  }
  // Get the index of the tree it's in
  vtkIdType treeId;
  this->HTG->GetIndexFromLevelZeroCoordinates(treeId, bin[0], bin[1], bin[2]);
  // Create cursor for looking for the point
  this->HTG->InitializeNonOrientedGeometryCursor(cursor, treeId, false);
  // recurse
  return this->RecursiveSearch(cursor, point);
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::RecursiveSearch(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, const double pt[3])
{
  if (cursor->IsMasked() ||
    (this->HTG->HasAnyGhostCells() &&
      this->HTG->GetGhostCells()->GetTuple1(cursor->GetGlobalNodeIndex())))
  {
    return -1;
  }
  if (this->CheckLeafOrChildrenMasked(cursor))
  {
    return cursor->GetGlobalNodeIndex();
  }
  const unsigned int dim = this->HTG->GetDimension();
  const unsigned int bf = this->HTG->GetBranchFactor();
  const double* origin = cursor->GetOrigin();
  if (origin == nullptr)
  {
    vtkErrorMacro("Cursor has no origin");
    return false;
  }
  const double* size = cursor->GetSize();
  if (size == nullptr)
  {
    vtkErrorMacro("Cursor has no size");
    return false;
  }
  double normalizedPt[3];
  std::copy(pt, pt + 3, normalizedPt);
  for (unsigned int d = 0; d < dim; d++)
  {
    normalizedPt[d] -= origin[d];
    normalizedPt[d] /= size[d];
  }
  vtkIdType childIndex = this->FindChildIndex(dim, bf, normalizedPt);
  cursor->ToChild(childIndex);
  return this->RecursiveSearch(cursor, pt);
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::FindCell(const double point[3],
  const double vtkNotUsed(tol), vtkGenericCell* cell, int& subId, double pcoords[3],
  double* weights)
{
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  vtkIdType globId = this->Search(point, cursor);
  if (globId < 0)
  {
    return globId;
  }
  if (!this->ConstructCell(cursor, cell))
  {
    vtkErrorMacro("Failed to construct cell");
    return -1;
  }
  double dist2 = 0.0;
  if (cell->EvaluatePosition(point, nullptr, subId, pcoords, dist2, weights) != 1)
  {
    vtkErrorMacro("Unable to evaluate position in cell");
    return -1;
  }
  return globId;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGeometricLocator::IntersectWithLine(const double p0[3], const double p1[3],
  const double tol, double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId,
  vtkGenericCell* cell)
{
  // initialialize outputs
  cellId = -1;
  t = -1.0;
  subId = 0;
  std::fill(x, x + 3, 0.0);
  std::fill(pcoords, pcoords + 3, 0.0);
  // setup calculation
  unsigned int dim = this->HTG->GetDimension();
  std::vector<double> sizes(dim, 0.0);
  std::vector<double> origin(dim, 0.0);
  this->GetZeroLevelOriginAndSize(origin.data(), sizes.data());
  // find intersection point with entire grid
  auto checkIsInCell = [dim](const double* origin_, const double* sizes_, const double* query) {
    bool isIn = true;
    for (unsigned int d = 0; d < dim; d++)
    {
      isIn &= ((query[d] - origin_[d]) < sizes_[d]);
    }
    return isIn;
  };
  bool p0InCell = checkIsInCell(origin.data(), sizes.data(), p0);
  if (!p0InCell)
  {
    if (!this->ConstructCell(origin.data(), sizes.data(), cell))
    {
      vtkErrorMacro("Could not construct cell");
      return -1;
    }
    // line does not intersect grid at all
    if (cell->IntersectWithLine(p0, p1, tol, t, x, pcoords, subId) == 0)
    {
      return 0;
    }
    // run FindCell on the intersection point + epsilon so we're sure we're in the cell
    std::vector<double> tangent(3, 0.0);
    vtkMath::Subtract(p1, p0, tangent.data());
    vtkMath::Normalize(tangent.data());
    double epsilon = 0.01 *
      (vtkMath::Norm(sizes.data()) /
        std::pow(this->HTG->GetBranchFactor(), this->HTG->GetNumberOfLevels()));
    vtkMath::MultiplyScalar(tangent.data(), epsilon);
    vtkMath::Add(x, tangent.data(), x);
  }
  else
  {
    std::copy(p0, p0 + dim, x);
  }
  {
    std::vector<double> locWeights(std::pow(2, dim), 0.0);
    std::vector<double> locPCoords(3, 0.0);
    cellId = this->FindCell(x, tol, cell, subId, locPCoords.data(),
      locWeights.data()); // potential speed-up here by automatically looking for the cell but also
                          // could be loss
  }
  if (cellId >= 0)
  {
    return cell->IntersectWithLine(p0, p1, tol, t, x, pcoords, subId);
  }
  else // could not find cell which must be masked
  {
    // iterate over trees
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
    bool rootFound = false;
    {
      // WARNING
      // A choice was made here to use buffers of size nTrees here to simplify the logic in the for
      // loop. In cases were nTrees is large these buffers could possibly become too memory
      // intensive. If this becomes an issue, the for loop should be refactored to use smaller
      // buffers.
      unsigned int nTrees = this->HTG->GetNumberOfNonEmptyTrees();
      std::vector<bool> intersects(nTrees, false);
      std::vector<int> subIds(nTrees, 0);
      std::vector<double> xs(nTrees * 3, 0.0);
      std::vector<double> locPCoords(nTrees * 3, 0.0);
      std::vector<double> ts(nTrees, std::numeric_limits<double>::infinity());
      for (unsigned int iT = 0; iT < nTrees; iT++)
      {
        this->HTG->InitializeNonOrientedGeometryCursor(cursor, iT, false);
        unsigned int offset = 3 * iT;
        if (!(this->ConstructCell(cursor, cell)))
        {
          vtkErrorMacro("Could not construct cell in line intersection tree iteration");
          return -1;
        }
        intersects[iT] = (cell->IntersectWithLine(p0, p1, tol, ts[iT], xs.data() + offset,
                            locPCoords.data() + offset, subIds[iT]) != 0);
      }
      unsigned int rightTree = std::distance(ts.begin(), std::min_element(ts.begin(), ts.end()));
      if (intersects[rightTree] && (ts[rightTree] >= 0.0) && (ts[rightTree] <= 1.0))
      {
        t = ts[rightTree];
        subId = subIds[rightTree];
        unsigned int offset = 3 * rightTree;
        std::copy(xs.begin() + offset, xs.begin() + offset + 3, x);
        std::copy(locPCoords.begin() + offset, locPCoords.begin() + offset + 3, pcoords);
        this->HTG->InitializeNonOrientedGeometryCursor(cursor, rightTree, false);
        if (!(this->ConstructCell(cursor, cell)))
        {
          vtkErrorMacro("Could not construct cell before line intersection recursion");
          return -1;
        }
        rootFound = true;
      }
      else
      {
        rootFound = false;
      }
    }
    if (rootFound)
    {
      cellId =
        this->RecurseSingleIntersectWithLine(p0, p1, tol, cursor, cell, t, subId, x, pcoords);
    }

    return (cellId >= 0);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::RecurseSingleIntersectWithLine(const double p0[3],
  const double p1[3], const double tol, vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
  vtkGenericCell* cell, double& t, int& subId, double x[3], double pcoords[3]) const
{
  if (this->CheckLeafOrChildrenMasked(cursor))
  {
    return cursor->GetGlobalNodeIndex();
  }
  {
    unsigned int nChildren = cursor->GetNumberOfChildren();
    std::vector<bool> intersects(nChildren, false);
    std::vector<int> subIds(nChildren, 0);
    std::vector<double> xs(nChildren * 3, 0.0);
    std::vector<double> locPCoords(nChildren * 3, 0.0);
    std::vector<double> ts(nChildren, std::numeric_limits<double>::infinity());
    for (unsigned int iC = 0; iC < nChildren; iC++)
    {
      cursor->ToChild(iC);
      if (cursor->IsMasked() ||
        (this->HTG->HasAnyGhostCells() &&
          this->HTG->GetGhostCells()->GetTuple1(cursor->GetGlobalNodeIndex())))
      {
        cursor->ToParent();
        continue;
      }
      unsigned int offset = 3 * iC;
      if (!(this->ConstructCell(cursor, cell)))
      {
        vtkErrorMacro("Could not construct cell in line intersection recursion child iteration");
        return -1;
      }
      intersects[iC] = (cell->IntersectWithLine(p0, p1, tol, ts[iC], xs.data() + offset,
                          locPCoords.data() + offset, subIds[iC]) != 0);
      cursor->ToParent();
    }
    unsigned int rightC = std::distance(ts.begin(), std::min_element(ts.begin(), ts.end()));
    if ((intersects[rightC]) && (ts[rightC] >= 0.0) && (ts[rightC] <= 1.0))
    {
      t = ts[rightC];
      subId = subIds[rightC];
      unsigned int offset = 3 * rightC;
      std::copy(xs.begin() + offset, xs.begin() + offset + 3, x);
      std::copy(locPCoords.begin() + offset, locPCoords.begin() + offset + 3, pcoords);
      cursor->ToChild(rightC);
      if (!(this->ConstructCell(cursor, cell)))
      {
        vtkErrorMacro("Could not construct cell before line intersection recursion");
        return -1;
      }
    }
    else
    {
      return -1;
    }
  }
  return this->RecurseSingleIntersectWithLine(p0, p1, tol, cursor, cell, t, subId, x, pcoords);
}

//------------------------------------------------------------------------------

struct vtkHyperTreeGridGeometricLocator::RecurseTreesFunctor
{
  vtkHyperTreeGridGeometricLocator* HTGLoc;
  const double* Pt0;
  const double* Pt1;
  double Tol;
  std::vector<double>* GlobTs;
  vtkPoints* GlobPts;
  vtkIdList* GlobCellIds;
  struct LocalData
  {
    std::vector<double> LocTs;
    vtkSmartPointer<vtkPoints> LocPts;
    vtkSmartPointer<vtkIdList> LocCellIds;
  };
  vtkSMPThreadLocal<LocalData> Loc;

  RecurseTreesFunctor(vtkHyperTreeGridGeometricLocator* htgloc, const double* pt0,
    const double* pt1, double tol, std::vector<double>* ts, vtkPoints* pts, vtkIdList* cIds)
  {
    this->HTGLoc = htgloc;
    this->Pt0 = pt0;
    this->Pt1 = pt1;
    this->Tol = tol;
    this->GlobTs = ts;
    this->GlobPts = pts;
    this->GlobCellIds = cIds;
  }

  void Initialize()
  {
    this->Loc.Local().LocTs = std::vector<double>();
    this->Loc.Local().LocPts = vtk::TakeSmartPointer(vtkPoints::New());
    this->Loc.Local().LocPts->Initialize();
    this->Loc.Local().LocCellIds = vtk::TakeSmartPointer(vtkIdList::New());
    this->Loc.Local().LocCellIds->Initialize();
  }

  void operator()(vtkIdType first, vtkIdType last)
  {
    vtkNew<vtkGenericCell> locCell;
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
    for (int iT = first; iT < last; iT++)
    {
      this->HTGLoc->GetHTG()->InitializeNonOrientedGeometryCursor(cursor, iT, false);
      this->HTGLoc->RecurseAllIntersectsWithLine(this->Pt0, this->Pt1, this->Tol, cursor,
        &(this->Loc.Local().LocTs), this->Loc.Local().LocPts, this->Loc.Local().LocCellIds,
        locCell);
    }
  }

  void Reduce()
  {
    unsigned int nPoints = 0;
    unsigned int nCells = 0;
    size_t nT = 0;
    for (auto it = this->Loc.begin(); it != this->Loc.end(); it++)
    {
      nT += it->LocTs.size();
      nPoints += it->LocPts->GetNumberOfPoints();
      nCells += it->LocCellIds->GetNumberOfIds();
    }
    unsigned int initPoints = this->GlobPts->GetNumberOfPoints();
    unsigned int initCells = this->GlobCellIds->GetNumberOfIds();
    this->GlobTs->resize(nT);
    this->GlobPts->Resize(initPoints + nPoints);
    this->GlobCellIds->Resize(initCells + nCells);
    nT = 0;
    nPoints = initPoints;
    nCells = initCells;
    for (auto it = this->Loc.begin(); it != this->Loc.end(); it++)
    {
      std::copy(it->LocTs.begin(), it->LocTs.end(), this->GlobTs->begin() + nT);
      nT += it->LocTs.size();
      it->LocTs.resize(0);
      this->GlobPts->InsertPoints(nPoints, it->LocPts->GetNumberOfPoints(), 0, it->LocPts);
      nPoints += it->LocPts->GetNumberOfPoints();
      if (it->LocPts->Resize(0) == 0)
      {
        vtkErrorWithObjectMacro(nullptr, << "Could not release local point memory.");
      }
      for (unsigned int i = 0; i < it->LocCellIds->GetNumberOfIds(); i++)
      {
        this->GlobCellIds->InsertId(nCells + i, it->LocCellIds->GetId(i));
      }
      nCells += it->LocCellIds->GetNumberOfIds();
      it->LocCellIds->SetNumberOfIds(0);
    }
  }
};

//------------------------------------------------------------------------------
int vtkHyperTreeGridGeometricLocator::IntersectWithLine(const double p0[3], const double p1[3],
  const double tol, vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  // do checks
  if (!points || !cellIds)
  {
    vtkErrorMacro("The points or cellIds are nullptr");
    return 0;
  }
  // setup computation
  unsigned int nInitialPoints = points->GetNumberOfPoints();
  std::vector<double> ts;
  double tBuff = 0.0;
  int subId = 0;
  std::vector<double> pcoords(3, 0.0);
  std::vector<double> x(3, 0.0);
  {
    std::vector<double> origin(3, 0.0);
    std::vector<double> sizes(3, 0.0);
    this->GetZeroLevelOriginAndSize(origin.data(), sizes.data());
    if (!(this->ConstructCell(origin.data(), sizes.data(), cell)))
    {
      vtkErrorMacro("Unable to construct level 0 cell");
      return 0;
    }
    if (cell->IntersectWithLine(p0, p1, tol, tBuff, x.data(), pcoords.data(), subId) == 0)
    {
      return 0;
    }
  }

  // iterate over trees
  RecurseTreesFunctor thisFunctor(this, p0, p1, tol, &ts, points, cellIds);
  vtkSMPTools::For(0, this->HTG->GetNumberOfNonEmptyTrees(), thisFunctor);

  // sort based on parametric coords
  {
    std::vector<int> map(ts.size(), 0);
    this->GetSortingMap(ts, &map);
    vtkNew<vtkPoints> buffPoints;
    buffPoints->SetNumberOfPoints(points->GetNumberOfPoints());
    vtkNew<vtkIdList> buffCellIds;
    buffCellIds->SetNumberOfIds(cellIds->GetNumberOfIds());
    std::vector<double> buffTs(ts.size(), 0.0);
    for (unsigned int counter = 0; counter < map.size(); counter++)
    {
      buffPoints->InsertPoint(counter, points->GetPoint(map[counter]));
      buffCellIds->SetId(counter, cellIds->GetId(map[counter]));
      buffTs[counter] = ts[map[counter]];
    }
    points->ShallowCopy(buffPoints);
    cellIds->DeepCopy(buffCellIds);
    std::swap(buffTs, ts);
  }

  return !(points->GetNumberOfPoints() == nInitialPoints);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometricLocator::RecurseAllIntersectsWithLine(const double p0[3],
  const double p1[3], const double tol, vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
  std::vector<double>* ts, vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell) const
{
  if (cursor->IsMasked() ||
    (this->HTG->HasAnyGhostCells() &&
      this->HTG->GetGhostCells()->GetTuple1(cursor->GetGlobalNodeIndex())))
  {
    return;
  }
  if (!this->ConstructCell(cursor, cell))
  {
    vtkErrorMacro("Could not construct cell in all intersections with line recursion");
    return;
  }
  {
    double t = -1.0;
    std::vector<double> x(3, 0.0);
    std::vector<double> pcoords(3, 0.0);
    int subId = 0;
    if (cell->IntersectWithLine(p0, p1, tol, t, x.data(), pcoords.data(), subId) == 0)
    {
      return;
    }
    if (this->CheckLeafOrChildrenMasked(cursor))
    {
      ts->emplace_back(t);
      points->InsertNextPoint(x.data());
      cellIds->InsertNextId(cursor->GetGlobalNodeIndex());
      return;
    }
  }
  unsigned int nChildren = cursor->GetNumberOfChildren();
  for (unsigned int iC = 0; iC < nChildren; iC++)
  {
    cursor->ToChild(iC);
    this->RecurseAllIntersectsWithLine(p0, p1, tol, cursor, ts, points, cellIds, cell);
    cursor->ToParent();
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::FindChildIndex(
  const unsigned int dim, const unsigned int bf, const double normalizedPt[3]) const
{
  std::vector<int> binPt(dim, -1);
  for (unsigned int d = 0; d < dim; d++)
  {
    binPt[d] = std::distance(
      Bins1D.begin(), std::upper_bound(Bins1D.begin(), Bins1D.end(), normalizedPt[d]));
  }
  // convert tuple index to single index
  vtkIdType childIndex = 0;
  for (unsigned int d = 0; d < dim; d++)
  {
    childIndex *= bf;
    childIndex += binPt[dim - d - 1];
  }
  return childIndex;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometricLocator::CheckLeafOrChildrenMasked(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor) const
{
  if (cursor->IsLeaf())
  {
    return true;
  }
  bool allMasked = false;
  // could optimize here by using a smaller cursor for checking masks of children
  for (unsigned int iChild = 0; iChild < cursor->GetNumberOfChildren(); iChild++)
  {
    cursor->ToChild(iChild);
    allMasked = (cursor->IsMasked()) ||
      (this->HTG->HasAnyGhostCells() &&
        this->HTG->GetGhostCells()->GetTuple1(cursor->GetGlobalNodeIndex()));
    cursor->ToParent();
    if (!allMasked)
    {
      break;
    }
  }
  return allMasked;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometricLocator::ConstructCell(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkGenericCell* cell) const
{
  const double* origin = cursor->GetOrigin();
  const double* size = cursor->GetSize();

  return this->ConstructCell(origin, size, cell);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometricLocator::ConstructCell(
  const double* origin, const double* size, vtkGenericCell* cell) const
{
  if (cell == nullptr || origin == nullptr || size == nullptr)
  {
    vtkErrorMacro("Cell, origin or size that was passed is nullptr");
    return false;
  }

  const unsigned int dim = this->HTG->GetDimension();
  switch (dim)
  {
    case (1):
      cell->SetCellTypeToLine();
      break;
    case (2):
      cell->SetCellTypeToPixel();
      break;
    case (3):
      cell->SetCellTypeToVoxel();
      break;
    default:
      vtkErrorMacro("Wrong HyperTreeGrid dimension");
      return false;
  }

  unsigned int nPoints = std::pow(2, dim);
  for (unsigned int iP = 0; iP < nPoints; iP++)
  {
    cell->PointIds->SetId(iP, iP);
  }

  auto cubePoint = [dim, origin, size](std::bitset<3>& pos, std::vector<double>* cubePt) {
    for (unsigned int d = 0; d < dim; d++)
    {
      cubePt->at(d) = origin[d] + pos[d] * size[d];
    }
  };
  std::vector<double> pt(3, 0.0);
  std::vector<std::bitset<3>> positions(8);
  positions[0] = 0; // 000
  positions[1] = 1; // 001 -> +x
  positions[2] = 2; // 010 -> +y
  positions[3] = 3; // 011 -> +xy
  positions[4] = 4; // 100 -> +z
  positions[5] = 5; // 101 -> +zx
  positions[6] = 6; // 110 -> +zy
  positions[7] = 7; // 111 -> +zxy
  for (unsigned int iP = 0; iP < nPoints; iP++)
  {
    cubePoint(positions[iP], &pt);
    cell->Points->SetPoint(iP, pt.data());
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometricLocator::GetZeroLevelOriginAndSize(
  double* origin, double* sizes) const
{
  unsigned int dim = this->HTG->GetDimension();
  auto getOriginSize = [](vtkDataArray* compArray, double& ori, double& s) {
    ori = compArray->GetComponent(0, 0);
    s = compArray->GetComponent(compArray->GetNumberOfTuples() - 1, 0) - ori;
  };
  getOriginSize(this->HTG->GetXCoordinates(), origin[0], sizes[0]);
  if (dim > 1)
  {
    getOriginSize(this->HTG->GetYCoordinates(), origin[1], sizes[1]);
  }
  if (dim > 2)
  {
    getOriginSize(this->HTG->GetZCoordinates(), origin[2], sizes[2]);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometricLocator::GetSortingMap(
  const std::vector<double>& other, std::vector<int>* map) const
{
  map->resize(other.size(), 0);
  std::iota(map->begin(), map->end(), 0);
  auto specialCompare = [&](int i0, int i1) { return (other[i0] < other[i1]); };
  std::sort(map->begin(), map->end(), specialCompare);
}
