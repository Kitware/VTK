// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov

#include "vtkExtractArray.h"
#include "vtkArrayData.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

///////////////////////////////////////////////////////////////////////////////
// vtkExtractArray

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractArray);

vtkExtractArray::vtkExtractArray()
  : Index(0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkExtractArray::~vtkExtractArray() = default;

void vtkExtractArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Index: " << this->Index << endl;
}

int vtkExtractArray::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
  }

  return 0;
}

int vtkExtractArray::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);

  if (this->Index < 0 || this->Index >= input->GetNumberOfArrays())
  {
    vtkErrorMacro(<< "Array index " << this->Index << " out-of-range for vtkArrayData containing "
                  << input->GetNumberOfArrays() << " arrays.");
    return 0;
  }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(input->GetArray(this->Index));

  this->CheckAbort();

  return 1;
}
VTK_ABI_NAMESPACE_END
