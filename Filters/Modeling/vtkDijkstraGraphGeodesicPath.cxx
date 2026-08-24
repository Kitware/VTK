// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDijkstraGraphGeodesicPath.h"

#include "vtkCellArray.h"
#include "vtkDijkstraGraphInternals.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDijkstraGraphGeodesicPath);
vtkCxxSetObjectMacro(vtkDijkstraGraphGeodesicPath, RepelVertices, vtkPoints);

//------------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::vtkDijkstraGraphGeodesicPath() = default;

//------------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::~vtkDijkstraGraphGeodesicPath() = default;

{
  this->IdList->Delete();
}
delete this->Internals;
this->SetRepelVertices(nullptr);
}

//------------------------------------------------------------------------------
return 1;
}

//------------------------------------------------------------------------------
int vtkDijkstraGraphGeodesicPath::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  if (this->AdjacencyBuildTime.GetMTime() < input->GetMTime())
  {
    this->Initialize(input);
  }
  else
  {
    this->Reset();
  }

  if (this->NumberOfVertices == 0)
  {
    return 0;
  }

  this->ShortestPath(input, this->StartVertex, this->EndVertex);
  this->TraceShortestPath(input, output, this->StartVertex, this->EndVertex);
  return 1;
}

//------------------------------------------------------------------------------
  vtkDataSet* inData, vtkIdType u, vtkIdType v)
  {
    return CalculateStaticEdgeCost(inData, nullptr, u, v);
  }

  //------------------------------------------------------------------------------
  double vtkDijkstraGraphGeodesicPath::CalculateStaticEdgeCost(
    vtkDataSet* inData, vtkIdType u, vtkIdType v)
  {
    double p1[3];
    inData->GetPoint(u, p1);
    double p2[3];
    inData->GetPoint(v, p2);

    double w = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));

    if (this->UseScalarWeights)
    {
      double s2 = 0.0;
      // Note this edge cost is not symmetric!
      if (inData->GetPointData())
      {
        vtkFloatArray* scalars = vtkFloatArray::SafeDownCast(inData->GetPointData()->GetScalars());
        if (scalars)
        {
          s2 = static_cast<double>(scalars->GetValue(v));
        }
      }

      double wt = s2 * s2;
      if (wt != 0.0)
      {
        w /= wt;
      }
    }
    return w;
  }

  //------------------------------------------------------------------------------
  // This is probably a horribly inefficient way to do it.
  void vtkDijkstraGraphGeodesicPath::BuildAdjacency(vtkDataSet* inData)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(inData);
    vtkIdType ncells = pd->GetNumberOfCells();

    for (vtkIdType i = 0; i < ncells; i++)
    {
      // Possible types
      //    VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE,
      //    VTK_POLY_LINE,VTK_TRIANGLE, VTK_QUAD,
      //    VTK_POLYGON, or VTK_TRIANGLE_STRIP.

      vtkIdType ctype = pd->GetCellType(i);

      // Until now only handle polys and triangles
      // TODO: All types
      if (ctype == VTK_POLYGON || ctype == VTK_TRIANGLE || ctype == VTK_LINE)
      {
        const vtkIdType* pts;
        vtkIdType npts;
        pd->GetCellPoints(i, npts, pts);
        double cost;

        for (int j = 0; j < npts; ++j)
        {
          vtkIdType u = pts[j];
          vtkIdType v = pts[((j + 1) % npts)];

          std::map<int, double>& mu = this->Internals->Adjacency[u];
          if (mu.find(v) == mu.end())
          {
            cost = this->CalculateStaticEdgeCost(inData, u, v);
            mu.insert(std::pair<int, double>(v, cost));
          }

          std::map<int, double>& mv = this->Internals->Adjacency[v];
          if (mv.find(u) == mv.end())
          {
            cost = this->CalculateStaticEdgeCost(inData, v, u);
            mv.insert(std::pair<int, double>(u, cost));
          }
        }
      }
    }

    this->AdjacencyBuildTime.Modified();
  }

  //------------------------------------------------------------------------------
  vtkDataArray* scalars = nullptr;
  if (this->UseScalarWeights && inData->GetPointData())
  {
    double* pt = this->RepelVertices->GetPoint(i);
    u = inData->FindPoint(pt);
    if (u < 0 || u == startv || u == endv)
    {
      continue;
    }
    this->Internals->BlockedVertices[u] = true;
  }
  }

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
        double w;
        if (this->Internals->BlockedVertices[v])
        {
          w = VTK_FLOAT_MAX;
        }
        else
        {
          w = (*it).second + this->CalculateDynamicEdgeCost(inData, u, v);
        }

        if (this->Internals->OpenVertices[v])
        {
          this->Relax(u, v, w);
        }
        // add edge v to OpenVertices
        else
        {
          this->Internals->OpenVertices[v] = true;
          this->Internals->CumulativeWeights[v] = this->Internals->CumulativeWeights[u] + w;

          // Set Predecessor of v to be u
          this->Internals->Predecessors[v] = u;
          this->Internals->HeapInsert(v);
        }
      }
    }
  }
  }

  //------------------------------------------------------------------------------
  void vtkDijkstraGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
  {
    this->Superclass::PrintSelf(os, indent);

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
  VTK_ABI_NAMESPACE_END
