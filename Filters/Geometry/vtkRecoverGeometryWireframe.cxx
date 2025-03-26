// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkRecoverGeometryWireframe.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------------
static const unsigned char NO_EDGE_FLAG = std::numeric_limits<unsigned char>::max();

//-----------------------------------------------------------------------------
// Simple class used internally to define an edge based on the endpoints.
// Endpoints are sorted.
struct EdgeEndpoints
{
  EdgeEndpoints(vtkIdType endpointA, vtkIdType endpointB)
    : MinEndPoint(std::min(endpointA, endpointB))
    , MaxEndPoint(std::max(endpointA, endpointB))
  {
  }

  EdgeEndpoints(const EdgeEndpoints&) = default;
  void operator=(const EdgeEndpoints&) = delete;

  inline bool operator==(const EdgeEndpoints& other) const
  {
    return ((this->MinEndPoint == other.MinEndPoint) && (this->MaxEndPoint == other.MaxEndPoint));
  }

  const vtkIdType MinEndPoint = -1;
  const vtkIdType MaxEndPoint = -1;
};

//-----------------------------------------------------------------------------
struct EdgeEndpointsHash
{
  size_t operator()(const EdgeEndpoints& edge) const
  {
    return static_cast<size_t>(edge.MinEndPoint + edge.MaxEndPoint);
  }
};

//-----------------------------------------------------------------------------
// Holds the information necessary for the facet this edge came from.
struct EdgeInformation
{
  vtkIdType OriginalCellId;
  vtkIdType OriginalFaceId;
  vtkIdType StartPointId;
};

//-----------------------------------------------------------------------------
// A map from edge endpoints to the information about that edge.
typedef std::unordered_map<EdgeEndpoints, EdgeInformation, EdgeEndpointsHash> EdgeMapType;

//-----------------------------------------------------------------------------
void RecordEdgeFlag(vtkPolyData* output, EdgeInformation& edgeInfo,
  vtkUnsignedCharArray* edgeFlagArray, unsigned char flag,
  std::vector<vtkIdType>& duplicatePointMap)
{
  const vtkIdType pnt = edgeInfo.StartPointId;
  if (edgeFlagArray->GetValue(pnt) == flag)
  {
    // Edge flag already set correctly.  Nothing to do.
    return;
  }
  else if (edgeFlagArray->GetValue(pnt) == NO_EDGE_FLAG)
  {
    // Nothing has set the edge flag yet.  Just set it and return.
    edgeFlagArray->SetValue(pnt, flag);
    return;
  }

  // If we are here then some other cell has already put a flag on this
  // point different than ours.  We have to adjust our cell topology to
  // use a duplicate point.
  if (duplicatePointMap[pnt] == -1)
  {
    // No duplicate made.  We need to make one.
    vtkPoints* points = output->GetPoints();
    double coords[3];
    points->GetPoint(pnt, coords);
    vtkIdType newPt = points->InsertNextPoint(coords);
    duplicatePointMap[pnt] = newPt;
    // Copying attributes from yourself seems weird, but is valid.
    vtkPointData* pdata = output->GetPointData();
    pdata->CopyData(pdata, pnt, newPt);
    edgeFlagArray->InsertValue(newPt, flag);
  }
  output->ReplaceCellPoint(edgeInfo.OriginalCellId, edgeInfo.StartPointId, duplicatePointMap[pnt]);
  edgeInfo.StartPointId = duplicatePointMap[pnt];
}

//=============================================================================
vtkStandardNewMacro(vtkRecoverGeometryWireframe);

//-----------------------------------------------------------------------------
vtkRecoverGeometryWireframe::vtkRecoverGeometryWireframe() = default;

//-----------------------------------------------------------------------------
vtkRecoverGeometryWireframe::~vtkRecoverGeometryWireframe() = default;

//-----------------------------------------------------------------------------
void vtkRecoverGeometryWireframe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CellIdsAttribute: " << this->CellIdsAttribute << std::endl;
}

//-----------------------------------------------------------------------------
int vtkRecoverGeometryWireframe::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  // If nothing to do, early return
  if (input->GetNumberOfCells() == 0 || input->GetNumberOfPoints() == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  if (!input->GetCellData()->HasArray(this->CellIdsAttribute.c_str()))
  {
    vtkWarningMacro("Couldn't find any cell attribute, passing through the input.");
    output->ShallowCopy(input);
    return 1;
  }

  vtkIdTypeArray* faceIds = vtkIdTypeArray::SafeDownCast(
    input->GetCellData()->GetAbstractArray(this->CellIdsAttribute.c_str()));
  if (!faceIds)
  {
    vtkErrorMacro(<< this->CellIdsAttribute.c_str() << " array is not of expected type.");
    return 0;
  }

  // Shallow copy the cell data.  All the cells get copied to output.
  output->GetCellData()->PassData(input->GetCellData());

  // Deep copy the point information and be ready to add points.
  vtkNew<vtkPoints> points;
  points->DeepCopy(input->GetPoints());
  output->SetPoints(points);
  vtkPointData* inputPD = input->GetPointData();
  vtkPointData* outputPD = output->GetPointData();
  outputPD->CopyAllocate(inputPD);
  const vtkIdType numOriginalPoints = points->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numOriginalPoints; ++i)
  {
    outputPD->CopyData(inputPD, i, i);
  }

  // Create an edge flag array.
  vtkNew<vtkUnsignedCharArray> edgeflags;
  edgeflags->SetName("vtkEdgeFlags");
  edgeflags->SetNumberOfComponents(1);
  edgeflags->SetNumberOfTuples(numOriginalPoints);
  auto edgeflagsRange = vtk::DataArrayValueRange<1>(edgeflags);
  std::fill(edgeflagsRange.begin(), edgeflagsRange.end(), NO_EDGE_FLAG);
  outputPD->AddArray(edgeflags);
  outputPD->SetActiveAttribute("vtkEdgeFlags", vtkDataSetAttributes::EDGEFLAG);

  auto tagEdgeFlags = [&edgeflags](vtkCellArray* inputCell, vtkIdType offset = 0) {
    auto cellIter = vtk::TakeSmartPointer(inputCell->NewIterator());
    vtkIdType npts;
    const vtkIdType* pts;
    for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      cellIter->GetCurrentCell(npts, pts);
      for (vtkIdType i = 0; i < npts + offset; i++)
      {
        edgeflags->SetValue(pts[i], 1);
      }
    }
  };

  // Shallow copy the verts.  Set the edge flags to true.
  vtkCellArray* inputVerts = input->GetVerts();
  output->SetVerts(inputVerts);
  tagEdgeFlags(inputVerts);

  // Shallow copy the lines. Set the edge flags to true.
  vtkCellArray* inputLines = input->GetLines();
  output->SetLines(inputLines);
  tagEdgeFlags(inputLines, -1);

  // Shallow copy the triangle strips.  Set the edge flags to true.
  vtkCellArray* inputStrips = input->GetStrips();
  output->SetStrips(inputStrips);
  tagEdgeFlags(inputStrips);

  // Deep copy the polygons because we will be changing some indices when we
  // duplicate points.
  vtkNew<vtkCellArray> outputPolys;
  outputPolys->DeepCopy(input->GetPolys());
  output->SetPolys(outputPolys);

  // Some (probably many) points will have to be duplicated because different
  // cells will need different edge flags. This array maps the original
  // point id to the duplicate id.
  std::vector<vtkIdType> duplicatePointMap(numOriginalPoints);
  std::fill(duplicatePointMap.begin(), duplicatePointMap.end(), -1);

  // Iterate over all the input facets and see which edge interfaces belonged to
  // different faces.  We do that by recording the original face id in a map.
  // When we find a pair of edges, we turn on the appropriate edge flag if they
  // came from different faces, or turn it off if they came from the same face.
  EdgeMapType edgeMap;
  vtkIdType inputCellId = inputVerts->GetNumberOfCells() + inputLines->GetNumberOfCells();
  auto outputPolyIter = vtk::TakeSmartPointer(outputPolys->NewIterator());
  std::vector<vtkIdType> originalPts;
  for (outputPolyIter->GoToFirstCell(); !outputPolyIter->IsDoneWithTraversal();
       outputPolyIter->GoToNextCell(), ++inputCellId)
  {
    if ((inputCellId % 4096) == 0)
    {
      this->UpdateProgress(static_cast<double>(inputCellId) / input->GetNumberOfCells());
      if (this->GetAbortExecute())
      {
        return 0;
      }
    }

    vtkIdType npts;
    const vtkIdType* pts;
    outputPolyIter->GetCurrentCell(npts, pts);

    // Record the original points of the polygon.  As we iterate over edges,
    // we may change the indices, but we always compare edges by the original
    // indices.
    originalPts.resize(npts);
    std::copy(pts, pts + npts, originalPts.begin());

    vtkIdType originalFace = faceIds->GetValue(inputCellId);
    for (vtkIdType i = 0; i < npts; i++)
    {
      EdgeEndpoints edge(originalPts[i], originalPts[(i + 1) % npts]);
      EdgeInformation edgeInfo;
      edgeInfo.OriginalCellId = inputCellId;
      edgeInfo.OriginalFaceId = originalFace;
      edgeInfo.StartPointId = pts[i];

      EdgeMapType::iterator edgeMatch = edgeMap.find(edge);
      if (edgeMatch == edgeMap.end())
      {
        // Not encountered yet.  Add to the map.
        edgeMap.insert(std::make_pair(edge, edgeInfo));
      }
      else
      {
        // The edge flag is true if the edge connects two different faces.
        unsigned char eflag =
          static_cast<unsigned char>(edgeMatch->second.OriginalFaceId != originalFace);
        RecordEdgeFlag(output, edgeMatch->second, edgeflags, eflag, duplicatePointMap);
        RecordEdgeFlag(output, edgeInfo, edgeflags, eflag, duplicatePointMap);
        // Remove the edge from the map since we already found the pair.
        edgeMap.erase(edgeMatch);
      }
    } // For each edge
  }   // For each cell.

  // Everything left in the edge map has no match.  It must necessarily be
  // on the outside of a face.
  for (EdgeMapType::iterator iter = edgeMap.begin(); iter != edgeMap.end(); ++iter)
  {
    RecordEdgeFlag(output, iter->second, edgeflags, 1, duplicatePointMap);
  }

  // If any points are unmarked, set some edge flag on them (although they
  // are probably not referenced by any cell).
  edgeflagsRange = vtk::DataArrayValueRange<1>(edgeflags);
  vtkSMPTools::Transform(edgeflagsRange.cbegin(), edgeflagsRange.cend(), edgeflagsRange.begin(),
    [](unsigned char value) { return (value == NO_EDGE_FLAG) ? 1 : value; });

  return 1;
}

VTK_ABI_NAMESPACE_END
