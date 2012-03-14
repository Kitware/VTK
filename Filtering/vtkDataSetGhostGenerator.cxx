/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkDataSetGhostGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkDataSetGhostGenerator.h"
#include "vtkDataObject.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIndent.h"

#include <cassert>

vtkDataSetGhostGenerator::vtkDataSetGhostGenerator()
{
  this->NumberOfGhostLayers = 0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkDataSetGhostGenerator::~vtkDataSetGhostGenerator()
{

}

//------------------------------------------------------------------------------
void vtkDataSetGhostGenerator::PrintSelf(ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}

//------------------------------------------------------------------------------
int vtkDataSetGhostGenerator::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkDataSetGhostGenerator::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );

  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkDataSetGhostGenerator::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector **inputVector,
    vtkInformationVector* outputVector )
{

 // STEP 0: Get input object
 vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
 assert( "pre: Null input object!" && (input != NULL)  );
 vtkMultiBlockDataSet *inputMultiBlock=
     vtkMultiBlockDataSet::SafeDownCast(
         input->Get(vtkDataObject::DATA_OBJECT() ) );
 assert( "pre: Input multi-block object is NULL" && (inputMultiBlock != NULL) );

 // STEP 1: Get output object
 vtkInformation *output = outputVector->GetInformationObject( 0 );
 assert( "pre: Null output object!" && (output != NULL) );
 vtkMultiBlockDataSet *outputMultiBlock =
     vtkMultiBlockDataSet::SafeDownCast(
         output->Get(vtkDataObject::DATA_OBJECT() ) );
 assert( "pre: Outpu multi-block object is NULL" && (outputMultiBlock != NULL));

 // STEP 2: Generate ghost layers
 if( this->NumberOfGhostLayers == 0 )
   {
   // Shallow copy the input object
   outputMultiBlock->ShallowCopy( inputMultiBlock );
   }
 else
   {
   // Create requested ghost layers
   this->GenerateGhostLayers( inputMultiBlock, outputMultiBlock );
   }
 return 1;
}
