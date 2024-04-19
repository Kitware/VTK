// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2011 LTSI INSERM U642
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHausdorffDistancePointSetFilter.h"

#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkCellLocator.h"
#include "vtkGenericCell.h"
#include "vtkKdTreePointLocator.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHausdorffDistancePointSetFilter);

//------------------------------------------------------------------------------
vtkHausdorffDistancePointSetFilter::vtkHausdorffDistancePointSetFilter()
{
  this->RelativeDistance[0] = 0.0;
  this->RelativeDistance[1] = 0.0;
  this->HausdorffDistance = 0.0;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfInputConnections(0, 1);
  this->SetNumberOfInputConnections(1, 1);

  this->SetNumberOfOutputPorts(2);

  this->TargetDistanceMethod = POINT_TO_POINT;
}

//------------------------------------------------------------------------------
vtkHausdorffDistancePointSetFilter::~vtkHausdorffDistancePointSetFilter() = default;

//------------------------------------------------------------------------------
int vtkHausdorffDistancePointSetFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfoA = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfoB = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfoA = outputVector->GetInformationObject(0);
  vtkInformation* outInfoB = outputVector->GetInformationObject(1);

  if (inInfoA == nullptr || inInfoB == nullptr)
  {
    return 0;
  }

  // Get the input
  vtkPointSet* inputA = vtkPointSet::SafeDownCast(inInfoA->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* inputB = vtkPointSet::SafeDownCast(inInfoB->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* outputA = vtkPointSet::SafeDownCast(outInfoA->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* outputB = vtkPointSet::SafeDownCast(outInfoB->Get(vtkDataObject::DATA_OBJECT()));

  if (inputA->GetNumberOfPoints() == 0 || inputB->GetNumberOfPoints() == 0)
  {
    return 0;
  }

  // Re-initialize the distances
  this->RelativeDistance[0] = 0.0;
  this->RelativeDistance[1] = 0.0;
  this->HausdorffDistance = 0.0;

  // TODO: using vtkStaticCellLocator, vtkStaticPointLocator is going to be much faster.
  // Need to investigate and replace if appropriate.
  vtkSmartPointer<vtkKdTreePointLocator> pointLocatorA =
    vtkSmartPointer<vtkKdTreePointLocator>::New();
  vtkSmartPointer<vtkKdTreePointLocator> pointLocatorB =
    vtkSmartPointer<vtkKdTreePointLocator>::New();

  vtkSmartPointer<vtkCellLocator> cellLocatorA = vtkSmartPointer<vtkCellLocator>::New();
  vtkSmartPointer<vtkCellLocator> cellLocatorB = vtkSmartPointer<vtkCellLocator>::New();

  if (this->TargetDistanceMethod == POINT_TO_POINT)
  {
    pointLocatorA->SetDataSet(inputA);
    pointLocatorA->BuildLocator();
    pointLocatorB->SetDataSet(inputB);
    pointLocatorB->BuildLocator();
  }
  else
  {
    cellLocatorA->SetDataSet(inputA);
    cellLocatorA->BuildLocator();
    cellLocatorB->SetDataSet(inputB);
    cellLocatorB->BuildLocator();
  }

  double dist;
  double currentPoint[3];
  double closestPoint[3];
  vtkIdType cellId;
  vtkSmartPointer<vtkGenericCell> cell = vtkSmartPointer<vtkGenericCell>::New();
  int subId;

  vtkSmartPointer<vtkDoubleArray> distanceAToB = vtkSmartPointer<vtkDoubleArray>::New();
  distanceAToB->SetNumberOfComponents(1);
  distanceAToB->SetNumberOfTuples(inputA->GetNumberOfPoints());
  distanceAToB->SetName("Distance");

  vtkSmartPointer<vtkDoubleArray> distanceBToA = vtkSmartPointer<vtkDoubleArray>::New();
  distanceBToA->SetNumberOfComponents(1);
  distanceBToA->SetNumberOfTuples(inputB->GetNumberOfPoints());
  distanceBToA->SetName("Distance");

  // Find the nearest neighbors to each point and add edges between them,
  // if they do not already exist and they are not self loops
  for (int i = 0; i < inputA->GetNumberOfPoints(); i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    inputA->GetPoint(i, currentPoint);
    if (this->TargetDistanceMethod == POINT_TO_POINT)
    {
      vtkIdType closestPointId = pointLocatorB->FindClosestPoint(currentPoint);
      inputB->GetPoint(closestPointId, closestPoint);
    }
    else
    {
      cellLocatorB->FindClosestPoint(currentPoint, closestPoint, cell, cellId, subId, dist);
    }

    dist = std::sqrt(std::pow(currentPoint[0] - closestPoint[0], 2) +
      std::pow(currentPoint[1] - closestPoint[1], 2) +
      std::pow(currentPoint[2] - closestPoint[2], 2));
    distanceAToB->SetValue(i, dist);

    if (dist > this->RelativeDistance[0])
    {
      this->RelativeDistance[0] = dist;
    }
  }

  for (int i = 0; i < inputB->GetNumberOfPoints(); i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    inputB->GetPoint(i, currentPoint);
    if (this->TargetDistanceMethod == POINT_TO_POINT)
    {
      vtkIdType closestPointId = pointLocatorA->FindClosestPoint(currentPoint);
      inputA->GetPoint(closestPointId, closestPoint);
    }
    else
    {
      cellLocatorA->FindClosestPoint(currentPoint, closestPoint, cell, cellId, subId, dist);
    }

    dist = std::sqrt(std::pow(currentPoint[0] - closestPoint[0], 2) +
      std::pow(currentPoint[1] - closestPoint[1], 2) +
      std::pow(currentPoint[2] - closestPoint[2], 2));
    distanceBToA->SetValue(i, dist);

    if (dist > this->RelativeDistance[1])
    {
      this->RelativeDistance[1] = dist;
    }
  }

  if (this->RelativeDistance[0] >= RelativeDistance[1])
  {
    this->HausdorffDistance = this->RelativeDistance[0];
  }
  else
  {
    this->HausdorffDistance = this->RelativeDistance[1];
  }

  vtkSmartPointer<vtkDoubleArray> relativeDistanceAtoB = vtkSmartPointer<vtkDoubleArray>::New();
  relativeDistanceAtoB->SetNumberOfComponents(1);
  relativeDistanceAtoB->SetName("RelativeDistanceAtoB");
  relativeDistanceAtoB->InsertNextValue(RelativeDistance[0]);

  vtkSmartPointer<vtkDoubleArray> relativeDistanceBtoA = vtkSmartPointer<vtkDoubleArray>::New();
  relativeDistanceBtoA->SetNumberOfComponents(1);
  relativeDistanceBtoA->SetName("RelativeDistanceBtoA");
  relativeDistanceBtoA->InsertNextValue(RelativeDistance[1]);

  vtkSmartPointer<vtkDoubleArray> hausdorffDistanceFieldDataA =
    vtkSmartPointer<vtkDoubleArray>::New();
  hausdorffDistanceFieldDataA->SetNumberOfComponents(1);
  hausdorffDistanceFieldDataA->SetName("HausdorffDistance");
  hausdorffDistanceFieldDataA->InsertNextValue(HausdorffDistance);

  vtkSmartPointer<vtkDoubleArray> hausdorffDistanceFieldDataB =
    vtkSmartPointer<vtkDoubleArray>::New();
  hausdorffDistanceFieldDataB->SetNumberOfComponents(1);
  hausdorffDistanceFieldDataB->SetName("HausdorffDistance");
  hausdorffDistanceFieldDataB->InsertNextValue(HausdorffDistance);

  outputA->DeepCopy(inputA);
  outputA->GetPointData()->AddArray(distanceAToB);
  outputA->GetFieldData()->AddArray(relativeDistanceAtoB);
  outputA->GetFieldData()->AddArray(hausdorffDistanceFieldDataA);

  outputB->DeepCopy(inputB);
  outputB->GetPointData()->AddArray(distanceBToA);
  outputB->GetFieldData()->AddArray(relativeDistanceBtoA);
  outputB->GetFieldData()->AddArray(hausdorffDistanceFieldDataB);

  return 1;
}

//------------------------------------------------------------------------------
int vtkHausdorffDistancePointSetFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  // The input should be two vtkPointsSets
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    return 1;
  }
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkHausdorffDistancePointSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HausdorffDistance: " << this->GetHausdorffDistance() << "\n";
  os << indent << "RelativeDistance: " << this->GetRelativeDistance()[0] << ", "
     << this->GetRelativeDistance()[1] << "\n";
  os << indent << "TargetDistanceMethod: " << this->GetTargetDistanceMethodAsString() << "\n";
}
VTK_ABI_NAMESPACE_END
