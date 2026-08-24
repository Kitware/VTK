// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGraphGeodesicPath.h"

#include "vtkCellCenters.h"
#include "vtkDataArray.h"
#include "vtkDijkstraGraphInternals.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkGraphGeodesicPath, RepelVertices, vtkPoints);

//------------------------------------------------------------------------------
vtkGraphGeodesicPath::vtkGraphGeodesicPath()
  : Internals(new vtkDijkstraGraphInternals())
{
  this->IdList = vtkIdList::New();
}

//------------------------------------------------------------------------------
vtkGraphGeodesicPath::~vtkGraphGeodesicPath()
{
  if (this->IdList)
  {
    this->IdList->Delete();
  }
  this->SetRepelVertices(nullptr);
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StartVertex: " << this->StartVertex << endl;
  os << indent << "EndVertex: " << this->EndVertex << endl;
  os << indent << "StopWhenEndReached: ";
  if (this->StopWhenEndReached)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
  }
  os << indent << "UseScalarWeights: ";
  if (this->UseScalarWeights)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
  }
  os << indent << "RepelPathFromVertices: ";
  if (this->RepelPathFromVertices)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
  }
  os << indent << "RepelVertices: " << this->RepelVertices << endl;
  os << indent << "IdList: " << this->IdList << endl;
  os << indent << "Number of vertices in input data: " << this->NumberOfVertices << endl;
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::GetCumulativeWeights(vtkDoubleArray* weights)
{
  if (!weights)
  {
    return;
  }

  weights->Initialize();
  double* weightsArray = new double[this->Internals->CumulativeWeights.size()];
  std::copy(this->Internals->CumulativeWeights.begin(), this->Internals->CumulativeWeights.end(),
    weightsArray);
  weights->SetArray(
    weightsArray, static_cast<vtkIdType>(this->Internals->CumulativeWeights.size()), 0);
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::Initialize(vtkDataSet* inData)
{
  this->NumberOfVertices = this->GetNumberOfNodes(inData);

  this->Internals->Initialize(this->NumberOfVertices);
  // The heap has elements from 1 to n
  this->Internals->InitializeHeap(this->NumberOfVertices);

  this->Reset();
  this->AdjacencyBuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::Reset()
{
  std::fill(
    this->Internals->CumulativeWeights.begin(), this->Internals->CumulativeWeights.end(), -1.0);
  std::fill(this->Internals->Predecessors.begin(), this->Internals->Predecessors.end(), -1);
  std::fill(this->Internals->OpenVertices.begin(), this->Internals->OpenVertices.end(), false);
  std::fill(this->Internals->ClosedVertices.begin(), this->Internals->ClosedVertices.end(), false);
  if (this->RepelPathFromVertices)
  {
    std::fill(
      this->Internals->BlockedVertices.begin(), this->Internals->BlockedVertices.end(), false);
  }

  this->IdList->Reset();
  this->Internals->ResetHeap();
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::TraceShortestPath(
  vtkDataSet* inData, vtkPolyData* outPoly, vtkIdType startv, vtkIdType endv)
{
  vtkPoints* points = vtkPoints::New();
  vtkCellArray* lines = vtkCellArray::New();

  // n is far to many. Adjusted later
  lines->InsertNextCell(this->NumberOfVertices);

  // trace backward
  vtkIdType v = endv;
  double pt[3];
  vtkIdType id;
  while (v != startv)
  {
    if (this->CheckAbort())
    {
      break;
    }
    if (v < 0)
    {
      // Invalid vertex. Path does not exist.
      break;
    }

    this->IdList->InsertNextId(v);
    this->GetNodeFromIndex(inData, v, pt);
    id = points->InsertNextPoint(pt);
    lines->InsertCellPoint(id);

    v = this->Internals->Predecessors[v];
  }

  if (v >= 0)
  {
    this->IdList->InsertNextId(v);
    this->GetNodeFromIndex(inData, v, pt);
    id = points->InsertNextPoint(pt);
    lines->InsertCellPoint(id);
    lines->UpdateCellCount(points->GetNumberOfPoints());
  }
  else
  {
    points->Reset();
    lines->Reset();
  }

  outPoly->SetPoints(points);
  points->Delete();
  outPoly->SetLines(lines);
  lines->Delete();
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::Relax(int u, int v, double w)
{
  double du = this->Internals->CumulativeWeights[u] + w;
  if (this->Internals->CumulativeWeights[v] > du)
  {
    this->Internals->CumulativeWeights[v] = du;
    this->Internals->Predecessors[v] = u;

    this->Internals->HeapDecreaseKey(v);
  }
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::ShortestPath(vtkDataSet* inData, int startv, int endv)
{
  int u, v;

  this->DiscardRepelVertices(inData, startv, endv);

  this->Internals->CumulativeWeights[startv] = 0;

  this->Internals->HeapInsert(startv);
  this->Internals->OpenVertices[startv] = true;

  bool stop = false;
  while ((u = this->Internals->HeapExtractMin()) >= 0 && !stop)
  {
    if (this->CheckAbort())
    {
      break;
    }
    // u is now in ClosedVertices since the shortest path to u is determined
    this->Internals->ClosedVertices[u] = true;
    // remove u from OpenVertices
    this->Internals->OpenVertices[u] = false;

    if (u == endv && this->StopWhenEndReached)
    {
      stop = true;
    }

    std::map<int, double>::iterator it = this->Internals->Adjacency[u].begin();

    // Update all vertices v adjacent to u
    for (; it != this->Internals->Adjacency[u].end(); ++it)
    {
      v = (*it).first;

      // ClosedVertices is the set of vertices with determined shortest path...
      // do not use them again
      if (!this->Internals->ClosedVertices[v])
      {
        // Only relax edges where the end is not in ClosedVertices
        // and edge is in OpenVertices
        double weight;
        if (this->Internals->BlockedVertices[v])
        {
          weight = VTK_FLOAT_MAX;
        }
        else
        {
          weight = (*it).second + this->CalculateDynamicEdgeCost(inData, u, v);
        }

        if (this->Internals->OpenVertices[v])
        {
          this->Relax(u, v, weight);
        }
        // add edge v to OpenVertices
        else
        {
          this->Internals->OpenVertices[v] = true;
          this->Internals->CumulativeWeights[v] = this->Internals->CumulativeWeights[u] + weight;

          // Set Predecessor of v to be u
          this->Internals->Predecessors[v] = u;
          this->Internals->HeapInsert(v);
        }
      }
    }
  }
}

void vtkGraphGeodesicPath::DiscardRepelVertices(vtkDataSet* inData, int startv, int endv)
{
  if (this->RepelPathFromVertices && this->RepelVertices)
  {
    // loop over the pts and if they are in the image
    // get the associated index for that point and mark it as blocked
    for (int i = 0; i < this->RepelVertices->GetNumberOfPoints(); ++i)
    {
      double* pt = this->RepelVertices->GetPoint(i);
      int u = inData->FindPoint(pt);
      if (u < 0 || u == startv || u == endv)
      {
        continue;
      }
      this->Internals->BlockedVertices[u] = true;
    }
  }
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::SetProcessedFieldArrayName(const std::string& name)
{
  if (this->ProcessedFieldArrayName != name)
  {
    this->ProcessedFieldArrayName = name;
    this->AdjacencyParametersTime.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::SetUseScalarWeights(vtkTypeBool value)
{
  if (this->UseScalarWeights != value)
  {
    this->UseScalarWeights = value;
    this->AdjacencyParametersTime.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::GetNodeFromIndex(vtkDataSet* inData, vtkIdType u, double pt[3])
{
  inData->GetPoint(u, pt);
}

//------------------------------------------------------------------------------
vtkIdType vtkGraphGeodesicPath::GetNumberOfNodes(vtkDataSet* inData)
{
  return inData->GetNumberOfPoints();
}

VTK_ABI_NAMESPACE_END
