/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkStructuredGridPartitioner.h"
#include "vtkObjectFactory.h"
#include "vtkIndent.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <cassert>

vtkStandardNewMacro( vtkStructuredGridPartitioner );

//------------------------------------------------------------------------------
vtkStructuredGridPartitioner::vtkStructuredGridPartitioner()
{
  this->NumberOfPartitions  = 2;
  this->NumberOfGhostLayers = 0;
}

//------------------------------------------------------------------------------
vtkStructuredGridPartitioner::~vtkStructuredGridPartitioner()
{

}

//------------------------------------------------------------------------------
void vtkStructuredGridPartitioner::PrintSelf(
    std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkStructuredGridPartitioner::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkStructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkStructuredGridPartitioner::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkStructuredGridPartitioner::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,vtkInformationVector* outputVector )
{
  // TODO: implement this
  // STEP 0: Get input object

  // STEP 1: Get output object

  // STEP 2: Get the global extent

  // STEP 3: Setup extent partitioner

  // STEP 4: Extract partitions in a multi-block
  return 1;
}

