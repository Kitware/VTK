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
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkGenericOutlineFilter.h"

#include <cassert>

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
    vtkInformation* vtkNotUsed(request), vtkInformationVector **inputVector,
    vtkInformationVector *outputVector )
{

  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject(0);
  assert( "pre: input information object is NULL" && (input != NULL) );
  vtkMultiBlockDataSet *particles =
      vtkMultiBlockDataSet::SafeDownCast(
          input->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: input particles dataset is NULL" && (particles != NULL) );

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: output information object is NULL" && (output != NULL) );
  vtkMultiBlockDataSet *outputParticles =
      vtkMultiBlockDataSet::SafeDownCast(
          output->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output particles dataset is NULL" && (outputParticles!=NULL));


  // STEP 2: Get outline
  vtkGenericOutlineFilter *outlineExtractor = vtkGenericOutlineFilter::New();
  outlineExtractor->SetInput( particles );
  outlineExtractor->Update();
  vtkPolyData *bbox = outlineExtractor->GetOutput( );
  outlineExtractor->Delete();

  outputParticles->SetNumberOfBlocks( particles->GetNumberOfBlocks() );
  outputParticles->SetBlock( 0, bbox );
  return 1;
}
