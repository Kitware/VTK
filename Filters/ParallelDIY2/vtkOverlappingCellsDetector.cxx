// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOverlappingCellsDetector.h"

#include "vtkAbstractPointLocator.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYExplicitAssigner.h"
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
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
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
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/assigner.hpp)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
namespace
{
constexpr char SPHERE_RADIUS_ARRAY_NAME[21] = "SphereRadius";
constexpr char ID_MAP_TO_ORIGIN_DATASET_IDS_NAME[32] = "IdMapToOriginDataSetIds";

//----------------------------------------------------------------------------
double ComputeEpsilon(const vtkBoundingBox& boundingBox)
{
  const double *minPoint = boundingBox.GetMinPoint(), *maxPoint = boundingBox.GetMaxPoint();
  double max = std::max({ std::fabs(minPoint[0]), std::fabs(minPoint[1]), std::fabs(minPoint[2]),
    std::fabs(maxPoint[0]), std::fabs(maxPoint[1]), std::fabs(maxPoint[2]) });

  // Factor 100.0 controls the angular resolution w.r.t. world axis. With this
  // set up, angles between the skrinking direction and a world axis that are
  // below asin(0.01) = 0.6 degrees do not deviate from the axis.
  return 100.0 * std::max(std::sqrt(VTK_DBL_MIN), VTK_DBL_EPSILON * max);
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
// Given an input pointCloud computed using ConvertCellsToBoundingSpheres,
// shared boundingBoxes of each input block, and the input source of local block,
// this method creates one vtkUnstructuredGrid for each block containing cells
// from source overlapping the bounding box of corresponding block.
// If no cells intersect, there is no allocation at corresponding global id
// in the returned map.
// The output vtkUnstructuredGrid holds the original cell id from source.
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

  // Creating the output
  std::map<int, vtkSmartPointer<vtkUnstructuredGrid>> ugList;

  if (!source->GetNumberOfCells())
  {
    return ugList;
  }

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
    int globalId = pair.first;
    CellArrayCellTypePointsIdTuple& cactptid = pair.second;
    std::set<vtkIdType>& pointIds = pointIdsList[globalId];
    vtkBoundingBox bb = boundingBoxes.find(globalId)->second;
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
    int globalId = pair.first;
    CellArrayCellTypePointsIdTuple& cactptid = pair.second;
    std::set<vtkIdType>& pointIds = pointIdsList[globalId];
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

  for (const auto& pair : cactptidList)
  {
    int globalId = pair.first;
    const CellArrayCellTypePointsIdTuple& cactptid = pair.second;
    vtkCellArray* ca = std::get<0>(cactptid);
    if (ca->GetNumberOfCells())
    {
      vtkUnsignedCharArray* ct = std::get<1>(cactptid);
      vtkPoints* pt = std::get<2>(cactptid);
      vtkIdTypeArray* idArray = std::get<3>(cactptid);

      vtkNew<vtkUnstructuredGrid> ug;
      ug->SetCells(ct, ca);
      ug->SetPoints(pt);
      idArray->SetName(ID_MAP_TO_ORIGIN_DATASET_IDS_NAME);
      ug->GetCellData()->AddArray(idArray);

      ugList[globalId] = ug;
    }
  }

  return ugList;
}

//----------------------------------------------------------------------------
// Block structure used for diy communication
struct Block : public diy::Serialization<vtkPointSet*>
{
  /**
   * Bounding boxes of all spatial neighbor blocks
   */
  std::map<int, vtkBoundingBox> BoundingBoxes;

  /**
   * DataSets containing potentially intersecting cells sent by other blocks
   */
  std::map<int, vtkSmartPointer<vtkDataSet>> DataSets;

  /**
   * Map from local dataset cell id to a list of cell ids from other blocks that intersect
   * local cell.
   * In other words, if CollisionListMaps[globalId][localCellId][cellId] exists, then
   * local cell of id localCellId intersects the cell from block globalId of id cellId.
   */
  std::map<int, std::unordered_map<vtkIdType, std::set<vtkIdType>>> CollisionListMaps;
};
} // anonymous namespace

vtkStandardNewMacro(vtkOverlappingCellsDetector);
vtkCxxSetObjectMacro(vtkOverlappingCellsDetector, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkOverlappingCellsDetector::vtkOverlappingCellsDetector()
  : Controller(nullptr)
  , NumberOfOverlapsPerCellArrayName(nullptr)
  , Tolerance(0.0)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetNumberOfOverlapsPerCellArrayName("NumberOfOverlapsPerCell");
}

//----------------------------------------------------------------------------
vtkOverlappingCellsDetector::~vtkOverlappingCellsDetector()
{
  this->SetController(nullptr);
  this->SetNumberOfOverlapsPerCellArrayName(nullptr);
}

//------------------------------------------------------------------------------
int vtkOverlappingCellsDetector::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkOverlappingCellsDetector::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  bool error = false;
  if (auto outputDS = vtkDataSet::SafeDownCast(outputDO))
  {
    if (auto inputDS = vtkDataSet::SafeDownCast(inputDO))
    {
      outputDS->ShallowCopy(inputDS);
    }
    else
    {
      error = true;
    }
  }
  else if (auto outputCDS = vtkCompositeDataSet::SafeDownCast(outputDO))
  {
    if (auto inputCDS = vtkCompositeDataSet::SafeDownCast(inputDO))
    {
      outputCDS->CopyStructure(inputCDS);
      auto iter = inputCDS->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        auto subInputDO = iter->GetCurrentDataObject();
        auto clone = subInputDO->NewInstance();
        clone->ShallowCopy(subInputDO);
        outputCDS->SetDataSet(iter, clone);
        clone->FastDelete();
      }
      iter->Delete();
    }
    else
    {
      error = true;
    }
  }
  else
  {
    error = true;
  }
  if (error)
  {
    vtkErrorMacro(<< "Could not generate output");
    return 0;
  }

  std::vector<vtkPointSet*> outputs = vtkCompositeDataSet::GetDataSets<vtkPointSet>(outputDO);

  return this->ExposeOverlappingCellsAmongBlocks(outputs);
}

//----------------------------------------------------------------------------
int vtkOverlappingCellsDetector::ExposeOverlappingCellsAmongBlocks(
  std::vector<vtkPointSet*>& outputs)
{
  std::vector<std::vector<vtkBoundingBox>> cellBoundingBoxesArray(outputs.size());
  std::vector<vtkSmartPointer<vtkPointSet>> pointCloudArray;

  const int size = static_cast<int>(outputs.size());

  vtkLogStartScope(TRACE, "extract cell bounding spheres");
  pointCloudArray.reserve(size);
  for (int localId = 0; localId < size; ++localId)
  {
    pointCloudArray.emplace_back(vtkSmartPointer<vtkPointSet>::Take(
      ConvertCellsToBoundingSpheres(outputs[localId], cellBoundingBoxesArray[localId])));
  }
  vtkLogEndScope("extract cell bounding spheres");

  // Setting up diy communication
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->Controller);

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new Block()); },
    [](void* b) -> void { delete static_cast<Block*>(b); });
  vtkDIYExplicitAssigner assigner(comm, size);

  vtkLogStartScope(TRACE, "populate master");
  std::vector<int> gids;
  assigner.local_gids(comm.rank(), gids);

  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
  decomposer.decompose(comm.rank(), assigner, master);
  vtkLogEndScope("populate master");

  int myrank = comm.rank();

  // First, we share bounding boxes with other blocks
  vtkLogStartScope(TRACE, "share bounding boxes");
  diy::all_to_all(master, assigner, [&master, &outputs](Block* block, const diy::ReduceProxy& srp) {
    int myBlockId = srp.gid();
    int localId = master.lid(myBlockId);
    auto& output = outputs[localId];
    if (srp.round() == 0)
    {
      for (int i = 0; i < srp.out_link().size(); ++i)
      {
        if (i != myBlockId)
        {
          srp.enqueue(srp.out_link().target(i), output->GetBounds(), 6);
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
  vtkLogEndScope("share bounding boxes");

  std::vector<std::map<int, vtkBoundingBox>> boundingBoxesArray;
  std::vector<std::map<int, vtkSmartPointer<vtkUnstructuredGrid>>>
    overlappingCellCandidatesDataSetsArray;

  vtkLogStartScope(TRACE, "isolate overlapping cell candidates for neighbor ranks");
  for (int localId = 0; localId < size; ++localId)
  {
    boundingBoxesArray.emplace_back(std::move(master.block<Block>(localId)->BoundingBoxes));

    // We create unstructured grids for each neighbor block, composed
    // from cells that are candidates for intersecting cells from the neighbor.
    overlappingCellCandidatesDataSetsArray.emplace_back(
      ExtractOverlappingCellCandidateByProcess(pointCloudArray[localId],
        boundingBoxesArray[localId], outputs[localId], cellBoundingBoxesArray[localId]));
  }
  vtkLogEndScope("isolate overlapping cell candidates for neighbor ranks");

  // We check if each rank found the same links between blocks.
  // If one block finds that a cell intersects the bounding box of another block, but this other
  // block does not find so, it means that those blocks should not be linked: there won't be any
  // overlaps.
  // After this diy communication, the link map is symmetric among blocks.
  diy::all_to_all(master, assigner,
    [&master, &overlappingCellCandidatesDataSetsArray](Block*, const diy::ReduceProxy& rp) {
      int myBlockId = rp.gid();
      int localId = master.lid(myBlockId);
      auto& overlappingCellCandidatesDataSets = overlappingCellCandidatesDataSetsArray[localId];
      if (rp.round() == 0)
      {
        for (int i = 0; i < rp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = rp.out_link().target(i);
          if (blockId.gid != myBlockId)
          {
            int connected = (overlappingCellCandidatesDataSets.count(blockId.gid) != 0);
            const auto dest = rp.out_link().target(i);
            rp.enqueue(dest, &connected, 1);
          }
        }
      }
      else
      {
        for (int i = 0; i < rp.in_link().size(); ++i)
        {
          const auto src = rp.in_link().target(i);
          if (src.gid != myBlockId)
          {
            int connected;
            rp.dequeue(src, &connected, 1);
            if (!connected)
            {
              auto it = overlappingCellCandidatesDataSets.find(src.gid);
              if (it != overlappingCellCandidatesDataSets.end())
              {
                overlappingCellCandidatesDataSets.erase(it);
              }
            }
          }
        }
      }
    });

  vtkLogStartScope(TRACE, "relink master");
  vtkDIYUtilities::Link(master, assigner, overlappingCellCandidatesDataSetsArray);
  vtkLogEndScope("relink master");

  // We share overlapping candidates with neighbor blocks.
  vtkLogStartScope(TRACE, "send cell candidates across ranks");
  master.foreach ([&master, &overlappingCellCandidatesDataSetsArray](
                    void*, const diy::Master::ProxyWithLink& cp) {
    int myBlockId = cp.gid();
    int localId = master.lid(myBlockId);
    auto& candidates = overlappingCellCandidatesDataSetsArray[localId];
    // enqueue
    for (int i = 0; i < static_cast<int>(cp.link()->size()); ++i)
    {
      vtkdiy2::BlockID& targetBlockId = cp.link()->target(i);
      cp.enqueue<vtkDataSet*>(targetBlockId, candidates.at(targetBlockId.gid));
    }
  });
  master.exchange();
  master.foreach ([](Block* block, const diy::Master::ProxyWithLink& cp) {
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
  vtkLogEndScope("send cell candidates across ranks");

  std::vector<std::map<int, std::unordered_map<vtkIdType, std::set<vtkIdType>>>>
    collisionListMapListArray(outputs.size());
  std::vector<std::map<int, vtkSmartPointer<vtkDataSet>>> queryCellDataSetsArray;

  vtkLogStartScope(TRACE, "locally treat received cells");
  for (int localId = 0; localId < size; ++localId)
  {
    auto& output = outputs[localId];
    auto& pointCloud = pointCloudArray[localId];
    vtkBoundingBox bounds(output->GetBounds());

    // Locator to be used for point search inside the point cloud of bounding spheres.
    if (bounds.ComputeInnerDimension() == 2)
    {
      vtkNew<vtkStaticPointLocator2D> locator;
      locator->SetDataSet(pointCloud);
      pointCloud->SetPointLocator(locator);
    }

    // Locator to be used for point search inside the point cloud of bounding spheres.
    if (bounds.ComputeInnerDimension() == 2)
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
    std::unordered_map<vtkIdType, std::set<vtkIdType>> localCollisionListMaps;
    queryCellDataSetsArray.emplace_back(std::move(master.block<Block>(localId)->DataSets));
    auto& queryCellDataSets = queryCellDataSetsArray[localId];

    const auto& cellBoundingBoxes = cellBoundingBoxesArray[localId];

    if (!this->DetectOverlappingCells(output, pointCloud, cellBoundingBoxes, output, pointCloud,
          cellBoundingBoxes, localCollisionListMaps, true /* updateProgress */))
    {
      vtkErrorMacro(<< "Failed to detect self colliding cells");
      return 0;
    }

    auto& collisionListMapList = collisionListMapListArray[localId];

    // We now detect collision with the cells sent by other blocks.
    for (auto& pair : queryCellDataSets)
    {
      vtkSmartPointer<vtkDataSet>& queryCellDataSet = pair.second;
      int globalId = pair.first;
      std::vector<vtkBoundingBox> queryCellBoundingBoxes;
      vtkSmartPointer<vtkPointSet> queryPointCloud = vtkSmartPointer<vtkPointSet>::Take(
        ConvertCellsToBoundingSpheres(queryCellDataSet, queryCellBoundingBoxes));

      if (!this->DetectOverlappingCells(queryCellDataSet, queryPointCloud, queryCellBoundingBoxes,
            output, pointCloud, cellBoundingBoxes, collisionListMapList[globalId]))
      {
        vtkErrorMacro(<< "Failed to detect self colliding cells in process " << myrank << " on "
                      << globalId << "th local block");
        return 0;
      }
    }
  }
  vtkLogEndScope("locally treat received cells");

  // We need to send back collision information to the original block, so they
  // can add the collisions they couldn't detect.
  vtkLogStartScope(TRACE, "send back detected overlaps");
  master.foreach (
    [&master, &collisionListMapListArray](void*, const diy::Master::ProxyWithLink& cp) {
      int myBlockId = cp.gid();
      int localId = master.lid(myBlockId);
      auto& collisionListMapList = collisionListMapListArray[localId];
      // enqueue
      for (int i = 0; i < static_cast<int>(cp.link()->size()); ++i)
      {
        vtkdiy2::BlockID& targetBlockId = cp.link()->target(i);
        cp.enqueue(targetBlockId, collisionListMapList[targetBlockId.gid]);
      }
    });
  master.exchange();
  master.foreach ([](Block* block, const diy::Master::ProxyWithLink& cp) {
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
  vtkLogEndScope("send back detected overlaps");

  vtkLogStartScope(TRACE, "add detected overlaps from other ranks");
  for (int localId = 0; localId < size; ++localId)
  {
    auto& collisionIdList = master.block<Block>(localId)->CollisionListMaps;
    auto& collisionListMapList = collisionListMapListArray[localId];
    vtkIdTypeArray* queryNumberOfOverlapsPerCell = vtkIdTypeArray::SafeDownCast(
      outputs[localId]->GetCellData()->GetAbstractArray(this->NumberOfOverlapsPerCellArrayName));
    auto& queryCellDataSets = queryCellDataSetsArray[localId];

    // Last pass. We look at what intersections were found in the other blocks, and check if
    // we found them or not, and increment collision count accordingly.
    for (const auto& collisionIdPair : collisionIdList)
    {
      int globalId = collisionIdPair.first;
      auto& collisionIds = collisionIdPair.second;
      auto& collisionListMap = collisionListMapList[globalId];
      vtkDataSet* queryCellDataSet = queryCellDataSets[globalId];
      vtkIdTypeArray* neighborIdMapArray = vtkIdTypeArray::SafeDownCast(
        queryCellDataSet->GetCellData()->GetArray(ID_MAP_TO_ORIGIN_DATASET_IDS_NAME));
      for (const auto& pair : collisionListMap)
      {
        // pair.first <=> id of neighbor process which has collision with at least one of our cell.
        // pair.second <=> list of ids from this process colliding with cell of id pair.first
        for (vtkIdType id : pair.second)
        {
          auto it = collisionIds.find(neighborIdMapArray->GetValue(id));
          // it->first <=> id in this process which has collision with at least one of our
          // neighbor's cell. it->second <=> list of ids from our neighbor process colliding with
          // cell of id it->first.
          if (it == collisionIds.end() || !pair.second.count(id))
          {
            queryNumberOfOverlapsPerCell->SetValue(
              pair.first, queryNumberOfOverlapsPerCell->GetValue(pair.first) + 1);
          }
        }
      }
    }
  }
  vtkLogEndScope("add detected overlaps from other ranks");

  this->UpdateProgress(1.0);

  for (auto output : outputs)
  {
    output->GetCellData()->SetActiveScalars(this->GetNumberOfOverlapsPerCellArrayName());
  }

  return 1;
}

//----------------------------------------------------------------------------
// Main collision detection algorithm.
bool vtkOverlappingCellsDetector::DetectOverlappingCells(vtkDataSet* queryCellDataSet,
  vtkPointSet* queryPointCloud, const std::vector<vtkBoundingBox>& queryCellBoundingBoxes,
  vtkDataSet* cellDataSet, vtkPointSet* pointCloud,
  const std::vector<vtkBoundingBox>& cellBoundingBoxes,
  std::unordered_map<vtkIdType, std::set<vtkIdType>>& collisionListMap, bool updateProgress)
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
    queryPointCloud->GetPointData()->GetArray(SPHERE_RADIUS_ARRAY_NAME);

  vtkIdType querySize = queryPointCloud->GetNumberOfPoints();
  vtkIdType twentieth = querySize / 20 + 1;
  double decimal = 0.0;

  vtkNew<vtkIdTypeArray> queryNumberOfOverlapsPerCellsArray;
  queryNumberOfOverlapsPerCellsArray->SetNumberOfComponents(1);
  queryNumberOfOverlapsPerCellsArray->SetNumberOfTuples(querySize);
  queryNumberOfOverlapsPerCellsArray->SetName(this->GetNumberOfOverlapsPerCellArrayName());
  queryNumberOfOverlapsPerCellsArray->Fill(0.0);

  // Handling case where both input data sets point to the same address.
  vtkIdTypeArray* numberOfCollisionPerCellsArray = queryCellDataSet != cellDataSet
    ? vtkArrayDownCast<vtkIdTypeArray>(
        cellDataSet->GetCellData()->GetArray(this->GetNumberOfOverlapsPerCellArrayName()))
    : queryNumberOfOverlapsPerCellsArray;

  vtkNew<vtkIdList> neighborIds;

  // We want to discard ghost cells, so we have to acknowledge them.
  vtkUnsignedCharArray* queryCellGhostArray = queryCellDataSet->GetCellGhostArray();
  vtkUnsignedCharArray* cellGhostArray = cellDataSet->GetCellGhostArray();

  // local cell bank to avoid calling ::New() too many times.
  std::map<int, vtkSmartPointer<vtkCell>> cellBank, neighborCellBank;
  for (vtkIdType id = 0; id < querySize; ++id)
  {
    if (updateProgress)
    {
      // Update progress
      if (!(id % twentieth))
      {
        decimal += 0.05;
        this->UpdateProgress(decimal);
      }
    }
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
    double currentCellTolerance = std::max(ComputeEpsilon(bbox), 0.5 * this->Tolerance);
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
        // We have a bank of cells to alleviate dynamic allocating when possible.
        int neighborCellType = cellDataSet->GetCellType(neighborId);
        auto neighborCellBankHandle = neighborCellBank.find(neighborCellType);
        vtkCell* neighborCell;
        if (neighborCellBankHandle == neighborCellBank.end())
        {
          neighborCell = vtkGenericCell::InstantiateCell(neighborCellType);
          neighborCellBank[neighborCellType] = vtkSmartPointer<vtkCell>::Take(neighborCell);
        }
        else
        {
          neighborCell = neighborCellBankHandle->second;
        }
        neighborCell->DeepCopy(cellDataSet->GetCell(neighborId));

        // Shrinking this cell as well.
        double neighborCellTolerance =
          std::max(ComputeEpsilon(cellBoundingBoxes[neighborId]), 0.5 * this->Tolerance);
        neighborCell->Inflate(-neighborCellTolerance);
        if (currentCell->IntersectWithCell(neighborCell, bbox, cellBoundingBoxes[neighborId]))
        {
          ++intersectionCount;
          numberOfCollisionPerCellsArray->SetValue(
            neighborId, numberOfCollisionPerCellsArray->GetValue(neighborId) + 1);
          collisionListMap[neighborId].insert(id);
        }
      }
    }
    if (intersectionCount)
    {
      queryNumberOfOverlapsPerCellsArray->SetValue(
        id, queryNumberOfOverlapsPerCellsArray->GetValue(id) + intersectionCount);
    }
  }
  queryCellDataSet->GetCellData()->AddArray(queryNumberOfOverlapsPerCellsArray);

  return true;
}

//----------------------------------------------------------------------------
void vtkOverlappingCellsDetector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "NumberOfOverlapsPerCellArrayName: " << this->NumberOfOverlapsPerCellArrayName
     << std::endl;
  os << indent << "Tolerance: " << this->Tolerance << std::endl;
}
VTK_ABI_NAMESPACE_END
