/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverlappingCellsDetector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOverlappingCellsDetector.h"

#include "vtkAbstractPointLocator.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDIYUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdTreePointLocator.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStaticPointLocator2D.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/assigner.hpp)
// clang-format on

namespace detail
{
static constexpr char SPHERE_RADIUS_ARRAY_NAME[21] = "detail::SphereRadius";
static constexpr char ID_MAP_TO_ORIGIN_DATASET_IDS_NAME[32] = "detail::IdMapToOriginDataSetIds";

//----------------------------------------------------------------------------
double ComputeEpsilon(const vtkBoundingBox& boundingBox)
{
  const double *minPoint = boundingBox.GetMinPoint(), *maxPoint = boundingBox.GetMaxPoint();
  double max = std::max({ std::fabs(minPoint[0]), std::fabs(minPoint[1]), std::fabs(minPoint[2]),
    std::fabs(maxPoint[0]), std::fabs(maxPoint[1]), std::fabs(maxPoint[2]) });

  // std::sqrt(2.0) comes from the shrinking cell step.
  // Cells are shrinked by the epsilon returned by this method
  // in edge directions. We want that angles
  return std::max(std::sqrt(VTK_DBL_MIN), std::sqrt(2.0) * VTK_DBL_EPSILON * max);
}

//----------------------------------------------------------------------------
// For each cell of the input dataset, returns a point cloud such that each
// point of this point cloud maps to the center of the bounding box of the cell
// of same id in the input dataset.
// Bounding boxes are also computed and returned by reference so they are
// computed only once in this filter.
vtkPointSet* ConvertCellsToBoundingSpheres(vtkDataSet* ds, std::vector<vtkBoundingBox>& bboxes)
{
  vtkIdType size = ds->GetNumberOfCells();

  auto pointCloud = vtkPolyData::New();
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(size);
  pointCloud->SetPoints(points);

  bboxes.reserve(size);

  vtkNew<vtkDoubleArray> SphereRadiusArray;
  SphereRadiusArray->SetName(SPHERE_RADIUS_ARRAY_NAME);
  SphereRadiusArray->SetNumberOfComponents(1);
  SphereRadiusArray->SetNumberOfTuples(size);

  for (vtkIdType id = 0; id < size; ++id)
  {
    vtkCell* cell = ds->GetCell(id);
    bboxes.emplace_back(ds->GetBounds());
    double center[3];
    double radius2 = cell->ComputeBoundingSphere(center);
    SphereRadiusArray->SetValue(id, std::sqrt(radius2));
    points->SetPoint(id, center);
  }

  pointCloud->GetPointData()->AddArray(SphereRadiusArray);

  return pointCloud;
}

//----------------------------------------------------------------------------
// Given an input pointCloud computed using detail::ConvertCellsToBoundingSpheres,
// shared boundingBoxes of each input block, and the input source of local block,
// this method creates one vtkUnstructuredGrid for each block containing cells
// from source overlapping the bounding box of corresponding block.
// If no cells intersect, the returned vtkUnstructuredGrid is set to nullptr.
// The output vtkUnstructuredGrid Holds the original cell id from source.
// This information is used later to figure out who intersected who in the last
// step of this filter.
std::map<int, vtkSmartPointer<vtkUnstructuredGrid>> ExtractOverlappingCellCandidateByProcess(
  vtkPointSet* pointCloud, const std::map<int, vtkBoundingBox>& boundingBoxes, vtkPointSet* source,
  const std::vector<vtkBoundingBox>& cellBoundingBoxes)
{
  // Each output vtkUnstructuredGrid needs those data objects in order to be constructed.
  // We tie them together in one tuple to facilitate iterating over blocks.
  using CellArrayCellTypePointsIdTuple =
    std::tuple<vtkSmartPointer<vtkCellArray>, vtkSmartPointer<vtkUnsignedCharArray>,
      vtkSmartPointer<vtkPoints>, vtkSmartPointer<vtkIdTypeArray>>;
  std::map<int, CellArrayCellTypePointsIdTuple> cactptidList;

  int pointsType = source->GetPoints()->GetDataType();

  // Initializing new pointers.
  std::transform(boundingBoxes.cbegin(), boundingBoxes.cend(),
    std::inserter(cactptidList, cactptidList.begin()),
    [pointsType](
      const std::pair<int, vtkBoundingBox> pair) -> std::pair<int, CellArrayCellTypePointsIdTuple> {
      return std::pair<int, CellArrayCellTypePointsIdTuple>(pair.first,
        CellArrayCellTypePointsIdTuple(vtkSmartPointer<vtkCellArray>::Take(vtkCellArray::New()),
          vtkSmartPointer<vtkUnsignedCharArray>::Take(vtkUnsignedCharArray::New()),
          vtkSmartPointer<vtkPoints>::Take(vtkPoints::New(pointsType)),
          vtkSmartPointer<vtkIdTypeArray>::Take(vtkIdTypeArray::New())));
    });

  vtkDataArray* radiusArray = pointCloud->GetPointData()->GetArray(SPHERE_RADIUS_ARRAY_NAME);

  // We store new point ids in of the cells in pointIdsList. It is necessary to construct
  // valid vtkUnstructuredGrid.
  std::map<int, std::set<vtkIdType>> pointIdsList;

  for (auto& pair : cactptidList)
  {
    int blockId = pair.first;
    CellArrayCellTypePointsIdTuple& cactptid = pair.second;
    std::set<vtkIdType>& pointIds = pointIdsList[blockId];
    vtkBoundingBox bb = boundingBoxes.find(blockId)->second;
    bb.Inflate(-ComputeEpsilon(bb));
    for (vtkIdType id = 0; id < pointCloud->GetNumberOfPoints(); ++id)
    {
      // For each point in our pointCloud, add corresponding cell from source
      // if the bounding sphere intersects neighbor's bounding box
      // and if the cell bounding box intersects with neighbor'bounding box as well.
      // We test both intersection to narrow candidates and limit MPI communication when possible.
      double radius = radiusArray->GetTuple1(id);
      if (bb.IntersectsSphere(pointCloud->GetPoint(id), radius - radius * VTK_DBL_EPSILON) &&
        bb.Intersects(cellBoundingBoxes[id]))
      {
        vtkCell* cell = source->GetCell(id);
        vtkIdList* idList = cell->GetPointIds();
        for (vtkIdType i = 0; i < idList->GetNumberOfIds(); ++i)
        {
          pointIds.insert(idList->GetId(i));
        }
        std::get<0>(cactptid)->InsertNextCell(cell);
        std::get<1>(cactptid)->InsertNextTuple1(cell->GetCellType());
        std::get<3>(cactptid)->InsertNextTuple1(id);
      }
    }
  }

  // For each block, we set correct point ids in each extracted cell, using pointIdsList
  for (auto& pair : cactptidList)
  {
    int blockId = pair.first;
    CellArrayCellTypePointsIdTuple& cactptid = pair.second;
    std::set<vtkIdType>& pointIds = pointIdsList[blockId];
    vtkIdType currentId = 0;
    vtkPoints* points = std::get<2>(cactptid);
    points->SetNumberOfPoints(static_cast<vtkIdType>(pointIds.size()));

    // We create an id inverse map so one can go from new cells to their homologue
    // in source
    std::unordered_map<vtkIdType, vtkIdType> inversePointIdsMap;
    for (vtkIdType pointId : pointIds)
    {
      points->SetPoint(currentId, source->GetPoint(pointId));
      double p[3];
      points->GetPoint(currentId, p);
      inversePointIdsMap[pointId] = currentId++;
    }

    // We can now replace cell point ids
    vtkCellArray* ca = std::get<0>(cactptid);
    vtkNew<vtkIdList> idList;
    for (vtkIdType cellId = 0; cellId < ca->GetNumberOfCells(); ++cellId)
    {
      ca->GetCellAtId(cellId, idList);
      for (vtkIdType i = 0; i < idList->GetNumberOfIds(); ++i)
      {
        idList->SetId(i, inversePointIdsMap[idList->GetId(i)]);
      }
      ca->ReplaceCellAtId(cellId, idList);
    }
  }

  // Creating the output
  std::map<int, vtkSmartPointer<vtkUnstructuredGrid>> ugList;

  // Filling output vtkUnstructured grids, with our list of tuples we just created
  std::transform(cactptidList.begin(), cactptidList.end(), std::inserter(ugList, ugList.begin()),
    [&source](const std::pair<int, CellArrayCellTypePointsIdTuple>& pair)
      -> std::pair<int, vtkSmartPointer<vtkUnstructuredGrid>> {
      int blockId = pair.first;
      const CellArrayCellTypePointsIdTuple& cactptid = pair.second;
      vtkCellArray* ca = std::get<0>(cactptid);
      if (ca->GetNumberOfCells())
      {
        vtkUnsignedCharArray* ct = std::get<1>(cactptid);
        vtkPoints* pt = std::get<2>(cactptid);
        vtkIdTypeArray* idArray = std::get<3>(cactptid);

        vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
        ug->SetCells(ct, ca);
        ug->SetPoints(pt);
        idArray->SetName(ID_MAP_TO_ORIGIN_DATASET_IDS_NAME);
        ug->GetCellData()->AddArray(idArray);

        vtkNew<vtkIdTypeArray> globalIds;
        globalIds->DeepCopy(source->GetCellData()->GetArray("GlobalCellIds"));
        ug->GetCellData()->AddArray(globalIds);

        return std::pair<int, vtkSmartPointer<vtkUnstructuredGrid>>(
          blockId, vtkSmartPointer<vtkUnstructuredGrid>::Take(ug));
      }
      return std::pair<int, vtkSmartPointer<vtkUnstructuredGrid>>(blockId, nullptr);
    });

  return ugList;
}

//----------------------------------------------------------------------------
// Block structure used for diy communication
struct Block : public diy::Serialization<vtkPointSet*>
{
  /**
   * Bouding boxes of all spatial neighbor blocks
   */
  std::map<int, vtkBoundingBox> BoundingBoxes;

  /**
   * DataSets containing potentially intersecting cells sent by other blocks
   */
  std::map<int, vtkSmartPointer<vtkDataSet>> DataSets;

  /**
   * Map from local dataset cell id to a list of cell ids from other blocks that intersect
   * local cell.
   * In other words, if CollisionListMaps[blockId][localCellId][cellId] exists, then
   * local cell of id localCellId intersects the cell from block blockId of id cellId.
   */
  std::map<int, std::unordered_map<vtkIdType, std::set<vtkIdType>>> CollisionListMaps;
};
} // namespace detail

vtkStandardNewMacro(vtkOverlappingCellsDetector);
vtkCxxSetObjectMacro(vtkOverlappingCellsDetector, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkOverlappingCellsDetector::vtkOverlappingCellsDetector()
  : Controller(nullptr)
  , NumberOfCollisionsPerCellArrayName(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetNumberOfCollisionsPerCellArrayName("NumberOfCollisionsPerCell");
}

//----------------------------------------------------------------------------
vtkOverlappingCellsDetector::~vtkOverlappingCellsDetector()
{
  this->SetController(nullptr);
  this->SetNumberOfCollisionsPerCellArrayName(nullptr);
}

//----------------------------------------------------------------------------
int vtkOverlappingCellsDetector::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPointSet* input = vtkPointSet::GetData(inputVector[0], 0);
  if (!input)
  {
    vtkErrorMacro(<< "Input is not a vtkPointSet");
    return 0;
  }

  vtkPointSet* output = vtkPointSet::GetData(outputVector, 0);
  if (!output)
  {
    vtkErrorMacro(<< "Output is not a vtkPointSet");
    return 0;
  }

  output->ShallowCopy(input);

  return this->ExposeOverlappingCellsAmongBlocks(output);
}

//----------------------------------------------------------------------------
int vtkOverlappingCellsDetector::ExposeOverlappingCellsAmongBlocks(vtkPointSet* output)
{

  std::vector<vtkBoundingBox> cellBoundingBoxes;

  // Computing a point cloud of bounding spheres
  vtkSmartPointer<vtkPointSet> pointCloud = vtkSmartPointer<vtkPointSet>::Take(
    detail::ConvertCellsToBoundingSpheres(output, cellBoundingBoxes));

  // Setting up diy communication
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->Controller);

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new detail::Block()); },
    [](void* b) -> void { delete static_cast<detail::Block*>(b); });

  diy::ContiguousAssigner assigner(comm.size(), comm.size());
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    1, diy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  assert(master.size() == 1);

  int myrank = comm.rank();

  double* bounds = output->GetBounds();
  vtkBoundingBox myBounds(bounds);

  // First, we share bounding boxes with other blocks
  diy::all_to_all(master, assigner, [bounds](detail::Block* block, const diy::ReduceProxy& srp) {
    int myBlockId = srp.gid();
    if (srp.round() == 0)
    {
      for (int i = 0; i < srp.out_link().size(); ++i)
      {
        if (i != myBlockId)
        {
          srp.enqueue(srp.out_link().target(i), bounds, 6);
        }
      }
    }
    else
    {
      double boundstmp[6];
      for (int i = 0; i < static_cast<int>(srp.in_link().size()); ++i)
      {
        if (i != myBlockId)
        {
          const diy::BlockID& blockId = srp.in_link().target(i);
          srp.dequeue(blockId, boundstmp, 6);
          block->BoundingBoxes.emplace(blockId.gid, vtkBoundingBox(boundstmp));
        }
      }
    }
  });

  std::map<int, vtkBoundingBox>& boundingBoxes = master.block<detail::Block>(0)->BoundingBoxes;

  // We create unstructured grids for each neighbor block, composed
  // from cells that are candidates for intersecting cells from the neighbor.
  std::map<int, vtkSmartPointer<vtkUnstructuredGrid>> overlappingCellCandidatesDataSets =
    detail::ExtractOverlappingCellCandidateByProcess(
      pointCloud, boundingBoxes, output, cellBoundingBoxes);

  // We prepare resetting links between blocks depending on the presence or not
  // of potential cell collisions.
  std::map<int, std::vector<int>> neighbors;
  diy::all_to_all(master, assigner,
    [myrank, &neighbors, &overlappingCellCandidatesDataSets](
      detail::Block*, const diy::ReduceProxy& rp) {
      auto it = overlappingCellCandidatesDataSets.cbegin();
      if (rp.round() == 0)
      {
        for (int i = 0; i < rp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = rp.out_link().target(i);
          if (blockId.gid != myrank)
          {
            int connected = (it->second != nullptr);
            const auto dest = rp.out_link().target(i);
            rp.enqueue(dest, &connected, 1);
            ++it;
          }
        };
      }
      else
      {
        for (int i = 0; i < rp.in_link().size(); ++i)
        {
          const auto src = rp.in_link().target(i);
          if (src.gid != myrank)
          {
            int connected;
            rp.dequeue(src, &connected, 1);
            if (connected)
            {
              neighbors[rp.gid()].push_back(src.gid);
            }
          }
        }
      }
    });

  // Update local links.
  if (neighbors.size())
  {
    for (auto& pair : neighbors)
    {
      auto l = new diy::Link();
      for (const auto& nid : pair.second)
      {
        l->add_neighbor(diy::BlockID(nid, assigner.rank(nid)));
      }
      master.replace_link(master.lid(pair.first), l);
    }
  }

  // We share overlapping candidates with neighbor blocks.
  master.foreach ([&overlappingCellCandidatesDataSets](
                    void*, const diy::Master::ProxyWithLink& cp) {
    // enqueue
    for (int i = 0; i < static_cast<int>(cp.link()->size()); ++i)
    {
      vtkdiy2::BlockID& targetBlockId = cp.link()->target(i);
      cp.enqueue<vtkDataSet*>(targetBlockId, overlappingCellCandidatesDataSets[targetBlockId.gid]);
    }
  });
  master.exchange();
  master.foreach ([](detail::Block* block, const diy::Master::ProxyWithLink& cp) {
    // dequeue
    std::vector<int> incoming;
    cp.incoming(incoming);
    for (int gid : incoming)
    {
      // we need this extra check because incoming is not empty when using only one block
      if (!cp.incoming(gid).empty())
      {
        vtkDataSet* ds = nullptr;
        cp.dequeue<vtkDataSet*>(gid, ds);
        block->DataSets.emplace(gid, vtkSmartPointer<vtkDataSet>::Take(ds));
      }
    }
  });

  // Locator to be used for point search inside the point cloud of bounding spheres.
  if (myBounds.ComputeInnerDimension() == 2)
  {
    vtkNew<vtkStaticPointLocator2D> locator;
    locator->SetDataSet(pointCloud);
    pointCloud->SetPointLocator(locator);
  }

  // Locator to be used for point search inside the point cloud of bounding spheres.
  if (myBounds.ComputeInnerDimension() == 2)
  {
    vtkNew<vtkStaticPointLocator2D> locator;
    locator->SetDataSet(pointCloud);
    pointCloud->SetPointLocator(locator);
  }
  else
  {
    vtkNew<vtkKdTreePointLocator> locator;
    locator->SetDataSet(pointCloud);
    pointCloud->SetPointLocator(locator);
  }

  // dummy variable needed in the main cell collision detection algorithm.
  std::unordered_map<vtkIdType, std::set<vtkIdType>> localCollisionListMap;
  if (!this->DetectOverlappingCells(output, pointCloud, cellBoundingBoxes, output, pointCloud,
        cellBoundingBoxes, localCollisionListMap))
  {
    vtkErrorMacro(<< "Failed to detect self colliding cells");
    return 0;
  }

  std::map<int, vtkSmartPointer<vtkDataSet>>& queryCellDataSets =
    master.block<detail::Block>(0)->DataSets;
  std::map<int, std::unordered_map<vtkIdType, std::set<vtkIdType>>> collisionListMapList;

  vtkSmartPointer<vtkPointSet> queryPointCloud;
  // We now detect collision with the cells sent by other blocks.
  for (auto& pair : queryCellDataSets)
  {
    vtkSmartPointer<vtkDataSet>& queryCellDataSet = pair.second;
    if (!queryCellDataSet)
    {
      continue;
    }
    int blockId = pair.first;
    std::vector<vtkBoundingBox> queryCellBoundingBoxes;
    queryPointCloud = vtkSmartPointer<vtkPointSet>::Take(
      detail::ConvertCellsToBoundingSpheres(queryCellDataSet, queryCellBoundingBoxes));

    if (!this->DetectOverlappingCells(queryCellDataSet, queryPointCloud, queryCellBoundingBoxes,
          output, pointCloud, cellBoundingBoxes, collisionListMapList[blockId]))
    {
      vtkErrorMacro(<< "Failed to detect self colliding cells in process " << myrank << " on "
                    << blockId << "th local block");
      return 0;
    }
  }

  // We need to send back collision information to the original block, so they
  // can add the collisions they couldn't detect.
  master.foreach ([&collisionListMapList](void*, const diy::Master::ProxyWithLink& cp) {
    // enqueue
    for (int i = 0; i < static_cast<int>(cp.link()->size()); ++i)
    {
      vtkdiy2::BlockID& targetBlockId = cp.link()->target(i);
      cp.enqueue(targetBlockId, collisionListMapList[targetBlockId.gid]);
    }
  });
  master.exchange();
  master.foreach ([](detail::Block* block, const diy::Master::ProxyWithLink& cp) {
    // dequeue
    std::vector<int> incoming;
    cp.incoming(incoming);
    for (int gid : incoming)
    {
      if (!cp.incoming(gid).empty())
      {
        std::unordered_map<vtkIdType, std::set<vtkIdType>> collisionListMap;
        cp.dequeue(gid, collisionListMap);
        block->CollisionListMaps.emplace(gid, std::move(collisionListMap));
      }
    }
  });

  const auto& collisionIdList = master.block<detail::Block>(0)->CollisionListMaps;
  vtkDataArray* queryNumberOfCollisionsPerCell =
    output->GetCellData()->GetArray("NumberOfCollisionsPerCell");

  // Last pass. We look at what intersections were found in the other blocks, and check if
  // we found them or not, and increment collision count accordingly.
  for (const auto& collisionIdPair : collisionIdList)
  {
    int blockId = collisionIdPair.first;
    auto& collisionIds = collisionIdPair.second;
    auto& collisionListMap = collisionListMapList[blockId];
    vtkDataSet* queryCellDataSet = queryCellDataSets[blockId];
    if (!queryCellDataSet)
    {
      continue;
    }
    vtkIdTypeArray* neighborIdMapArray = vtkIdTypeArray::SafeDownCast(
      queryCellDataSet->GetCellData()->GetArray(detail::ID_MAP_TO_ORIGIN_DATASET_IDS_NAME));
    for (const auto& pair : collisionListMap)
    {
      // pair.first <=> id of neighbor process which has collision with at least one of our cell.
      // pair.second <=> list of ids from this process colliding with cell of id pair.first
      for (vtkIdType id : pair.second)
      {
        auto it = collisionIds.find(neighborIdMapArray->GetValue(id));
        // it->first <=> id in this process which has collision with at least one of our neighbor's
        // cell. it->second <=> list of ids from our neighbor process colliding with cell of id
        // it->first.
        if (it == collisionIds.end() || !pair.second.count(id))
        {
          queryNumberOfCollisionsPerCell->SetTuple1(
            pair.first, queryNumberOfCollisionsPerCell->GetTuple1(pair.first) + 1);
        }
      }
    }
  }

  output->GetCellData()->SetActiveScalars(this->GetNumberOfCollisionsPerCellArrayName());

  return 1;
}

//----------------------------------------------------------------------------
// Main collision detection algorithm.
bool vtkOverlappingCellsDetector::DetectOverlappingCells(vtkDataSet* queryCellDataSet,
  vtkPointSet* queryPointCloud, const std::vector<vtkBoundingBox>& queryCellBoundingBoxes,
  vtkDataSet* cellDataSet, vtkPointSet* pointCloud,
  const std::vector<vtkBoundingBox>& cellBoundingBoxes,
  std::unordered_map<vtkIdType, std::set<vtkIdType>>& collisionListMap)
{
  assert(cellDataSet->GetNumberOfCells() == pointCloud->GetNumberOfPoints() &&
    static_cast<vtkIdType>(cellBoundingBoxes.size()) == pointCloud->GetNumberOfPoints());
  assert(queryCellDataSet->GetNumberOfCells() == queryPointCloud->GetNumberOfPoints() &&
    static_cast<vtkIdType>(queryCellBoundingBoxes.size()) == queryPointCloud->GetNumberOfPoints());

  vtkAbstractPointLocator* locator = pointCloud->GetPointLocator();
  if (!locator)
  {
    pointCloud->BuildPointLocator();
    locator = pointCloud->GetPointLocator();
  }

  vtkDataArray* querySphereRadiusArray =
    queryPointCloud->GetPointData()->GetArray(detail::SPHERE_RADIUS_ARRAY_NAME);

  vtkIdType querySize = queryPointCloud->GetNumberOfPoints();

  vtkNew<vtkIdTypeArray> queryNumberOfCollisionsPerCellsArray;
  queryNumberOfCollisionsPerCellsArray->SetNumberOfComponents(1);
  queryNumberOfCollisionsPerCellsArray->SetNumberOfTuples(querySize);
  queryNumberOfCollisionsPerCellsArray->SetName(this->GetNumberOfCollisionsPerCellArrayName());
  queryNumberOfCollisionsPerCellsArray->Fill(0.0);

  // Handling case where both input data sets point to the same address.
  vtkDataArray* numberOfCollisionPerCellsArray = queryCellDataSet != cellDataSet
    ? cellDataSet->GetCellData()->GetArray(this->GetNumberOfCollisionsPerCellArrayName())
    : queryNumberOfCollisionsPerCellsArray;

  vtkNew<vtkIdList> neighborIds;

  // We want to discard ghost cells, so we have to acknowledge them.
  vtkUnsignedCharArray* queryCellGhostArray = queryCellDataSet->GetCellGhostArray();
  vtkUnsignedCharArray* cellGhostArray = cellDataSet->GetCellGhostArray();

  // local cell bank to avoid calling ::New() too many times.
  std::map<int, vtkSmartPointer<vtkCell>> cellBank, neighborCellBank;
  for (vtkIdType id = 0; id < querySize; ++id)
  {
    if (queryCellGhostArray && queryCellGhostArray->GetValue(id))
    {
      continue;
    }
    locator->FindPointsWithinRadius(2.0 * std::sqrt(querySphereRadiusArray->GetTuple1(id)),
      queryPointCloud->GetPoint(id), neighborIds);
    int cellType = queryCellDataSet->GetCellType(id);
    auto cellBankHandle = cellBank.find(cellType);
    vtkCell* currentCell;
    if (cellBankHandle == cellBank.end())
    {
      currentCell = vtkGenericCell::InstantiateCell(cellType);
      cellBank[cellType] = vtkSmartPointer<vtkCell>::Take(currentCell);
    }
    else
    {
      currentCell = cellBankHandle->second;
    }
    // We need to deep copy because if the 2 inputs share the same address,
    // currentCell and the other cell on which we want to detect collision
    // will step on each other: they inner data would share the same address.
    currentCell->DeepCopy(queryCellDataSet->GetCell(id));

    const vtkBoundingBox& bbox = queryCellBoundingBoxes[id];

    // We need to shrink currentCell to discard false positive from adjacent cells.
    double currentCellTolerance = detail::ComputeEpsilon(bbox);
    currentCell->Inflate(-currentCellTolerance);

    vtkIdType intersectionCount = 0;
    auto collisionListMapHandle = collisionListMap.find(id);
    for (vtkIdType i = 0; i < neighborIds->GetNumberOfIds(); ++i)
    {
      vtkIdType neighborId = neighborIds->GetId(i);
      if (cellGhostArray && cellGhostArray->GetValue(neighborId))
      {
        continue;
      }

      // We do not want to compute the same collision twice, so we use collisionListMap info here
      if ((queryCellDataSet != cellDataSet ||
            (id != neighborId &&
              (collisionListMapHandle == collisionListMap.end() ||
                !collisionListMapHandle->second.count(neighborId)))))
      {
        // Same procedure as for currentCell.
        // We have a bank of cells to aleviate dynamic allocating when possible.
        int neighborCellType = cellDataSet->GetCellType(neighborId);
        auto neighborCellBankHandle = neighborCellBank.find(neighborCellType);
        vtkCell* neighborCell;
        if (neighborCellBankHandle == neighborCellBank.end())
        {
          neighborCell = vtkGenericCell::InstantiateCell(neighborCellType);
          neighborCellBank[cellType] = vtkSmartPointer<vtkCell>::Take(neighborCell);
        }
        else
        {
          neighborCell = neighborCellBankHandle->second;
        }
        neighborCell->DeepCopy(cellDataSet->GetCell(neighborId));

        // Shrinking this cell as well.
        double neighborCellTolerance = detail::ComputeEpsilon(cellBoundingBoxes[neighborId]);
        neighborCell->Inflate(-neighborCellTolerance);
        if (currentCell->IntersectWithCell(neighborCell, bbox, cellBoundingBoxes[neighborId]))
        {
          ++intersectionCount;
          numberOfCollisionPerCellsArray->SetTuple1(
            neighborId, numberOfCollisionPerCellsArray->GetTuple1(neighborId) + 1);
          collisionListMap[neighborId].insert(id);
        }
      }
    }
    if (intersectionCount)
    {
      queryNumberOfCollisionsPerCellsArray->SetValue(
        id, queryNumberOfCollisionsPerCellsArray->GetValue(id) + intersectionCount);
    }
  }
  queryCellDataSet->GetCellData()->AddArray(queryNumberOfCollisionsPerCellsArray);

  return true;
}

//----------------------------------------------------------------------------
void vtkOverlappingCellsDetector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "NumberOfCollisionsPerCellArrayName: " << this->NumberOfCollisionsPerCellArrayName
     << std::endl;
}
