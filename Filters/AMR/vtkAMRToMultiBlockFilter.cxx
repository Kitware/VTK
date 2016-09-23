/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRToMultiBlockFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRToMultiBlockFilter.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIndent.h"
#include "vtkMultiProcessController.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"

#include <cassert>

vtkStandardNewMacro(vtkAMRToMultiBlockFilter);

//------------------------------------------------------------------------------
vtkAMRToMultiBlockFilter::vtkAMRToMultiBlockFilter()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
}

//------------------------------------------------------------------------------
vtkAMRToMultiBlockFilter::~vtkAMRToMultiBlockFilter()
{

}

//------------------------------------------------------------------------------
void vtkAMRToMultiBlockFilter::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkAMRToMultiBlockFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  info->Set(
      vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRToMultiBlockFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRToMultiBlockFilter::CopyAMRToMultiBlock(
    vtkOverlappingAMR *amr, vtkMultiBlockDataSet *mbds )
{
  assert( "pre: input AMR dataset is NULL" && (amr != NULL) );
  assert( "pre: output multi-block dataset is NULL" && (mbds != NULL) );

  mbds->SetNumberOfBlocks( amr->GetTotalNumberOfBlocks( ) );
  unsigned int blockIdx = 0;
  unsigned int levelIdx = 0;
  for( ; levelIdx < amr->GetNumberOfLevels(); ++levelIdx )
  {
    unsigned int dataIdx = 0;
    for( ; dataIdx < amr->GetNumberOfDataSets( levelIdx ); ++dataIdx )
    {
      vtkUniformGrid *grid = amr->GetDataSet( levelIdx, dataIdx );
      if( grid != NULL )
      {
        vtkUniformGrid *gridCopy = vtkUniformGrid::New();
        gridCopy->ShallowCopy( grid );
        mbds->SetBlock( blockIdx, gridCopy );
      }
      else
      {
        mbds->SetBlock( blockIdx, NULL );
      }
      ++blockIdx;
    } // END for all data
  } // END for all levels
}

//------------------------------------------------------------------------------
int vtkAMRToMultiBlockFilter::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{

  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input information object is NULL" && (input != NULL) );
  vtkOverlappingAMR *amrds=
      vtkOverlappingAMR::SafeDownCast(
          input->Get( vtkDataObject::DATA_OBJECT( ) ) );
  assert( "pre: input data-structure is NULL" && (amrds != NULL) );

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: output Co information is NULL" && (output != NULL) );
  vtkMultiBlockDataSet *mbds=
      vtkMultiBlockDataSet::SafeDownCast(
          output->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output multi-block dataset is NULL" && (mbds != NULL) );

  // STEP 2: Copy AMR data to multi-block
  this->CopyAMRToMultiBlock( amrds, mbds );

  return 1;
}
