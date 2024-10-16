// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGenerateRegionIds.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"

#include <vector>

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkGenerateRegionIds);

//-----------------------------------------------------------------------------
void vtkGenerateRegionIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Max Angle: " << this->MaxAngle << "\n";
  os << indent << "Region Ids Array Name: " << this->RegionIdsArrayName << "\n";
}

//-----------------------------------------------------------------------------
int vtkGenerateRegionIds::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* inputPolyData = vtkPolyData::GetData(inInfo);
  vtkPolyData* outputPolyData = vtkPolyData::GetData(outInfo);

  if (!inputPolyData || !outputPolyData)
  {
    vtkErrorMacro("invalid data");
    return 0;
  }

  vtkIdTypeArray* regionIds = this->InitializeOutput(inputPolyData, outputPolyData);
  vtkIdType currentRegionId = -1;

  vtkDataArray* normals = outputPolyData->GetCellData()->GetNormals();
  const double cosMaxAngle = std::cos(vtkMath::RadiansFromDegrees(this->MaxAngle));
  const vtkIdType numberOfCells = outputPolyData->GetNumberOfCells();

  for (vtkIdType currentCellIndex = 0; currentCellIndex < numberOfCells; currentCellIndex++)
  {
    if (regionIds->GetValue(currentCellIndex) != -1)
    {
      // skip already assigned cell
      continue;
    }

    currentRegionId++;
    regionIds->SetValue(currentCellIndex, currentRegionId);

    std::vector<vtkIdType> candidates;
    candidates.push_back(currentCellIndex);

    while (!candidates.empty())
    {
      const vtkIdType currentCandidateIndex = candidates.back();
      candidates.pop_back();

      std::set<vtkIdType> cellNeighbors =
        this->GetCellNeighbors(outputPolyData, currentCandidateIndex);

      for (vtkIdType neighborId : cellNeighbors)
      {
        if (regionIds->GetValue(neighborId) != -1)
        {
          // skip already assigned cell
          continue;
        }

        if (this->SameRegion(normals, cosMaxAngle, neighborId, currentCandidateIndex))
        {
          regionIds->SetValue(neighborId, currentRegionId);
          candidates.push_back(neighborId);
        }
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkGenerateRegionIds::SameRegion(
  vtkDataArray* normals, double threshold, vtkIdType first, vtkIdType second)
{
  double firstNormal[3];
  normals->GetTuple(first, firstNormal);

  double secondNormal[3];
  normals->GetTuple(second, secondNormal);

  return vtkMath::Dot(firstNormal, secondNormal) > threshold;
}

//-----------------------------------------------------------------------------
vtkIdTypeArray* vtkGenerateRegionIds::InitializeOutput(
  vtkPolyData* inputPolyData, vtkPolyData* outputPolyData)
{
  // prepare data
  vtkNew<vtkPolyData> cleanInput;
  if (!outputPolyData->GetCellData()->GetNormals())
  {
    vtkNew<vtkPolyDataNormals> generateNormals;
    generateNormals->ComputeCellNormalsOn();
    generateNormals->SetInputData(inputPolyData);
    generateNormals->Update();
    cleanInput->ShallowCopy(generateNormals->GetOutput());
  }
  else
  {
    cleanInput->ShallowCopy(inputPolyData);
  }
  outputPolyData->ShallowCopy(cleanInput);

  vtkNew<vtkIdTypeArray> regionIds;
  regionIds->SetName(this->RegionIdsArrayName.c_str());
  vtkIdType numberOfCells = outputPolyData->GetNumberOfCells();
  regionIds->SetNumberOfTuples(numberOfCells);
  regionIds->Fill(-1);
  outputPolyData->GetCellData()->AddArray(regionIds);

  return regionIds;
}

//-----------------------------------------------------------------------------
std::set<vtkIdType> vtkGenerateRegionIds::GetCellNeighbors(vtkPolyData* polydata, vtkIdType cellId)
{
  std::set<vtkIdType> cellNeighbors;
  vtkNew<vtkIdList> cellPoints;
  polydata->GetCellPoints(cellId, cellPoints);
  for (vtkIdType pointId = 0; pointId < cellPoints->GetNumberOfIds(); pointId++)
  {
    vtkNew<vtkIdList> cells;
    polydata->GetPointCells(cellPoints->GetId(pointId), cells);
    for (vtkIdType pointCellId = 0; pointCellId < cells->GetNumberOfIds(); pointCellId++)
    {
      cellNeighbors.emplace(cells->GetId(pointCellId));
    }
  }

  return cellNeighbors;
}

VTK_ABI_NAMESPACE_END
