// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractEdges.h"

#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkEdgeTable.h"
#include "vtkGenericCell.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStaticEdgeLocatorTemplate.h"

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
struct ExtractEdges
{
  // The data of the edge tuple is cell id
  using EdgeTupleType = EdgeTuple<vtkIdType, vtkIdType>;
  using EdgesType = std::vector<EdgeTupleType>;

  vtkPolyData* Output;
  vtkCellData* InCD;
  vtkCellData* OutCD;
  vtkSMPThreadLocal<EdgesType> Edges;

  // If inCD==nullptr, don't produce output cell data
  ExtractEdges(vtkPolyData* output, vtkCellData* inCD, vtkCellData* outCD)
    : Output(output)
    , InCD(inCD)
    , OutCD(outCD)
  {
  }

  // operator() provided by subclasses

  // Composite the threads local data into a final output
  void Reduce()
  {
    // Count the number of edges which likely contains duplicates.
    EdgesType edges;
    vtkSMPThreadLocal<EdgesType>::iterator ldItr;
    vtkSMPThreadLocal<EdgesType>::iterator ldEnd = this->Edges.end();
    for (ldItr = this->Edges.begin(); ldItr != ldEnd; ++ldItr)
    {
      edges.insert(edges.end(), (*ldItr).begin(), (*ldItr).end());
    } // over all threads

    // Now sort the edges with an edge locator. This will gather all duplicate
    // edges together and indicate the number of unique edges.
    vtkStaticEdgeLocatorTemplate<vtkIdType, vtkIdType> edgeLoc;
    vtkIdType totalEdges = 0;
    const vtkIdType* edgeOffsets =
      edgeLoc.MergeEdges(static_cast<vtkIdType>(edges.size()), edges.data(), totalEdges);

    // Allocate output VTK structures and construct the output lines.
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(totalEdges + 1);
    vtkIdType* offsetsPtr = offsets->GetPointer(0);
    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(2 * totalEdges);
    vtkIdType* connPtr = conn->GetPointer(0);

    // In place lambda to do the threaded copying of edges
    vtkSMPTools::For(0, totalEdges,
      [&edgeOffsets, &edges, offsetsPtr, connPtr](vtkIdType edgeId, vtkIdType endEdgeId)
      {
        for (; edgeId < endEdgeId; ++edgeId)
        {
          vtkIdType* c = connPtr + 2 * edgeId;
          const EdgeTupleType& edge = edges[edgeOffsets[edgeId]];
          offsetsPtr[edgeId] = 2 * edgeId;
          c[0] = edge.V0;
          c[1] = edge.V1;
        }
      });

    vtkCellArray* newLines = this->Output->GetLines();
    offsetsPtr[totalEdges] = 2 * totalEdges; // top off cell array offsets
    newLines->SetData(offsets, conn);

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

// Extract polydata edges: lines, polygons, triangle strips. We don't
// use the general ExtractDataSetEdges because it is slower, and it is
// currently not thread safe (GetCell(id,genCell)) invokes BuildCells().
struct ExtractPolyDataEdges : public ExtractEdges
{
  vtkCellArray* Lines;
  vtkIdType NumLines;
  vtkCellArray* Polys;
  vtkIdType NumPolys;
  vtkCellArray* Strips;
  vtkIdType NumStrips;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> LinesIterator;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> PolysIterator;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> StripsIterator;

  // If inCD==nullptr, don't produce output cell data
  ExtractPolyDataEdges(
    vtkPolyData* input, vtkPolyData* output, vtkCellData* inCD, vtkCellData* outCD)
    : ExtractEdges(output, inCD, outCD)
  {
    this->Lines = input->GetLines();
    this->NumLines = this->Lines->GetNumberOfCells();
    this->Polys = input->GetPolys();
    this->NumPolys = this->Polys->GetNumberOfCells();
    this->Strips = input->GetStrips();
    this->NumStrips = this->Strips->GetNumberOfCells();
  }

  void Initialize()
  {
    if (this->NumLines > 0)
    {
      this->LinesIterator.Local().TakeReference(this->Lines->NewIterator());
    }
    if (this->NumPolys > 0)
    {
      this->PolysIterator.Local().TakeReference(this->Polys->NewIterator());
    }
    if (this->NumStrips > 0)
    {
      this->StripsIterator.Local().TakeReference(this->Strips->NewIterator());
    }
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& edges = this->Edges.Local();
    vtkCellArrayIterator* linesIter = this->LinesIterator.Local();
    vtkCellArrayIterator* polysIter = this->PolysIterator.Local();
    vtkCellArrayIterator* stripsIter = this->StripsIterator.Local();
    vtkIdType npts;
    const vtkIdType* pts;

    // We use a bit of hack here. To keep things simple in vtkSMPTools::For()
    // the numCells in the for loop may not match the numCells in each of the
    // line, polys, and strips, so may have to truncate the traversal.
    vtkIdType linesId = cellId;
    vtkIdType endLinesId = (endCellId < this->NumLines ? endCellId : this->NumLines);
    for (; linesId < endLinesId; ++linesId)
    {
      linesIter->GetCellAtId(linesId, npts, pts);
      for (auto i = 0; i < (npts - 1); ++i)
      {
        vtkIdType v0 = pts[i];
        vtkIdType v1 = pts[i + 1];
        edges.emplace_back(v0, v1, linesId);
      }
    } // for all line cells in this batch

    // Polygons
    vtkIdType polysId = cellId;
    vtkIdType endPolysId = (endCellId < this->NumPolys ? endCellId : this->NumPolys);
    for (; polysId < endPolysId; ++polysId)
    {
      polysIter->GetCellAtId(polysId, npts, pts);
      for (auto i = 0; i < npts; ++i)
      {
        vtkIdType v0 = pts[i];
        vtkIdType v1 = pts[(i + 1) % npts];
        edges.emplace_back(v0, v1, polysId);
      }
    } // for all polygons in this batch

    // Strips
    vtkIdType stripsId = cellId;
    vtkIdType endStripsId = (endCellId < this->NumStrips ? endCellId : this->NumStrips);
    for (; stripsId < endStripsId; ++stripsId)
    {
      stripsIter->GetCellAtId(stripsId, npts, pts);
      vtkIdType v0 = pts[0];
      vtkIdType v1 = pts[1];
      vtkIdType v2;
      for (auto i = 0; i < (npts - 2); ++i)
      {
        v2 = pts[i + 1];
        edges.emplace_back(v0, v1, stripsId);
        edges.emplace_back(v1, v2, stripsId);
        edges.emplace_back(v2, v0, stripsId);
        v0 = v1;
        v1 = v2;
      }
    } // for all triangle strip cells in this batch
  }
}; // ExtractPolyDataEdges

// Extract edges from arbitrary dataset
struct ExtractDataSetEdges : public ExtractEdges
{
  vtkDataSet* Input;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> HEedgeIds;
  vtkSMPThreadLocal<vtkSmartPointer<vtkPoints>> HEedgePts;

  // If inCD==nullptr, don't produce output cell data
  ExtractDataSetEdges(vtkDataSet* input, vtkPolyData* output, vtkCellData* inCD, vtkCellData* outCD)
    : ExtractEdges(output, inCD, outCD)
    , Input(input)
  {
  }

  void Initialize()
  {
    this->Cell.Local() = vtkSmartPointer<vtkGenericCell>::New();
    this->HEedgeIds.Local() = vtkSmartPointer<vtkIdList>::New();
    this->HEedgePts.Local() = vtkSmartPointer<vtkPoints>::New();
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& edges = this->Edges.Local();
    auto& genCell = this->Cell.Local();
    auto& hEedgeIds = this->HEedgeIds.Local();
    auto& hEedgePts = this->HEedgePts.Local();
    vtkDataSet* input = this->Input;
    vtkIdList* edgeIds;
    vtkIdType pts[2];

    for (; cellId < endCellId; ++cellId)
    {
      input->GetCell(cellId, genCell);
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
            pts[0] = edgeIds->GetId(2 * i);
            pts[1] = edgeIds->GetId(2 * i + 1);
            edges.emplace_back(pts[0], pts[1], cellId);
          }
        } // if non-linear edge

        else // linear edges
        {
          edgeIds = edge->PointIds;

          for (vtkIdType i = 0; i < numEdgePts; i++, pts[0] = pts[1])
          {
            pts[1] = edgeIds->GetId(i);
            if (i > 0)
            {
              edges.emplace_back(pts[0], pts[1], cellId);
            }
          } // if linear edge
        }
      } // for all edges of cell
    }   // for all cells in this batch
  }     // Reduce
};      // ExtractDataSetEdges

//------------------------------------------------------------------------------
// Extract edges from a dataset without a locator - meaning all of the
// original points exist in the output (i.e., point numbering does not
// change).
int NonLocatorExtraction(
  vtkIdType numPts, vtkIdType numCells, vtkDataSet* input, vtkPolyData* output)
{
  vtkLog(TRACE, "Executing edge extractor with original point numbering");

  // Is the input a pointset?  In that case we can just reuse the input's
  // points.
  vtkPointSet* ps = vtkPointSet::SafeDownCast(input);
  if (ps)
  {
    output->SetPoints(ps->GetPoints());
  }
  else
  {
    // We need to copy the points
    vtkNew<vtkPoints> newPts;
    newPts->SetNumberOfPoints(numPts);
    output->SetPoints(newPts);

    vtkSMPTools::For(0, numPts,
      [&input, &newPts](vtkIdType ptId, vtkIdType endPtId)
      {
        double pnt[3];
        for (; ptId < endPtId; ++ptId)
        {
          input->GetPoint(ptId, pnt);
          newPts->SetPoint(ptId, pnt);
        }
      });
  }

  // Instantiate a cell array to collect all the edges as
  // lines.
  vtkNew<vtkCellArray> newLines;
  output->SetLines(newLines);

  // Since we are using all of the points, we can
  // simply pass through the Point Data
  output->GetPointData()->PassData(input->GetPointData());

  // Assigning cell data to edges requires special work to ensure the output
  // is deterministic. Edges are typically shared by multiple cells, so
  // assigning cell data to an edge is undefined unless an ordering is
  // applied. In sequential processing, the order is simply the increasing
  // order of cell visitation (i.e., the minimum cell id is used). This shows
  // up in threaded environments because the order of execution of threads
  // affects the output. To ensure deterministic output, the minimum cell id
  // is used (consistent with sequential processing). This requires extra
  // work, so setup a fast path in case there is no cell data.
  vtkCellData* cd = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  cd = (cd->GetNumberOfArrays() > 0 ? cd : nullptr);

  // Algorithm proper. There is a fast path for polydata.
  vtkPolyData* inPolyData = vtkPolyData::SafeDownCast(input);
  if (inPolyData)
  {
    ExtractPolyDataEdges extractPE(inPolyData, output, cd, outCD);
    vtkSMPTools::For(0, numCells, extractPE);
  }
  else
  {
    ExtractDataSetEdges extractDSE(input, output, cd, outCD);
    vtkSMPTools::For(0, numCells, extractDSE);
  }

  vtkLog(TRACE, "Created " << newLines->GetNumberOfCells() << " edges");

  return 1;
}

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// Generate feature edges for mesh. If UseAllPoints is disabled, then a locator
// is employed which is slower and inherently serial. (This could be sped up
// if the output of the filter is allowed to change - which may affect past behavior).
// If UseAllPoints is true, then a threaded approach is used which avoids the
// use of a point locator.
int vtkExtractEdges::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //  Check input
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  if (numCells < 1 || numPts < 1)
  {
    return 1;
  }

  // If we are using all of the points use a non-locator based approach.
  if (this->UseAllPoints)
  {
    return NonLocatorExtraction(numPts, numCells, input, output);
  }

  vtkLog(TRACE, "Executing edge extractor: points are renumbered");

  // Using a locator
  vtkIdType cellNum, newId;
  int edgeNum, numEdgePts, numCellEdges;
  int i, abort = 0;
  vtkIdType pts[2];
  vtkIdType pt1 = 0, pt2;
  double x[3];
  vtkCell* edge;
  vtkPointData *pd, *outPD;
  vtkCellData *cd, *outCD;

  // Set up processing
  //
  vtkNew<vtkEdgeTable> edgeTable;
  edgeTable->InitEdgeInsertion(numPts);
  vtkNew<vtkPoints> newPts;
  newPts->Allocate(numPts);
  vtkNew<vtkCellArray> newLines;
  newLines->AllocateEstimate(numPts * 4, 2);

  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd, numPts);

  cd = input->GetCellData();
  outCD = output->GetCellData();
  outCD->CopyAllocate(cd, numCells);

  vtkNew<vtkGenericCell> cell;
  vtkIdList* edgeIds;
  vtkNew<vtkIdList> HEedgeIds;
  vtkPoints* edgePts;
  vtkNew<vtkPoints> HEedgePts;

  // Get our locator for merging points
  //
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds());

  // Loop over all cells, extracting non-visited edges.
  //
  vtkIdType tenth = numCells / 10 + 1;
  for (cellNum = 0; cellNum < numCells && !abort; cellNum++)
  {
    if (!(cellNum % tenth)) // manage progress reports / early abort
    {
      this->UpdateProgress(static_cast<double>(cellNum) / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(cellNum, cell);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum = 0; edgeNum < numCellEdges; edgeNum++)
    {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();

      // Tessellate higher-order edges
      if (!edge->IsLinear())
      {
        edge->Triangulate(0, HEedgeIds, HEedgePts);
        edgeIds = HEedgeIds;
        edgePts = HEedgePts;

        for (i = 0; i < (edgeIds->GetNumberOfIds() / 2); i++)
        {
          pt1 = edgeIds->GetId(2 * i);
          pt2 = edgeIds->GetId(2 * i + 1);
          edgePts->GetPoint(2 * i, x);
          if (this->Locator->InsertUniquePoint(x, pts[0]))
          {
            outPD->CopyData(pd, pt1, pts[0]);
          }
          edgePts->GetPoint(2 * i + 1, x);
          if (this->Locator->InsertUniquePoint(x, pts[1]))
          {
            outPD->CopyData(pd, pt2, pts[1]);
          }
          if (edgeTable->IsEdge(pt1, pt2) == -1)
          {
            edgeTable->InsertEdge(pt1, pt2);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        }
      } // if non-linear edge

      else // linear edges
      {
        edgeIds = edge->PointIds;
        edgePts = edge->Points;

        for (i = 0; i < numEdgePts; i++, pt1 = pt2, pts[0] = pts[1])
        {
          pt2 = edgeIds->GetId(i);
          edgePts->GetPoint(i, x);
          if (this->Locator->InsertUniquePoint(x, pts[1]))
          {
            outPD->CopyData(pd, pt2, pts[1]);
          }
          if (i > 0 && edgeTable->IsEdge(pt1, pt2) == -1)
          {
            edgeTable->InsertEdge(pt1, pt2);
            newId = newLines->InsertNextCell(2, pts);
            outCD->CopyData(cd, cellNum, newId);
          }
        } // if linear edge
      }
    } // for all edges of cell
  }   // for all cells

  vtkLog(TRACE, "Created " << newLines->GetNumberOfCells() << " edges");

  //  Update ourselves.
  //
  output->SetPoints(newPts);
  output->SetLines(newLines);

  output->Squeeze();

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
