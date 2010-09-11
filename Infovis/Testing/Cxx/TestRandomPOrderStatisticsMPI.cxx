/*=========================================================================

Program:   Visualization Toolkit
Module:    TestRandomPOrderStatisticsMPI.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2009 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Philippe Pebay for implementing this test.

#include <mpi.h>

#include "vtkOrderStatistics.h"
#include "vtkPOrderStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

struct RandomOrderStatisticsArgs
{
  int nVals;
  double stdev;
  double absTol;
  int* retVal;
  int ioRank;
  int argc;
  char** argv;
};

// This will be called by all processes
void RandomOrderStatistics( vtkMultiProcessController* controller, void* arg )
{
  // Get test parameters
  RandomOrderStatisticsArgs* args = reinterpret_cast<RandomOrderStatisticsArgs*>( arg );
  *(args->retVal) = 0;

  // Get MPI communicator
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast( controller->GetCommunicator() );

  // Get local rank
  int myRank = com->GetLocalProcessId();

  // Seed random number generator
  vtkMath::RandomSeed( static_cast<int>( vtkTimerLog::GetUniversalTime() ) * ( myRank + 1 ) );

  // Generate an input table that contains samples of mutually independent discrete random variables
  int nVariables = 1;
  vtkIntArray* intArray[1];
  vtkStdString columnNames[] = { "Rounded Normal" };

  vtkTable* inputData = vtkTable::New();
  // Discrete rounded normal samples
  for ( int c = 0; c < nVariables; ++ c )
    {
    intArray[c] = vtkIntArray::New();
    intArray[c]->SetNumberOfComponents( 1 );
    intArray[c]->SetName( columnNames[c] );

    for ( int r = 0; r < args->nVals; ++ r )
      {
      intArray[c]->InsertNextValue( static_cast<int>( vtkMath::Round( vtkMath::Gaussian() * args->stdev ) ) );
      }

    inputData->AddColumn( intArray[c] );
    intArray[c]->Delete();
    }

  // ************************** Order Statistics **************************

  // Synchronize and start clock
  com->Barrier();
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  // Instantiate a parallel order statistics engine and set its ports
  vtkPOrderStatistics* pos = vtkPOrderStatistics::New();
  pos->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pos->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  // Select column pairs (uniform vs. uniform, normal vs. normal)
  pos->AddColumn( columnNames[0] );

  // Test (in parallel) with Learn, Derive, and Assess options turned on
  pos->SetLearnOption( true );
  pos->SetDeriveOption( true );
  pos->SetAssessOption( true );
  pos->SetTestOption( false );
  pos->SetNumericType( true ); // Data set is numeric
  pos->Update();

  // Synchronize and stop clock
  com->Barrier();
  timer->StopTimer();

  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Completed parallel calculation of order statistics (with assessment):\n"
         << "   Wall time: "
         << timer->GetElapsedTime()
         << " sec.\n";
    }

  // Now perform verifications
  vtkTable* outputHistogram = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );
  vtkTable* outputQuantiles = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 2 ) );

  int testIntValue;
  int numProcs = controller->GetNumberOfProcesses();

  // Verify that all processes have the same grand total and histograms size
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that all processes have the same grand total and histograms size.\n";
    }

  // Gather all grand totals
  int GT_l = outputQuantiles->GetValueByName( 0, "Cardinality" ).ToInt();
  int* GT_g = new int[numProcs];
  com->AllGather( &GT_l,
                  GT_g,
                  1 );

  // Use the first grand total as reference (as they all must be equal)
  testIntValue = GT_g[0];

  // Print out all grand totals
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    for ( int i = 0; i < numProcs; ++ i )
      {
      cout << "     On process "
           << i
           << ", grand total = "
           << GT_g[i]
           << ", histogram size = "
           << outputHistogram->GetNumberOfRows()
           << "\n";

      if ( GT_g[i] != testIntValue )
        {
        vtkGenericWarningMacro("Incorrect CDF.");
        *(args->retVal) = 1;
        }
      }
    }

  // Clean up
  pos->Delete();
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


  // Parameters for regression test.
  int testValue = 0;
  RandomOrderStatisticsArgs args;

  args.nVals = 1000000;
  args.stdev = 5.;
  args.absTol = 1.e-6;
  args.retVal = &testValue;
  args.ioRank = ioRank;
  args.argc = argc;
  args.argv = argv;

  // Check how many processes have been made available
  int numProcs = controller->GetNumberOfProcesses();
  if ( controller->GetLocalProcessId() == ioRank )
    {
    cout << "\n# Running test with "
         << numProcs
         << " processes and standard deviation = "
         << args.stdev
         << ".\n";
    }

  // Execute the function named "process" on both processes
  controller->SetSingleMethod( RandomOrderStatistics, &args );
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
