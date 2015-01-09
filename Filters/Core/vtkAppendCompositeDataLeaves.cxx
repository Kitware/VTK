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
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetCollection.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkAppendCompositeDataLeaves);

//----------------------------------------------------------------------------
vtkAppendCompositeDataLeaves::vtkAppendCompositeDataLeaves()
{
  this->AppendFieldData = 0;
}

//----------------------------------------------------------------------------
vtkAppendCompositeDataLeaves::~vtkAppendCompositeDataLeaves()
{
}

//----------------------------------------------------------------------------
int vtkAppendCompositeDataLeaves::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{
  // this filter preserves input data type.
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
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
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
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if ( numInputs <= 0 )
    {
    // Fail silently when there are no inputs.
    return 1;
    }

  // get the output info object
  vtkCompositeDataSet* output = vtkCompositeDataSet::GetData(outputVector, 0);
  vtkCompositeDataSet* input0 = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (numInputs == 1)
    {
    // trivial case.
    output->ShallowCopy(input0);
    return 1;
    }

  // since composite structure is expected to be same on all inputs, we copy the
  // structure from the 1st input.
  output->CopyStructure(input0);

  vtkDebugMacro(<<"Appending data together");

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(output->NewIterator());

  iter->SkipEmptyNodesOff(); // We're iterating over the output, whose leaves are all empty.
  static bool first = true;
  for (iter->InitTraversal(); ! iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    // Loop over all inputs at this "spot" in the composite data tree. locate
    // the first input that has a non-null data-object at this location, if any.
    vtkDataObject* obj = 0;
    int inputIndex;
    for (inputIndex = 0; inputIndex < numInputs && !obj; ++inputIndex)
      {
      vtkCompositeDataSet* inputX = vtkCompositeDataSet::GetData(inputVector[0],
        inputIndex);
      obj = inputX? inputX->GetDataSet(iter) : NULL;
      }

    if (obj == NULL)
      {
      continue; // no input had a non-NULL dataset
      }

    if (vtkUnstructuredGrid::SafeDownCast(obj))
      {
      this->AppendUnstructuredGrids(
        inputVector[0], inputIndex - 1, numInputs, iter, output);
      }
    else if (vtkPolyData::SafeDownCast(obj))
      {
      this->AppendPolyData(inputVector[0],
        inputIndex - 1, numInputs, iter, output);
      }
    else if (vtkTable *table = vtkTable::SafeDownCast(obj))
      {
      vtkTable *newTable = vtkTable::New();
      newTable->ShallowCopy(table);
      output->SetDataSet(iter, newTable);
      newTable->Delete();
      }
    else if (vtkImageData* img = vtkImageData::SafeDownCast(obj))
      {
      vtkImageData* clone = img->NewInstance();
      clone->ShallowCopy(img);
      output->SetDataSet(iter, clone);
      clone->FastDelete();
      }
    else if (vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(obj))
      {
      vtkStructuredGrid* clone = sg->NewInstance();
      clone->ShallowCopy(sg);
      output->SetDataSet(iter, clone);
      clone->FastDelete();
      }
    else if (first)
      {
      first = false;
      vtkWarningMacro(
        << "Input " << inputIndex << " was of type \""
        << obj->GetClassName() << "\" which is not handled\n" );
      }
    }
  first = true;
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
}

//----------------------------------------------------------------------------
void vtkAppendCompositeDataLeaves::AppendUnstructuredGrids(
  vtkInformationVector* inputVector,
  int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output )
{
  vtkNew<vtkAppendFilter> appender;

  for ( int idx = i; idx < numInputs; ++ idx )
    {
    vtkCompositeDataSet* icdset= vtkCompositeDataSet::GetData(inputVector, idx);
    if ( icdset )
      {
      vtkUnstructuredGrid* iudset = vtkUnstructuredGrid::SafeDownCast( icdset->GetDataSet( iter ) );
      if ( iudset )
        {
        appender->AddInputDataObject( iudset );
        }
      }
    }
  appender->Update();
  output->SetDataSet(iter, appender->GetOutputDataObject(0));
  this->AppendFieldDataArrays(inputVector,
    i, numInputs, iter, appender->GetOutput(0));
}

//----------------------------------------------------------------------------
void vtkAppendCompositeDataLeaves::AppendPolyData(
  vtkInformationVector* inputVector,
  int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output )
{
  vtkNew<vtkAppendPolyData> appender;

  for ( int idx = i; idx < numInputs; ++ idx )
    {
    vtkCompositeDataSet* icdset= vtkCompositeDataSet::GetData(inputVector, idx);
    if ( icdset )
      {
      vtkPolyData* ipdset = vtkPolyData::SafeDownCast( icdset->GetDataSet( iter ) );
      if ( ipdset )
        {
        appender->AddInputDataObject( ipdset );
        }
      }
    }

  appender->Update();
  output->SetDataSet(iter, appender->GetOutputDataObject(0));
  this->AppendFieldDataArrays(inputVector,
    i, numInputs, iter, appender->GetOutput(0));
}

//----------------------------------------------------------------------------
void vtkAppendCompositeDataLeaves::AppendFieldDataArrays(
  vtkInformationVector* inputVector,
  int i, int numInputs, vtkCompositeDataIterator* iter, vtkDataSet* odset )
{
  if ( ! this->AppendFieldData )
    return;

  vtkFieldData* ofd = odset->GetFieldData();
  for ( int idx = i; idx < numInputs; ++ idx )
    {
    vtkCompositeDataSet* icdset= vtkCompositeDataSet::GetData(inputVector, idx);
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
