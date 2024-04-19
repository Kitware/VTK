// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOverlappingAMRAlgorithm.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOverlappingAMRAlgorithm);

//------------------------------------------------------------------------------
vtkOverlappingAMRAlgorithm::vtkOverlappingAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkOverlappingAMRAlgorithm::~vtkOverlappingAMRAlgorithm() = default;

//------------------------------------------------------------------------------
void vtkOverlappingAMRAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkOverlappingAMR* vtkOverlappingAMRAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkOverlappingAMR* vtkOverlappingAMRAlgorithm::GetOutput(int port)
{
  vtkDataObject* output =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return (vtkOverlappingAMR::SafeDownCast(output));
}

//------------------------------------------------------------------------------
int vtkOverlappingAMRAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkOverlappingAMRAlgorithm::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  return 1;
}
VTK_ABI_NAMESPACE_END
