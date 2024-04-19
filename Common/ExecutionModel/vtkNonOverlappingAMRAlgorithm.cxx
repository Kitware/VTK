// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNonOverlappingAMRAlgorithm.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkNonOverlappingAMRAlgorithm);

//------------------------------------------------------------------------------
vtkNonOverlappingAMRAlgorithm::vtkNonOverlappingAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkNonOverlappingAMRAlgorithm::~vtkNonOverlappingAMRAlgorithm() = default;

//------------------------------------------------------------------------------
void vtkNonOverlappingAMRAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkNonOverlappingAMR* vtkNonOverlappingAMRAlgorithm::GetOutput()
{
  return (this->GetOutput(0));
}

//------------------------------------------------------------------------------
vtkNonOverlappingAMR* vtkNonOverlappingAMRAlgorithm::GetOutput(int port)
{
  vtkDataObject* output =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return (vtkNonOverlappingAMR::SafeDownCast(output));
}

//------------------------------------------------------------------------------
int vtkNonOverlappingAMRAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkNonOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkNonOverlappingAMRAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkNonOverlappingAMR");
  return 1;
}
VTK_ABI_NAMESPACE_END
