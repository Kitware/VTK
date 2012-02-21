/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkOverlappingAMRAlgorithm.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkOverlappingAMRAlgorithm.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkInformation.h"
#include "vtkCompositeDataPipeline.h"

vtkStandardNewMacro(vtkOverlappingAMRAlgorithm);

//------------------------------------------------------------------------------
vtkOverlappingAMRAlgorithm::vtkOverlappingAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkOverlappingAMRAlgorithm::~vtkOverlappingAMRAlgorithm()
{

}

//------------------------------------------------------------------------------
void vtkOverlappingAMRAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
vtkOverlappingAMR* vtkOverlappingAMRAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkOverlappingAMR* vtkOverlappingAMRAlgorithm::GetOutput(int port)
{
  vtkDataObject *output =
      vtkCompositeDataPipeline::SafeDownCast(
          this->GetExecutive())->GetCompositeOutputData(port);
  return( vtkOverlappingAMR::SafeDownCast(output));
}

//------------------------------------------------------------------------------
int vtkOverlappingAMRAlgorithm::FillOutputPortInformation(
    int vtkNotUsed(port),vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkOverlappingAMRAlgorithm::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkOverlappingAMR");
  return 1;
}
