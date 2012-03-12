/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkNonOverlappingAMRAlgorithm.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkNonOverlappingAMRAlgorithm.h"
#include "vtkObjectFactory.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkInformation.h"
#include "vtkCompositeDataPipeline.h"

vtkStandardNewMacro(vtkNonOverlappingAMRAlgorithm);

//------------------------------------------------------------------------------
vtkNonOverlappingAMRAlgorithm::vtkNonOverlappingAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkNonOverlappingAMRAlgorithm::~vtkNonOverlappingAMRAlgorithm()
{

}

//------------------------------------------------------------------------------
void vtkNonOverlappingAMRAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkNonOverlappingAMR* vtkNonOverlappingAMRAlgorithm::GetOutput()
{
  return( this->GetOutput(0) );
}

//------------------------------------------------------------------------------
vtkNonOverlappingAMR* vtkNonOverlappingAMRAlgorithm::GetOutput(int port)
{
  vtkDataObject *output =
      vtkCompositeDataPipeline::SafeDownCast(
          this->GetExecutive())->GetCompositeOutputData(port);
  return( vtkNonOverlappingAMR::SafeDownCast(output) );
}

//------------------------------------------------------------------------------
int vtkNonOverlappingAMRAlgorithm::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation* info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkNonOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkNonOverlappingAMRAlgorithm::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkNonOverlappingAMR");
  return 1;
}
