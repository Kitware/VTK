// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOrientPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkPriorityQueue.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOrientPolyData);

//----------------------------------------------------------------------------
vtkOrientPolyData::vtkOrientPolyData()
  : Consistency(true)
  , AutoOrientNormals(false)
  , NonManifoldTraversal(false)
  , FlipNormals(false)
{
}

//----------------------------------------------------------------------------
vtkOrientPolyData::~vtkOrientPolyData() = default;

//----------------------------------------------------------------------------
void vtkOrientPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Consistency: " << (this->Consistency ? "On\n" : "Off\n");
  os << indent << "AutoOrientNormals: " << (this->AutoOrientNormals ? "On\n" : "Off\n");
  os << indent << "NonManifoldTraversal: " << (this->NonManifoldTraversal ? "On\n" : "Off\n");
  os << indent << "FlipNormals: " << (this->FlipNormals ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
static constexpr char VTK_CELL_NOT_VISITED = 0;
static constexpr char VTK_CELL_VISITED = 1;

//-----------------------------------------------------------------------------
//  Propagate wave of consistently ordered polygons.
void vtkOrientPolyData::TraverseAndOrder(vtkPolyData* input, vtkPolyData* output, vtkIdList* wave,
  vtkIdList* wave2, vtkIdList* cellPointIds, vtkIdList* cellIds, vtkIdList* neighborPointIds,
  std::vector<char>& visited, vtkIdType& numFlips)
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

      output->GetCellPoints(cellId, npts, pts, cellPointIds);
      if (npts < 3)
      {
        continue;
      }

      for (j = 0, j1 = 1; j < npts; ++j, (j1 = (++j1 < npts) ? j1 : 0)) // for each edge neighbor
      {
        input->GetCellEdgeNeighbors(cellId, pts[j], pts[j1], cellIds);

        //  Check the direction of the neighbor ordering.  Should be
        //  consistent with us (i.e., if we are n1->n2, neighbor should be n2->n1).
        if (cellIds->GetNumberOfIds() == 1 || this->NonManifoldTraversal)
        {
          for (k = 0; k < cellIds->GetNumberOfIds(); k++)
          {
            neighbor = cellIds->GetId(k);
            if (visited[neighbor] == VTK_CELL_NOT_VISITED)
            {
              output->GetCellPoints(neighbor, numNeiPts, neiPts, neighborPointIds);

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
                output->ReverseCell(neighbor);
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

//----------------------------------------------------------------------------
int vtkOrientPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  auto input = vtkPolyData::GetData(inputVector[0]);
  auto output = vtkPolyData::GetData(outputVector);

  vtkPoints* inPoints = input->GetPoints();
  const vtkIdType numInPoints = input->GetNumberOfPoints();
  const vtkIdType numInPolys = input->GetNumberOfPolys();
  const vtkIdType numberOfCells = input->GetNumberOfCells();

  if (numInPoints == 0)
  {
    return 1;
  }
  if (numInPolys == 0 || (!this->AutoOrientNormals && !this->Consistency))
  {
    // don't do anything! pass data through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  ///////////////////////////////////////////////////////////////////
  // Build Cells And Links if needed
  ///////////////////////////////////////////////////////////////////
  if (input->NeedToBuildCells())
  {
    input->BuildCells();
  }
  input->BuildLinks();
  this->UpdateProgress(0.30);
  if (this->CheckAbort())
  {
    return 1;
  }

  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  output->SetVerts(input->GetVerts());
  output->SetLines(input->GetLines());
  // create a copy because we're modifying it
  vtkNew<vtkCellArray> outPolys;
  outPolys->DeepCopy(input->GetPolys());
  output->SetPolys(outPolys);
  output->GetCellData()->PassData(input->GetCellData());
  output->BuildCells(); // builds connectivity
  // Copy the links from the input to the output so that subsequent filters can use them.
  auto links = vtkSmartPointer<vtkAbstractCellLinks>::Take(input->GetLinks()->NewInstance());
  output->SetLinks(links);
  links->SetDataSet(output);
  links->ShallowCopy(input->GetLinks());

  ///////////////////////////////////////////////////////////////////
  //  Traverse all polygons insuring proper direction of ordering.  This
  //  works by propagating a wave from a seed polygon to the polygon's
  //  edge neighbors. Each neighbor may be reordered to maintain consistency
  //  with its (already checked) neighbors.
  ///////////////////////////////////////////////////////////////////
  vtkIdType numFlips = 0;
  // The visited array keeps track of which cells have been visited.
  std::vector<char> visited;
  visited.resize(numberOfCells, VTK_CELL_NOT_VISITED);
  vtkNew<vtkIdList> wave, wave2, cellPointIds, cellIds, neighborPointIds;
  wave->Allocate(numberOfCells / 4 + 1, numberOfCells);
  wave2->Allocate(numberOfCells / 4 + 1, numberOfCells);
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
    bool bestReverseFlag;
    vtkNew<vtkPriorityQueue> leftmostPoints;

    // Put all the points in the priority queue, based on x coord
    // So that we can find leftmost point
    leftmostPoints->Allocate(numInPoints);
    for (vtkIdType ptId = 0; ptId < numInPoints; ptId++)
    {
      leftmostPoints->Insert(inPoints->GetPoint(ptId)[0], ptId);
    }

    // Repeat this while loop as long as the queue is not empty,
    // because there may be multiple connected components, each of
    // which needs to be seeded independently with a correctly
    // oriented polygon.
    double n[3];
    const vtkIdType checkAbortInterval = std::min(numInPoints / 10 + 1, (vtkIdType)1000);
    vtkIdType progressCounter = 0;
    while (leftmostPoints->GetNumberOfItems())
    {
      if (progressCounter % checkAbortInterval == 0 && this->CheckAbort())
      {
        break;
      }
      progressCounter++;
      foundLeftmostCell = 0;
      // Keep iterating through leftmost points and cells located at
      // those points until I've got a leftmost point with
      // unvisited cells attached, and I've found the best cell
      // at that point
      do
      {
        currentPointID = leftmostPoints->Pop();
        input->GetPointCells(currentPointID, nleftmostCells, leftmostCells);
        bestNormalAbsXComponent = 0.0;
        bestReverseFlag = false;
        for (cIdx = 0; cIdx < nleftmostCells; cIdx++)
        {
          currentCellID = leftmostCells[cIdx];
          if (visited[currentCellID] == VTK_CELL_VISITED)
          {
            continue;
          }
          input->GetCellPoints(currentCellID, nCellPts, cellPts);
          vtkPolygon::ComputeNormal(inPoints, nCellPts, cellPts, n);
          // Ok, see if this leftmost cell candidate is the best
          // so far
          if (std::abs(n[0]) > bestNormalAbsXComponent)
          {
            bestNormalAbsXComponent = std::abs(n[0]);
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
          output->ReverseCell(leftmostCellID);
          numFlips++;
        }
        wave->InsertNextId(leftmostCellID);
        visited[leftmostCellID] = VTK_CELL_VISITED;
        this->TraverseAndOrder(
          input, output, wave, wave2, cellPointIds, cellIds, neighborPointIds, visited, numFlips);
        wave->Reset();
        wave2->Reset();
      } // if found leftmost cell
    }   // Still some points in the queue
    vtkDebugMacro(<< "Reversed ordering of " << numFlips << " polygons");
  }    // automatically orient normals
  else // this->Consistency
  {
    const vtkIdType checkAbortInterval = std::min(numberOfCells / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = 0; cellId < numberOfCells; cellId++)
    {
      if (cellId % checkAbortInterval == 0 && this->CheckAbort())
      {
        break;
      }
      if (visited[cellId] == VTK_CELL_NOT_VISITED)
      {
        if (this->FlipNormals)
        {
          numFlips++;
          output->ReverseCell(cellId);
        }
        wave->InsertNextId(cellId);
        visited[cellId] = VTK_CELL_VISITED;
        this->TraverseAndOrder(
          input, output, wave, wave2, cellPointIds, cellIds, neighborPointIds, visited, numFlips);
      }
      wave->Reset();
      wave2->Reset();
    }
    vtkDebugMacro(<< "Reversed ordering of " << numFlips << " polygons");
  } // Consistent ordering

  this->UpdateProgress(1.00);
  if (this->CheckAbort())
  {
    return 1;
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
