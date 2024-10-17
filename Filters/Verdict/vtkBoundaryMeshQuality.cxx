// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBoundaryMeshQuality.h"

#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPlane.h"
#include "vtkPolyDataNormals.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPTools.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGridBase.h"

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBoundaryMeshQuality);

//----------------------------------------------------------------------------
vtkBoundaryMeshQuality::vtkBoundaryMeshQuality() = default;

//----------------------------------------------------------------------------
vtkBoundaryMeshQuality::~vtkBoundaryMeshQuality() = default;

//----------------------------------------------------------------------------
void vtkBoundaryMeshQuality::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DistanceFromCellCenterToFaceCenter: "
     << (this->DistanceFromCellCenterToFaceCenter ? "On" : "Off") << endl;
  os << indent << "DistanceFromCellCenterToFacePlane: "
     << (this->DistanceFromCellCenterToFacePlane ? "On" : "Off") << endl;
  os << indent << "AngleFaceNormalAndCellCenterToFaceCenterVector: "
     << (this->AngleFaceNormalAndCellCenterToFaceCenterVector ? "On" : "Off") << endl;
}

//----------------------------------------------------------------------------
int vtkBoundaryMeshQuality::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGridBase");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkExplicitStructuredGrid");
  return 1;
}
//----------------------------------------------------------------------------
int vtkBoundaryMeshQuality::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataSet::GetData(inputVector[0]);
  auto output = vtkPolyData::GetData(outputVector);

  auto inputUG = vtkUnstructuredGridBase::SafeDownCast(input);
  auto inputSG = vtkStructuredGrid::SafeDownCast(input);
  auto inputRG = vtkRectilinearGrid::SafeDownCast(input);
  auto inputID = vtkImageData::SafeDownCast(input);

  if (inputUG)
  {
    // check if it has non 3d elements
    if (inputUG->GetMinSpatialDimension() < 3)
    {
      vtkErrorMacro("Input unstructured grid has non 3D cells.");
      return 1;
    }
  }
  if (inputSG && inputSG->GetDataDimension() != 3)
  {
    vtkErrorMacro("Input structured grid is not 3D.");
    return 1;
  }
  if (inputRG && inputRG->GetDataDimension() != 3)
  {
    vtkErrorMacro("Input rectilinear grid is not 3D.");
    return 1;
  }
  if (inputID && inputID->GetDataDimension() != 3)
  {
    vtkErrorMacro("Input image data is not 3D.");
    return 1;
  }

  if (!this->DistanceFromCellCenterToFaceCenter && !this->DistanceFromCellCenterToFacePlane &&
    !this->AngleFaceNormalAndCellCenterToFaceCenterVector)
  {
    vtkErrorMacro(
      "At least one of the DistanceFromCellCenterToFaceCenter, "
      "DistanceFromCellCenterToFacePlane, or AngleFaceNormalAndCellCenterToFaceCenterVector "
      "must be enabled.");
    return 0;
  }

  // extract the boundary cells
  vtkNew<vtkGeometryFilter> geometryFilter;
  geometryFilter->SetContainerAlgorithm(this);
  geometryFilter->SetInputData(input);
  geometryFilter->PassThroughCellIdsOn();
  geometryFilter->Update();
  output->ShallowCopy(geometryFilter->GetOutput());

  const auto originalCellIds = vtkIdTypeArray::FastDownCast(
    output->GetCellData()->GetArray(geometryFilter->GetOriginalCellIdsName()));
  if (!originalCellIds)
  {
    vtkErrorMacro("Failed to get original cell ids.");
    return 0;
  }
  this->UpdateProgress(0.4);

  // compute surface cell centers
  vtkNew<vtkCellCenters> surfaceCellCentersFilter;
  surfaceCellCentersFilter->SetContainerAlgorithm(this);
  surfaceCellCentersFilter->SetInputData(output);
  surfaceCellCentersFilter->CopyArraysOff();
  surfaceCellCentersFilter->VertexCellsOff();
  surfaceCellCentersFilter->Update();
  auto outputCellCenters =
    vtkDoubleArray::SafeDownCast(surfaceCellCentersFilter->GetOutput()->GetPoints()->GetData());
  if (!outputCellCenters)
  {
    vtkErrorMacro("Failed to get output cell centers.");
    return 0;
  }
  this->UpdateProgress(0.5);
  if (this->CheckAbort())
  {
    return 0;
  }

  // compute the volume cell centers
  auto copyInput = vtkSmartPointer<vtkDataSet>::Take(input->NewInstance());
  copyInput->ShallowCopy(input);

  vtkNew<vtkCellCenters> volumeCellCentersFilter;
  volumeCellCentersFilter->SetContainerAlgorithm(this);
  volumeCellCentersFilter->SetInputData(copyInput);
  volumeCellCentersFilter->CopyArraysOff();
  volumeCellCentersFilter->VertexCellsOff();
  volumeCellCentersFilter->Update();
  auto inputCellCenters =
    vtkDoubleArray::SafeDownCast(volumeCellCentersFilter->GetOutput()->GetPoints()->GetData());
  if (!inputCellCenters)
  {
    vtkErrorMacro("Failed to get input cell centers.");
    return 0;
  }
  this->UpdateProgress(0.6);
  if (this->CheckAbort())
  {
    return 0;
  }

  const auto numberOfOutputCells = output->GetNumberOfCells();
  // compute distance from cell center to face center
  if (this->DistanceFromCellCenterToFaceCenter)
  {
    vtkNew<vtkDoubleArray> distanceFromCellCenterToFaceCenterArray;
    distanceFromCellCenterToFaceCenterArray->SetName("DistanceFromCellCenterToFaceCenter");
    distanceFromCellCenterToFaceCenterArray->SetNumberOfValues(numberOfOutputCells);

    vtkSMPTools::For(0, numberOfOutputCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto distanceArray = vtk::DataArrayValueRange<1>(distanceFromCellCenterToFaceCenterArray);

        bool isFirst = vtkSMPTools::GetSingleThread();
        auto checkAbortInterval = std::min(numberOfOutputCells / 10 + 1, (vtkIdType)1000);
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          if (cellId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              this->CheckAbort();
            }
            if (this->GetAbortOutput())
            {
              break;
            }
          }
          const auto originalCellId = originalCellIds->GetValue(cellId);
          const auto faceCenter = outputCellCenters->GetPointer(3 * cellId);
          const auto cellCenter = inputCellCenters->GetPointer(3 * originalCellId);
          distanceArray[cellId] =
            std::sqrt(vtkMath::Distance2BetweenPoints(faceCenter, cellCenter));
        }
      });
    output->GetCellData()->AddArray(distanceFromCellCenterToFaceCenterArray);
  }
  this->UpdateProgress(0.7);
  if (this->CheckAbort())
  {
    return 0;
  }

  if (this->DistanceFromCellCenterToFacePlane ||
    this->AngleFaceNormalAndCellCenterToFaceCenterVector)
  {
    vtkNew<vtkPolyDataNormals> normalsFilter;
    normalsFilter->SetContainerAlgorithm(this);
    normalsFilter->SetInputData(output);
    normalsFilter->ComputePointNormalsOff();
    normalsFilter->ComputeCellNormalsOn();
    normalsFilter->AutoOrientNormalsOff();
    normalsFilter->ConsistencyOff();
    normalsFilter->SplittingOff();
    normalsFilter->Update();
    output->ShallowCopy(normalsFilter->GetOutput());
  }
  this->UpdateProgress(0.8);
  if (this->CheckAbort())
  {
    return 0;
  }

  if (this->DistanceFromCellCenterToFacePlane)
  {
    const auto outputNormals = vtkFloatArray::FastDownCast(output->GetCellData()->GetNormals());
    if (!outputNormals)
    {
      vtkErrorMacro("Failed to get output normals.");
      return 0;
    }
    vtkNew<vtkDoubleArray> distanceFromCellCenterToFacePlaneArray;
    distanceFromCellCenterToFacePlaneArray->SetName("DistanceFromCellCenterToFacePlane");
    distanceFromCellCenterToFacePlaneArray->SetNumberOfValues(numberOfOutputCells);

    vtkSMPTools::For(0, numberOfOutputCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto distanceArray = vtk::DataArrayValueRange<1>(distanceFromCellCenterToFacePlaneArray);
        double faceNormal[3];

        bool isFirst = vtkSMPTools::GetSingleThread();
        auto checkAbortInterval = std::min(numberOfOutputCells / 10 + 1, (vtkIdType)1000);
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          if (cellId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              this->CheckAbort();
            }
            if (this->GetAbortOutput())
            {
              break;
            }
          }
          const auto originalCellId = originalCellIds->GetValue(cellId);
          outputNormals->GetTuple(cellId, faceNormal);
          const auto faceCenter = outputCellCenters->GetPointer(3 * cellId);
          const auto cellCenter = inputCellCenters->GetPointer(3 * originalCellId);
          distanceArray[cellId] = vtkPlane::DistanceToPlane(faceCenter, faceNormal, cellCenter);
        }
      });
    output->GetCellData()->AddArray(distanceFromCellCenterToFacePlaneArray);
  }
  this->UpdateProgress(0.9);
  if (this->CheckAbort())
  {
    return 0;
  }

  if (this->AngleFaceNormalAndCellCenterToFaceCenterVector)
  {
    const auto outputNormals = vtkFloatArray::FastDownCast(output->GetCellData()->GetNormals());
    if (!outputNormals)
    {
      vtkErrorMacro("Failed to get output normals.");
      return 0;
    }
    vtkNew<vtkDoubleArray> angleFaceNormalAndCellCenterToFaceCenterVectorArray;
    angleFaceNormalAndCellCenterToFaceCenterVectorArray->SetName(
      "AngleFaceNormalAndCellCenterToFaceCenterVector");
    angleFaceNormalAndCellCenterToFaceCenterVectorArray->SetNumberOfValues(numberOfOutputCells);

    vtkSMPTools::For(0, numberOfOutputCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto angleArray =
          vtk::DataArrayValueRange<1>(angleFaceNormalAndCellCenterToFaceCenterVectorArray);
        double cellCenterToFaceCenterVector[3];
        double normal[3];

        bool isFirst = vtkSMPTools::GetSingleThread();
        auto checkAbortInterval = std::min(numberOfOutputCells / 10 + 1, (vtkIdType)1000);
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          if (cellId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              this->CheckAbort();
            }
            if (this->GetAbortOutput())
            {
              break;
            }
          }
          const auto originalCellId = originalCellIds->GetValue(cellId);
          const auto faceCenter = outputCellCenters->GetPointer(3 * cellId);
          const auto cellCenter = inputCellCenters->GetPointer(3 * originalCellId);
          outputNormals->GetTuple(cellId, normal);
          // compute normal from cell center to face center
          vtkMath::Subtract(faceCenter, cellCenter, cellCenterToFaceCenterVector);
          vtkMath::Normalize(cellCenterToFaceCenterVector);
          angleArray[cellId] = vtkMath::DegreesFromRadians(
            vtkMath::AngleBetweenVectors(normal, cellCenterToFaceCenterVector));
        }
      });
    output->GetCellData()->AddArray(angleFaceNormalAndCellCenterToFaceCenterVectorArray);
  }
  this->UpdateProgress(1.0);

  return 1;
}

VTK_ABI_NAMESPACE_END
