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

vtkCxxRevisionMacro(vtkStatisticsAlgorithm, "1.32");

// ----------------------------------------------------------------------
vtkStatisticsAlgorithm::vtkStatisticsAlgorithm()
{
  this->SetNumberOfInputPorts( 2 );
  this->SetNumberOfOutputPorts( 2 );

  // If not told otherwise, only run Learn option
  this->Learn = true;
  this->Derive = true;
  this->FullWasDerived = false;
  this->Assess = false;
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
  os << indent << "Learn: " << this->Learn << endl;
  os << indent << "Derive: " << this->Derive << endl;
  os << indent << "FullWasDerived: " << this->FullWasDerived << endl;
  os << indent << "Assess: " << this->Assess << endl;
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
void vtkStatisticsAlgorithm::SetAssessParameter( vtkIdType id, vtkStdString name )
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
int vtkStatisticsAlgorithm::RequestData( vtkInformation*,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector )
{
  // Extract input data table
  vtkTable* inData = vtkTable::GetData( inputVector[0], 0 );
  if ( ! inData )
    {
    return 1;
    }

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
  if ( this->Learn )
    {
    this->ExecuteLearn( inData, outMeta1 );

    // The full model (if available) is no longer in sync
    this->FullWasDerived = false;
    }
  else
    {
    // Extract input meta table
    inMeta = vtkDataObject::GetData( inputVector[1], 0 );

    if ( ! inMeta )
      {
      vtkErrorMacro( "No model available AND no Learn phase requested. Cannot proceed with statistics algorithm." );
      return 1;
      }

    outMeta1->ShallowCopy( inMeta );

    // A full model was not derived so far
    this->FullWasDerived = false;
    }

  if ( this->Derive )
    {
    this->ExecuteDerive( outMeta1 );

    // A full model was derived from the minimal model
    this->FullWasDerived = true;
    }

  if ( this->Assess )
    {
    this->ExecuteAssess( inData, outMeta1, outData, outMeta2 );
    }

  return 1;
}

// ----------------------------------------------------------------------
int vtkStatisticsAlgorithm::FillInputPortInformation( int port, vtkInformation* info )
{
  if ( port == 0 )
    {
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable" );
    return 1;
    }
  else if ( port == 1 )
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
void vtkStatisticsAlgorithm::SetInputStatisticsConnection( vtkAlgorithmOutput* in )
{ 
  this->SetInputConnection( 1, in );
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

