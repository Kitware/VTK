// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDistancePolyDataFilter.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkImplicitPolyDataDistance.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"

// The 3D cell with the maximum number of points is VTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define VTK_MAXIMUM_NUMBER_OF_POINTS 216

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDistancePolyDataFilter);

//------------------------------------------------------------------------------
vtkDistancePolyDataFilter::vtkDistancePolyDataFilter()
{
  this->SignedDistance = 1;
  this->NegateDistance = 0;
  this->ComputeSecondDistance = 1;
  this->ComputeCellCenterDistance = 1;
  this->ComputeDirection = 0;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
}

//------------------------------------------------------------------------------
vtkDistancePolyDataFilter::~vtkDistancePolyDataFilter() = default;

//------------------------------------------------------------------------------
int vtkDistancePolyDataFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPolyData* input0 = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData* input1 = vtkPolyData::GetData(inputVector[1], 0);
  vtkPolyData* output0 = vtkPolyData::GetData(outputVector, 0);
  vtkPolyData* output1 = vtkPolyData::GetData(outputVector, 1);

  output0->CopyStructure(input0);
  output0->GetPointData()->PassData(input0->GetPointData());
  output0->GetCellData()->PassData(input0->GetCellData());
  output0->BuildCells();
  this->GetPolyDataDistance(output0, input1);

  if (this->ComputeSecondDistance)
  {
    output1->CopyStructure(input1);
    output1->GetPointData()->PassData(input1->GetPointData());
    output1->GetCellData()->PassData(input1->GetCellData());
    output1->BuildCells();
    this->GetPolyDataDistance(output1, input0);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkDistancePolyDataFilter::GetPolyDataDistance(vtkPolyData* mesh, vtkPolyData* src)
{
  vtkDebugMacro(<< "Start vtkDistancePolyDataFilter::GetPolyDataDistance");

  if (mesh->GetNumberOfCells() == 0 || mesh->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro(<< "No points/cells to operate on");
    return;
  }

  if (src->GetNumberOfPolys() == 0 || src->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro(<< "No points/cells to difference from");
    return;
  }

  vtkNew<vtkImplicitPolyDataDistance> imp;
  imp->SetInput(src);

  // Calculate distance from points.
  const vtkIdType numPts = mesh->GetNumberOfPoints();

  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetName("Distance");
  pointArray->SetNumberOfComponents(1);
  pointArray->SetNumberOfTuples(numPts);

  vtkNew<vtkDoubleArray> directionArray;
  if (this->ComputeDirection)
  {
    directionArray->SetName("Direction");
    directionArray->SetNumberOfComponents(3);
    directionArray->SetNumberOfTuples(numPts);
  }

  auto DistanceWithSign = [&](const double& val)
  { return this->SignedDistance ? (this->NegateDistance ? -val : val) : std::abs(val); };

  vtkSMPTools::For(0, numPts,
    [&](vtkIdType begin, vtkIdType end)
    {
      double pt[3];
      for (vtkIdType ptId = begin; ptId < end; ptId++)
      {
        mesh->GetPoint(ptId, pt);
        if (this->ComputeDirection)
        {
          double closestPoint[3];
          double direction[3];
          double val = imp->EvaluateFunctionAndGetClosestPoint(pt, closestPoint);
          double dist = DistanceWithSign(val);
          vtkMath::Subtract(closestPoint, pt, direction);
          vtkMath::Normalize(direction);
          pointArray->SetValue(ptId, dist);
          directionArray->SetTuple(ptId, direction);
        }
        else
        {
          double val = imp->EvaluateFunction(pt);
          double dist = DistanceWithSign(val);
          pointArray->SetValue(ptId, dist);
        }
      }
    });

  mesh->GetPointData()->AddArray(pointArray);
  mesh->GetPointData()->SetActiveScalars("Distance");

  if (this->ComputeDirection)
  {
    mesh->GetPointData()->AddArray(directionArray);
    mesh->GetPointData()->SetActiveVectors("Direction");
  }

  // Calculate distance from cell centers.
  if (this->ComputeCellCenterDistance)
  {
    const vtkIdType numCells = mesh->GetNumberOfCells();

    vtkNew<vtkDoubleArray> cellArray;
    cellArray->SetName("Distance");
    cellArray->SetNumberOfComponents(1);
    cellArray->SetNumberOfTuples(numCells);

    vtkNew<vtkDoubleArray> cellDirectionArray;
    cellDirectionArray->SetName("Direction");
    if (this->ComputeDirection)
    {
      cellDirectionArray->SetNumberOfComponents(3);
      cellDirectionArray->SetNumberOfTuples(numCells);
    }
    vtkSMPThreadLocalObject<vtkGenericCell> TLCell;
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto cell = TLCell.Local();
        int subId;
        double pcoords[3], x[3], weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
        for (vtkIdType cellId = begin; cellId < end; cellId++)
        {
          mesh->GetCell(cellId, cell);
          cell->GetParametricCenter(pcoords);
          cell->EvaluateLocation(subId, pcoords, x, weights);
          if (this->ComputeDirection)
          {
            double closestPoint[3];
            double direction[3];
            double val = imp->EvaluateFunctionAndGetClosestPoint(x, closestPoint);
            double dist = DistanceWithSign(val);
            vtkMath::Subtract(closestPoint, x, direction);
            vtkMath::Normalize(direction);
            cellArray->SetValue(cellId, dist);
            cellDirectionArray->SetTuple(cellId, direction);
          }
          else
          {
            double val = imp->EvaluateFunction(x);
            double dist = DistanceWithSign(val);
            cellArray->SetValue(cellId, dist);
          }
        }
      });

    mesh->GetCellData()->AddArray(cellArray);
    mesh->GetCellData()->SetActiveScalars("Distance");

    if (this->ComputeDirection)
    {
      mesh->GetCellData()->AddArray(cellDirectionArray);
      mesh->GetCellData()->SetActiveVectors("Direction");
    }
  }

  vtkDebugMacro(<< "End vtkDistancePolyDataFilter::GetPolyDataDistance");
}

//------------------------------------------------------------------------------
vtkPolyData* vtkDistancePolyDataFilter::GetSecondDistanceOutput()
{
  if (!this->ComputeSecondDistance)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(1));
}

//------------------------------------------------------------------------------
void vtkDistancePolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SignedDistance: " << this->SignedDistance << "\n";
  os << indent << "NegateDistance: " << this->NegateDistance << "\n";
  os << indent << "ComputeSecondDistance: " << this->ComputeSecondDistance << "\n";
  os << indent << "ComputeCellCenterDistance: " << this->ComputeCellCenterDistance << "\n";
  os << indent << "ComputeDirection: " << this->ComputeDirection << "\n";
}
VTK_ABI_NAMESPACE_END
