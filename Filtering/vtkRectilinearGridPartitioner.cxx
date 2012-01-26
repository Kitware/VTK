/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkRectilinearGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkRectilinearGridPartitioner.h"
#include "vtkObjectFactory.h"
#include "vtkIndent.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <cassert>

vtkStandardNewMacro( vtkRectilinearGridPartitioner );

vtkRectilinearGridPartitioner::vtkRectilinearGridPartitioner()
{
  this->NumberOfPartitions  = 2;
  this->NumberOfGhostLayers = 0;
}


vtkRectilinearGridPartitioner::~vtkRectilinearGridPartitioner()
{

}

//------------------------------------------------------------------------------
void vtkRectilinearGridPartitioner::PrintSelf(
    std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkRectilinearGridPartitioner::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkRectilinearGridPartitioner::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkRectilinearGridPartitioner::RequestData(
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
