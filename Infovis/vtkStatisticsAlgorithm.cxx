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
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkStatisticsAlgorithm, "1.10");

// ----------------------------------------------------------------------
vtkStatisticsAlgorithm::vtkStatisticsAlgorithm()
{
  this->SetNumberOfInputPorts( 2 );
  this->SetNumberOfOutputPorts( 2 );

  // If not told otherwise, run in Learn mode
  this->ExecutionMode = vtkStatisticsAlgorithm::LearnMode;
}

// ----------------------------------------------------------------------
vtkStatisticsAlgorithm::~vtkStatisticsAlgorithm()
{
}

// ----------------------------------------------------------------------
void vtkStatisticsAlgorithm::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "ExecutionMode: " << this->ExecutionMode << endl;
  os << indent << "SampleSize: " << this->SampleSize << endl;
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
    vtkWarningMacro( "Input port 0 is null. Doing nothing." );
    return 0;
    }

  // Extract output tables
  vtkTable* outData = vtkTable::GetData( outputVector, 0 );
  vtkTable* outMeta = vtkTable::GetData( outputVector, 1 );

  outData->ShallowCopy( inData );

  switch ( this->ExecutionMode )
    {
    case LearnMode:
      {
      this->ExecuteLearn( inData, outMeta );
      break;
      }
    case AssessMode:
      {
      // Extract additional tables
      vtkTable* inMeta = vtkTable::GetData( inputVector[1], 0 );
      if ( ! inMeta )
        {
        vtkWarningMacro( "Input port 1 is null. Doing nothing." );
        return 0;
        }

      this->ExecuteAssess( inData, inMeta, outData, outMeta );
      break;
      }
    default:
      vtkWarningMacro( "Incorrect execution mode requested: "<<this->ExecutionMode<<". Doing nothing." );
      return 0;
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
  if ( port == 0 )
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
    }
  else if ( port == 1 )
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

// ----------------------------------------------------------------------
void vtkStatisticsAlgorithm::SetExecutionMode( vtkIdType em )
{
  switch ( em )
    {
    case vtkStatisticsAlgorithm::LearnMode:
      break;
    case vtkStatisticsAlgorithm::AssessMode:
      break;
    default:
      vtkWarningMacro( "Incorrect type of execution mode: "
                       <<em
                       <<". Ignoring it." );
      return;
    }
  
  this->ExecutionMode =  static_cast<vtkStatisticsAlgorithm::ExecutionModeType>( em );

  return;
}
