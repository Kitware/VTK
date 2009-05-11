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
#include "vtkTimerLog.h"
#include "vtkUnicodeString.h"
#include "vtkUnicodeStringArray.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPointSetToLabelHierarchy);
vtkCxxRevisionMacro(vtkPointSetToLabelHierarchy,"1.9");

vtkPointSetToLabelHierarchy::vtkPointSetToLabelHierarchy()
{
  this->MaximumDepth = 5;
  this->TargetLabelCount = 32;
  this->UseUnicodeStrings = false;
  this->SetInputArrayToProcess( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Priority" );
  this->SetInputArrayToProcess( 1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelSize" );
  this->SetInputArrayToProcess( 2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelText" );
  this->SetInputArrayToProcess( 3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ID" );
}

vtkPointSetToLabelHierarchy::~vtkPointSetToLabelHierarchy()
{
}

void vtkPointSetToLabelHierarchy::PrintSelf( ostream& os, vtkIndent indent )
{
  os << indent << "MaximumDepth: " << this->MaximumDepth << "\n";
  os << indent << "TargetLabelCount: " << this->TargetLabelCount << "\n";
  os << indent << "UseUnicodeStrings: " << (this->UseUnicodeStrings ? "ON" : "OFF") << endl; 
  this->Superclass::PrintSelf( os, indent );
}

int vtkPointSetToLabelHierarchy::FillInputPortInformation(
  int port, vtkInformation* info )
{
  if ( port == 0 )
    {
    info->Remove( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE() );
    info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet" );
    info->Append( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph" );
    info->Set( vtkAlgorithm::INPUT_IS_REPEATABLE(), 1 );
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

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  vtkstd::vector< vtkSmartPointer<vtkDataObject> > inData;
  vtkIdType totalPoints = 0;
  for ( int i = 0; i < numInputs; ++i )
    {
    vtkInformation* inInfo  =  inputVector[0]->GetInformationObject( i );
    vtkDataObject* data = inInfo->Get( vtkDataObject::DATA_OBJECT() );
    vtkDataObject* copy = 
      vtkDataObjectTypes::NewDataObject( data->GetDataObjectType() );
    copy->ShallowCopy(data);
    inData.push_back( vtkSmartPointer<vtkDataObject>::New() );
    inData[i].TakeReference(copy);

    vtkGraph* graph = vtkGraph::SafeDownCast( inData[i] );
    if ( graph )
      {
      totalPoints += graph->GetNumberOfVertices();
      }

    vtkPointSet* ptset = vtkPointSet::SafeDownCast( inData[i] );
    if ( ptset )
      {
      totalPoints += ptset->GetNumberOfPoints();
      }
    }
  int maxDepth = this->MaximumDepth;
  //maxDepth = (int)ceil(log(1.0 + 7.0*totalPoints/this->TargetLabelCount) / log(8.0));

  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );

  vtkLabelHierarchy* ouData = vtkLabelHierarchy::SafeDownCast(
    outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  if ( numInputs == 0 )
    {
    return 1;
    }

  if ( ! ouData )
    {
    vtkErrorMacro( "No output data" );
    return 0;
    }

  ouData->SetTargetLabelCount( this->TargetLabelCount );
  ouData->SetMaximumDepth( maxDepth );

  vtkSmartPointer<vtkPoints> pts =
    vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkDoubleArray> priorities =
    vtkSmartPointer<vtkDoubleArray>::New();
  priorities->SetName( "Priority" );
  vtkSmartPointer<vtkDoubleArray> size =
    vtkSmartPointer<vtkDoubleArray>::New();
  size->SetName( "LabelSize" );
  size->SetNumberOfComponents( 4 );
  vtkSmartPointer<vtkIntArray> type =
    vtkSmartPointer<vtkIntArray>::New();
  type->SetName( "Type" );
  vtkSmartPointer<vtkIntArray> idArr =
    vtkSmartPointer<vtkIntArray>::New();
  idArr->SetName( "ID" );
  vtkSmartPointer<vtkIntArray> iconIndex =
    vtkSmartPointer<vtkIntArray>::New();
  iconIndex->SetName( "IconIndex" );
  vtkSmartPointer<vtkStringArray> labelString =
    vtkSmartPointer<vtkStringArray>::New();
  labelString->SetName( "LabelText" );
  vtkSmartPointer<vtkUnicodeStringArray> labelUString =
    vtkSmartPointer<vtkUnicodeStringArray>::New();
  labelUString->SetName( "LabelText" );

  for ( int i = 0; i < numInputs; ++i )
    {
    if ( ! inData[i] )
      {
      vtkErrorMacro( "Null input data" );
      return 0;
      }

    vtkPoints* curPts = 0;
    vtkDataSetAttributes* pdata = 0;

    vtkGraph* graph = vtkGraph::SafeDownCast( inData[i] );
    if ( graph )
      {
      curPts = graph->GetPoints();
      pdata = graph->GetVertexData();
      //cout << "PS2LH graph " << graph << " points: " << curPts << "\n";
      }

    vtkPointSet* ptset = vtkPointSet::SafeDownCast( inData[i] );
    if ( ptset )
      {
      curPts = ptset->GetPoints();
      pdata = ptset->GetPointData();
      //cout << "PS2LH point set " << ptset << " points: " << curPts << "\n";
      }

    vtkDataArray* curPriorities = vtkDataArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess( 4*i+0, inputVector ) );
    vtkDataArray* curSize = vtkDataArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess( 4*i+1, inputVector ) );
    vtkAbstractArray* curStringOrIndex =
      this->GetInputAbstractArrayToProcess( 4*i+2, inputVector );
    vtkIntArray* curId = vtkIntArray::SafeDownCast(
      this->GetInputAbstractArrayToProcess( 4*i+3, inputVector ) );

    if ( curPts && curSize )
      {

      // We need an indicator of whether the third input array
      // is a label or index array. Currently, we will look for an
      // array name of "IconIndex".
      bool labels = true;
      if ( !strcmp( curStringOrIndex->GetName(), "IconIndex" ) )
        {
        labels = false;
        }

      vtkstd::vector<double> sz;
      int nc = curSize->GetNumberOfComponents();
      sz.resize( nc > 4 ? nc : 4 );
      for ( int c = nc; c < 4; ++ c )
        sz[c] = 0;
      for (int p = 0; p < curPts->GetNumberOfPoints(); ++p )
        {
        pts->InsertNextPoint( curPts->GetPoint(p) );
        if ( curPriorities )
          {
          priorities->InsertNextValue( curPriorities->GetTuple1( p ) );
          }
        else
          {
          priorities->InsertNextValue( 1.0 );
          }
        curSize->GetTuple( p, &sz[0] );
        size->InsertNextTuple( &sz[0] );
        if ( curId )
          {
          idArr->InsertNextValue( curId->GetValue( p ) );
          }
        else
          {
          idArr->InsertNextValue( 0 );
          }
        if ( labels )
          {
          type->InsertNextValue( 0 );
          iconIndex->InsertNextValue( 0 );
          if( this->UseUnicodeStrings )
            {
            labelUString->InsertNextValue(
              curStringOrIndex->GetVariantValue(p).ToUnicodeString() );
            }
          else
            {
            labelString->InsertNextValue(
              curStringOrIndex->GetVariantValue(p).ToString() );
            }
          }
        else // icons
          {
          type->InsertNextValue( 1 );
          iconIndex->InsertNextValue(
            curStringOrIndex->GetVariantValue(p).ToInt() );
          if( this->UseUnicodeStrings )
            {
            labelUString->InsertNextValue( vtkUnicodeString::from_utf8( "" ) );
            }
          else
            {
            labelString->InsertNextValue( "" );
            }
          }
        }
      }
    }

  //ouData->GetPointData()->ShallowCopy( pdata );
  if ( ! ouData->GetPoints() )
    {
    vtkPoints* oupts = vtkPoints::New();
    ouData->SetPoints( oupts );
    oupts->FastDelete();
    }
  ouData->GetPoints()->ShallowCopy( pts );
  ouData->SetPriorities( priorities );
  ouData->GetPointData()->AddArray( size );
  ouData->GetPointData()->AddArray( type );
  ouData->GetPointData()->AddArray( iconIndex );
  if( this->UseUnicodeStrings )
    {
    ouData->GetPointData()->AddArray( labelUString );
    }
  else
    {
    ouData->GetPointData()->AddArray( labelString );
    }
  ouData->GetPointData()->AddArray( idArr );
  ouData->ComputeHierarchy();

  timer->StopTimer();
  cout << "StartupTime: " << timer->GetElapsedTime() << endl;

  return 1;
}

