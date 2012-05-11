/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToLabelHierarchy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkPointSetToLabelHierarchy.h"

#include "vtkDataObjectTypes.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkUnicodeString.h"
#include "vtkUnicodeStringArray.h"

#include <vector>

vtkStandardNewMacro(vtkPointSetToLabelHierarchy);
vtkCxxSetObjectMacro(vtkPointSetToLabelHierarchy, TextProperty, vtkTextProperty);

vtkPointSetToLabelHierarchy::vtkPointSetToLabelHierarchy()
{
  this->MaximumDepth = 5;
  this->TargetLabelCount = 32;
  this->UseUnicodeStrings = false;
  this->TextProperty = vtkTextProperty::New();
  this->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::POINT, "Priority" );
  this->SetInputArrayToProcess( 1, 0, 0, vtkDataObject::POINT, "LabelSize" );
  this->SetInputArrayToProcess( 2, 0, 0, vtkDataObject::POINT, "LabelText" );
  this->SetInputArrayToProcess( 3, 0, 0, vtkDataObject::POINT, "IconIndex" );
  this->SetInputArrayToProcess( 4, 0, 0, vtkDataObject::POINT, "Orientation" );
  this->SetInputArrayToProcess( 5, 0, 0, vtkDataObject::POINT, "BoundedSize" );
}

vtkPointSetToLabelHierarchy::~vtkPointSetToLabelHierarchy()
{
  if (this->TextProperty)
    {
    this->TextProperty->Delete();
    }
}

void vtkPointSetToLabelHierarchy::SetPriorityArrayName(const char* name)
{
  this->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::POINT, name );
}

const char* vtkPointSetToLabelHierarchy::GetPriorityArrayName()
{
  vtkInformation* info = this->GetInformation()->Get( vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS() )->
    GetInformationObject( 0 );
  return info->Get( vtkDataObject::FIELD_NAME() );
}

void vtkPointSetToLabelHierarchy::SetSizeArrayName(const char* name)
{
  this->SetInputArrayToProcess( 1, 0, 0, vtkDataObject::POINT, name );
}

const char* vtkPointSetToLabelHierarchy::GetSizeArrayName()
{
  vtkInformation* info = this->GetInformation()->Get( vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS() )->
    GetInformationObject( 1 );
  return info->Get( vtkDataObject::FIELD_NAME() );
}

void vtkPointSetToLabelHierarchy::SetLabelArrayName(const char* name)
{
  this->SetInputArrayToProcess( 2, 0, 0, vtkDataObject::POINT, name );
}

const char* vtkPointSetToLabelHierarchy::GetLabelArrayName()
{
  vtkInformation* info = this->GetInformation()->Get( vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS() )->
    GetInformationObject( 2 );
  return info->Get( vtkDataObject::FIELD_NAME() );
}

void vtkPointSetToLabelHierarchy::SetIconIndexArrayName(const char* name)
{
  this->SetInputArrayToProcess( 3, 0, 0, vtkDataObject::POINT, name );
}

const char* vtkPointSetToLabelHierarchy::GetIconIndexArrayName()
{
  vtkInformation* info = this->GetInformation()->Get( vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS() )->
    GetInformationObject( 3 );
  return info->Get( vtkDataObject::FIELD_NAME() );
}

void vtkPointSetToLabelHierarchy::SetOrientationArrayName(const char* name)
{
  this->SetInputArrayToProcess( 4, 0, 0, vtkDataObject::POINT, name );
}

const char* vtkPointSetToLabelHierarchy::GetOrientationArrayName()
{
  vtkInformation* info = this->GetInformation()->Get( vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS() )->
    GetInformationObject( 4 );
  return info->Get( vtkDataObject::FIELD_NAME() );
}

void vtkPointSetToLabelHierarchy::SetBoundedSizeArrayName(const char* name)
{
  this->SetInputArrayToProcess( 5, 0, 0, vtkDataObject::POINT, name );
}

const char* vtkPointSetToLabelHierarchy::GetBoundedSizeArrayName()
{
  vtkInformation* info = this->GetInformation()->Get( vtkAlgorithm::INPUT_ARRAYS_TO_PROCESS() )->
    GetInformationObject( 5 );
  return info->Get( vtkDataObject::FIELD_NAME() );
}

int vtkPointSetToLabelHierarchy::FillInputPortInformation(
  int port, vtkInformation* info )
{
  if ( port == 0 )
    {
    info->Remove( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE() );
    info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet" );
    info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph" );
    }
  return 1;
}

int vtkPointSetToLabelHierarchy::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();

  vtkIdType numPoints = 0;
  vtkInformation* inInfo  =  inputVector[0]->GetInformationObject( 0 );
  vtkDataObject* inData = inInfo->Get( vtkDataObject::DATA_OBJECT() );

  vtkGraph* graph = vtkGraph::SafeDownCast( inData );
  if ( graph )
    {
    numPoints = graph->GetNumberOfVertices();
    }

  vtkPointSet* ptset = vtkPointSet::SafeDownCast( inData );
  if ( ptset )
    {
    numPoints = ptset->GetNumberOfPoints();
    }

  int maxDepth = this->MaximumDepth;
  //maxDepth = (int)ceil(log(1.0 + 7.0*totalPoints/this->TargetLabelCount) / log(8.0));

  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );

  vtkLabelHierarchy* ouData = vtkLabelHierarchy::SafeDownCast(
    outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  if ( ! ouData )
    {
    vtkErrorMacro( "No output data" );
    return 0;
    }

  ouData->SetTargetLabelCount( this->TargetLabelCount );
  ouData->SetMaximumDepth( maxDepth );

  if ( ! inData )
    {
    vtkErrorMacro( "Null input data" );
    return 0;
    }

  vtkPoints* pts = 0;
  vtkDataSetAttributes* pdata = 0;

  if ( graph )
    {
    pts = graph->GetPoints();
    pdata = graph->GetVertexData();
    }

  if ( ptset )
    {
    pts = ptset->GetPoints();
    pdata = ptset->GetPointData();
    }

  vtkDataArray* priorities = vtkDataArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess( 0, inputVector ) );
  vtkDataArray* sizes = vtkDataArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess( 1, inputVector ) );
  vtkAbstractArray* labels =
    this->GetInputAbstractArrayToProcess( 2, inputVector );
  vtkIntArray* iconIndices = vtkIntArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess( 3, inputVector ) );
  vtkDataArray* orientations = vtkDataArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess( 4, inputVector ) );
  vtkDataArray* boundedSizes = vtkDataArray::SafeDownCast(
    this->GetInputAbstractArrayToProcess( 5, inputVector ) );

  if ( ! ouData->GetPoints() )
    {
    vtkPoints* oupts = vtkPoints::New();
    ouData->SetPoints( oupts );
    oupts->FastDelete();
    }
  if ( pts )
    {
    ouData->GetPoints()->ShallowCopy( pts );
    }
  ouData->GetPointData()->ShallowCopy( pdata );
  vtkSmartPointer<vtkIntArray> type = vtkSmartPointer<vtkIntArray>::New();
  type->SetName( "Type" );
  type->SetNumberOfTuples( numPoints );
  type->FillComponent( 0, 0 );
  ouData->GetPointData()->AddArray( type );
  ouData->SetPriorities( priorities );
  if ( labels )
    {
    if ( ( this->UseUnicodeStrings && vtkUnicodeStringArray::SafeDownCast( labels ) ) ||
         ( !this->UseUnicodeStrings && vtkStringArray::SafeDownCast( labels ) ) )
      {
      ouData->SetLabels( labels );
      }
    else if ( this->UseUnicodeStrings )
      {
      vtkSmartPointer<vtkUnicodeStringArray> arr =
        vtkSmartPointer<vtkUnicodeStringArray>::New();
      vtkIdType numComps = labels->GetNumberOfComponents();
      vtkIdType numTuples = labels->GetNumberOfTuples();
      arr->SetNumberOfComponents( numComps );
      arr->SetNumberOfTuples( numTuples );
      for (vtkIdType i = 0; i < numTuples; ++i )
        {
        for (vtkIdType j = 0; j < numComps; ++j )
          {
          vtkIdType ind = i*numComps + j;
          arr->SetValue( ind, labels->GetVariantValue(ind).ToUnicodeString() );
          }
        }
      arr->SetName( labels->GetName() );
      ouData->GetPointData()->AddArray( arr );
      ouData->SetLabels( arr );
      }
    else
      {
      vtkSmartPointer<vtkStringArray> arr =
        vtkSmartPointer<vtkStringArray>::New();
      vtkIdType numComps = labels->GetNumberOfComponents();
      vtkIdType numTuples = labels->GetNumberOfTuples();
      arr->SetNumberOfComponents( numComps );
      arr->SetNumberOfTuples( numTuples );
      for (vtkIdType i = 0; i < numTuples; ++i )
        {
        for (vtkIdType j = 0; j < numComps; ++j )
          {
          vtkIdType ind = i*numComps + j;
          arr->SetValue( ind, labels->GetVariantValue(ind).ToString() );
          }
        }
      arr->SetName( labels->GetName() );
      ouData->GetPointData()->AddArray( arr );
      ouData->SetLabels( arr );
      }
    }
  ouData->SetIconIndices( iconIndices );
  ouData->SetOrientations( orientations );
  ouData->SetSizes( sizes );
  ouData->SetBoundedSizes( boundedSizes );
  ouData->SetTextProperty( this->TextProperty );
  ouData->ComputeHierarchy();

  timer->StopTimer();
  vtkDebugMacro("StartupTime: " << timer->GetElapsedTime() << endl);

  return 1;
}

void vtkPointSetToLabelHierarchy::PrintSelf( ostream& os, vtkIndent indent )
{
  os << indent << "MaximumDepth: " << this->MaximumDepth << "\n";
  os << indent << "TargetLabelCount: " << this->TargetLabelCount << "\n";
  os << indent << "UseUnicodeStrings: " << this->UseUnicodeStrings << "\n";
  os << indent << "TextProperty: " << this->TextProperty << "\n";
  this->Superclass::PrintSelf( os, indent );
}
