// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDijkstraGraphGeodesicPath.h"

#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDijkstraGraphInternals.h"
#include "vtkExecutive.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDijkstraGraphGeodesicPath);

//------------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::vtkDijkstraGraphGeodesicPath() = default;

//------------------------------------------------------------------------------
vtkDijkstraGraphGeodesicPath::~vtkDijkstraGraphGeodesicPath() = default;

//------------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::AddBidirectionalEdge(vtkDataSet* inData, vtkDataArray* scalars,
  std::vector<std::map<int, double>>& adjacency, vtkIdType u, vtkIdType v)
{
  std::map<int, double>& adjU = adjacency[u];
  if (adjU.find(v) == adjU.end())
  {
    double cost = this->CalculateStaticEdgeCost(inData, scalars, u, v);
    adjU.insert(std::make_pair(v, cost));
  }

  std::map<int, double>& adjV = adjacency[v];
  if (adjV.find(u) == adjV.end())
  {
    double cost = this->CalculateStaticEdgeCost(inData, scalars, v, u);
    adjV.insert(std::make_pair(u, cost));
  }
}

//------------------------------------------------------------------------------
int vtkDijkstraGraphGeodesicPath::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkDijkstraGraphGeodesicPath::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  vtkIdType startVertex = this->StartVertex;
  vtkIdType endVertex = this->EndVertex;
  if (!this->UseNodeIndices)
  {
    if (this->GraphType == vtkDataObject::AttributeTypes::POINT)
    {
      startVertex = input->FindPoint(this->StartPoint);
      endVertex = input->FindPoint(this->EndPoint);
    }
    else
    {
      vtkNew<vtkCellLocator> cellLocator;
      cellLocator->SetDataSet(input);
      cellLocator->BuildLocator();

      double closestPoint[3];
      vtkIdType cellId;
      int subId;
      double dist2;

      cellLocator->FindClosestPoint(this->StartPoint, closestPoint, cellId, subId, dist2);
      startVertex = cellId;
      cellLocator->FindClosestPoint(this->EndPoint, closestPoint, cellId, subId, dist2);
      endVertex = cellId;
    }
  }

  if (startVertex < 0 || endVertex < 0)
  {
    vtkWarningMacro("Invalid Start or End vertex.");
    return 0;
  }

  if (this->AdjacencyBuildTime.GetMTime() < input->GetMTime() ||
    this->AdjacencyBuildTime.GetMTime() < AdjacencyParametersTime.GetMTime())
  {
    this->Initialize(input);
    this->GraphType == vtkDataObject::AttributeTypes::POINT ? this->BuildAdjacency(input)
                                                            : this->BuildCellAdjacency(input);
  }
  else
  {
    this->Reset();
  }

  if (this->NumberOfVertices == 0)
  {
    return 0;
  }

  this->ShortestPath(input, startVertex, endVertex);
  this->TraceShortestPath(input, output, startVertex, endVertex);
  return 1;
}

//------------------------------------------------------------------------------
double vtkDijkstraGraphGeodesicPath::CalculateStaticEdgeCost(
  vtkDataSet* inData, vtkIdType u, vtkIdType v)
{
  return CalculateStaticEdgeCost(inData, nullptr, u, v);
}

//------------------------------------------------------------------------------
double vtkDijkstraGraphGeodesicPath::CalculateStaticEdgeCost(
  vtkDataSet* inData, vtkDataArray* scalars, vtkIdType u, vtkIdType v)
{
  double p1[3], p2[3];
  if (this->GraphType == vtkDataObject::AttributeTypes::POINT)
  {
    inData->GetPoint(u, p1);
    inData->GetPoint(v, p2);
  }
  else
  {
    this->CellCenters->GetPoint(u, p1);
    this->CellCenters->GetPoint(v, p2);
  }

  double w = std::sqrt(vtkMath::Distance2BetweenPoints(p1, p2));

  if (!scalars)
  {
    return w;
  }

  double s = scalars->GetTuple1(v);
  const double wt = s * s;

  if (wt > 1e-6)
  {
    w /= wt;
  }

  return w;
}

//------------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::BuildCellAdjacency(vtkDataSet* inData)
{
  vtkNew<vtkCellCenters> cellCentersFilter;
  cellCentersFilter->SetInputData(inData);
  cellCentersFilter->Update();
  this->CellCenters->DeepCopy(cellCentersFilter->GetOutput());

  vtkIdType numberOfCells = inData->GetNumberOfCells();
  this->Internals->Adjacency.clear();
  this->Internals->Adjacency.resize(numberOfCells);

  vtkDataArray* scalars = nullptr;
  if (this->UseScalarWeights && inData->GetCellData())
  {
    scalars = vtkDataArray::SafeDownCast(
      inData->GetCellData()->GetArray(this->ProcessedFieldArrayName.c_str()));
  }

  vtkNew<vtkIdList> neighborIds;
  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    vtkCell* cell = inData->GetCell(cellId);
    int numberOfFacets =
      cell->GetCellDimension() == 3 ? cell->GetNumberOfFaces() : cell->GetNumberOfEdges();
    for (int f = 0; f < numberOfFacets; ++f)
    {
      vtkCell* facet = cell->GetCellDimension() == 3 ? cell->GetFace(f) : cell->GetEdge(f);
      neighborIds->Initialize();
      inData->GetCellNeighbors(cellId, facet->GetPointIds(), neighborIds);
      for (vtkIdType k = 0; k < neighborIds->GetNumberOfIds(); ++k)
      {
        this->AddBidirectionalEdge(
          inData, scalars, this->Internals->Adjacency, cellId, neighborIds->GetId(k));
      }
    }
  }
}

//------------------------------------------------------------------------------
// This is probably a horribly inefficient way to do it.
void vtkDijkstraGraphGeodesicPath::BuildAdjacency(vtkDataSet* inData)
{
  vtkIdType ncells = inData->GetNumberOfCells();
  vtkNew<vtkIdList> pts;

  vtkDataArray* scalars = nullptr;
  if (this->UseScalarWeights && inData->GetPointData())
  {
    scalars = vtkDataArray::SafeDownCast(
      inData->GetPointData()->GetArray(this->ProcessedFieldArrayName.c_str()));
  }

  for (vtkIdType i = 0; i < ncells; i++)
  {
    vtkCell* cell = inData->GetCell(i);
    if (!cell)
    {
      continue;
    }
    int nedges = cell->GetNumberOfEdges();
    for (int e = 0; e < nedges; ++e)
    {
      vtkCell* edge = cell->GetEdge(e);
      vtkIdType u = edge->GetPointId(0);
      vtkIdType v = edge->GetPointId(1);
      this->AddBidirectionalEdge(inData, scalars, this->Internals->Adjacency, u, v);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "UseNodeIndices: ";
  if (this->UseNodeIndices)
  {
    os << "On\n";
  }
  else
  {
    os << "Off\n";
    os << indent << "StartPoint: " << "(" << StartPoint[0] << ", " << StartPoint[1] << ", "
       << StartPoint[2] << ")\n";
    os << indent << "EndPoint: " << "(" << EndPoint[0] << ", " << EndPoint[1] << ", " << EndPoint[2]
       << ")\n";
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
void vtkDijkstraGraphGeodesicPath::DiscardRepelVertices(vtkDataSet* inData, int startv, int endv)
{
  if (this->GraphType == vtkDataObject::AttributeTypes::POINT && this->RepelPathFromVertices &&
    this->RepelVertices)
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
void vtkDijkstraGraphGeodesicPath::SetGraphType(int type)
{
  if (this->GraphType != type &&
    (type == vtkDataObject::AttributeTypes::POINT || type == vtkDataObject::AttributeTypes::CELL))
  {
    this->GraphType = type;
    this->AdjacencyParametersTime.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkDijkstraGraphGeodesicPath::GetNodeFromIndex(vtkDataSet* inData, vtkIdType u, double pt[3])
{
  this->GraphType == vtkDataObject::AttributeTypes::POINT ? inData->GetPoint(u, pt)
                                                          : this->CellCenters->GetPoint(u, pt);
}

//------------------------------------------------------------------------------
vtkIdType vtkDijkstraGraphGeodesicPath::GetNumberOfNodes(vtkDataSet* inData)
{
  return this->GraphType == vtkDataObject::AttributeTypes::POINT ? inData->GetNumberOfPoints()
                                                                 : inData->GetNumberOfCells();
}

VTK_ABI_NAMESPACE_END
