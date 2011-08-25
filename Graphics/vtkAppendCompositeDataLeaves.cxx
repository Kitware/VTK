/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendCompositeDataLeaves.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendCompositeDataLeaves.h"

#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetCollection.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkAppendCompositeDataLeaves);

//----------------------------------------------------------------------------
vtkAppendCompositeDataLeaves::vtkAppendCompositeDataLeaves()
{
  this->AppendFieldData = 0;
  this->AppendUG = 0;
  this->AppendPD = 0;
}

//----------------------------------------------------------------------------
vtkAppendCompositeDataLeaves::~vtkAppendCompositeDataLeaves()
{
  if ( this->AppendUG )
    this->AppendUG->Delete();
  if ( this->AppendPD )
    this->AppendPD->Delete();
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkAppendCompositeDataLeaves::GetInput( int idx )
{
  if ( idx >= this->GetNumberOfInputConnections( 0 ) || idx < 0 )
    {
    return 0;
    }

  return vtkCompositeDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData( 0, idx ) );
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendCompositeDataLeaves::RemoveInput( vtkDataSet* ds )
{
  vtkAlgorithmOutput* algOutput = 0;
  if ( ds )
    {
    algOutput = ds->GetProducerPort();
    }

  this->RemoveInputConnection( 0, algOutput );
}

//----------------------------------------------------------------------------
int vtkAppendCompositeDataLeaves::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject( 0 );
  if ( ! inInfo )
    {
    return 0;
    }
  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  if ( input )
    {
    // for each output
    for ( int i = 0; i < this->GetNumberOfOutputPorts(); ++ i )
      {
      vtkInformation* info = outputVector->GetInformationObject( i );
      vtkCompositeDataSet *output = vtkCompositeDataSet::SafeDownCast(
        info->Get( vtkDataObject::DATA_OBJECT() ) );

      if ( ! output || ! output->IsA( input->GetClassName() ) )
        {
        vtkCompositeDataSet* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation( info );
        newOutput->Delete();
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendCompositeDataLeaves::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  int numInputs = this->GetNumberOfInputConnections( 0 );
  if ( numInputs <= 0 )
    {
    // Fail silently when there are no inputs.
    return 1;
    }

  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );

  // get the ouptut
  vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(
    outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  vtkDebugMacro(<<"Copying structure to output");

  vtkCompositeDataSet* anInput = vtkCompositeDataSet::SafeDownCast(
    this->GetInput( 0 ) );

  if (numInputs == 1)
    {
    output->ShallowCopy(anInput);
    return 1;
    }

  output->CopyStructure( anInput );

  vtkDebugMacro(<<"Appending data together");

  vtkCompositeDataIterator* iter = output->NewIterator();
  iter->VisitOnlyLeavesOn();
  iter->SkipEmptyNodesOff(); // We're iterating over the output, whose leaves are all empty.
  int idx = 0;
  int i;
  static bool first = true;
  for ( iter->InitTraversal(); ! iter->IsDoneWithTraversal(); iter->GoToNextItem(), ++idx )
    {
    // Loop over all inputs at this "spot" in the composite data.
    vtkDataObject* obj = 0;
    for ( i = 0; i < numInputs && ! obj; ++ i )
      {
      obj = this->GetInput( i )->GetDataSet( iter );
      }
    if ( ! obj )
      {
      continue; // no input had a non-NULL dataset
      }
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast( obj );
    if ( ug )
      {
      this->AppendUnstructuredGrids( i - 1, numInputs, iter, output );
      continue;
      }
    vtkPolyData* pd = vtkPolyData::SafeDownCast( obj );
    if ( pd )
      {
      this->AppendPolyData( i - 1, numInputs, iter, output );
      continue;
      }
    vtkTable *table = vtkTable::SafeDownCast(obj);
    if(table)
      {
      vtkTable *newTable = vtkTable::New();
      newTable->ShallowCopy(table);
      output->SetDataSet(iter, newTable);
      newTable->Delete();
      continue;
      }
    if ( first )
      {
      first = false;
      vtkWarningMacro(
        << "Input " << i << " was of type \""
        << obj->GetClassName() << "\" which is not handled\n" );
      }
    }
  first = true;
  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendCompositeDataLeaves::FillInputPortInformation( int, vtkInformation* info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet" );
  info->Set( vtkAlgorithm::INPUT_IS_REPEATABLE(), 1 );
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendCompositeDataLeaves::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "AppendFieldData: " << this->AppendFieldData << "\n";
  os << indent << "AppendUG: " << this->AppendUG << "\n";
  os << indent << "AppendPD: " << this->AppendPD << "\n";
}

//----------------------------------------------------------------------------
void vtkAppendCompositeDataLeaves::AppendUnstructuredGrids(
  int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output )
{
  if ( this->AppendUG )
    {
    this->AppendUG->Delete();
    }
  this->AppendUG = vtkAppendFilter::New();

  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
  output->SetDataSet( iter, ug );
  ug->Delete();

  for ( int idx = i; idx < numInputs; ++ idx )
    {
    vtkCompositeDataSet* icdset = this->GetInput( idx );
    if ( icdset )
      {
      vtkUnstructuredGrid* iudset = vtkUnstructuredGrid::SafeDownCast( icdset->GetDataSet( iter ) );
      if ( iudset )
        {
        this->AppendUG->AddInput( iudset );
        }
      }
    }
  this->AppendUG->Update();
  ug->ShallowCopy( this->AppendUG->GetOutput() );

  this->AppendFieldDataArrays( i, numInputs, iter, ug );
}

//----------------------------------------------------------------------------
void vtkAppendCompositeDataLeaves::AppendPolyData(
  int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output )
{
  if ( this->AppendPD )
    {
    this->AppendPD->Delete();
    }
  this->AppendPD = vtkAppendPolyData::New();

  vtkPolyData* pd = vtkPolyData::New();
  output->SetDataSet( iter, pd );
  pd->Delete();

  for ( int idx = i; idx < numInputs; ++ idx )
    {
    vtkCompositeDataSet* icdset = this->GetInput( idx );
    if ( icdset )
      {
      vtkPolyData* ipdset = vtkPolyData::SafeDownCast( icdset->GetDataSet( iter ) );
      if ( ipdset )
        {
        this->AppendPD->AddInput( ipdset );
        }
      }
    }
  this->AppendPD->Update();
  pd->ShallowCopy( this->AppendPD->GetOutput() );

  this->AppendFieldDataArrays( i, numInputs, iter, pd );
}

//----------------------------------------------------------------------------
void vtkAppendCompositeDataLeaves::AppendFieldDataArrays(
  int i, int numInputs, vtkCompositeDataIterator* iter, vtkDataSet* odset )
{
  if ( ! this->AppendFieldData )
    return;

  vtkFieldData* ofd = odset->GetFieldData();
  for ( int idx = i; idx < numInputs; ++ idx )
    {
    vtkCompositeDataSet* icdset = this->GetInput( idx );
    if ( icdset )
      {
      vtkDataObject* idobj = icdset->GetDataSet( iter );
      if ( idobj )
        {
        vtkFieldData* ifd = idobj->GetFieldData();
        int numArr = ifd->GetNumberOfArrays();
        for ( int a = 0; a < numArr; ++ a )
          {
          vtkAbstractArray* arr = ifd->GetAbstractArray( a );
          if ( ofd->HasArray( arr->GetName() ) )
            {
            // Do something?
            }
          else
            {
            ofd->AddArray( arr );
            }
          }
        }
      }
    }
}
