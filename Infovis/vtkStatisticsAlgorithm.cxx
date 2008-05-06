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

vtkCxxRevisionMacro(vtkStatisticsAlgorithm, "1.2");

// ----------------------------------------------------------------------
vtkStatisticsAlgorithm::vtkStatisticsAlgorithm()
{
  this->SetNumberOfInputPorts( 2 );
  this->SetNumberOfOutputPorts( 1 );

  // If not told otherwise, run in Learn mode
  this->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
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
  // Extract dataset table
  vtkInformation* datasetInfo = inputVector[0]->GetInformationObject( 0 );
  vtkTable* dataset = vtkTable::SafeDownCast( datasetInfo->Get(vtkDataObject::DATA_OBJECT() ) );

  // Extract output table
  vtkInformation* outputInfo = outputVector->GetInformationObject( 0 );
  vtkTable* output = vtkTable::SafeDownCast( outputInfo->Get(vtkDataObject::DATA_OBJECT() ) );

  switch ( this->ExecutionMode )
    {
    case LearnMode:
      {
      this->ExecuteLearn( dataset, output );
      break;
      }
    case ValidateMode:
      {
      vtkWarningMacro( "Incorrect execution mode requested: "<<this->ExecutionMode<<". Doing nothing." );
      return 0;
      }
    case EvinceMode:
      {
      // Extract params table
      vtkInformation* paramsInfo = inputVector[1]->GetInformationObject( 0 );
      vtkTable* params = vtkTable::SafeDownCast( paramsInfo->Get(vtkDataObject::DATA_OBJECT() ) );
      this->ExecuteEvince( dataset, params, output );
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
