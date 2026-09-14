// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkKdTreePointLocator.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellArray.h"
#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include "vtk_nanoflann.h"

#include <limits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
//------------------------------------------------------------------------------
void AddBoxFaces(vtkPoints* pts, vtkCellArray* polys, const double min[3], const double max[3])
{
  vtkIdType ids[8];
  double x[3];

  x[0] = min[0];
  x[1] = max[1];
  x[2] = min[2];
  ids[0] = pts->InsertNextPoint(x);

  x[0] = max[0];
  x[1] = max[1];
  x[2] = min[2];
  ids[1] = pts->InsertNextPoint(x);

  x[0] = max[0];
  x[1] = max[1];
  x[2] = max[2];
  ids[2] = pts->InsertNextPoint(x);

  x[0] = min[0];
  x[1] = max[1];
  x[2] = max[2];
  ids[3] = pts->InsertNextPoint(x);

  x[0] = min[0];
  x[1] = min[1];
  x[2] = min[2];
  ids[4] = pts->InsertNextPoint(x);

  x[0] = max[0];
  x[1] = min[1];
  x[2] = min[2];
  ids[5] = pts->InsertNextPoint(x);

  x[0] = max[0];
  x[1] = min[1];
  x[2] = max[2];
  ids[6] = pts->InsertNextPoint(x);

  x[0] = min[0];
  x[1] = min[1];
  x[2] = max[2];
  ids[7] = pts->InsertNextPoint(x);

  vtkIdType face[4];

  face[0] = ids[0];
  face[1] = ids[1];
  face[2] = ids[2];
  face[3] = ids[3];
  polys->InsertNextCell(4, face);

  face[0] = ids[1];
  face[1] = ids[5];
  face[2] = ids[6];
  face[3] = ids[2];
  polys->InsertNextCell(4, face);

  face[0] = ids[5];
  face[1] = ids[4];
  face[2] = ids[7];
  face[3] = ids[6];
  polys->InsertNextCell(4, face);

  face[0] = ids[4];
  face[1] = ids[0];
  face[2] = ids[3];
  face[3] = ids[7];
  polys->InsertNextCell(4, face);

  face[0] = ids[3];
  face[1] = ids[2];
  face[2] = ids[6];
  face[3] = ids[7];
  polys->InsertNextCell(4, face);

  face[0] = ids[1];
  face[1] = ids[0];
  face[2] = ids[4];
  face[3] = ids[5];
  polys->InsertNextCell(4, face);
}

//------------------------------------------------------------------------------
// Recursively draws the splitting plane of each internal node (clipped to
// that node's current bounding box), mirroring
// vtkKdTree::_generateRepresentationWholeSpace.
template <class NodeT>
void GenerateRepresentationRecursive(const NodeT* nodes, std::size_t nodeIdx, const double min[3],
  const double max[3], int level, vtkPoints* pts, vtkCellArray* polys)
{
  const NodeT* node = nodes + nodeIdx;
  if (level < 0 || node->isLeaf())
  {
    return;
  }

  int dim = node->node_type.sub.divfeat;
  double divLow = node->node_type.sub.divlow;
  double divHigh = node->node_type.sub.divhigh;

  double p[4][3];
  switch (dim)
  {
    case 0: // X
      p[0][0] = divLow;
      p[0][1] = max[1];
      p[0][2] = max[2];
      p[1][0] = divLow;
      p[1][1] = max[1];
      p[1][2] = min[2];
      p[2][0] = divLow;
      p[2][1] = min[1];
      p[2][2] = min[2];
      p[3][0] = divLow;
      p[3][1] = min[1];
      p[3][2] = max[2];
      break;
    case 1: // Y
      p[0][0] = min[0];
      p[0][1] = divLow;
      p[0][2] = max[2];
      p[1][0] = min[0];
      p[1][1] = divLow;
      p[1][2] = min[2];
      p[2][0] = max[0];
      p[2][1] = divLow;
      p[2][2] = min[2];
      p[3][0] = max[0];
      p[3][1] = divLow;
      p[3][2] = max[2];
      break;
    default: // Z
      p[0][0] = min[0];
      p[0][1] = min[1];
      p[0][2] = divLow;
      p[1][0] = min[0];
      p[1][1] = max[1];
      p[1][2] = divLow;
      p[2][0] = max[0];
      p[2][1] = max[1];
      p[2][2] = divLow;
      p[3][0] = max[0];
      p[3][1] = min[1];
      p[3][2] = divLow;
      break;
  }

  vtkIdType ids[4];
  for (int i = 0; i < 4; i++)
  {
    ids[i] = pts->InsertNextPoint(p[i]);
  }
  polys->InsertNextCell(4, ids);

  double leftMax[3] = { max[0], max[1], max[2] };
  leftMax[dim] = divLow;

  double rightMin[3] = { min[0], min[1], min[2] };
  rightMin[dim] = divHigh;

  // The nodes are stored in depth-first pre-order: the left child directly
  // follows its parent and child2 is the offset from the parent to the right
  // child.
  GenerateRepresentationRecursive(nodes, nodeIdx + 1, min, leftMax, level - 1, pts, polys);
  GenerateRepresentationRecursive(
    nodes, nodeIdx + node->child2, rightMin, max, level - 1, pts, polys);
}

//------------------------------------------------------------------------------
// Adapts a vtkDataArray's tuple range directly to nanoflann's dataset-adaptor
// interface. No point coordinates are copied: nanoflann reads them straight
// out of the array through this wrapper.
template <class ArrayT>
class vtkKdTreePointLocatorPointCloud
{
public:
  using TupleRangeType = decltype(vtk::DataArrayTupleRange<3>(std::declval<ArrayT*>()));
  using ValueType = typename TupleRangeType::ComponentType;

  vtkKdTreePointLocatorPointCloud(ArrayT* points, const double bounds[6])
    : Points(vtk::DataArrayTupleRange<3>(points))
  {
    std::copy_n(bounds, 6, this->Bounds);
  }

  inline std::size_t kdtree_get_point_count() const { return this->Points.size(); }

  inline ValueType kdtree_get_pt(std::size_t idx, std::size_t dim) const
  {
    return this->Points[idx][dim];
  }

  // The bounding box has as many dimensions as the tree (2 or 3).
  template <class BBoxT>
  bool kdtree_get_bbox(BBoxT& bb) const
  {
    for (std::size_t i = 0; i < bb.size(); ++i)
    {
      bb[i].low = this->Bounds[2 * i];
      bb[i].high = this->Bounds[2 * i + 1];
    }
    return true;
  }

private:
  TupleRangeType Points;
  double Bounds[6];
};

//------------------------------------------------------------------------------
// PIMPL base: the concrete nanoflann tree type depends on the point array's
// storage type (AOS/SOA, float/double), so vtkKdTreePointLocator can only
// hold a single, non-templated handle to "whichever tree got built" through
// this interface. Deliberately a plain polymorphic class (not a vtkObject) --
// see vtkBucketList in vtkStaticPointLocatorPrivate.h for the same pattern.
class vtkKdTreePointLocatorTreeBase
{
public:
  virtual ~vtkKdTreePointLocatorTreeBase() = default;

  virtual vtkIdType FindClosestPoint(const double x[3]) = 0;
  virtual vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double& dist2) = 0;
  virtual void FindClosestNPoints(int N, const double x[3], vtkIdList* result) = 0;
  virtual void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result) = 0;
  virtual void GenerateRepresentation(
    int level, const double bounds[6], vtkPoints* pts, vtkCellArray* polys) = 0;
  virtual int GetDimension() const = 0;
};

//------------------------------------------------------------------------------
// The tree has 3 dimensions, or 2 when all the points are in a plane of
// constant z (the z bounds are equal). The 2D tree ignores the z coordinate of
// the points, and of the query points and petals: all its distances are in
// the x-y plane, as those of vtkStaticPointLocator2D. The points are indexed
// with TIds: int when there are fewer than VTK_INT_MAX points, vtkIdType
// otherwise, as vtkStaticPointLocator does, so that the index array of the
// tree, and the results of its queries, take half the memory.
template <class ArrayT, int DIM, typename TIds>
class vtkKdTreePointLocatorTree : public vtkKdTreePointLocatorTreeBase
{
public:
  using PointCloudType = vtkKdTreePointLocatorPointCloud<ArrayT>;
  using KdTreeType =
    nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<double, PointCloudType>,
      PointCloudType, DIM, TIds>;

  vtkKdTreePointLocatorTree(
    ArrayT* points, const double bounds[6], int numberOfPointsPerLeaf, unsigned int nThreadBuild)
    : Cloud(points, bounds)
    , Tree(std::make_unique<KdTreeType>(DIM, this->Cloud,
        nanoflann::KDTreeSingleIndexAdaptorParams(static_cast<std::size_t>(numberOfPointsPerLeaf),
          nanoflann::KDTreeSingleIndexAdaptorFlags::None, nThreadBuild)))
  {
  }

  // The squared distance of two points: in the x-y plane for a 2D tree.
  static double Distance2(const double* a, const double* b)
  {
    if constexpr (DIM == 2)
    {
      return vtkMath::Distance2BetweenPoints2D(a, b);
    }
    else
    {
      return vtkMath::Distance2BetweenPoints(a, b);
    }
  }

  vtkIdType FindClosestPoint(const double x[3]) override
  {
    TIds id;
    double d2;
    return this->Tree->knnSearch(x, 1, &id, &d2) ? static_cast<vtkIdType>(id) : -1;
  }

  // nanoflann only finds the points strictly closer than a squared radius: the
  // next double also finds those at the radius, as the other locators do.
  static double InclusiveRadius2(double radius)
  {
    return std::nextafter(radius * radius, std::numeric_limits<double>::infinity());
  }

  vtkIdType FindClosestPointWithinRadius(double radius, const double x[3], double& dist2) override
  {
    // The search is limited to the radius from the start.
    TIds id;
    double d2;
    if (!this->Tree->rknnSearch(x, 1, &id, &d2, InclusiveRadius2(radius)))
    {
      return -1;
    }
    dist2 = d2;
    return static_cast<vtkIdType>(id);
  }

  void FindClosestNPoints(int N, const double x[3], vtkIdList* result) override
  {
    result->Reset();
    if (N <= 0)
    {
      return;
    }

    // nanoflann casts the indices to those of the result set: they are written
    // straight into the id list. Their distances are on the stack, unless N is
    // large.
    double distBuffer[64];
    double* dists = distBuffer;
    if (N > 64)
    {
      auto& tlDists = this->TLData.Local().Dists;
      tlDists.resize(N);
      dists = tlDists.data();
    }
    result->SetNumberOfIds(N);
    nanoflann::KNNResultSet<double, vtkIdType, vtkIdType> resultSet(N);
    resultSet.init(result->GetPointer(0), dists);
    (void)this->Tree->findNeighbors(resultSet, x);
    result->SetNumberOfIds(resultSet.size());
  }

  // A result set of nanoflann for the points within a radius, which appends
  // them straight to an id list.
  struct RadiusIdListResultSet
  {
    using DistanceType = double;
    using IndexType = TIds;

    vtkIdList* Ids;
    double Radius2;

    std::size_t size() const { return static_cast<std::size_t>(this->Ids->GetNumberOfIds()); }
    bool full() const { return true; }
    double worstDist() const { return this->Radius2; }
    bool addPoint(double, TIds index)
    {
      this->Ids->InsertNextId(static_cast<vtkIdType>(index));
      return true;
    }
    void sort() {}
  };

  void FindPointsWithinRadius(double R, const double x[3], vtkIdList* result) override
  {
    result->Reset();

    // nanoflann only adds the points closer than worstDist().
    RadiusIdListResultSet resultSet{ result, InclusiveRadius2(R) };
    nanoflann::SearchParameters params;
    params.sorted = false;
    (void)this->Tree->radiusSearchCustomCallback(x, resultSet, params);
  }

  int GetDimension() const override { return DIM; }

  void GenerateRepresentation(
    int level, const double bounds[6], vtkPoints* pts, vtkCellArray* polys) override
  {
    double min[3] = { bounds[0], bounds[2], bounds[4] };
    double max[3] = { bounds[1], bounds[3], bounds[5] };

    ::AddBoxFaces(pts, polys, min, max);

    if (level != 0 && !this->Tree->nodes_.empty())
    {
      int remainingLevels = level < 0 ? std::numeric_limits<int>::max() : level - 1;
      ::GenerateRepresentationRecursive(
        this->Tree->nodes_.data(), 0, min, max, remainingLevels, pts, polys);
    }
  }

private:
  PointCloudType Cloud;
  std::unique_ptr<KdTreeType> Tree;

  struct ThreadLocalData
  {
    std::vector<double> Dists;
  };
  vtkSMPThreadLocal<ThreadLocalData> TLData;
};

//------------------------------------------------------------------------------
struct vtkKdTreePointLocatorBuilder
{
  template <class ArrayT>
  void operator()(ArrayT* points, std::unique_ptr<vtkKdTreePointLocatorTreeBase>& impl,
    const double bounds[6], int numberOfPointsPerLeaf, unsigned int nThreadBuild) const
  {
    // The points of a plane of constant z only need a 2D tree, and fewer than
    // VTK_INT_MAX points only need int indices.
    const bool planar = bounds[4] == bounds[5];
    if (points->GetNumberOfTuples() >= VTK_INT_MAX)
    {
      if (planar)
      {
        impl = std::make_unique<vtkKdTreePointLocatorTree<ArrayT, 2, vtkIdType>>(
          points, bounds, numberOfPointsPerLeaf, nThreadBuild);
      }
      else
      {
        impl = std::make_unique<vtkKdTreePointLocatorTree<ArrayT, 3, vtkIdType>>(
          points, bounds, numberOfPointsPerLeaf, nThreadBuild);
      }
    }
    else
    {
      if (planar)
      {
        impl = std::make_unique<vtkKdTreePointLocatorTree<ArrayT, 2, int>>(
          points, bounds, numberOfPointsPerLeaf, nThreadBuild);
      }
      else
      {
        impl = std::make_unique<vtkKdTreePointLocatorTree<ArrayT, 3, int>>(
          points, bounds, numberOfPointsPerLeaf, nThreadBuild);
      }
    }
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
struct vtkKdTreePointLocator::vtkInternals
{
  std::unique_ptr<vtkKdTreePointLocatorTreeBase> Impl;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkKdTreePointLocator);

//------------------------------------------------------------------------------
vtkKdTreePointLocator::vtkKdTreePointLocator() = default;

//------------------------------------------------------------------------------
vtkKdTreePointLocator::~vtkKdTreePointLocator() = default;

//------------------------------------------------------------------------------
vtkIdType vtkKdTreePointLocator::FindClosestPoint(const double x[3])
{
  this->BuildLocator();
  if (!this->Internals)
  {
    return -1;
  }

  return this->Internals->Impl->FindClosestPoint(x);
}

//------------------------------------------------------------------------------
vtkIdType vtkKdTreePointLocator::FindClosestPointWithinRadius(
  double radius, const double x[3], double& dist2)
{
  this->BuildLocator();
  if (!this->Internals)
  {
    return -1;
  }

  return this->Internals->Impl->FindClosestPointWithinRadius(radius, x, dist2);
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::FindClosestNPoints(int N, const double x[3], vtkIdList* result)
{
  this->BuildLocator();
  result->Reset();
  if (!this->Internals)
  {
    return;
  }

  this->Internals->Impl->FindClosestNPoints(N, x, result);
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::FindPointsWithinRadius(double R, const double x[3], vtkIdList* result)
{
  this->BuildLocator();
  result->Reset();
  if (!this->Internals)
  {
    return;
  }

  this->Internals->Impl->FindPointsWithinRadius(R, x, result);
}

//------------------------------------------------------------------------------
int vtkKdTreePointLocator::GetDimension()
{
  return this->Internals && this->Internals->Impl ? this->Internals->Impl->GetDimension() : 0;
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::FreeSearchStructure()
{
  this->Internals.reset();
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::BuildLocator()
{
  // if a search structure already exists
  if (this->Internals)
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
void vtkKdTreePointLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::BuildLocatorInternal()
{
  if (!this->DataSet || (this->DataSet->GetNumberOfPoints()) < 1)
  {
    vtkErrorMacro(<< "No points to build");
    return;
  }
  // Prepare
  this->FreeSearchStructure();

  this->DataSet->GetBounds(this->Bounds);

  vtkDataArray* points = this->DataSet->GetPoints()->GetData();
  if (!points)
  {
    vtkErrorMacro("Points array is null");
    return;
  }
  this->Internals = std::make_unique<vtkInternals>();

  // Build the tree using the same number of threads vtkSMPTools would use elsewhere in
  // VTK, so this respects the backend/thread-count the user has configured.
  int estimatedThreads = vtkSMPTools::GetEstimatedNumberOfThreads();
  unsigned int nThreadBuild =
    estimatedThreads > 0 ? static_cast<unsigned int>(estimatedThreads) : 1u;

  vtkKdTreePointLocatorBuilder builder;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllPointArrays>::Execute(points, builder,
        this->Internals->Impl, this->Bounds, this->NumberOfPointsPerLeaf, nThreadBuild))
  {
    builder(points, this->Internals->Impl, this->Bounds, this->NumberOfPointsPerLeaf, nThreadBuild);
  }

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  if (!this->Internals || !this->Internals->Impl)
  {
    vtkErrorMacro(<< "vtkKdTreePointLocator::GenerateRepresentation empty tree");
    return;
  }

  vtkNew<vtkPoints> pts;
  vtkNew<vtkCellArray> polys;

  this->Internals->Impl->GenerateRepresentation(level, this->Bounds, pts, polys);

  pd->SetPoints(pts);
  pd->SetPolys(polys);
  pd->Squeeze();
}

//------------------------------------------------------------------------------
void vtkKdTreePointLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Internals (nanoflann kd-tree): " << this->Internals.get() << "\n";
}
VTK_ABI_NAMESPACE_END
