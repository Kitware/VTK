// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkYieldCriteria.h"

#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTensorPrincipalInvariants.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkYieldCriteria);

namespace
{
const std::map<vtkYieldCriteria::Criterion, std::string> Names = {
  { vtkYieldCriteria::Criterion::PrincipalStress, "Principal Stress" },
  { vtkYieldCriteria::Criterion::Tresca, "Tresca Criterion" },
  { vtkYieldCriteria::Criterion::VonMises, "Von Mises Criterion" }
};
}

//------------------------------------------------------------------------------
vtkYieldCriteria::vtkYieldCriteria()
{
  this->PointDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkYieldCriteria::Modified);
  this->CellDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkYieldCriteria::Modified);
  this->CriteriaSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkYieldCriteria::Modified);
}

//------------------------------------------------------------------------------
int vtkYieldCriteria::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Call RequestInformation in the principal invariants filter to fill the
  // selection list of available point and cell arrays
  // (arrays with 3 components named "XX", "YY", "XY" or with 6 components)
  if (this->InvariantsFilter->ProcessRequest(request, inputVector, outputVector) == 0)
  {
    return 0;
  }

  vtkDataArraySelection* pointArraySelection = this->InvariantsFilter->GetPointDataArraySelection();
  vtkDataArraySelection* cellArraySelection = this->InvariantsFilter->GetCellDataArraySelection();

  for (vtkIdType idx = 0; idx < pointArraySelection->GetNumberOfArrays(); idx++)
  {
    this->PointDataArraySelection->AddArray(pointArraySelection->GetArrayName(idx));
  }

  for (vtkIdType idx = 0; idx < cellArraySelection->GetNumberOfArrays(); idx++)
  {
    this->CellDataArraySelection->AddArray(cellArraySelection->GetArrayName(idx));
  }

  // Fill selection list of yield criteria
  this->CriteriaSelection->AddArray(Names.at(Criterion::PrincipalStress).c_str());
  this->CriteriaSelection->AddArray(Names.at(Criterion::Tresca).c_str());
  this->CriteriaSelection->AddArray(Names.at(Criterion::VonMises).c_str());

  return 1;
}

//------------------------------------------------------------------------------
int vtkYieldCriteria::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  if (!input || !output)
  {
    vtkErrorMacro("Could not retrieve input or output.");
    return 0;
  }

  if (this->CriteriaSelection->GetNumberOfArraysEnabled() == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Compute principal values and vectors
  this->InvariantsFilter->SetScaleVectors(this->ScaleVectors);
  this->InvariantsFilter->GetPointDataArraySelection()->CopySelections(
    this->PointDataArraySelection);
  this->InvariantsFilter->GetCellDataArraySelection()->CopySelections(this->CellDataArraySelection);

  if (this->InvariantsFilter->ProcessRequest(request, inputVector, outputVector) == 0)
  {
    return 0;
  }

  // Retrieve output that contains principal invariants
  output = vtkDataSet::GetData(outputVector);

  if (!output)
  {
    vtkErrorMacro("Could not retrieve output.");
    return 0;
  }

  vtkPointData* pointData = input->GetPointData();
  vtkCellData* cellData = input->GetCellData();
  vtkIdType nbPoints = input->GetNumberOfPoints();
  vtkIdType nbCells = input->GetNumberOfCells();

  // Compute yield criteria for selected point arrays
  for (vtkIdType idx = 0; idx < this->PointDataArraySelection->GetNumberOfArrays(); idx++)
  {
    if (this->PointDataArraySelection->GetArraySetting(idx) == 0)
    {
      continue;
    }

    // Retrieve array from name
    std::string arrayName = this->PointDataArraySelection->GetArrayName(idx);
    vtkDataArray* array = pointData->GetArray(arrayName.c_str());

    if (!array)
    {
      vtkWarningMacro("Could not retrieve point array '" << arrayName << "', skipping.");
      continue;
    }

    // Compute derived yield criteria data arrays
    if (!this->ComputeYieldCriteria(output, array, arrayName, nbPoints, true))
    {
      vtkWarningMacro(
        "Could not compute yield criteria for point array '" << arrayName << "', skipping.");
      continue;
    }
  }

  // Compute yield criteria for selected cell arrays
  for (vtkIdType idx = 0; idx < this->CellDataArraySelection->GetNumberOfArrays(); idx++)
  {
    if (this->CellDataArraySelection->GetArraySetting(idx) == 0)
    {
      continue;
    }

    // Retrieve array from name
    std::string arrayName = this->CellDataArraySelection->GetArrayName(idx);
    vtkDataArray* array = cellData->GetArray(arrayName.c_str());

    if (!array)
    {
      vtkWarningMacro("Could not retrieve cell array '" << arrayName << "', skipping.");
      continue;
    }

    // Compute derived yield criteria data arrays
    if (!this->ComputeYieldCriteria(output, array, arrayName, nbCells, false))
    {
      vtkWarningMacro(
        "Could not compute yield criteria for cell array '" << arrayName << "', skipping.");
      continue;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkYieldCriteria::ComputeYieldCriteria(vtkDataSet* output, vtkDataArray* array,
  const std::string& arrayName, vtkIdType nbTuples, bool isPointData) const
{
  // Check number of components for input array
  vtkIdType nbComp = array->GetNumberOfComponents();

  if (nbComp != 3 && nbComp != 6)
  {
    vtkWarningMacro("Array '" << arrayName << "' does not have 3 or 6 components, skipping.");
    return false;
  }

  bool keepStress =
    this->CriteriaSelection->ArrayIsEnabled(Names.at(Criterion::PrincipalStress).c_str());
  bool computeTresca = this->CriteriaSelection->ArrayIsEnabled(Names.at(Criterion::Tresca).c_str());
  bool computeVonMises =
    this->CriteriaSelection->ArrayIsEnabled(Names.at(Criterion::VonMises).c_str());

  // Retrieve principal values
  vtkDataSetAttributes* attributes = isPointData
    ? vtkDataSetAttributes::SafeDownCast(output->GetPointData())
    : vtkDataSetAttributes::SafeDownCast(output->GetCellData());

  vtkDoubleArray* sigma1 = vtkDoubleArray::SafeDownCast(attributes->GetArray(
    vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 1).c_str()));
  vtkDoubleArray* sigma2 = vtkDoubleArray::SafeDownCast(attributes->GetArray(
    vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 2).c_str()));
  vtkDoubleArray* sigma3 = vtkDoubleArray::SafeDownCast(attributes->GetArray(
    vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 3).c_str()));

  if (!sigma1 || !sigma2 || !sigma3)
  {
    vtkWarningMacro(
      "Could not retrieve principal values for array '" << arrayName << "', skipping.");
    return false;
  }

  // Create derived data arrays
  vtkNew<vtkDoubleArray> tresca;
  vtkNew<vtkDoubleArray> vonMises;

  tresca->SetName((arrayName + " - Tresca Criterion").c_str());
  vonMises->SetName((arrayName + " - Von Mises Criterion").c_str());

  if (computeTresca)
  {
    tresca->SetNumberOfTuples(nbTuples);
  }

  if (computeVonMises)
  {
    vonMises->SetNumberOfTuples(nbTuples);
  }

  for (vtkIdType idx = 0; idx < nbTuples; idx++)
  {
    // Check for NaN values
    if (std::isnan(array->GetComponent(idx, 0)))
    {
      if (computeTresca)
      {
        tresca->SetValue(idx, std::nan(""));
      }

      if (computeVonMises)
      {
        vonMises->SetValue(idx, std::nan(""));
      }

      continue;
    }

    // Fill criteria data arrays
    // Tresca criterion : |sigma3 - sigma1|
    // Von Mises criterion:
    // sqrt( (sigma1 - sigma2)^2 + (sigma2 - sigma3)^2 + (sigma1 - sigma3)^2 ) / sqrt(2)
    double s1 = sigma1->GetValue(idx);
    double s2 = sigma2->GetValue(idx);
    double s3 = sigma3->GetValue(idx);

    if (computeTresca)
    {
      tresca->SetValue(idx, std::abs(s3 - s1));
    }

    if (computeVonMises)
    {
      double vonMisesValue =
        std::sqrt((s1 - s2) * (s1 - s2) + (s2 - s3) * (s2 - s3) + (s1 - s3) * (s1 - s3)) /
        std::sqrt(2.0);
      vonMises->SetValue(idx, vonMisesValue);
    }
  }

  // Add arrays to output
  if (!keepStress)
  {
    attributes->RemoveArray(
      vtkTensorPrincipalInvariants::GetSigmaVectorArrayName(arrayName, 1).c_str());
    attributes->RemoveArray(
      vtkTensorPrincipalInvariants::GetSigmaVectorArrayName(arrayName, 2).c_str());
    attributes->RemoveArray(
      vtkTensorPrincipalInvariants::GetSigmaVectorArrayName(arrayName, 3).c_str());
    attributes->RemoveArray(
      vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 1).c_str());
    attributes->RemoveArray(
      vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 2).c_str());
    attributes->RemoveArray(
      vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 3).c_str());
  }

  if (computeTresca)
  {
    attributes->AddArray(tresca);
  }

  if (computeVonMises)
  {
    attributes->AddArray(vonMises);
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkYieldCriteria::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleVectors: " << this->ScaleVectors << endl;
}
VTK_ABI_NAMESPACE_END
