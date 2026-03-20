// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractEdges.h"

#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStaticCleanUnstructuredGrid.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStaticPointLocator.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractEdges);

//------------------------------------------------------------------------------
// Construct object.
vtkExtractEdges::vtkExtractEdges()
{
  this->UseAllPoints = false;
}
VTK_ABI_NAMESPACE_END

// The following namespace supports a threaded algorithm for extracting edges
// while retaining the initial point ids. Using this threading approach, each
// thread extracts all the edges from the cells it processes, and then
// eventually composites / reduces them (including repeats) into a list which
// is then sorted.  After sorting, duplicate edges are easily found, and only
// one edge from the set of duplicate edges is output. This approach is
// codified in the classes defined by vtkStaticEdgeLocatorTemplate, with
// MergeEdges() being the essential method.
//
namespace
{ // anonymous

// Base class for extracting edges, preserving point numbering.
template <typename TDataSet>
struct ExtractEdges
{
  // The data of the edge tuple is cell id
  using EdgeTupleType = EdgeTuple<vtkIdType, vtkIdType>;
  using EdgesType = std::vector<EdgeTupleType>;

  TDataSet* Input;
  vtkPolyData* Output;
  vtkCellData* InCD;
  vtkCellData* OutCD;

  vtkSMPThreadLocal<EdgesType> Edges;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> TLCell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> TLHEedgeIds;
  vtkSMPThreadLocal<vtkSmartPointer<vtkPoints>> TLHEedgePts;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> TLPointIds;

  // If inCD==nullptr, don't produce output cell data
  ExtractEdges(TDataSet* input, vtkPolyData* output, vtkCellData* inCD, vtkCellData* outCD)
    : Input(input)
    , Output(output)
    , InCD(inCD)
    , OutCD(outCD)
  {
    if constexpr (std::is_same_v<TDataSet, vtkPolyData>)
    {
      if (this->Input->NeedToBuildCells())
      {
        this->Input->BuildCells();
      }
    }
  }

  void Initialize()
  {
    this->TLCell.Local() = vtkSmartPointer<vtkGenericCell>::New();
    this->TLHEedgeIds.Local() = vtkSmartPointer<vtkIdList>::New();
    this->TLHEedgePts.Local() = vtkSmartPointer<vtkPoints>::New();
    this->TLPointIds.Local() = vtkSmartPointer<vtkIdList>::New();
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& edges = this->Edges.Local();
    auto& genCell = this->TLCell.Local();
    auto& hEedgeIds = this->TLHEedgeIds.Local();
    auto& hEedgePts = this->TLHEedgePts.Local();
    auto& pointIds = this->TLPointIds.Local();
    vtkIdType npts;
    const vtkIdType* pts;
    vtkIdList* edgeIds;
    vtkIdType edgePts[2];

    for (; cellId < endCellId; ++cellId)
    {
      const int cellType = this->Input->GetCellType(cellId);
      switch (cellType)
      {
        case VTK_EMPTY_CELL:
        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
          break;
        case VTK_LINE:
        case VTK_POLY_LINE:
        {
          this->Input->GetCellPoints(cellId, npts, pts, pointIds);
          for (auto i = 0; i < (npts - 1); ++i)
          {
            vtkIdType v0 = pts[i];
            vtkIdType v1 = pts[i + 1];
            edges.emplace_back(v0, v1, cellId);
          }
          break;
        }
        case VTK_TRIANGLE:
        case VTK_QUAD:
        case VTK_POLYGON:
        {
          this->Input->GetCellPoints(cellId, npts, pts, pointIds);
          for (auto i = 0; i < npts; ++i)
          {
            vtkIdType v0 = pts[i];
            vtkIdType v1 = pts[(i + 1) % npts];
            edges.emplace_back(v0, v1, cellId);
          }
          break;
        }
        case VTK_TRIANGLE_STRIP:
        {
          this->Input->GetCellPoints(cellId, npts, pts, pointIds);
          vtkIdType v0 = pts[0];
          vtkIdType v1 = pts[1];
          vtkIdType v2;
          for (auto i = 0; i < (npts - 2); ++i)
          {
            v2 = pts[i + 1];
            edges.emplace_back(v0, v1, cellId);
            edges.emplace_back(v1, v2, cellId);
            edges.emplace_back(v2, v0, cellId);
            v0 = v1;
            v1 = v2;
          }
          break;
        }
        default:
        {
          this->Input->GetCell(cellId, genCell);
          int numCellEdges = genCell->GetNumberOfEdges();
          for (int edgeNum = 0; edgeNum < numCellEdges; edgeNum++)
          {
            vtkCell* edge = genCell->GetEdge(edgeNum);
            int numEdgePts = edge->GetNumberOfPoints();

            // Tessellate higher-order edges
            if (!edge->IsLinear())
            {
              edge->Triangulate(0, hEedgeIds, hEedgePts);
              edgeIds = hEedgeIds;

              for (vtkIdType i = 0; i < (edgeIds->GetNumberOfIds() / 2); i++)
              {
                edgePts[0] = edgeIds->GetId(2 * i);
                edgePts[1] = edgeIds->GetId(2 * i + 1);
                edges.emplace_back(edgePts[0], edgePts[1], cellId);
              }
            } // if non-linear edge

            else // linear edges
            {
              edgeIds = edge->PointIds;

              for (vtkIdType i = 0; i < numEdgePts; i++, edgePts[0] = edgePts[1])
              {
                edgePts[1] = edgeIds->GetId(i);
                if (i > 0)
                {
                  edges.emplace_back(edgePts[0], edgePts[1], cellId);
                }
              } // if linear edge
            }
          } // for all edges of cell
          break;
        }
      }
    }
  }

  // Composite the threads local data into a final output
  void Reduce()
  {
    // Count the number of edges which likely contains duplicates.
    vtkIdType totalDuplicateEdges = 0;
    for (const auto& tlEdges : this->Edges)
    {
      totalDuplicateEdges += static_cast<vtkIdType>(tlEdges.size());
    } // over all threads
    EdgesType edges;
    edges.reserve(static_cast<size_t>(totalDuplicateEdges));
    for (const auto& tlEdges : this->Edges)
    {
      edges.insert(edges.end(), tlEdges.begin(), tlEdges.end());
    } // over all threads

    // Now sort the edges with an edge locator. This will gather all duplicate
    // edges together and indicate the number of unique edges.
    vtkStaticEdgeLocatorTemplate<vtkIdType, vtkIdType> edgeLoc;
    vtkIdType totalEdges = 0;
    const vtkIdType* edgeOffsets =
      edgeLoc.MergeEdges(static_cast<vtkIdType>(edges.size()), edges.data(), totalEdges);

    // Allocate output VTK structures and construct the output lines.
    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfValues(2 * totalEdges);
    vtkIdType* connPtr = conn->GetPointer(0);

    // In place lambda to do the threaded copying of edges
    vtkSMPTools::For(0, totalEdges,
      [&edgeOffsets, &edges, connPtr](vtkIdType edgeId, vtkIdType endEdgeId)
      {
        for (; edgeId < endEdgeId; ++edgeId)
        {
          vtkIdType* c = connPtr + 2 * edgeId;
          const EdgeTupleType& edge = edges[edgeOffsets[edgeId]];
          c[0] = edge.V0;
          c[1] = edge.V1;
        }
      });

    vtkCellArray* newLines = this->Output->GetLines();
    newLines->SetData(2, conn);

    // If cell data has been requested, produce it. Note that because of the
    // undefined nature of cell data on an edge, we use the lowest cell id
    // as the winning cell from which to copy cell data.
    if (this->InCD)
    {
      ArrayList cellArrays;
      this->OutCD->CopyAllocate(this->InCD, totalEdges);
      cellArrays.AddArrays(
        totalEdges, this->InCD, this->OutCD, /*nullValue*/ 0.0, /*promote*/ false);

      vtkSMPTools::For(0, totalEdges,
        [&edgeOffsets, &edges, &cellArrays](vtkIdType edgeId, vtkIdType endEdgeId)
        {
          for (; edgeId < endEdgeId; ++edgeId)
          {
            vtkIdType numEdges = edgeOffsets[edgeId + 1] - edgeOffsets[edgeId];
            vtkIdType cellId, minCellId = VTK_ID_MAX;
            for (auto i = 0; i < numEdges; ++i)
            {
              const EdgeTupleType& edge = edges[edgeOffsets[edgeId] + i];
              cellId = edge.Data;
              minCellId = (cellId < minCellId ? cellId : minCellId);
            }
            cellArrays.Copy(minCellId, edgeId);
          }
        });
    } // cell data copied onto edges
  }
}; // ExtractEdges

//  Update the cell connectivity array.
struct UpdateCellArrayConnectivity : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* vtkNotUsed(offsets), ConnectivityT* connectivity, vtkIdType* ptMap)
  {
    auto connRange = GetRange(connectivity);
    vtkSMPTools::For(0, connectivity->GetNumberOfValues(),
      [&](vtkIdType beginId, vtkIdType endId)
      {
        std::transform(connRange.begin() + beginId, connRange.begin() + endId,
          connRange.begin() + beginId, [&ptMap](vtkIdType ptId) { return ptMap[ptId]; });
      }); // end lambda
  }
};
} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// Generate feature edges for mesh.
int vtkExtractEdges::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  //  Check input
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  if (numCells < 1 || numPts < 1)
  {
    return 1;
  }

  vtkPoints* inPts = input->GetPoints();
  vtkNew<vtkPoints> outPts;
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();

  auto CopyPointsAndData = [&]()
  {
    // Is the input a pointset?  In that case we can just reuse the input's points.
    if (vtkPointSet::SafeDownCast(input))
    {
      outPts->GetData()->ShallowCopy(inPts->GetData());
      output->SetPoints(outPts);
    }
    else
    {
      // We need to copy the points
      outPts->SetDataType(inPts->GetDataType());
      outPts->SetNumberOfPoints(numPts);
      outPts->GetData()->DeepCopy(inPts->GetData());
      output->SetPoints(outPts);
    }

    // Since we are using all the points, we can simply pass through the Point Data
    output->GetPointData()->PassData(input->GetPointData());
  };

  // Instantiate a cell array to collect all the edges as lines.
  vtkNew<vtkCellArray> edges;
  output->SetLines(edges);

  // Assigning cell data to edges requires special work to ensure the output
  // is deterministic. Edges are typically shared by multiple cells, so
  // assigning cell data to an edge is undefined unless an ordering is
  // applied. In sequential processing, the order is simply the increasing
  // order of cell visitation (i.e., the minimum cell id is used). This shows
  // up in threaded environments because the order of execution of threads
  // affects the output. To ensure deterministic output, the minimum cell id
  // is used (consistent with sequential processing). This requires extra
  // work, so setup a fast path in case there is no cell data.
  auto actualCD = (inCD->GetNumberOfArrays() > 0 ? inCD : nullptr);

  // Algorithm proper.
  if (auto inPolyData = vtkPolyData::SafeDownCast(input))
  {
    ExtractEdges<vtkPolyData> extract(inPolyData, output, actualCD, outCD);
    vtkSMPTools::For(0, numCells, extract);
  }
  else if (auto inUGrid = vtkUnstructuredGrid::SafeDownCast(input))
  {
    ExtractEdges<vtkUnstructuredGrid> extract(inUGrid, output, actualCD, outCD);
    vtkSMPTools::For(0, numCells, extract);
  }
  else
  {
    ExtractEdges<vtkDataSet> extract(input, output, actualCD, outCD);
    vtkSMPTools::For(0, numCells, extract);
  }
  this->UpdateProgress(0.5);

  // If we are using all the points don't remove unused points.
  if (this->UseAllPoints)
  {
    CopyPointsAndData();
    this->UpdateProgress(1.0);
    return 1;
  }

  // vtkStaticPointLocator is used to remove unused points
  vtkNew<vtkStaticPointLocator> locator;
  locator->SetDataSet(input);
  locator->BuildLocator();
  this->UpdateProgress(0.55);

  // Now merge the points to create a merge map.
  std::vector<vtkIdType> mergeMap(numPts);
  locator->MergePoints(/*tol*/ 0, mergeMap.data());
  this->UpdateProgress(0.6);

  // traverse the connectivity array to mark the points that are used by one or more cells.
  std::vector<unsigned char> ptUses(numPts, 0);
  vtkStaticCleanUnstructuredGrid::MarkPointUses(edges, mergeMap.data(), ptUses.data());
  this->UpdateProgress(0.7);

  // Create a map that maps old point ids into new, renumbered point ids.
  // Build the map from old points to new points.
  std::vector<vtkIdType> pmap(numPts);
  vtkIdType numNewPts =
    vtkStaticCleanUnstructuredGrid::BuildPointMap(numPts, pmap.data(), ptUses.data(), mergeMap);
  this->UpdateProgress(0.8);
  if (numNewPts == numPts)
  {
    // if all points are used, then getting used points
    CopyPointsAndData();
    this->UpdateProgress(1.0);
    return 1;
  }

  // Produce output points and associated point data.
  outPts->SetDataType(inPts->GetDataType());
  outPts->SetNumberOfPoints(numNewPts);
  output->SetPoints(outPts);
  outPD->CopyAllocate(inPD, numNewPts);
  vtkStaticCleanUnstructuredGrid::CopyPoints(inPts, inPD, outPts, outPD, pmap.data());
  this->UpdateProgress(0.9);

  // Update the cell connectivity using the point map.
  edges->Dispatch(UpdateCellArrayConnectivity{}, pmap.data());
  this->UpdateProgress(1.0);

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractEdges::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator.TakeReference(vtkMergePoints::New());
  }
}

//------------------------------------------------------------------------------
int vtkExtractEdges::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkExtractEdges::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//------------------------------------------------------------------------------
void vtkExtractEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << " UseAllPoints:" << this->UseAllPoints << "\n";
  }
  else
  {
    os << indent << "Locator: (none) UseAllPoints:" << this->UseAllPoints << "\n";
  }
}
VTK_ABI_NAMESPACE_END
