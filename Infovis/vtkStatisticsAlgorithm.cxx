/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStatisticsAlgorithm.cxx

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

#include "vtkToolkits.h"

#include "vtkStatisticsAlgorithm.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkStatisticsAlgorithm, "1.37");

// ----------------------------------------------------------------------
vtkStatisticsAlgorithm::vtkStatisticsAlgorithm()
{
  this->SetNumberOfInputPorts( 3 );
  this->SetNumberOfOutputPorts( 2 );

  // If not told otherwise, only run Learn option
  this->LearnOption = true;
  this->DeriveOption = true;
  this->FullWasDerived = false;
  this->AssessOption = false;
  this->AssessNames = vtkStringArray::New();
  this->AssessParameters = 0;
  this->Internals = new vtkStatisticsAlgorithmPrivate;
}

// ----------------------------------------------------------------------
vtkStatisticsAlgorithm::~vtkStatisticsAlgorithm()
{
  if ( this->AssessNames )
    {
    this->AssessNames->Delete();
    }
  delete this->Internals;
}

// ----------------------------------------------------------------------
void vtkStatisticsAlgorithm::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Learn: " << this->LearnOption << endl;
  os << indent << "Derive: " << this->DeriveOption << endl;
  os << indent << "FullWasDerived: " << this->FullWasDerived << endl;
  os << indent << "Assess: " << this->AssessOption << endl;
  if ( this->AssessParameters )
    {
    this->AssessParameters->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->AssessNames )
    {
    this->AssessNames->PrintSelf( os, indent.GetNextIndent() );
    }
  os << indent << "Internals: " << this->Internals << endl;
}

// ----------------------------------------------------------------------
void vtkStatisticsAlgorithm::SetAssessOptionParameter( vtkIdType id, vtkStdString name )
{
  if ( id >= 0 && id < this->AssessParameters->GetNumberOfValues() )
    {
    this->AssessParameters->SetValue( id, name );
    this->Modified();
    }
} 

// ----------------------------------------------------------------------
vtkStdString vtkStatisticsAlgorithm::GetAssessParameter( vtkIdType id )
{
  if ( id >= 0 && id < this->AssessParameters->GetNumberOfValues() )
    {
    return this->AssessParameters->GetValue( id );
    }
  return 0;
} 

// ----------------------------------------------------------------------
bool vtkStatisticsAlgorithm::SetParameter( const char* vtkNotUsed(parameter),
                                           int vtkNotUsed(index),
                                           vtkVariant vtkNotUsed(value) )
{ 
  return false;
}

// ----------------------------------------------------------------------
int vtkStatisticsAlgorithm::RequestData( vtkInformation*,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector )
{
  // Extract input data table
  vtkTable* inData = vtkTable::GetData( inputVector[INPUT_DATA], 0 );
  if ( ! inData )
    {
    return 1;
    }

  vtkTable* inParameters = vtkTable::GetData( inputVector[LEARN_PARAMETERS], 0 );
  //
  // Extract output tables
  vtkTable* outData = vtkTable::GetData( outputVector, 0 );
  vtkDataObject* outMeta1 = vtkDataObject::GetData( outputVector, 1 );
  vtkDataObject* outMeta2 = 0; // Unused for now

  outData->ShallowCopy( inData );

  // If there are any columns selected in the buffer which have not been
  // turned into a request by RequestSelectedColumns(), add them now.
  // There should be no effect if vtkStatisticsAlgorithmPrivate::Buffer is empty.
  // This is here to accommodate the simpler user interfaces in OverView for
  // univariate and bivariate algorithms which will not call RequestSelectedColumns()
  // on their own.
  this->RequestSelectedColumns();

  vtkDataObject* inMeta;
  if ( this->LearnOption )
    {
    this->ExecuteLearn( inData, inParameters, outMeta1 );

    // The full model (if available) is no longer in sync
    this->FullWasDerived = false;
    }
  else
    {
    // Extract input meta table
    inMeta = vtkDataObject::GetData( inputVector[INPUT_MODEL], 0 );

    if ( ! inMeta )
      {
      vtkErrorMacro( "No model available AND no Learn phase requested. Cannot proceed with statistics algorithm." );
      return 1;
      }

    outMeta1->ShallowCopy( inMeta );

    // A full model was not derived so far
    this->FullWasDerived = false;
    }

  if ( this->DeriveOption )
    {
    this->ExecuteDerive( outMeta1 );

    // A full model was derived from the minimal model
    this->FullWasDerived = true;
    }

  if ( this->AssessOption )
    {
    this->ExecuteAssess( inData, outMeta1, outData, outMeta2 );
    }

  return 1;
}

// ----------------------------------------------------------------------
int vtkStatisticsAlgorithm::FillInputPortInformation( int port, vtkInformation* info )
{
  if ( port == INPUT_DATA )
    {
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable" );
    return 1;
    }
  else if ( port == LEARN_PARAMETERS )
    {
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable" );
    return 1;
    }
  else if ( port == INPUT_MODEL )
    {
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable" );
    return 1;
    }
  return 0;
}

// ----------------------------------------------------------------------
int vtkStatisticsAlgorithm::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port >= 0 )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkStatisticsAlgorithm::SetColumnStatus( const char* namCol, int status )
{
  this->Internals->SetBufferColumnStatus( namCol, status );
}

//---------------------------------------------------------------------------
void vtkStatisticsAlgorithm::ResetAllColumnStates()
{
  this->Internals->ResetBuffer();
}

//---------------------------------------------------------------------------
int vtkStatisticsAlgorithm::RequestSelectedColumns()
{
  return this->Internals->AddBufferToRequests();
}

//---------------------------------------------------------------------------
void vtkStatisticsAlgorithm::ResetRequests()
{
  this->Internals->ResetRequests();
}

vtkIdType vtkStatisticsAlgorithm::GetNumberOfRequests()
{
  return this->Internals->GetNumberOfRequests();
}

vtkIdType vtkStatisticsAlgorithm::GetNumberOfColumnsForRequest( vtkIdType request )
{
  return this->Internals->GetNumberOfColumnsForRequest( request );
}

const char* vtkStatisticsAlgorithm::GetColumnForRequest( vtkIdType r, vtkIdType c )
{
  static vtkStdString columnName;
  if ( this->Internals->GetColumnForRequest( r, c, columnName ) )
    {
    return columnName.c_str();
    }
  return 0;
}

int vtkStatisticsAlgorithm::GetColumnForRequest( vtkIdType r, vtkIdType c, vtkStdString& columnName )
{
  return this->Internals->GetColumnForRequest( r, c, columnName ) ? 1 : 0;
}
