/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParallelRandomStatisticsMPI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories 
// for implementing this test.

#include <mpi.h>

#include "vtkPCorrelativeStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

struct RandomSampleStatisticsArgs
{
  int nVals;
  int* retVal;
  int ioRank;
  int argc;
  char** argv;
};

// This will be called by all processes
void RandomSampleStatistics( vtkMultiProcessController* controller, void* arg )
{
  // Get test parameters
  RandomSampleStatisticsArgs* args = reinterpret_cast<RandomSampleStatisticsArgs*>( arg );
  *(args->retVal) = 0;

  // Get MPI communicator
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast( controller->GetCommunicator() );

  // Get local rank
  int myRank = com->GetLocalProcessId();

  // Seed random number generator
  vtkMath::RandomSeed( static_cast<int>( vtkTimerLog::GetUniversalTime() ) * ( myRank + 1 ) );

  // Generate an input table that contains samples of mutually independent random variables over [0, 1]
  int nUniform = 2;
  int nNormal  = 2;
  int nVariables = nUniform + nNormal;

  vtkTable* inputData = vtkTable::New();
  vtkDoubleArray* doubleArray[4];
  vtkStdString columnNames[] = { "Standard Uniform 0", 
                                 "Standard Uniform 1",
                                 "Standard Normal 0",
                                 "Standard Normal 1" };
  
  // Standard uniform samples
  for ( int c = 0; c < nUniform; ++ c )
    {
    doubleArray[c] = vtkDoubleArray::New();
    doubleArray[c]->SetNumberOfComponents( 1 );
    doubleArray[c]->SetName( columnNames[c] );

    double x;
    for ( int r = 0; r < args->nVals; ++ r )
      {
      x = vtkMath::Random();
      doubleArray[c]->InsertNextValue( x );
      }
    
    inputData->AddColumn( doubleArray[c] );
    doubleArray[c]->Delete();
    }

  // Standard normal samples
  for ( int c = nUniform; c < nVariables; ++ c )
    {
    doubleArray[c] = vtkDoubleArray::New();
    doubleArray[c]->SetNumberOfComponents( 1 );
    doubleArray[c]->SetName( columnNames[c] );

    double x;
    for ( int r = 0; r < args->nVals; ++ r )
      {
      x = vtkMath::Gaussian();
      doubleArray[c]->InsertNextValue( x );
      }
    
    inputData->AddColumn( doubleArray[c] );
    doubleArray[c]->Delete();
    }

  // ************************** Correlative Statistics ************************** 

  // Synchronize and start clock
  com->Barrier();
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  // Instantiate a parallel correlative statistics engine and set its input
  vtkPCorrelativeStatistics* pcs = vtkPCorrelativeStatistics::New();
  pcs->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

  // Select column pairs (uniform vs. uniform, normal vs. normal)
  pcs->AddColumnPair( columnNames[0], columnNames[1] );
  pcs->AddColumnPair( columnNames[2], columnNames[3] );

  // Test (in parallel) with Learn, Derive, and Assess options turned on
  pcs->SetLearnOption( true );
  pcs->SetDeriveOption( true );
  pcs->SetAssessOption( true );
  pcs->Update();

  // Get output data and meta tables
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pcs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputPrimary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
  vtkTable* outputDerived = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );
  vtkTable* outputData = pcs->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );

    // Synchronize and stop clock
  com->Barrier();
  timer->StopTimer();

  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Completed parallel calculation of correlative statistics (with assessment):\n"
         << "   Total sample size: "
         << outputPrimary->GetValueByName( 0, "Cardinality" ).ToInt()   
         << " \n"
         << "   Wall time: "
         << timer->GetElapsedTime()
         << " sec.\n";

    cout << "   Calculated the following primary statistics:\n";
    for ( vtkIdType r = 0; r < outputPrimary->GetNumberOfRows(); ++ r )
      {
      cout << "   ";
      for ( int i = 0; i < outputPrimary->GetNumberOfColumns(); ++ i )
        {
        cout << outputPrimary->GetColumnName( i )
             << "="
             << outputPrimary->GetValue( r, i ).ToString()
             << "  ";
        }
      cout << "\n";
      }
    
    cout << "   Calculated the following derived statistics:\n";
    for ( vtkIdType r = 0; r < outputDerived->GetNumberOfRows(); ++ r )
      {
      cout << "   ";
      for ( int i = 0; i < outputDerived->GetNumberOfColumns(); ++ i )
        {
        cout << outputDerived->GetColumnName( i )
             << "="
             << outputDerived->GetValue( r, i ).ToString()
             << "  ";
        }
      cout << "\n";
      }
    }
  
  // Clean up
  pcs->Delete();
  inputData->Delete();
  timer->Delete();

}

//----------------------------------------------------------------------------
int main( int argc, char** argv )
{
  // **************************** MPI Initialization *************************** 
  vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize( &argc, &argv );

  // If an MPI controller was not created, terminate in error.
  if ( ! controller->IsA( "vtkMPIController" ) )
    {
    vtkGenericWarningMacro("Failed to initialize a MPI controller.");
    controller->Delete();
    return 1;
    } 

  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast( controller->GetCommunicator() );

  // ************************** Find an I/O node ******************************** 
  int* ioPtr;
  int ioRank;
  int flag;

  MPI_Attr_get( MPI_COMM_WORLD, 
                MPI_IO,
                &ioPtr,
                &flag );

  if ( ( ! flag ) || ( *ioPtr == MPI_PROC_NULL ) )
    {
    // Getting MPI attributes did not return any I/O node found.
    ioRank = MPI_PROC_NULL;
    vtkGenericWarningMacro("No MPI I/O nodes found.");

    // As no I/O node was found, we need an unambiguous way to report the problem.
    // This is the only case when a testValue of -1 will be returned
    controller->Finalize();
    controller->Delete();
    
    return -1;
    }
  else 
    {
    if ( *ioPtr == MPI_ANY_SOURCE )
      {
      // Anyone can do the I/O trick--just pick node 0.
      ioRank = 0;
      }
    else
      {
      // Only some nodes can do I/O. Make sure everyone agrees on the choice (min).
      com->AllReduce( ioPtr,
                      &ioRank,
                      1,
                      vtkCommunicator::MIN_OP );
      }
    }

  // ************************** Initialize test ********************************* 
  if ( com->GetLocalProcessId() == ioRank )
    {
    cout << "\n# Process "
         << ioRank
         << " will be the I/O node.\n";
    }
      
  // Check how many processes have been made available
  int numProcs = controller->GetNumberOfProcesses();
  if ( controller->GetLocalProcessId() == ioRank )
    {
    cout << "\n# Running test with "
         << numProcs
         << " processes...\n";
    }

  // Parameters for regression test.
  int testValue = 0;
  RandomSampleStatisticsArgs args;
  args.nVals = 100000;
  args.retVal = &testValue;
  args.ioRank = ioRank;
  args.argc = argc;
  args.argv = argv;

  // Execute the function named "process" on both processes
  controller->SetSingleMethod( RandomSampleStatistics, &args );
  controller->SingleMethodExecute();

  // Clean up and exit
  if ( com->GetLocalProcessId() == ioRank )
    {
    cout << "\n# Test completed.\n\n";
    }

  controller->Finalize();
  controller->Delete();
  
  return testValue;
}
