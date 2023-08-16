// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSelectPolyData.h"

#include "vtkCellData.h"
#include "vtkDijkstraGraphGeodesicPath.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTriangleFilter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSelectPolyData);

vtkCxxSetObjectMacro(vtkSelectPolyData, Loop, vtkPoints);

// Description:
// Instantiate object with InsideOut turned off.
vtkSelectPolyData::vtkSelectPolyData()
  : SelectionScalarsArrayName(nullptr)
{
  this->GenerateSelectionScalars = 0;
  this->SetSelectionScalarsArrayName("Selection");
  this->InsideOut = 0;
  this->EdgeSearchMode = VTK_GREEDY_EDGE_SEARCH;
  this->Loop = nullptr;
  this->SelectionMode = VTK_INSIDE_SMALLEST_REGION;
  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;
  this->GenerateUnselectedOutput = 0;

  this->SetNumberOfOutputPorts(3);

  vtkNew<vtkPolyData> output2;
  this->GetExecutive()->SetOutputData(1, output2);

  vtkNew<vtkPolyData> output3;
  this->GetExecutive()->SetOutputData(2, output3);
}

//------------------------------------------------------------------------------
vtkSelectPolyData::~vtkSelectPolyData()
{
  this->SetSelectionScalarsArrayName(nullptr);
  if (this->Loop)
  {
    this->Loop->Delete();
  }
}

//------------------------------------------------------------------------------
vtkPolyData* vtkSelectPolyData::GetUnselectedOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//------------------------------------------------------------------------------
vtkAlgorithmOutput* vtkSelectPolyData::GetUnselectedOutputPort()
{
  return this->GetOutputPort(1);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkSelectPolyData::GetSelectionEdges()
{
  if (this->GetNumberOfOutputPorts() < 3)
  {
    return nullptr;
  }

  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(2));
}

//------------------------------------------------------------------------------
void vtkSelectPolyData::GreedyEdgeSearch(vtkPolyData* mesh, vtkIdList* edgePointIds)
{
  vtkIdType numLoopPts = this->Loop->GetNumberOfPoints();

  // First thing to do is find the closest mesh points to the loop
  // points. This creates a list of mesh point ids corresponding to the loop point positions.
  vtkNew<vtkIdList> loopIds;
  loopIds->SetNumberOfIds(numLoopPts);

  vtkPoints* inPts = mesh->GetPoints();
  vtkIdType numPts = mesh->GetNumberOfPoints();
  for (vtkIdType loopPointId = 0; loopPointId < numLoopPts; loopPointId++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    double xLoop[3];
    this->Loop->GetPoint(loopPointId, xLoop);
    vtkIdType closestMeshPointId = 0;
    double closestDist2 = VTK_DOUBLE_MAX;

    for (vtkIdType meshPointId = 0; meshPointId < numPts; meshPointId++)
    {
      double x[3];
      inPts->GetPoint(meshPointId, x);
      double dist2 = vtkMath::Distance2BetweenPoints(x, xLoop);
      if (dist2 < closestDist2)
      {
        closestMeshPointId = meshPointId;
        closestDist2 = dist2;
      }
    } // for all input points

    loopIds->SetId(loopPointId, closestMeshPointId);
  } // for all loop points

  edgePointIds->InsertNextId(loopIds->GetId(0));

  // Now that we've got point ids, we build the loop. Start with the
  // first two points in the loop (which define a line), and find the
  // mesh edge that is directed along the line, and whose
  // end point is closest to the line. Continue until loop closes in on
  // itself.
  vtkNew<vtkIdList> neighbors;
  neighbors->Allocate(10000);
  for (vtkIdType loopPointIndex = 0; loopPointIndex < numLoopPts; loopPointIndex++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    vtkIdType currentId = loopIds->GetId(loopPointIndex);
    vtkIdType nextId = loopIds->GetId((loopPointIndex + 1) % numLoopPts);
    vtkIdType prevId = (-1);
    double x[3];
    double x0[3];
    double x1[3];
    double vec[3];
    inPts->GetPoint(currentId, x);
    inPts->GetPoint(currentId, x0);
    inPts->GetPoint(nextId, x1);
    for (int j = 0; j < 3; j++)
    {
      vec[j] = x1[j] - x0[j];
    }

    // track edge
    for (vtkIdType id = currentId; id != nextId;)
    {
      vtkSelectPolyData::GetPointNeighbors(mesh, id, neighbors); // points connected by edge
      vtkIdType numNei = neighbors->GetNumberOfIds();
      vtkIdType closest = -1;
      double closestDist2 = VTK_DOUBLE_MAX;
      for (vtkIdType j = 0; j < numNei; j++)
      {
        vtkIdType neiId = neighbors->GetId(j);
        if (neiId == nextId)
        {
          closest = neiId;
          break;
        }
        else
        {
          double neiX[3];
          inPts->GetPoint(neiId, neiX);
          double dir[3];
          for (vtkIdType k = 0; k < 3; k++)
          {
            dir[k] = neiX[k] - x[k];
          }
          if (neiId != prevId && vtkMath::Dot(dir, vec) > 0.0) // candidate
          {
            double dist2 = vtkLine::DistanceToLine(neiX, x0, x1);
            if (dist2 < closestDist2)
            {
              closest = neiId;
              closestDist2 = dist2;
            }
          } // in direction of line
        }
      } // for all neighbors

      if (closest < 0)
      {
        vtkErrorMacro(<< "Can't follow edge. Set EdgeSearchMode to Dijkstra to avoid this error.");
        edgePointIds->Initialize(); // clear edge list to indicate error
        return;
      }
      else
      {
        edgePointIds->InsertNextId(closest);
        prevId = id;
        id = closest;
        inPts->GetPoint(id, x);
      }
    } // for tracking edge
  }   // for all edges of loop
}

//------------------------------------------------------------------------------
void vtkSelectPolyData::DijkstraEdgeSearch(vtkPolyData* mesh, vtkIdList* edgePointIds)
{
  vtkNew<vtkDijkstraGraphGeodesicPath> edgeSearchFilter;
  edgeSearchFilter->StopWhenEndReachedOn();
  edgeSearchFilter->SetInputData(mesh);

  vtkNew<vtkPointLocator> pointLocator;
  pointLocator->SetDataSet(mesh);

  vtkPoints* inPts = mesh->GetPoints();
  vtkIdType numLoopPts = this->Loop->GetNumberOfPoints();

  vtkIdType currentId = 0;
  double xLoop[3];
  this->Loop->GetPoint(0, xLoop);
  vtkIdType nextId = pointLocator->FindClosestPoint(xLoop);
  for (vtkIdType i = 0; i < numLoopPts; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    currentId = nextId;
    this->Loop->GetPoint((i + 1) % numLoopPts, xLoop);
    nextId = pointLocator->FindClosestPoint(xLoop);

    edgeSearchFilter->SetStartVertex(currentId);
    edgeSearchFilter->SetEndVertex(nextId);
    edgeSearchFilter->Update();
    vtkPolyData* outputPath = edgeSearchFilter->GetOutput();
    double x0[3];
    inPts->GetPoint(currentId, x0);
    for (int j = outputPath->GetNumberOfPoints() - 1; j >= 0; --j)
    {
      double x[3];
      outputPath->GetPoint(j, x);
      double dist2 = vtkMath::Distance2BetweenPoints(x, x0);
      if (dist2 > 0.0)
      {
        // Find point ID to add in the input mesh to remember the next edge point
        edgePointIds->InsertNextId(pointLocator->FindClosestPoint(x));
        for (int k = 0; k < 3; ++k)
        {
          // Remember last added point so that it does not get added twice
          x0[k] = x[k];
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkSelectPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Initialize and check data
  vtkDebugMacro(<< "Selecting data...");

  this->GetUnselectedOutput()->Initialize();
  this->GetSelectionEdges()->Initialize();

  // CHeck if inputs are valid
  if (input->GetNumberOfPoints() < 1)
  {
    vtkErrorMacro("Input contains no points");
    return 1;
  }
  vtkIdType numLoopPts;
  if (this->Loop == nullptr || (numLoopPts = this->Loop->GetNumberOfPoints()) < 3)
  {
    vtkErrorMacro("Please define a loop with at least three points");
    return 1;
  }

  // Convert to triangle mesh. All further computations are done on the triangulated mesh.
  vtkSmartPointer<vtkPolyData> triMesh;
  {
    vtkNew<vtkTriangleFilter> tf;
    tf->SetInputData(input);
    tf->PassLinesOff();
    tf->PassVertsOff();
    tf->SetContainerAlgorithm(this);
    tf->Update();
    triMesh = tf->GetOutput();
  }
  vtkCellArray* inPolys = triMesh->GetPolys();
  if (inPolys->GetNumberOfCells() < 1)
  {
    vtkErrorMacro("This filter operates on surface primitives");
    return 1;
  }

  // Create a mesh that only contains points and polys
  // (probably to avoid potential interference of other cell types)
  // and links are computed (so that neighbors can be retrieved).
  vtkNew<vtkPolyData> mesh;
  vtkPoints* inPts = triMesh->GetPoints();
  mesh->SetPoints(inPts);
  mesh->SetPolys(inPolys);
  mesh->BuildLinks(); // to do neighborhood searching
  vtkIdType numCells = mesh->GetNumberOfCells();

  // Get a list of point IDs of the mesh that forms a continuous closed loop
  vtkNew<vtkIdList> edgePointIds;
  edgePointIds->Allocate(numLoopPts * 10, 1000);
  switch (this->EdgeSearchMode)
  {
    case VTK_GREEDY_EDGE_SEARCH:
      this->GreedyEdgeSearch(mesh, edgePointIds);
      break;
    case VTK_DIJKSTRA_EDGE_SEARCH:
      this->DijkstraEdgeSearch(triMesh, edgePointIds);
      break;
    default:
      vtkErrorMacro("Unknown edge search mode: " << this->EdgeSearchMode);
  }
  if (edgePointIds->GetNumberOfIds() == 0 || this->CheckAbort())
  {
    return 1;
  }

  // Save the found edge list into SelectionEdges polydata
  vtkIdType numMeshLoopPts = edgePointIds->GetNumberOfIds();
  vtkNew<vtkCellArray> selectionEdges;
  selectionEdges->AllocateEstimate(1, numMeshLoopPts);
  selectionEdges->InsertNextCell(numMeshLoopPts);
  for (vtkIdType i = 0; i < numMeshLoopPts; i++)
  {
    selectionEdges->InsertCellPoint(edgePointIds->GetId(i));
  }
  this->GetSelectionEdges()->SetPoints(inPts);
  this->GetSelectionEdges()->SetLines(selectionEdges);

  // Store distance from edge in point and cell marks and
  // get ID of the cell that is farthest from the loop.
  vtkNew<vtkIntArray> pointMarks;
  vtkNew<vtkIntArray> cellMarks;
  vtkIdType cellIdInSelectedRegion =
    this->ComputeTopologicalDistance(mesh, edgePointIds, pointMarks, cellMarks);

  // If the region that is closest to a specific point needs to be extracted then get
  // a cell that is closes to that position.
  if (this->SelectionMode == VTK_INSIDE_CLOSEST_POINT_REGION)
  {
    // find closest point and use as a seed
    cellIdInSelectedRegion = this->GetClosestCellId(mesh, pointMarks);
  }

  // Set point and cell mark values in the selected region to -1.
  // We'll end up having >0 values outside the selected region, -1 inside.
  this->FillMarksInRegion(mesh, edgePointIds, pointMarks, cellMarks, cellIdInSelectedRegion);

  // Invert mark value if we want to get the smallest region.
  if (this->SelectionMode == VTK_INSIDE_SMALLEST_REGION)
  {
    for (vtkIdType i = 0; i < numCells; i++)
    {
      int markValue = cellMarks->GetValue(i);
      cellMarks->SetValue(i, -markValue);
    }
    vtkIdType numPts = pointMarks->GetNumberOfValues();
    for (vtkIdType i = 0; i < numPts; i++)
    {
      int markValue = pointMarks->GetValue(i);
      pointMarks->SetValue(i, -markValue);
    }
  }

  // Write filter output.
  vtkPointData* inPD = triMesh->GetPointData();
  vtkCellData* inCD = triMesh->GetCellData();
  if (this->GenerateSelectionScalars)
  {
    // Write distance from contour as scalars to the output mesh.
    // This can be used for example for later cliipping the mesh with vtkClipPolyData.
    this->SetSelectionScalarsToOutput(inPD, inCD, mesh, edgePointIds, pointMarks, output);
  }
  else
  {
    // crop the input mesh to the selected region
    this->SetClippedResultToOutput(inPD, inCD, mesh, cellMarks, output);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkSelectPolyData::ComputeTopologicalDistance(
  vtkPolyData* mesh, vtkIdList* edgePointIds, vtkIntArray* pointMarks, vtkIntArray* cellMarks)
{
  vtkIdType numPts = mesh->GetNumberOfPoints();
  vtkIdType numCells = mesh->GetNumberOfCells();

  // Next, prepare to mark off inside/outside and on boundary of loop.
  // Mark the boundary of the loop using point marks. Also initialize
  // the advancing front (used to mark traversal/compute scalars).
  // Prepare to compute the advancing front

  // Mark all points and cells as unvisited
  cellMarks->SetNumberOfValues(numCells);
  const int unvisited = VTK_INT_MAX;
  for (vtkIdType i = 0; i < numCells; i++)
  {
    cellMarks->SetValue(i, unvisited);
  }
  pointMarks->SetNumberOfValues(numPts);
  for (vtkIdType i = 0; i < numPts; i++)
  {
    pointMarks->SetValue(i, unvisited);
  }

  // Current and next front contain point IDs
  vtkSmartPointer<vtkIdList> currentFront = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkIdList> nextFront = vtkSmartPointer<vtkIdList>::New();
  vtkIdType numMeshLoopPts = edgePointIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < numMeshLoopPts; i++)
  {
    vtkIdType id = edgePointIds->GetId(i);
    pointMarks->SetValue(id, 0); // marks the start of the front
    currentFront->InsertNextId(id);
  }

  // Traverse the front as long as we can. We're basically computing a
  // topological distance.
  int maxFrontValue = 0;
  vtkIdType maxFrontCell = (-1);
  int currentFrontValue = 1;
  vtkIdType numPtsInFront = 0;
  while ((numPtsInFront = currentFront->GetNumberOfIds()))
  {
    // Process all triangles around the current front points
    for (vtkIdType i = 0; i < numPtsInFront; i++)
    {
      vtkIdType id = currentFront->GetId(i);
      vtkIdType* cells;
      vtkIdType ncells;
      mesh->GetPointCells(id, ncells, cells);
      for (vtkIdType j = 0; j < ncells; j++)
      {
        id = cells[j];
        if (cellMarks->GetValue(id) != unvisited)
        {
          // the cell is already visited
          continue;
        }
        // This cell has not been visited yet
        if (currentFrontValue > maxFrontValue)
        {
          // update maximum distance
          maxFrontCell = id;
        }
        cellMarks->SetValue(id, currentFrontValue);
        // Add all unvisited points of this triangle to the front
        const vtkIdType* pts;
        vtkIdType npts;
        mesh->GetCellPoints(id, npts, pts);
        for (vtkIdType k = 0; k < npts; k++)
        {
          if (pointMarks->GetValue(pts[k]) == unvisited)
          {
            pointMarks->SetValue(pts[k], 1);
            nextFront->InsertNextId(pts[k]);
          }
        }
      }
    }

    // All points in the current front has been processed, start a new iteration
    currentFrontValue++;
    // Swap currentFront and nextFront
    vtkSmartPointer<vtkIdList> tmpFront = currentFront;
    currentFront = nextFront;
    nextFront = tmpFront;
    nextFront->Reset();
  }

  return maxFrontCell;
}

//------------------------------------------------------------------------------
vtkIdType vtkSelectPolyData::GetClosestCellId(vtkPolyData* mesh, vtkIntArray* pointMarks)
{
  vtkPoints* inPts = mesh->GetPoints();
  vtkIdType numPts = inPts->GetNumberOfPoints();

  vtkIdType closestCellId = -1;
  double closestDist2 = VTK_DOUBLE_MAX;
  vtkIdType closestPointId = -1;
  for (vtkIdType pointId = 0; pointId < numPts; pointId++)
  {
    double x[3];
    inPts->GetPoint(pointId, x);
    double dist2 = vtkMath::Distance2BetweenPoints(x, this->ClosestPoint);
    // get closest point not on the boundary
    if (dist2 < closestDist2 && pointMarks->GetValue(pointId) != 0)
    {
      closestPointId = pointId;
      closestDist2 = dist2;
    }
  }
  if (closestPointId >= 0)
  {
    vtkIdType ncells;
    vtkIdType* cells;
    mesh->GetPointCells(closestPointId, ncells, cells);
    if (ncells > 0)
    {
      closestCellId = cells[0];
    }
  }
  return closestCellId;
}

//------------------------------------------------------------------------------
void vtkSelectPolyData::FillMarksInRegion(vtkPolyData* mesh, vtkIdList* edgePointIds,
  vtkIntArray* pointMarks, vtkIntArray* cellMarks, vtkIdType cellIdInSelectedRegion)
{
  // We do the fill as a moving front. This is an alternative to recursion. The
  // fill negates one region of the mesh on one side of the loop.
  // In contrast to ComputeTopologicalDistance, current and next front
  // in this method contain cell IDs.
  vtkSmartPointer<vtkIdList> currentFront = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkIdList> nextFront = vtkSmartPointer<vtkIdList>::New();
  currentFront->InsertNextId(cellIdInSelectedRegion);

  // Initialize the front with the received cell ID
  const int fillValue = -1;
  const int boundaryValue = 0;
  cellMarks->SetValue(cellIdInSelectedRegion, fillValue);

  vtkNew<vtkIdList> neighbors;
  neighbors->Allocate(10000);
  vtkIdType numCellsInFront;
  while ((numCellsInFront = currentFront->GetNumberOfIds()) > 0)
  {
    // Iterate through all the triangles and visit all the neighbor triangles
    for (vtkIdType i = 0; i < numCellsInFront; i++)
    {
      vtkIdType id = currentFront->GetId(i);

      const vtkIdType* pts;
      vtkIdType npts;
      mesh->GetCellPoints(id, npts, pts);
      for (vtkIdType j = 0; j < 3; j++)
      {
        vtkIdType cellPointId1 = pts[j];
        vtkIdType cellPointId2 = pts[(j + 1) % 3];
        int cellPointValue1 = pointMarks->GetValue(cellPointId1);
        int cellPointValue2 = pointMarks->GetValue(cellPointId2);

        if (cellPointValue1 != boundaryValue)
        {
          pointMarks->SetValue(cellPointId1, fillValue);
        }

        if (cellPointValue1 == boundaryValue && cellPointValue2 == boundaryValue)
        {
          // This may be a boundary edge or just an edge that connects two boundary points.
          // Do a full search in the boundary edge list to find out.
          if (vtkSelectPolyData::IsBoundaryEdge(cellPointId1, cellPointId2, edgePointIds))
          {
            // cannot cross boundary
            continue;
          }
        }

        // add neighbors of this edge to the advancing front
        mesh->GetCellEdgeNeighbors(id, cellPointId1, cellPointId2, neighbors);
        vtkIdType numNei = neighbors->GetNumberOfIds();
        for (vtkIdType k = 0; k < numNei; k++)
        {
          vtkIdType neiId = neighbors->GetId(k);
          int val = cellMarks->GetValue(neiId);
          if (val == fillValue)
          {
            // already processed
            continue;
          }
          cellMarks->SetValue(neiId, fillValue);
          nextFront->InsertNextId(neiId);
        }

      } // for all edges of cell
    }   // all cells in front

    // Swap currentFront and nextFront
    vtkSmartPointer<vtkIdList> tmpFront = currentFront;
    currentFront = nextFront;
    nextFront = tmpFront;
    nextFront->Reset();
  } // while still advancing
}

//------------------------------------------------------------------------------
bool vtkSelectPolyData::IsBoundaryEdge(
  vtkIdType pointId1, vtkIdType pointId2, vtkIdList* edgePointIds)
{
  vtkIdType numMeshLoopPts = edgePointIds->GetNumberOfIds();
  for (vtkIdType edgePointIndex = 0; edgePointIndex < numMeshLoopPts; ++edgePointIndex)
  {
    vtkIdType edgePointId = edgePointIds->GetId(edgePointIndex);
    if (edgePointId == pointId1)
    {
      vtkIdType wrappedEdgePointIndex = (edgePointIndex + 1) % numMeshLoopPts;
      vtkIdType edgePointIdAfter = edgePointIds->GetId(wrappedEdgePointIndex);
      if (edgePointIdAfter == pointId2)
      {
        // boundary edge
        return true;
      }

      // Conceptually (edgePointIndex - 1) % numMeshLoopPts, but avoiding negatives.
      wrappedEdgePointIndex = (edgePointIndex > 0) ? (edgePointIndex - 1) : (numMeshLoopPts - 1);
      vtkIdType edgePointIdBefore = edgePointIds->GetId(wrappedEdgePointIndex);
      if (edgePointIdBefore == pointId2)
      {
        // boundary edge
        return true;
      }
    }
    if (edgePointId == pointId2)
    {
      vtkIdType wrappedEdgePointIndex = (edgePointIndex + 1) % numMeshLoopPts;
      vtkIdType edgePointIdAfter = edgePointIds->GetId(wrappedEdgePointIndex);
      if (edgePointIdAfter == pointId1)
      {
        // boundary edge
        return true;
      }

      // Conceptually (edgePointIndex - 1) % numMeshLoopPts, but avoiding negatives.
      wrappedEdgePointIndex = (edgePointIndex > 0) ? (edgePointIndex - 1) : (numMeshLoopPts - 1);
      vtkIdType edgePointIdBefore = edgePointIds->GetId(wrappedEdgePointIndex);
      if (edgePointIdBefore == pointId1)
      {
        // boundary edge
        return true;
      }
    }
  }
  // not a boundary edge
  return false;
}

//------------------------------------------------------------------------------
void vtkSelectPolyData::SetSelectionScalarsToOutput(vtkPointData* originalPointData,
  vtkCellData* originalCellData, vtkPolyData* mesh, vtkIdList* edgePointIds,
  vtkIntArray* pointMarks, vtkPolyData* output)
{
  vtkPoints* inPts = mesh->GetPoints();
  vtkIdType numPts = inPts->GetNumberOfPoints();

  vtkNew<vtkFloatArray> selectionScalars;
  selectionScalars->SetName(this->SelectionScalarsArrayName);
  selectionScalars->SetNumberOfTuples(numPts);

  // "Boundary" here refers to a polyline that connects the loop point positions.

  // Compute signed distance to loop for non-boundary points.
  vtkIdType numLoopPts = this->Loop->GetNumberOfPoints();
  for (vtkIdType pointId = 0; pointId < numPts; pointId++)
  {
    if (this->CheckAbort())
    {
      break;
    }

    if (pointMarks->GetValue(pointId) == 0)
    {
      // boundary point, we'll deal with these later
      continue;
    }

    // Not an edge point.
    double x[3];
    inPts->GetPoint(pointId, x);
    double closestDist2 = VTK_DOUBLE_MAX;
    for (vtkIdType i = 0; i < numLoopPts; i++)
    {
      double x0[3];
      double x1[3];
      this->Loop->GetPoint(i, x0);
      this->Loop->GetPoint((i + 1) % numLoopPts, x1);
      double t;
      double xLoop[3];
      double dist2 = vtkLine::DistanceToLine(x, x0, x1, t, xLoop);
      if (dist2 < closestDist2)
      {
        closestDist2 = dist2;
      }
    }
    // Set signed distance
    double closestDist = 0.0;
    if (pointMarks->GetValue(pointId) < 0)
    {
      closestDist = -sqrt(closestDist2);
    }
    else
    {
      closestDist = sqrt(closestDist2);
    }
    selectionScalars->SetComponent(pointId, 0, closestDist);
  }

  // Compute signed distance to loop for boundary points.
  vtkIdType numMeshLoopPts = edgePointIds->GetNumberOfIds();
  vtkNew<vtkIdList> neighbors;
  neighbors->Allocate(10000);
  for (vtkIdType edgePointIndex = 0; edgePointIndex < numMeshLoopPts; edgePointIndex++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    vtkIdType edgePointId = edgePointIds->GetId(edgePointIndex);
    double x[3];
    inPts->GetPoint(edgePointId, x);

    // Find the boundary line segment closest to this point
    double closestPointOnBoundaryPos[3] = { 0.0, 0.0,
      0.0 };                              // closest position of edgePoint on the boundary
    double closestDist2 = VTK_DOUBLE_MAX; // distance from closest boundary
    {
      for (vtkIdType loopPointId = 0; loopPointId < numLoopPts; loopPointId++)
      {
        double x0[3];
        double x1[3];
        this->Loop->GetPoint(loopPointId, x0);
        this->Loop->GetPoint((loopPointId + 1) % numLoopPts, x1);
        double t;
        double xLoop[3]; // closest position on the boundary
        double dist2 = vtkLine::DistanceToLine(x, x0, x1, t, xLoop);
        if (dist2 < closestDist2)
        {
          closestDist2 = dist2;
          closestPointOnBoundaryPos[0] = xLoop[0];
          closestPointOnBoundaryPos[1] = xLoop[1];
          closestPointOnBoundaryPos[2] = xLoop[2];
        }
      }
    }

    // Find neighbor farthest from the boundary (inside/outside information
    // is the most reliable for this neighbor).
    vtkIdType farthestNeighborPointId = 0;
    {
      vtkSelectPolyData::GetPointNeighbors(mesh, edgePointId, neighbors);
      vtkIdType numNei = neighbors->GetNumberOfIds();
      double maxDist = 0.0;
      for (vtkIdType i = 0; i < numNei; i++)
      {
        vtkIdType neiId = neighbors->GetId(i);
        if (pointMarks->GetValue(neiId) != 0) // find the furthest away
        {
          double dist = fabs(selectionScalars->GetComponent(neiId, 0));
          if (dist > maxDist)
          {
            farthestNeighborPointId = neiId;
            maxDist = dist;
          }
        }
      }
    }

    // First compute distance assuming that x is on the same side of the boundary as the farthest
    // neighbor.
    double dist = sqrt(closestDist2);
    if (pointMarks->GetValue(farthestNeighborPointId) < 0)
    {
      dist = -dist;
    }
    // If x and the farthest neighbor are actually different sides of the boundary then invert the
    // signed distance value.
    double farthestNeighborPointPos[3];
    inPts->GetPoint(farthestNeighborPointId, farthestNeighborPointPos);
    if (vtkMath::Distance2BetweenPoints(farthestNeighborPointPos, x) >
      vtkMath::Distance2BetweenPoints(farthestNeighborPointPos, closestPointOnBoundaryPos))
    {
      // x is on the other side of the boundary
      dist = -dist;
    }

    selectionScalars->SetComponent(edgePointId, 0, dist);
  } // for all boundary points

  output->CopyStructure(mesh); // pass geometry/topology unchanged

  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllOn();
  outPD->PassData(originalPointData);
  int idx = outPD->AddArray(selectionScalars);
  outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);

  vtkCellData* outCD = output->GetCellData();
  outCD->PassData(originalCellData);
}

//------------------------------------------------------------------------------
void vtkSelectPolyData::SetClippedResultToOutput(vtkPointData* originalPointData,
  vtkCellData* originalCellData, vtkPolyData* mesh, vtkIntArray* cellMarks, vtkPolyData* output)
{
  vtkCellData* outCD = output->GetCellData();
  outCD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  outCD->CopyAllocate(originalCellData);

  // spit out all the negative cells
  vtkNew<vtkCellArray> newPolys;
  vtkIdType numCells = mesh->GetNumberOfCells();
  newPolys->AllocateEstimate(numCells / 2, 3);
  vtkIdType newID = 0;
  for (vtkIdType i = 0; i < numCells; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    if ((cellMarks->GetValue(i) < 0) || (cellMarks->GetValue(i) > 0 && this->InsideOut))
    {
      const vtkIdType* pts;
      vtkIdType npts;
      mesh->GetCellPoints(i, npts, pts);
      newID = newPolys->InsertNextCell(npts, pts);
      outCD->CopyData(originalCellData, i, newID);
    }
  }
  vtkPoints* inPts = mesh->GetPoints();
  output->SetPoints(inPts);
  output->SetPolys(newPolys);
  vtkPointData* outPD = output->GetPointData();
  outPD->PassData(originalPointData);

  if (this->GenerateUnselectedOutput)
  {
    vtkCellData* unCD = this->GetUnselectedOutput()->GetCellData();
    unCD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
    unCD->CopyAllocate(originalCellData);

    vtkNew<vtkCellArray> unPolys;
    unPolys->AllocateEstimate(numCells / 2, 3);
    for (vtkIdType i = 0; i < numCells; i++)
    {
      if (this->CheckAbort())
      {
        break;
      }
      if ((cellMarks->GetValue(i) >= 0) || this->InsideOut)
      {
        const vtkIdType* pts;
        vtkIdType npts;
        mesh->GetCellPoints(i, npts, pts);
        newID = unPolys->InsertNextCell(npts, pts);
        unCD->CopyData(originalCellData, i, newID);
      }
    }
    this->GetUnselectedOutput()->SetPoints(inPts);
    this->GetUnselectedOutput()->SetPolys(unPolys);
    this->GetUnselectedOutput()->GetPointData()->PassData(originalPointData);
  }
}

//------------------------------------------------------------------------------
void vtkSelectPolyData::GetPointNeighbors(vtkPolyData* mesh, vtkIdType ptId, vtkIdList* nei)
{
  nei->Reset();
  vtkIdType ncells;
  vtkIdType* cells;
  mesh->GetPointCells(ptId, ncells, cells);
  for (vtkIdType i = 0; i < ncells; i++)
  {
    const vtkIdType* pts;
    vtkIdType npts;
    mesh->GetCellPoints(cells[i], npts, pts);
    for (vtkIdType j = 0; j < 3; j++)
    {
      if (pts[j] != ptId)
      {
        nei->InsertUniqueId(pts[j]);
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkSelectPolyData::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Loop != nullptr)
  {
    time = this->Loop->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkSelectPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "Generate Unselected Output: " << (this->GenerateUnselectedOutput ? "On\n" : "Off\n");

  os << indent << "Inside Mode: ";
  os << this->GetSelectionModeAsString() << "\n";

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " << this->ClosestPoint[1]
     << ", " << this->ClosestPoint[2] << ")\n";

  os << indent
     << "Generate Selection Scalars: " << (this->GenerateSelectionScalars ? "On\n" : "Off\n");

  if (this->GenerateSelectionScalars)
  {
    os << indent << "Selection Scalars array name: " << this->SelectionScalarsArrayName << "\n";
  }

  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");

  os << indent << "Edge Search Mode: ";
  os << this->GetEdgeSearchModeAsString() << "\n";

  if (this->Loop)
  {
    os << indent << "Loop of " << this->Loop->GetNumberOfPoints() << "points defined\n";
  }
  else
  {
    os << indent << "Loop not defined\n";
  }
}
VTK_ABI_NAMESPACE_END
