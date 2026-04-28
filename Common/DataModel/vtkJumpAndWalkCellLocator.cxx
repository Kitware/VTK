// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkJumpAndWalkCellLocator.h"

#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkStaticPointLocator.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkJumpAndWalkCellLocator);

//------------------------------------------------------------------------------
vtkJumpAndWalkCellLocator::vtkJumpAndWalkCellLocator()
{
  this->CacheCellBounds = true;
  this->UseExistingSearchStructure = 0;
}

//------------------------------------------------------------------------------
vtkJumpAndWalkCellLocator::~vtkJumpAndWalkCellLocator() = default;

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->PointLocator)
  {
    os << indent << "PointLocator: " << this->PointLocator << "\n";
    this->PointLocator->PrintSelf(os, indent);
  }
  else
  {
    os << indent << "PointLocator: (none)\n";
  }
}

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->PointLocator, "PointLocator");
}

//------------------------------------------------------------------------------
struct vtkJumpAndWalkCellLocator::ThreadLocalData
{
  std::unordered_set<vtkIdType> VisitedCellIds;
  vtkSmartPointer<vtkIdList> PointIds;
  vtkSmartPointer<vtkIdList> Neighbors;
  vtkSmartPointer<vtkIdList> CellIds;
  vtkSmartPointer<vtkIdList> NearPointIds;
  std::vector<double> Weights;

  ThreadLocalData()
  {
    this->PointIds = vtkSmartPointer<vtkIdList>::New();
    this->Neighbors = vtkSmartPointer<vtkIdList>::New();
    this->CellIds = vtkSmartPointer<vtkIdList>::New();
    this->NearPointIds = vtkSmartPointer<vtkIdList>::New();
  }

  ThreadLocalData(const ThreadLocalData& other)
  {
    this->VisitedCellIds = other.VisitedCellIds;
    this->PointIds = vtkSmartPointer<vtkIdList>::New();
    this->Neighbors = vtkSmartPointer<vtkIdList>::New();
    this->CellIds = vtkSmartPointer<vtkIdList>::New();
    this->NearPointIds = vtkSmartPointer<vtkIdList>::New();
    this->Weights = other.Weights;
  }

  void Swap(ThreadLocalData& other)
  {
    std::swap(this->VisitedCellIds, other.VisitedCellIds);
    std::swap(this->PointIds, other.PointIds);
    std::swap(this->Neighbors, other.Neighbors);
    std::swap(this->CellIds, other.CellIds);
    std::swap(this->NearPointIds, other.NearPointIds);
    std::swap(this->Weights, other.Weights);
  }

  ThreadLocalData& operator=(const ThreadLocalData& other)
  {
    if (this != &other)
    {
      ThreadLocalData tmp = ThreadLocalData(other);
      this->Swap(tmp);
    }
    return *this;
  }
};

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::FreeSearchStructure()
{
  for (auto& tl : this->TLData)
  {
    std::unordered_set<vtkIdType>().swap(tl.VisitedCellIds);
    tl.PointIds->Reset();
    tl.PointIds->Squeeze();
    tl.Neighbors->Reset();
    tl.Neighbors->Squeeze();
    tl.CellIds->Reset();
    tl.CellIds->Squeeze();
    tl.NearPointIds->Reset();
    tl.NearPointIds->Squeeze();
    tl.Weights.clear();
    tl.Weights.shrink_to_fit();
  }
}

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::BuildLocator()
{
  // if a search structure already exists
  if (this->PointLocator)
  {
    // don't rebuild if UseExistingSearchStructure is ON
    if (this->UseExistingSearchStructure)
    {
      return;
    }
    // don't rebuild if build time is newer than modified and dataset modified time
    if (this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
    {
      return;
    }
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::BuildLocatorInternal()
{
  if (!this->DataSet || this->DataSet->GetNumberOfCells() < 1)
  {
    vtkErrorMacro(<< "No cells to build");
    return;
  }

  this->FreeSearchStructure();

  vtkBoundingBox bbox(this->DataSet->GetBounds());
  bbox.Inflate(); // make sure non-zero volume
  bbox.GetBounds(this->Bounds);
  this->ComputeCellBounds();

  if (!this->PointLocator)
  {
    vtkNew<vtkStaticPointLocator> newPointLocator;
    this->SetPointLocator(newPointLocator);
  }
  this->PointLocator->SetDataSet(this->DataSet);
  this->PointLocator->BuildLocator();

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  if (this->PointLocator)
  {
    this->PointLocator->GenerateRepresentation(level, pd);
    pd->GetPoints()->Modified();
    pd->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkJumpAndWalkCellLocator::ShallowCopy(vtkAbstractCellLocator* locator)
{
  auto cellLocator = vtkJumpAndWalkCellLocator::SafeDownCast(locator);
  if (!cellLocator)
  {
    vtkErrorMacro("Cannot cast " << locator->GetClassName() << " to " << this->GetClassName());
    return;
  }
  // we only copy what's actually used by vtkJumpAndWalkCellLocator
  this->SetPointLocator(cellLocator->GetPointLocator());
  std::copy_n(cellLocator->Bounds, 6, this->Bounds);
  this->NumberOfClosestPoints = cellLocator->NumberOfClosestPoints;
  this->CacheCellBounds = cellLocator->CacheCellBounds;
  this->CellBoundsSharedPtr = cellLocator->CellBoundsSharedPtr; // This is important
  this->CellBounds = this->CellBoundsSharedPtr.get() ? this->CellBoundsSharedPtr->data() : nullptr;
  this->UseExistingSearchStructure = cellLocator->UseExistingSearchStructure;
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
vtkIdType vtkJumpAndWalkCellLocator::FindCellWalk(vtkIdList* cellIds, double x[3], double tol2,
  vtkGenericCell* genCell, int& subId, double pcoords[3], double* weights,
  std::unordered_set<vtkIdType>& visitedCellIds, vtkIdList* ptIds, vtkIdList* neighbors)
{
  static constexpr int VTK_MAX_WALK = 12;
  const double tol = std::sqrt(tol2);
  double closestPoint[3];
  double dist2;
  for (vtkIdType i = 0; i < cellIds->GetNumberOfIds(); ++i)
  {
    vtkIdType cellId = cellIds->GetId(i);
    for (int walk = 0; walk < VTK_MAX_WALK; walk++)
    {
      // Check to see if we already visited this cell.
      const bool inserted = visitedCellIds.insert(cellId).second;
      if (!inserted)
      {
        break;
      }

      // Get information for the cell.
      this->DataSet->GetCell(cellId, genCell);
      if (this->InsideCellBounds(x, cellId, tol))
      {
        // Check to see if the current cell contains the point.
        const int stat = genCell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
        if (stat != -1 && dist2 <= tol2)
        {
          return cellId;
        }
      }

      // This is not the right cell.  Find the next one.
      genCell->CellBoundary(subId, pcoords, ptIds);
      this->DataSet->GetCellNeighbors(cellId, ptIds, neighbors);
      // If there is no next one, exit.
      if (neighbors->GetNumberOfIds() < 1)
      {
        break;
      }
      // Set the next cell as the current one and iterate.
      cellId = neighbors->GetId(0);
    }
  }
  // Could not find a cell.
  return -1;
}

//------------------------------------------------------------------------------
vtkIdType vtkJumpAndWalkCellLocator::FindCell(
  double x[3], double tol2, vtkGenericCell* genCell, int& subId, double pcoords[3], double* weights)
{
  // Make sure locator has been built successfully
  this->BuildLocator();
  if (!this->PointLocator)
  {
    return -1;
  }
  // Check to see if the point is within the bounds of the data.
  // This is not a strict check, but it is fast.
  const double tol = std::sqrt(tol2);
  if (!vtkAbstractCellLocator::IsInBounds(this->Bounds, x, tol))
  {
    return -1;
  }

  ThreadLocalData& tl = this->TLData.Local();
  std::unordered_set<vtkIdType>& visibleCellIds = tl.VisitedCellIds;
  vtkSmartPointer<vtkIdList>& pointIds = tl.PointIds;
  vtkSmartPointer<vtkIdList>& neighbors = tl.Neighbors;
  vtkSmartPointer<vtkIdList>& cellIds = tl.CellIds;
  vtkSmartPointer<vtkIdList>& nearPointIds = tl.NearPointIds;

  // reset the visited cell ids
  visibleCellIds.clear();

  // find the point closest to the coordinates given and search the attached cells.
  vtkIdType ptId = this->PointLocator->FindClosestPoint(x);
  if (ptId < 0)
  {
    vtkWarningMacro(<< "2) No close point found for " << x[0] << ", " << x[1] << ", " << x[2]);
    return -1;
  }

  this->DataSet->GetPointCells(ptId, cellIds);
  vtkIdType foundCellId = this->FindCellWalk(
    cellIds, x, tol2, genCell, subId, pcoords, weights, visibleCellIds, pointIds, neighbors);
  if (foundCellId >= 0)
  {
    return foundCellId;
  }

  // It is possible that the topology is not fully connected as points may be
  // coincident.  Handle this by looking at every point within the tolerance
  // and consider all cells connected.  It has been suggested that we should
  // really do this coincident point check at every point as we walk through
  // neighbors, which would happen in FindCellWalk.  If that were ever
  // implemented, this step might become unnecessary.
  double ptCoord[3];
  this->DataSet->GetPoint(ptId, ptCoord);
  this->PointLocator->FindPointsWithinRadius(tol, ptCoord, nearPointIds);
  nearPointIds->DeleteId(ptId); // Already searched this one.
  for (vtkIdType i = 0, numPts = nearPointIds->GetNumberOfIds(); i < numPts; i++)
  {
    const vtkIdType nearPointId = nearPointIds->GetId(i);
    this->DataSet->GetPointCells(nearPointId, cellIds);
    foundCellId = this->FindCellWalk(
      cellIds, x, tol2, genCell, subId, pcoords, weights, visibleCellIds, pointIds, neighbors);
    if (foundCellId >= 0)
    {
      return foundCellId;
    }
  }

  // Couldn't find anything so try more time-consuming strategy.  It is
  // possible that the closest point is not part of a cell containing the
  // query point (i.e., a hanging node situation). In this case, look for the
  // N closest points (beyond any coincident points identified
  // previously). Typically, N=9 (somewhat arbitrary, empirical, based on 2:1
  // subdivision of hexahedral cells). Using large N affects performance but
  // produces better results.
  if (this->NumberOfClosestPoints > 1)
  {
    this->PointLocator->FindClosestNPoints(
      nearPointIds->GetNumberOfIds() + this->NumberOfClosestPoints, x, nearPointIds);
    for (vtkIdType i = 0, numPts = nearPointIds->GetNumberOfIds(); i < numPts; ++i)
    {
      ptId = nearPointIds->GetId(i);
      this->DataSet->GetPointCells(ptId, cellIds);
      foundCellId = this->FindCellWalk(
        cellIds, x, tol2, genCell, subId, pcoords, weights, visibleCellIds, pointIds, neighbors);
      if (foundCellId >= 0)
      {
        return foundCellId;
      }
    }
  }

  // Could not find a containing cell. Either the query point is outside
  // the dataset, or there is an uncommon pathology of disconnected cells and
  // points (if using a point locator approach). In this latter case, a cell
  // locator is necessary.
  return -1;
}

//------------------------------------------------------------------------------
vtkIdType vtkJumpAndWalkCellLocator::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* genCell, vtkIdType& closestCellId, int& closestSubId,
  double& minDist2, int& inside)
{
  // Make sure locator has been built successfully
  this->BuildLocator();
  if (!this->PointLocator)
  {
    return -1;
  }
  // the implementation of this function is a copy of an old approach in
  // vtkAbstractInterpolatedVelocityField, check git history for more info
  // in the future something better could be implemented
  vtkIdType retVal = 0;

  // find the point closest to the coordinates
  // given and search the attached cells.
  vtkIdType ptId = this->PointLocator->FindClosestPoint(x);
  if (ptId < 0)
  {
    return retVal;
  }

  ThreadLocalData& tl = this->TLData.Local();
  std::unordered_set<vtkIdType>& visibleCellIds = tl.VisitedCellIds;
  vtkSmartPointer<vtkIdList>& pointIds = tl.PointIds;
  vtkSmartPointer<vtkIdList>& neighbors = tl.Neighbors;
  vtkSmartPointer<vtkIdList>& cellIds = tl.CellIds;
  std::vector<double>& weights = tl.Weights;

  // reset the visited cell ids
  visibleCellIds.clear();

  // find the cells adjacent to the closest point
  this->DataSet->GetPointCells(ptId, cellIds);
  double point[3], pcoords[3], dist2, closestPcoords[3];
  int subId, stat;
  closestSubId = -1;
  closestCellId = -1;
  minDist2 = this->DataSet->GetLength2();
  // find the closest adjacent cell
  for (vtkIdType id = 0, numCells = cellIds->GetNumberOfIds(); id < numCells; ++id)
  {
    const vtkIdType cellId = cellIds->GetId(id);
    this->DataSet->GetCell(cellId, genCell);
    this->Weights.resize(static_cast<size_t>(genCell->GetNumberOfPoints()));
    stat = genCell->EvaluatePosition(x, point, subId, pcoords, dist2, this->Weights.data());
    if (stat != -1 && dist2 < minDist2)
    {
      retVal = 1;
      inside = stat;
      minDist2 = dist2;
      closestCellId = cellId;
      closestSubId = subId;
      closestPoint[0] = point[0];
      closestPoint[1] = point[1];
      closestPoint[2] = point[2];
      closestPcoords[0] = pcoords[0];
      closestPcoords[1] = pcoords[1];
      closestPcoords[2] = pcoords[2];
    }
  }
  if (closestCellId == -1)
  {
    return retVal;
  }
  // Recover the closest cell
  this->DataSet->GetCell(closestCellId, genCell);
  // get the boundary point ids closest to the parametric coordinates
  genCell->CellBoundary(closestSubId, closestPcoords, pointIds);
  // get the neighbors cells of the boundary points
  this->DataSet->GetCellNeighbors(closestCellId, pointIds, neighbors);
  // check if any of the neighbors is closer to the query point
  for (vtkIdType id = 0, numNeighbors = neighbors->GetNumberOfIds(); id < numNeighbors; ++id)
  {
    const vtkIdType cellId = neighbors->GetId(id);
    this->DataSet->GetCell(cellId, genCell);
    weights.resize(static_cast<size_t>(genCell->GetNumberOfPoints()));
    stat = genCell->EvaluatePosition(x, point, subId, pcoords, dist2, weights.data());
    if (stat != -1 && dist2 < minDist2)
    {
      retVal = 1;
      inside = stat;
      minDist2 = dist2;
      closestCellId = cellId;
      closestSubId = subId;
      closestPoint[0] = point[0];
      closestPoint[1] = point[1];
      closestPoint[2] = point[2];
      // pcoords are not used again
    }
  }
  // If reach here, closestCellId != -1 is guaranteed.
  this->DataSet->GetCell(closestCellId, genCell);
  // the closest is within the given radius
  if (minDist2 > radius * radius)
  {
    retVal = 0;
  }
  return retVal;
}
VTK_ABI_NAMESPACE_END
