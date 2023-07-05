// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTensorPrincipalInvariants.h"

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

#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTensorPrincipalInvariants);

//------------------------------------------------------------------------------
vtkTensorPrincipalInvariants::vtkTensorPrincipalInvariants()
{
  this->PointDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkTensorPrincipalInvariants::Modified);
  this->CellDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkTensorPrincipalInvariants::Modified);
}

//------------------------------------------------------------------------------
int vtkTensorPrincipalInvariants::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);

  if (!input)
  {
    vtkErrorMacro("Missing input.");
    return 0;
  }

  // Add all arrays with 3 components named "XX", "YY", "XY" or with 6 components
  // in the selections
  vtkPointData* pointData = input->GetPointData();

  if (!pointData)
  {
    vtkErrorMacro("Missing point data from input.");
    return 0;
  }

  for (vtkIdType idx = 0; idx < pointData->GetNumberOfArrays(); idx++)
  {
    vtkDataArray* array = pointData->GetArray(idx);

    if (array && this->IsSymmetricTensor(array))
    {
      this->PointDataArraySelection->AddArray(array->GetName());
    }
  }

  vtkCellData* cellData = input->GetCellData();

  if (!cellData)
  {
    vtkErrorMacro("Missing cell data from input.");
    return 0;
  }

  for (vtkIdType idx = 0; idx < cellData->GetNumberOfArrays(); idx++)
  {
    vtkDataArray* array = cellData->GetArray(idx);

    if (array && this->IsSymmetricTensor(array))
    {
      this->CellDataArraySelection->AddArray(array->GetName());
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkTensorPrincipalInvariants::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  if (!output)
  {
    vtkErrorMacro("Missing output.");
    return 0;
  }

  if (this->PointDataArraySelection->GetNumberOfArraysEnabled() == 0 &&
    this->CellDataArraySelection->GetNumberOfArraysEnabled() == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Principal invariants arrays will be added to the input dataset
  output->DeepCopy(input);

  vtkPointData* pointData = input->GetPointData();
  vtkCellData* cellData = input->GetCellData();
  vtkIdType nbPoints = input->GetNumberOfPoints();
  vtkIdType nbCells = input->GetNumberOfCells();

  // Compute principal invariants for selected point arrays
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

    // Compute principal invariants data arrays
    if (!this->ComputePrincipalInvariants(output, array, arrayName, nbPoints, true))
    {
      vtkWarningMacro(
        "Could not compute principal invariants for point array '" << arrayName << "', skipping.");
      continue;
    }
  }

  // Compute principal invariants for selected cell arrays
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

    // Compute principal invariants data arrays
    if (!this->ComputePrincipalInvariants(output, array, arrayName, nbCells, false))
    {
      vtkWarningMacro(
        "Could not compute principal invariants for cell array '" << arrayName << "', skipping.");
      continue;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkTensorPrincipalInvariants::IsSymmetricTensor(vtkDataArray* array) const
{
  // 3D symmetric tensor
  if (array->GetNumberOfComponents() == 6)
  {
    return true;
  }

  // 2D symmetric tensor
  if (array->GetNumberOfComponents() != 3)
  {
    return false;
  }

  // Check component names to differentiate from vectors
  if (!array->GetComponentName(0) || !array->GetComponentName(1) || !array->GetComponentName(2))
  {
    return false;
  }

  return (std::strcmp(array->GetComponentName(0), "XX") == 0 &&
    std::strcmp(array->GetComponentName(1), "YY") == 0 &&
    std::strcmp(array->GetComponentName(2), "XY") == 0);
}

//------------------------------------------------------------------------------
bool vtkTensorPrincipalInvariants::ComputePrincipalInvariants(vtkDataSet* output,
  vtkDataArray* array, const std::string& arrayName, vtkIdType nbTuples, bool isPointData) const
{
  // Check number of components for input array
  vtkIdType nbComp = array->GetNumberOfComponents();

  if (nbComp != 3 && nbComp != 6)
  {
    vtkWarningMacro("Array '" << arrayName << "' does not have 3 or 6 components, skipping.");
    return false;
  }

  // Create data arrays
  vtkNew<vtkDoubleArray> sigma1Vector;
  vtkNew<vtkDoubleArray> sigma2Vector;
  vtkNew<vtkDoubleArray> sigma3Vector;
  vtkNew<vtkDoubleArray> sigma1Value;
  vtkNew<vtkDoubleArray> sigma2Value;
  vtkNew<vtkDoubleArray> sigma3Value;

  sigma1Vector->SetName(
    vtkTensorPrincipalInvariants::GetSigmaVectorArrayName(arrayName, 1).c_str());
  sigma2Vector->SetName(
    vtkTensorPrincipalInvariants::GetSigmaVectorArrayName(arrayName, 2).c_str());
  sigma3Vector->SetName(
    vtkTensorPrincipalInvariants::GetSigmaVectorArrayName(arrayName, 3).c_str());
  sigma1Value->SetName(vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 1).c_str());
  sigma2Value->SetName(vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 2).c_str());
  sigma3Value->SetName(vtkTensorPrincipalInvariants::GetSigmaValueArrayName(arrayName, 3).c_str());

  sigma1Vector->SetNumberOfComponents(3);
  sigma2Vector->SetNumberOfComponents(3);
  sigma3Vector->SetNumberOfComponents(3);
  sigma1Vector->SetNumberOfTuples(nbTuples);
  sigma2Vector->SetNumberOfTuples(nbTuples);
  sigma3Vector->SetNumberOfTuples(nbTuples);
  sigma1Value->SetNumberOfTuples(nbTuples);
  sigma2Value->SetNumberOfTuples(nbTuples);
  sigma3Value->SetNumberOfTuples(nbTuples);

  for (vtkIdType idx = 0; idx < nbTuples; idx++)
  {
    double tensor[3][3] = { { 0.0 } };
    double eigenvals[3] = { 0.0 };
    double eigenvecs[3][3] = { { 0.0 } };

    // Check for NaN values
    if (std::isnan(array->GetComponent(idx, 0)))
    {
      sigma1Vector->SetTuple3(idx, std::nan(""), std::nan(""), std::nan(""));
      sigma2Vector->SetTuple3(idx, std::nan(""), std::nan(""), std::nan(""));
      sigma3Vector->SetTuple3(idx, std::nan(""), std::nan(""), std::nan(""));
      sigma1Value->SetValue(idx, std::nan(""));
      sigma2Value->SetValue(idx, std::nan(""));
      sigma3Value->SetValue(idx, std::nan(""));

      continue;
    }

    // Fill symmetric tensor according to number of components:
    // 3 components (2D) -> XX, YY, XY
    // 6 components (3D) -> XX, YY, ZZ, XY, YZ, XZ
    if (nbComp == 3)
    {
      tensor[0][0] = array->GetComponent(idx, 0);
      tensor[1][1] = array->GetComponent(idx, 1);
      tensor[0][1] = array->GetComponent(idx, 2);
      tensor[1][0] = array->GetComponent(idx, 2);
    }
    else
    {
      tensor[0][0] = array->GetComponent(idx, 0);
      tensor[1][1] = array->GetComponent(idx, 1);
      tensor[2][2] = array->GetComponent(idx, 2);
      tensor[0][1] = array->GetComponent(idx, 3);
      tensor[1][0] = array->GetComponent(idx, 3);
      tensor[1][2] = array->GetComponent(idx, 4);
      tensor[2][1] = array->GetComponent(idx, 4);
      tensor[0][2] = array->GetComponent(idx, 5);
      tensor[2][0] = array->GetComponent(idx, 5);
    }

    // Diagonalize
    vtkMath::Diagonalize3x3(tensor, eigenvals, eigenvecs);

    // Determine eigenvalues ordering from largest to smallest
    std::array<int, 3> order = this->GetDecreasingOrder(eigenvals);

    // Fill data arrays with principal values and vectors.
    // The values are ordered from largest to smallest.
    // SigmaN value: Nth principal eigenvalue
    // SigmaN vector: Nth principal eigenvector (scaled if needed)
    double s1 = ScaleVectors ? eigenvals[order[0]] : 1.0;
    double s2 = ScaleVectors ? eigenvals[order[1]] : 1.0;
    double s3 = ScaleVectors ? eigenvals[order[2]] : 1.0;

    sigma1Vector->SetTuple3(
      idx, s1 * eigenvecs[order[0]][0], s1 * eigenvecs[order[0]][1], s1 * eigenvecs[order[0]][2]);
    sigma2Vector->SetTuple3(
      idx, s2 * eigenvecs[order[1]][0], s2 * eigenvecs[order[1]][1], s2 * eigenvecs[order[1]][2]);
    sigma3Vector->SetTuple3(
      idx, s3 * eigenvecs[order[2]][0], s3 * eigenvecs[order[2]][1], s3 * eigenvecs[order[2]][2]);
    sigma1Value->SetValue(idx, eigenvals[order[0]]);
    sigma2Value->SetValue(idx, eigenvals[order[1]]);
    sigma3Value->SetValue(idx, eigenvals[order[2]]);
  }

  // Add arrays to output
  vtkDataSetAttributes* attributes = isPointData
    ? vtkDataSetAttributes::SafeDownCast(output->GetPointData())
    : vtkDataSetAttributes::SafeDownCast(output->GetCellData());

  attributes->AddArray(sigma1Vector);
  attributes->AddArray(sigma2Vector);
  attributes->AddArray(sigma3Vector);
  attributes->AddArray(sigma1Value);
  attributes->AddArray(sigma2Value);
  attributes->AddArray(sigma3Value);

  return true;
}

//------------------------------------------------------------------------------
std::array<int, 3> vtkTensorPrincipalInvariants::GetDecreasingOrder(double values[3]) const
{
  std::array<int, 3> order;

  if (values[0] > values[1])
  {
    if (values[1] > values[2])
    {
      order = { 0, 1, 2 };
    }
    else
    {
      if (values[0] > values[2])
      {
        order = { 0, 2, 1 };
      }
      else
      {
        order = { 2, 0, 1 };
      }
    }
  }
  else
  {
    if (values[0] > values[2])
    {
      order = { 1, 0, 2 };
    }
    else
    {
      if (values[1] > values[2])
      {
        order = { 1, 2, 0 };
      }
      else
      {
        order = { 2, 1, 0 };
      }
    }
  }

  return order;
}

//------------------------------------------------------------------------------
std::string vtkTensorPrincipalInvariants::GetSigmaValueArrayName(
  const std::string& baseName, int index)
{
  return baseName + " - Sigma " + std::to_string(index);
}

//------------------------------------------------------------------------------
std::string vtkTensorPrincipalInvariants::GetSigmaVectorArrayName(
  const std::string& baseName, int index)
{
  return baseName + " - Sigma " + std::to_string(index) + " (Vector)";
}

//------------------------------------------------------------------------------
void vtkTensorPrincipalInvariants::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "ScaleVectors: " << this->ScaleVectors << endl;
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
