// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov

#include "vtkNormalizeMatrixVectors.h"
#include "vtkArrayCoordinates.h"
#include "vtkArrayData.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTypedArray.h"

#include <algorithm>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////
// vtkNormalizeMatrixVectors

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkNormalizeMatrixVectors);

vtkNormalizeMatrixVectors::vtkNormalizeMatrixVectors()
  : VectorDimension(1)
  , PValue(2)
{
}

vtkNormalizeMatrixVectors::~vtkNormalizeMatrixVectors() = default;

void vtkNormalizeMatrixVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VectorDimension: " << this->VectorDimension << endl;
  os << indent << "PValue: " << this->PValue << endl;
}

int vtkNormalizeMatrixVectors::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int vector_dimension = std::min(1, std::max(0, this->VectorDimension));
  double p_value = std::max(1.0, this->PValue);

  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if (input->GetNumberOfArrays() != 1)
  {
    vtkErrorMacro(
      << "vtkNormalizeMatrixVectors requires vtkArrayData containing exactly one array as input.");
    return 0;
  }

  vtkTypedArray<double>* const input_array =
    vtkTypedArray<double>::SafeDownCast(input->GetArray(static_cast<vtkIdType>(0)));
  if (!input_array)
  {
    vtkErrorMacro(<< "vtkNormalizeMatrixVectors requires a vtkTypedArray<double> as input.");
    return 0;
  }
  if (input_array->GetDimensions() != 2)
  {
    vtkErrorMacro(<< "vtkNormalizeMatrixVectors requires a matrix as input.");
    return 0;
  }

  vtkTypedArray<double>* const output_array =
    vtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());

  const vtkArrayRange vectors = input_array->GetExtent(vector_dimension);
  const vtkIdType value_count = input_array->GetNonNullSize();

  // Create temporary storage for computed vector weights ...
  std::vector<double> weight(vectors.GetSize(), 0.0);

  // Store the sum of the squares of each vector value ...
  vtkArrayCoordinates coordinates;
  for (vtkIdType n = 0; n != value_count; ++n)
  {
    if (this->CheckAbort())
    {
      break;
    }
    output_array->GetCoordinatesN(n, coordinates);
    weight[coordinates[vector_dimension] - vectors.GetBegin()] +=
      pow(output_array->GetValueN(n), p_value);
  }

  // Convert the sums into weights, avoiding divide-by-zero ...
  for (vtkIdType i = 0; i != vectors.GetSize(); ++i)
  {
    if (this->CheckAbort())
    {
      break;
    }
    const double length = pow(weight[i], 1.0 / p_value);
    weight[i] = length ? 1.0 / length : 0.0;
  }

  // Apply the weights to each vector ...
  for (vtkIdType n = 0; n != value_count; ++n)
  {
    if (this->CheckAbort())
    {
      break;
    }
    output_array->GetCoordinatesN(n, coordinates);
    output_array->SetValueN(
      n, output_array->GetValueN(n) * weight[coordinates[vector_dimension] - vectors.GetBegin()]);
  }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(output_array);
  output_array->Delete();

  return 1;
}
VTK_ABI_NAMESPACE_END
