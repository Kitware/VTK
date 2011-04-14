/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRSliceFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRSliceFilter.h"
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkAMRUtilities.h"
#include "vtkPlane.h"
#include "vtkAMRBox.h"

#include <cassert>

vtkStandardNewMacro(vtkAMRSliceFilter);

vtkAMRSliceFilter::vtkAMRSliceFilter()
{
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
  this->OffSetFromOrigin = 0.0;
  this->Normal           = 1;
}

//------------------------------------------------------------------------------
vtkAMRSliceFilter::~vtkAMRSliceFilter()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRSliceFilter::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
int vtkAMRSliceFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkHierarchicalBoxDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRSliceFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
    vtkDataObject::DATA_TYPE_NAME(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
bool vtkAMRSliceFilter::IsAMRData2D( vtkHierarchicalBoxDataSet *input )
{
  assert( "pre: Input AMR dataset is NULL" && (input != NULL)  );

  vtkAMRBox box;
  input->GetMetaData( 0, 0, box );

  if( box.GetDimensionality() == 2 )
    return true;

  return false;
}

//------------------------------------------------------------------------------
vtkPlane* vtkAMRSliceFilter::GetCutPlane( vtkHierarchicalBoxDataSet *inp )
{
  vtkPlane *pl = vtkPlane::New();

  switch( this->Normal )
    {
      case 1:
        // X-Normal
        break;
      case 2:
        // Y-Normal
        break;
      case 3:
        // Z-Normal
        break;
      default:
        vtkErrorMacro( "Undefined plane normal" );
    }

  return( pl );

}

//------------------------------------------------------------------------------
int vtkAMRSliceFilter::RequestData(
    vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input information object is NULL" && (input != NULL) );
  vtkHierarchicalBoxDataSet *inputAMR=
      vtkHierarchicalBoxDataSet::SafeDownCast(
          input->Get(vtkDataObject::DATA_OBJECT() ) );

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: output information object is NULL" && (output != NULL) );
  vtkHierarchicalBoxDataSet *outputAMR=
      vtkHierarchicalBoxDataSet::SafeDownCast(
        output->Get( vtkDataObject::DATA_OBJECT() ) );

  if( this->IsAMRData2D( inputAMR ) )
    {
      outputAMR->ShallowCopy( inputAMR );
      return 1;
    }

  // STEP 2: Compute global origin
  vtkPlane *cutPlane = this->GetCutPlane( inputAMR );
  assert( "Cut plane is NULL" && (cutPlane != NULL) );

  // TODO: Implement this
  outputAMR->GenerateVisibilityArrays();
  return 1;
}
