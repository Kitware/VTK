/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRParticlesSliceFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRParticlesSliceFilter.h"
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkPlane.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIndent.h"

vtkStandardNewMacro(vtkAMRParticlesSliceFilter);

vtkAMRParticlesSliceFilter::vtkAMRParticlesSliceFilter()
{
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
  this->OffSetFromOrigin = 0.0;
  this->DX               = 1.0;
}

//------------------------------------------------------------------------------
vtkAMRParticlesSliceFilter::~vtkAMRParticlesSliceFilter()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRParticlesSliceFilter::PrintSelf(
    std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
int vtkAMRParticlesSliceFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkMultiBlockDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRParticlesSliceFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRParticlesSliceFilter::RequestData(
    vtkInformation* vtkNotUsed(request), vtkInformationVector **input,
    vtkInformationVector *output )
{

  return 1;
}
