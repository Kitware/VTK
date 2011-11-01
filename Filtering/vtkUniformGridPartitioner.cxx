/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkUniformGridPartitioner.h"
#include "vtkObjectFactory.h"
#include "vtkIndent.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStructuredData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <cassert>

vtkStandardNewMacro( vtkUniformGridPartitioner );

//------------------------------------------------------------------------------
vtkUniformGridPartitioner::vtkUniformGridPartitioner()
{
  this->NumberOfPartitions = 2;
}

//------------------------------------------------------------------------------
vtkUniformGridPartitioner::~vtkUniformGridPartitioner()
{

}

//------------------------------------------------------------------------------
void vtkUniformGridPartitioner::PrintSelf(std::ostream &oss, vtkIndent indent)
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkUniformGridPartitioner::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkImageData" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkUniformGridPartitioner::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkUniformGridPartitioner::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector, vtkInformationVector* outputVector )
{
  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input information object is NULL" && (input != NULL) );

  vtkImageData *grd =
    vtkImageData::SafeDownCast( input->Get(vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: input grid is NULL!" && (grd != NULL));

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: output information object is NULL" && (output != NULL) );
  vtkMultiBlockDataSet *multiblock =
      vtkMultiBlockDataSet::SafeDownCast(
        output->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: multiblock grid is NULL!" && (multiblock != NULL) );

  // STEP 2: Get the global extent
  int extent[6];
  int dims[3];
  grd->GetDimensions( dims );
  extent[0] = extent[1] = extent[2] = 0;
  extent[3] = dims[0]-1;
  extent[4] = dims[1]-1;
  extent[5] = dims[2]-1;

  // STEP 3: Setup extent partitioner
  vtkExtentRCBPartitioner *extentPartitioner = vtkExtentRCBPartitioner::New();
  assert( "pre: extent partitioner is NULL" && (extentPartitioner != NULL) );
  extentPartitioner->SetGlobalExtent( extent );
  extentPartitioner->SetNumberOfPartitions( this->NumberOfPartitions );

  // STEP 4: Partition
  extentPartitioner->Partition();

  // STEP 5: Extract partitions into a multi-block dataset.
  multiblock->SetNumberOfBlocks( extentPartitioner->GetNumExtents() );
  unsigned int blockIdx = 0;
  for( ; blockIdx < multiblock->GetNumberOfBlocks(); ++blockIdx )
    {
      int ext[6];
      extentPartitioner->GetPartitionExtent( blockIdx, ext );

      double origin[3];
      int ijk[3];
      ijk[0] = ext[0];
      ijk[1] = ext[1];
      ijk[2] = ext[2];

      int subdims[3];
      subdims[0] = ext[3]-ext[0]+1;
      subdims[1] = ext[4]-ext[1]+1;
      subdims[2] = ext[5]-ext[2]+1;
      int pntIdx = vtkStructuredData::ComputePointId( dims, ijk );

      grd->GetPoint( pntIdx, origin );

      vtkUniformGrid *subgrid = vtkUniformGrid::New();
      subgrid->SetOrigin( origin );
      subgrid->SetSpacing( grd->GetSpacing() );
      subgrid->SetDimensions( subdims );

      multiblock->SetBlock( blockIdx, subgrid );
      subgrid->Delete();
    } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
