/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataNormals.h"

#include "vtkAtomicMutex.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkPriorityQueue.h"
#include "vtkSMPTools.h"
#include "vtkTriangleStrip.h"

#include <mutex>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyDataNormals);

//-----------------------------------------------------------------------------
// Construct with feature angle=30, splitting and consistency turned on,
// flipNormals turned off, and non-manifold traversal turned on.
vtkPolyDataNormals::vtkPolyDataNormals()
{
  this->FeatureAngle = 30.0;
  this->Splitting = 1;
  this->Consistency = 1;
  this->FlipNormals = 0;
  this->ComputePointNormals = 1;
  this->ComputeCellNormals = 0;
  this->NonManifoldTraversal = 1;
  this->AutoOrientNormals = 0;
  // some internal data
  this->NumFlips = 0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
  this->CosAngle = 0.0;
}

static constexpr char VTK_CELL_NOT_VISITED = 0;
static constexpr char VTK_CELL_VISITED = 1;

//-----------------------------------------------------------------------------
// Generate normals for polygon meshes
int vtkPolyDataNormals::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSetAttributes* outCD = output->GetCellData();

  vtkDebugMacro(<< "Generating surface normals");

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numVerts = input->GetNumberOfVerts();
  vtkIdType numLines = input->GetNumberOfLines();
  vtkIdType numPolys = input->GetNumberOfPolys();
  vtkIdType numStrips = input->GetNumberOfStrips();
  if (numPts < 1)
  {
    vtkDebugMacro(<< "No data to generate normals for!");
    return 1;
  }

  // If there is nothing to do, pass the data through
  if ((this->ComputePointNormals == 0 && this->ComputeCellNormals == 0) ||
    (numPolys < 1 && numStrips < 1))
  { // don't do anything! pass data through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  if (numStrips < 1)
  {
    output->GetCellData()->PassData(input->GetCellData());
  }

  // Load data into cell structure.  We need two copies: one is a
  // non-writable mesh used to perform topological queries.  The other
  // is used to write into and modify the connectivity of the mesh.
  //
  vtkPoints* inPoints = input->GetPoints();
  vtkCellArray* inPolys = input->GetPolys();
  vtkCellArray* inStrips = input->GetStrips();

  ///////////////////////////////////////////////////////////////////
  // Decompose strips (if any) into triangles.
  ///////////////////////////////////////////////////////////////////

  vtkCellArray* polys;
  vtkNew<vtkPolyData> oldMesh;
  oldMesh->SetPoints(inPoints);
  if (numStrips > 0) // have to decompose strips into triangles
  {
    vtkDataSetAttributes* inCD = input->GetCellData();
    // When we have triangle strips, make sure to create and copy
    // the cell data appropriately. Since strips are broken into
    // triangles, cell data cannot be passed as it is and needs to
    // be copied tuple by tuple.
    outCD->CopyAllocate(inCD);
    if (numPolys > 0)
    {
      polys = vtkCellArray::New();
      polys->DeepCopy(inPolys);
      outCD->CopyData(inCD, 0, numPolys, 0);
    }
    else
    {
      polys = vtkCellArray::New();
      polys->AllocateEstimate(numStrips, 5);
    }
    vtkNew<vtkIdList> tempCellPointIds;
    vtkIdType npts = 0;
    const vtkIdType* pts = nullptr;
    for (vtkIdType stripId = 0, inCellIdx = numPolys, outCellIdx = numPolys; stripId < numStrips;
         ++stripId, ++inCellIdx)
    {
      inStrips->GetCellAtId(stripId, npts, pts, tempCellPointIds);
      vtkTriangleStrip::DecomposeStrip(npts, pts, polys);
      // Copy the cell data for the strip to each triangle.
      for (vtkIdType i = 0; i < npts - 2; i++)
      {
        outCD->CopyData(inCD, inCellIdx, outCellIdx++);
      }
    }
    oldMesh->SetPolys(polys);
    polys->Delete();
    numPolys = polys->GetNumberOfCells(); // added some new triangles
  }
  else
  {
    oldMesh->SetPolys(inPolys);
    polys = inPolys;
  }
  oldMesh->BuildLinks();
  this->UpdateProgress(0.10);

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();

  vtkNew<vtkPolyData> newMesh;
  newMesh->SetPoints(inPoints);
  // create a copy because we're modifying it
  vtkNew<vtkCellArray> newPolys;
  newPolys->DeepCopy(polys);
  newMesh->SetPolys(newPolys);
  newMesh->BuildCells(); // builds connectivity

  //  Traverse all polygons insuring proper direction of ordering.  This
  //  works by propagating a wave from a seed polygon to the polygon's
  //  edge neighbors. Each neighbor may be reordered to maintain consistency
  //  with its (already checked) neighbors.
  this->NumFlips = 0;
  if (this->AutoOrientNormals || this->Consistency)
  {
    // The visited array keeps track of which polygons have been visited.
    std::vector<char> visited;
    visited.resize(numPolys, VTK_CELL_NOT_VISITED);
    vtkNew<vtkIdList> wave, wave2, cellPointIds, cellIds, neighborPointIds;
    wave->Allocate(numPolys / 4 + 1, numPolys);
    wave2->Allocate(numPolys / 4 + 1, numPolys);
    cellPointIds->Allocate(VTK_CELL_SIZE);
    cellIds->Allocate(VTK_CELL_SIZE);
    neighborPointIds->Allocate(VTK_CELL_SIZE);

    if (this->AutoOrientNormals)
    {
      // No need to check this->Consistency. It's implied.

      // Ok, here's the basic idea: the "left-most" polygon should
      // have its outward pointing normal facing left. If it doesn't,
      // reverse the vertex order. Then use it as the seed for other
      // connected polys. To find left-most polygon, first find left-most
      // point, and examine neighboring polys and see which one
      // has a normal that's "most aligned" with the X-axis. This process
      // will need to be repeated to handle all connected components in
      // the mesh. Report bugs/issues to cvolpe@ara.com.
      int foundLeftmostCell;
      vtkIdType leftmostCellID = -1, currentPointID, currentCellID;
      vtkIdType* leftmostCells;
      vtkIdType nleftmostCells;
      const vtkIdType* cellPts;
      vtkIdType nCellPts;
      int cIdx;
      double bestNormalAbsXComponent;
      int bestReverseFlag;
      vtkNew<vtkPriorityQueue> leftmostPoints;

      // Put all the points in the priority queue, based on x coord
      // So that we can find leftmost point
      leftmostPoints->Allocate(numPts);
      for (vtkIdType ptId = 0; ptId < numPts; ptId++)
      {
        leftmostPoints->Insert(inPoints->GetPoint(ptId)[0], ptId);
      }

      // Repeat this while loop as long as the queue is not empty,
      // because there may be multiple connected components, each of
      // which needs to be seeded independently with a correctly
      // oriented polygon.
      double n[3];
      while (leftmostPoints->GetNumberOfItems())
      {
        foundLeftmostCell = 0;
        // Keep iterating through leftmost points and cells located at
        // those points until I've got a leftmost point with
        // unvisited cells attached and I've found the best cell
        // at that point
        do
        {
          currentPointID = leftmostPoints->Pop();
          oldMesh->GetPointCells(currentPointID, nleftmostCells, leftmostCells);
          bestNormalAbsXComponent = 0.0;
          bestReverseFlag = 0;
          for (cIdx = 0; cIdx < nleftmostCells; cIdx++)
          {
            currentCellID = leftmostCells[cIdx];
            if (visited[currentCellID] == VTK_CELL_VISITED)
            {
              continue;
            }
            oldMesh->GetCellPoints(currentCellID, nCellPts, cellPts);
            vtkPolygon::ComputeNormal(inPoints, nCellPts, cellPts, n);
            // Ok, see if this leftmost cell candidate is the best
            // so far
            if (fabs(n[0]) > bestNormalAbsXComponent)
            {
              bestNormalAbsXComponent = fabs(n[0]);
              leftmostCellID = currentCellID;
              // If the current leftmost cell's normal is pointing to the
              // right, then the vertex ordering is wrong
              bestReverseFlag = (n[0] > 0);
              foundLeftmostCell = 1;
            } // if this normal is most x-aligned so far
          }   // for each cell at current leftmost point
        } while (leftmostPoints->GetNumberOfItems() && !foundLeftmostCell);
        if (foundLeftmostCell)
        {
          // We've got the seed for a connected component! But do
          // we need to flip it first? We do, if it was pointed the wrong
          // way to begin with, or if the user requested flipping all
          // normals, but if both are true, then we leave it as it is.
          if (bestReverseFlag ^ this->FlipNormals)
          {
            newMesh->ReverseCell(leftmostCellID);
            this->NumFlips++;
          }
          wave->InsertNextId(leftmostCellID);
          visited[leftmostCellID] = VTK_CELL_VISITED;
          this->TraverseAndOrder(oldMesh, newMesh, wave, wave2, cellPointIds, cellIds,
            neighborPointIds, visited, this->NumFlips);
          wave->Reset();
          wave2->Reset();
        } // if found leftmost cell
      }   // Still some points in the queue
      vtkDebugMacro(<< "Reversed ordering of " << this->NumFlips << " polygons");
    }    // automatically orient normals
    else // this->Consistency
    {
      for (vtkIdType cellId = 0; cellId < numPolys; cellId++)
      {
        if (visited[cellId] == VTK_CELL_NOT_VISITED)
        {
          if (this->FlipNormals)
          {
            this->NumFlips++;
            newMesh->ReverseCell(cellId);
          }
          wave->InsertNextId(cellId);
          visited[cellId] = VTK_CELL_VISITED;
          this->TraverseAndOrder(oldMesh, newMesh, wave, wave2, cellPointIds, cellIds,
            neighborPointIds, visited, this->NumFlips);
        }
        wave->Reset();
        wave2->Reset();
      }
      vtkDebugMacro(<< "Reversed ordering of " << this->NumFlips << " polygons");
    } // Consistent ordering
  }

  this->UpdateProgress(0.333);

  //  Initial pass to compute polygon normals without effects of neighbors
  vtkNew<vtkFloatArray> cellNormals;
  cellNormals->SetName("Normals");
  cellNormals->SetNumberOfComponents(3);
  cellNormals->SetNumberOfTuples(numVerts + numLines + numPolys);

  vtkIdType offsetCells = numVerts + numLines;
  vtkSMPTools::For(0, offsetCells, [&](vtkIdType begin, vtkIdType end) {
    static const double n[3] = { 1.0, 0.0, 0.0 };
    for (vtkIdType cellId = begin; cellId < end; cellId++)
    {
      // add a default value for vertices and lines
      // normals do not have meaningful values, we set them to X
      cellNormals->SetTuple(cellId, n);
    }
  });

  // Compute Cell Normals of polys
  vtkSMPTools::For(0, newPolys->GetNumberOfCells(), [&](vtkIdType begin, vtkIdType end) {
    vtkNew<vtkIdList> tempCellPointIds;
    vtkIdType npts = 0;
    const vtkIdType* pts = nullptr;
    double n[3];
    for (vtkIdType polyId = begin; polyId < end; polyId++)
    {
      newPolys->GetCellAtId(polyId, npts, pts, tempCellPointIds);
      vtkPolygon::ComputeNormal(inPoints, npts, pts, n);
      cellNormals->SetTuple(offsetCells + polyId, n);
    }
  });

  // Split mesh if sharp features
  vtkNew<vtkPoints> newPoints;
  vtkIdType numNewPts;
  if (this->Splitting)
  {
    //  Traverse all nodes; evaluate loops and feature edges.  If feature
    //  edges found, split mesh creating new nodes.  Update polygon
    // connectivity.
    this->CosAngle = std::cos(vtkMath::RadiansFromDegrees(this->FeatureAngle));
    //  Splitting will create new points.  We have to create index array
    // to map new points into old points.
    vtkNew<vtkIdList> newToOldPointsMap;
    newToOldPointsMap->SetNumberOfIds(numPts);
    vtkSMPTools::For(0, numPts, [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType i = begin; i < end; i++)
      {
        newToOldPointsMap->SetId(i, i);
      }
    });

    this->ExecuteMarkAndSplit(
      oldMesh, newMesh, cellNormals, newToOldPointsMap, numPts, numPolys, this->CosAngle);
    numNewPts = newToOldPointsMap->GetNumberOfIds();

    vtkDebugMacro(<< "Created " << numNewPts - numPts << " new points");

    //  Now need to map attributes of old points into new points.
    outPD->CopyNormalsOff();
    outPD->CopyAllocate(inPD, numNewPts);

    // set precision for the points in the output
    if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
    {
      if (auto inputPointSet = vtkPointSet::SafeDownCast(input))
      {
        newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
      }
      else
      {
        newPoints->SetDataType(VTK_FLOAT);
      }
    }
    else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
    {
      newPoints->SetDataType(VTK_FLOAT);
    }
    else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
    {
      newPoints->SetDataType(VTK_DOUBLE);
    }

    newPoints->SetNumberOfPoints(numNewPts);
    outPD->SetNumberOfTuples(numNewPts);
    vtkIdType* mapPtr = newToOldPointsMap->GetPointer(0);
    vtkSMPTools::For(0, numNewPts, [&](vtkIdType begin, vtkIdType end) {
      double p[3];
      for (vtkIdType newPointId = begin; newPointId < end; newPointId++)
      {
        vtkIdType& oldPointId = mapPtr[newPointId];
        inPoints->GetPoint(oldPointId, p);
        newPoints->SetPoint(newPointId, p);
        outPD->CopyData(inPD, oldPointId, newPointId);
      }
    });
  }
  else // no splitting, so no new points
  {
    numNewPts = numPts;
    outPD->CopyNormalsOff();
    outPD->PassData(inPD);
  }
  this->UpdateProgress(0.80);

  //  Finally, traverse all elements, computing polygon normals and
  //  accumulating them at the vertices.
  double flipDirection = 1.0;
  if (this->FlipNormals && !this->Consistency)
  {
    flipDirection = -1.0;
  }

  if (this->ComputePointNormals)
  {
    vtkNew<vtkFloatArray> pointNormals;
    pointNormals->SetName("Normals");
    pointNormals->SetNumberOfComponents(3);
    pointNormals->SetNumberOfTuples(numNewPts);
    float* pointNormalsPtr = pointNormals->WritePointer(0, 3 * numNewPts);
    vtkSMPTools::Fill(pointNormalsPtr, pointNormalsPtr + 3 * numNewPts, 0.0);
    float* cellNormalsPtr = cellNormals->WritePointer(3 * offsetCells, 3 * numPolys);

    // locks are needed because many cells can share the same points
    std::vector<vtkAtomicMutex> pointLocks(numNewPts);

    const auto numNewPolys = newPolys->GetNumberOfCells();
    vtkSMPTools::For(0, numNewPolys, [&](vtkIdType begin, vtkIdType end) {
      vtkNew<vtkIdList> tempCellPointIds;
      vtkIdType npts = 0;
      const vtkIdType* pts = nullptr;
      for (vtkIdType polyId = begin; polyId < end; ++polyId)
      {
        newPolys->GetCellAtId(polyId, npts, pts, tempCellPointIds);
        for (vtkIdType i = 0; i < npts; ++i)
        {
          const vtkIdType& cellPointId = pts[i];
          std::lock_guard<vtkAtomicMutex> pointLockGuard(pointLocks[cellPointId]);
          vtkMath::Add(&pointNormalsPtr[3 * cellPointId], &cellNormalsPtr[3 * polyId],
            &pointNormalsPtr[3 * cellPointId]);
        }
      }
    });

    // Normalize normals
    vtkSMPTools::For(0, numNewPts, [&](vtkIdType begin, vtkIdType end) {
      double length;
      for (vtkIdType pointId = begin; pointId < end; ++pointId)
      {
        length = vtkMath::Norm(&pointNormalsPtr[3 * pointId]) * flipDirection;
        if (length != 0.0)
        {
          vtkMath::MultiplyScalar(&pointNormalsPtr[3 * pointId], 1.0 / length);
        }
      }
    });
    outPD->SetNormals(pointNormals);
  }
  if (this->ComputeCellNormals)
  {
    outCD->SetNormals(cellNormals);
  }

  //  Update ourselves.  If no new nodes have been created (i.e., no
  //  splitting), we can simply pass data through.
  if (!this->Splitting)
  {
    output->SetPoints(inPoints);
  }
  else
  {
    // If there is splitting, then have to send down the new data.
    output->SetPoints(newPoints);
  }
  output->SetPolys(newPolys);

  // copy the original vertices and lines to the output
  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());

  return 1;
}

//-----------------------------------------------------------------------------
//  Propagate wave of consistently ordered polygons.
void vtkPolyDataNormals::TraverseAndOrder(vtkPolyData* oldMesh, vtkPolyData* newMesh,
  vtkIdList* wave, vtkIdList* wave2, vtkIdList* cellPointIds, vtkIdList* cellIds,
  vtkIdList* neighborPointIds, std::vector<char>& visited, vtkIdType& numFlips)
{
  vtkIdType i, k;
  int j, l, j1;
  vtkIdType numIds, cellId;
  const vtkIdType* pts;
  const vtkIdType* neiPts;
  vtkIdType npts;
  vtkIdType numNeiPts;
  vtkIdType neighbor;

  // propagate wave until nothing left in wave
  while ((numIds = wave->GetNumberOfIds()) > 0)
  {
    for (i = 0; i < numIds; i++)
    {
      cellId = wave->GetId(i);

      newMesh->GetCellPoints(cellId, npts, pts, cellPointIds);

      for (j = 0, j1 = 1; j < npts; ++j, (j1 = (++j1 < npts) ? j1 : 0)) // for each edge neighbor
      {
        oldMesh->GetCellEdgeNeighbors(cellId, pts[j], pts[j1], cellIds);

        //  Check the direction of the neighbor ordering.  Should be
        //  consistent with us (i.e., if we are n1->n2, neighbor should be n2->n1).
        if (cellIds->GetNumberOfIds() == 1 || this->NonManifoldTraversal)
        {
          for (k = 0; k < cellIds->GetNumberOfIds(); k++)
          {
            neighbor = cellIds->GetId(k);
            if (visited[neighbor] == VTK_CELL_NOT_VISITED)
            {
              newMesh->GetCellPoints(neighbor, numNeiPts, neiPts, neighborPointIds);

              for (l = 0; l < numNeiPts; l++)
              {
                if (neiPts[l] == pts[j1])
                {
                  break;
                }
              }

              //  Have to reverse ordering if neighbor not consistent
              if (neiPts[(l + 1) % numNeiPts] != pts[j])
              {
                numFlips++;
                newMesh->ReverseCell(neighbor);
              }
              visited[neighbor] = VTK_CELL_VISITED;
              wave2->InsertNextId(neighbor);
            } // if cell not visited
          }   // for each edge neighbor
        }     // for manifold or non-manifold traversal allowed
      }       // for all edges of this polygon
    }         // for all cells in wave

    // swap wave and proceed with propagation
    std::swap(wave, wave2);
    wave2->Reset();
  } // while wave still propagating
}

//-----------------------------------------------------------------------------
// Mark polygons around vertex.  Create new vertex (if necessary) and
// replace (i.e., split mesh).
struct vtkPolyDataNormals::MarkAndSplitFunctor
{
  vtkPolyData* OldMesh;
  vtkPolyData* NewMesh;
  vtkFloatArray* CellNormals;
  vtkIdList* Map;
  vtkIdType NumPoints;
  vtkIdType NumPolys;
  double CosAngle;

  struct CellPointReplacementInformation
  {
    vtkIdType CellId;
    int NumberOfRegions;
    CellPointReplacementInformation()
      : CellId(0)
      , NumberOfRegions(0)
    {
    }
    CellPointReplacementInformation(vtkIdType cellId, int numberOfRegions)
      : CellId(cellId)
      , NumberOfRegions(numberOfRegions)
    {
    }
  };
  std::vector<std::vector<CellPointReplacementInformation>> CellPointsReplacementInfo;

  struct LocalData
  {
    vtkSmartPointer<vtkIdList> TempCellPointIds;
    vtkSmartPointer<vtkIdList> CellIds;
    std::vector<int> Visited; // Used to check if cell is visited and the number of regions
  };
  vtkSMPThreadLocal<LocalData> TLData;

  MarkAndSplitFunctor(vtkPolyData* oldMesh, vtkPolyData* newMesh, vtkFloatArray* cellNormals,
    vtkIdList* map, vtkIdType numPoints, vtkIdType numPolys, double cosAngle)
    : OldMesh(oldMesh)
    , NewMesh(newMesh)
    , CellNormals(cellNormals)
    , Map(map)
    , NumPoints(numPoints)
    , NumPolys(numPolys)
    , CosAngle(cosAngle)
  {
    this->CellPointsReplacementInfo.resize(numPoints);
  }

  void Initialize()
  {
    auto& tlData = this->TLData.Local();
    tlData.TempCellPointIds = vtkSmartPointer<vtkIdList>::New();
    tlData.CellIds = vtkSmartPointer<vtkIdList>::New();
    tlData.Visited.resize(this->NumPolys, -1);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& tlData = this->TLData.Local();
    auto& tempCellPointIds = tlData.TempCellPointIds;
    auto& cellIds = tlData.CellIds;
    auto& visited = tlData.Visited;
    float* cellNormals = this->CellNormals->GetPointer(0);

    vtkIdType ncells, *cells, i, j, numPts;
    const vtkIdType* pts;
    for (vtkIdType pointId = begin; pointId < end; ++pointId)
    {
      // Get the cells using this point and make sure that we have to do something
      this->OldMesh->GetPointCells(pointId, ncells, cells);
      if (ncells <= 1)
      {
        continue; // point does not need to be further disconnected
      }

      // Start moving around the "cycle" of points using the point. Label
      // each point as requiring a visit. Then label each subregion of cells
      // connected to this point that are connected (and not separated by
      // a feature edge) with a given region number. For each N regions
      // created, N-1 duplicate (split) points are created. The split point
      // replaces the current point ptId in the polygons connectivity array.
      //
      // Start by initializing the cells as unvisited
      for (i = 0; i < ncells; i++)
      {
        visited[cells[i]] = -1;
      }

      // Loop over all cells and mark the region that each is in.
      int numRegions = 0;
      vtkIdType spot, neiPt[2], nei, cellId, neiCellId;
      float *thisNormal, *neiNormal;
      for (j = 0; j < ncells; j++) // for all cells connected to point
      {
        if (visited[cells[j]] < 0) // for all unvisited cells
        {
          visited[cells[j]] = numRegions;
          // okay, mark all the cells connected to this seed cell and using ptId
          this->OldMesh->GetCellPoints(cells[j], numPts, pts, tempCellPointIds);

          // find the two edges
          for (spot = 0; spot < numPts; spot++)
          {
            if (pts[spot] == pointId)
            {
              break;
            }
          }

          if (spot == 0)
          {
            neiPt[0] = pts[spot + 1];
            neiPt[1] = pts[numPts - 1];
          }
          else if (spot == (numPts - 1))
          {
            neiPt[0] = pts[spot - 1];
            neiPt[1] = pts[0];
          }
          else
          {
            neiPt[0] = pts[spot + 1];
            neiPt[1] = pts[spot - 1];
          }

          for (i = 0; i < 2; i++) // for each of the two edges of the seed cell
          {
            cellId = cells[j];
            nei = neiPt[i];
            while (cellId >= 0) // while we can grow this region
            {
              this->OldMesh->GetCellEdgeNeighbors(cellId, pointId, nei, cellIds);
              if (cellIds->GetNumberOfIds() == 1 && visited[(neiCellId = cellIds->GetId(0))] < 0)
              {
                thisNormal = cellNormals + 3 * cellId;
                neiNormal = cellNormals + 3 * neiCellId;

                if (vtkMath::Dot(thisNormal, neiNormal) > this->CosAngle)
                {
                  // visit and arrange to visit next edge neighbor
                  visited[neiCellId] = numRegions;
                  cellId = neiCellId;
                  this->OldMesh->GetCellPoints(cellId, numPts, pts, tempCellPointIds);

                  for (spot = 0; spot < numPts; spot++)
                  {
                    if (pts[spot] == pointId)
                    {
                      break;
                    }
                  }

                  if (spot == 0)
                  {
                    nei = (pts[spot + 1] != nei ? pts[spot + 1] : pts[numPts - 1]);
                  }
                  else if (spot == (numPts - 1))
                  {
                    nei = (pts[spot - 1] != nei ? pts[spot - 1] : pts[0]);
                  }
                  else
                  {
                    nei = (pts[spot + 1] != nei ? pts[spot + 1] : pts[spot - 1]);
                  }

                } // if not separated by edge angle
                else
                {
                  cellId = -1; // separated by edge angle
                }
              } // if can move to edge neighbor
              else
              {
                cellId = -1; // separated by previous visit, boundary, or non-manifold
              }
            } // while visit wave is propagating
          }   // for each of the two edges of the starting cell
          numRegions++;
        } // if cell is unvisited
      }   // for all cells connected to point ptId

      if (numRegions <= 1)
      {
        continue; // a single region, no splitting ever required
      }

      // store all cells not in the first region that require splitting
      auto& cellPointReplacementInfo = this->CellPointsReplacementInfo[pointId];
      for (j = 0; j < ncells; ++j)
      {
        if (visited[cells[j]] > 0)
        {
          cellPointReplacementInfo.emplace_back(cells[j], visited[cells[j]]);
        }
      }
    }
  }

  void Reduce()
  {
    // This part needs to be done sequentially.
    vtkNew<vtkIdList> tempCellPointIds;
    vtkIdType replacementPointId;
    vtkIdType lastId;
    for (vtkIdType pointId = 0; pointId < this->NumPoints; ++pointId)
    {
      // For all cells not in the first region, the info pointId is
      // replaced with a new pointId, which is a duplicate of the first
      // point, but disconnected topologically.
      lastId = this->Map->GetNumberOfIds();
      for (auto& cellPointReplacementInfo : this->CellPointsReplacementInfo[pointId])
      {
        const auto& cellId = cellPointReplacementInfo.CellId;
        const auto& numberOfRegions = cellPointReplacementInfo.NumberOfRegions;
        replacementPointId = lastId + numberOfRegions - 1;
        this->Map->InsertId(replacementPointId, pointId);
        this->NewMesh->ReplaceCellPoint(cellId, pointId, replacementPointId, tempCellPointIds);
      } // for all cells connected to pointId and not in first region that require splitting
    }
  }
};

//-----------------------------------------------------------------------------
void vtkPolyDataNormals::ExecuteMarkAndSplit(vtkPolyData* oldMesh, vtkPolyData* newMesh,
  vtkFloatArray* cellNormals, vtkIdList* map, vtkIdType numPoints, vtkIdType numPolys,
  double cosAngle)
{
  MarkAndSplitFunctor functor(oldMesh, newMesh, cellNormals, map, numPoints, numPolys, cosAngle);
  vtkSMPTools::For(0, numPoints, functor);
}

//-----------------------------------------------------------------------------
void vtkPolyDataNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Splitting: " << (this->Splitting ? "On\n" : "Off\n");
  os << indent << "Consistency: " << (this->Consistency ? "On\n" : "Off\n");
  os << indent << "Flip Normals: " << (this->FlipNormals ? "On\n" : "Off\n");
  os << indent << "Auto Orient Normals: " << (this->AutoOrientNormals ? "On\n" : "Off\n");
  os << indent << "Num Flips: " << this->NumFlips << endl;
  os << indent << "Compute Point Normals: " << (this->ComputePointNormals ? "On\n" : "Off\n");
  os << indent << "Compute Cell Normals: " << (this->ComputeCellNormals ? "On\n" : "Off\n");
  os << indent << "Non-manifold Traversal: " << (this->NonManifoldTraversal ? "On\n" : "Off\n");
  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
}
