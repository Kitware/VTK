/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkStreamingStatistics.h"

#include "vtkDataObjectCollection.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkStreamingStatistics);

vtkCxxSetObjectMacro(vtkStreamingStatistics,
                     StatisticsAlgorithm,
                     vtkStatisticsAlgorithm);

// ----------------------------------------------------------------------
vtkStreamingStatistics::vtkStreamingStatistics()
{
  // Setup input/output ports
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(3);

  // Initialize internal stats algorithm to NULL
  this->StatisticsAlgorithm = 0;
  this->SetStatisticsAlgorithm(0);

  // Initialize internal model
  this->InternalModel = vtkMultiBlockDataSet::New();
}

// ----------------------------------------------------------------------
vtkStreamingStatistics::~vtkStreamingStatistics()
{
  // Release/delete internal stats algorithm
  this->SetStatisticsAlgorithm(0);
  this->StatisticsAlgorithm = 0;

  // Release/delete internal model to NULL
  this->InternalModel->Delete();
  this->InternalModel = 0;
}

// ----------------------------------------------------------------------
int vtkStreamingStatistics::FillInputPortInformation( int port, vtkInformation* info )
{
  if ( port == INPUT_DATA )
    {
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable" );
    return 1;
    }
  else if ( port == INPUT_MODEL )
    {
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet" );
    return 1;
    }
  else if ( port == LEARN_PARAMETERS )
    {
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable" );
    return 1;
    }

  return 0;
}

// ----------------------------------------------------------------------
int vtkStreamingStatistics::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port == OUTPUT_DATA )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
    }
  else if ( port == OUTPUT_MODEL )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
    return 1;
    }
  else if ( port == OUTPUT_TEST )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
    }

  return 0;
}

// ----------------------------------------------------------------------
int vtkStreamingStatistics::RequestData( vtkInformation*,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector )
{
  // Input handles
  vtkTable*             inData       = vtkTable::GetData( inputVector[INPUT_DATA], 0 );

  // Output handles
  vtkTable*             outData  = vtkTable::GetData( outputVector, OUTPUT_DATA );
  vtkMultiBlockDataSet* outModel = vtkMultiBlockDataSet::GetData( outputVector, OUTPUT_MODEL );

  // These will be used later
  /*
  vtkMultiBlockDataSet* inModel      = vtkMultiBlockDataSet::GetData( inputVector[INPUT_MODEL], 0 );
  */
  vtkDataObject*        inParameters = vtkDataObject::GetData( inputVector[LEARN_PARAMETERS], 0 );
  vtkTable*             outTest  = vtkTable::GetData( outputVector, OUTPUT_TEST );



  // Note: Experimental code. Lots of use case are currently not handled in
  // any way and there are a lot of assumptions made about this or that

  // Make sure the statistics algorithm is set
  if ( !this->StatisticsAlgorithm )
    {
    vtkErrorMacro("StatisticsAlgorithm not set! Punting!")
    cerr << "StatisticsAlgorithm not set! Punting!" << endl;
    return 0;
    }

  // Set the input into my stats algorithms
  this->StatisticsAlgorithm->SetInput(inData);
  this->StatisticsAlgorithm->SetLearnOptionParameters( inParameters );
  this->StatisticsAlgorithm->SetInputModel( this->InternalModel );

  // Force an update
  this->StatisticsAlgorithm->Update();

  // Grab (DeepCopy) the model for next time
  this->InternalModel->DeepCopy( this->StatisticsAlgorithm->GetOutputDataObject( OUTPUT_MODEL ) );

  // Shallow copy the internal output to external output
  outData->ShallowCopy( this->StatisticsAlgorithm->GetOutput( OUTPUT_DATA ) );
  outModel->ShallowCopy( this->StatisticsAlgorithm->GetOutputDataObject( OUTPUT_MODEL ) );
  outTest->ShallowCopy( this->StatisticsAlgorithm->GetOutput( OUTPUT_TEST ) );

  return 1;
}




// ----------------------------------------------------------------------
void vtkStreamingStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  if (this->StatisticsAlgorithm)
    {
    os << indent << "StatisticsAlgorithm:\n";
    vtkIndent i2 = indent.GetNextIndent();
    this->StatisticsAlgorithm->PrintSelf(os,i2);
    }
  os << indent << "InternalModel: " << this->InternalModel << "\n";
}
