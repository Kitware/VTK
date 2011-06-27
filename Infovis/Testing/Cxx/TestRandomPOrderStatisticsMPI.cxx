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
 * Copyright 2011 Sandia Corporation.
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

#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

#include "vtksys/CommandLineArguments.hxx"

struct RandomOrderStatisticsArgs
{
  int nVals;
  double stdev;
  bool quantize;
  int maxHistoSize;
  double absTol;
  int* retVal;
  int ioRank;
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

  // Generate an input table that contains samples of a truncated Gaussian pseudo-random variable
  int nVariables = 1;
  vtkIntArray* intArray[1];
  vtkStdString columnNames[] = { "Rounded Normal" };

  // Storage for local extrema
  int* min_l = new int[nVariables];
  int* max_l = new int[nVariables];

  vtkTable* inputData = vtkTable::New();
  // Discrete rounded normal samples
  for ( int c = 0; c < nVariables; ++ c )
    {
    intArray[c] = vtkIntArray::New();
    intArray[c]->SetNumberOfComponents( 1 );
    intArray[c]->SetName( columnNames[c] );

    // Store first value
    int v = static_cast<int>( vtkMath::Round( vtkMath::Gaussian() * args->stdev ) );
    intArray[c]->InsertNextValue( v );

    // Initialize local extrema
    min_l[c] = v;
    max_l[c] = v;

    // Continue up to nVals values have been generated
    for ( int r = 1; r < args->nVals; ++ r )
      {
      // Store new value
      v = static_cast<int>( vtkMath::Round( vtkMath::Gaussian() * args->stdev ) );
      intArray[c]->InsertNextValue( v );

      // Update local extrema
      if ( v < min_l[c] )
        {
        min_l[c] = v;
        }
      else if ( v > max_l[c] )
        {
        max_l[c] = v;
        }
      }

    inputData->AddColumn( intArray[c] );
    intArray[c]->Delete();
    }

  // Reduce all minima for this variable
  int min_g;
  com->AllReduce( min_l,
                  &min_g,
                  1,
                  vtkCommunicator::MIN_OP );

  // Reduce all maxima for this variable
  int max_g;
  com->AllReduce( max_l,
                  &max_g,
                  1,
                  vtkCommunicator::MAX_OP );

  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Generated pseudo-random sample which globally ranges from "
         << min_g
         << " to "
         << max_g
         << ".\n";
    }

  // ************************** Order Statistics **************************

  // Synchronize and start clock
  com->Barrier();
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  // Instantiate a parallel order statistics engine and set its ports
  vtkPOrderStatistics* pos = vtkPOrderStatistics::New();
  pos->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );
  vtkMultiBlockDataSet* outputModelDS = vtkMultiBlockDataSet::SafeDownCast( pos->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  // Select column pairs (uniform vs. uniform, normal vs. normal)
  pos->AddColumn( columnNames[0] );

  // Test (in parallel) with Learn, Derive, and Assess options turned on
  pos->SetLearnOption( true );
  pos->SetDeriveOption( true );
  pos->SetAssessOption( false );
  pos->SetTestOption( false );
  pos->SetQuantize( args->quantize );
  pos->SetMaximumHistogramSize( args->maxHistoSize );
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
  vtkTable* outputHistogram = vtkTable::SafeDownCast( outputModelDS->GetBlock( 0 ) );
  unsigned nbq = outputModelDS->GetNumberOfBlocks() - 1;
  vtkTable* outputCard = vtkTable::SafeDownCast( outputModelDS->GetBlock( nbq - 1 ) );
  vtkTable* outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( nbq ) );

  // Verify that all processes have the same grand total and histograms size
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that all processes have the same grand total and histograms size.\n";
    }

  // Gather all cardinalities
  int numProcs = controller->GetNumberOfProcesses();
  int card_l = outputCard->GetValueByName( 0, "Cardinality" ).ToInt();
  int* card_g = new int[numProcs];
  com->AllGather( &card_l,
                  card_g,
                  1 );

  // Known global cardinality
  int testIntValue = args->nVals * numProcs;

  // Print out and verify all cardinalities
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    for ( int i = 0; i < numProcs; ++ i )
      {
      cout << "   On process "
           << i
           << ", cardinality = "
           << card_g[i]
           << ", histogram size = "
           << outputHistogram->GetNumberOfRows()
           << "\n";

      if ( card_g[i] != testIntValue )
        {
        vtkGenericWarningMacro("Incorrect cardinality:"
                               << card_g[i]
                               << " <> "
                               << testIntValue
                               << ")");
        *(args->retVal) = 1;
        }
      }
    }

  // Print out and verify global extrema
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that calculated global extrema are correct (within "
         << args->absTol
         << " absolute tolerance).\n";

    double min_c = outputQuantiles->GetValueByName( 0,
                                                    columnNames[0] ).ToDouble();

    double max_c = outputQuantiles->GetValueByName( outputQuantiles->GetNumberOfRows() - 1 ,
                                                    columnNames[0] ).ToDouble();

    cout << "   Calculated minimum = "
           << min_c
           << ", maximum = "
           << max_c
           << "\n";

    if ( fabs( min_c - min_g ) > args->absTol )
        {
        vtkGenericWarningMacro("Incorrect minimum.");
        *(args->retVal) = 1;
        }

    if ( fabs( max_c - max_g ) > args->absTol )
        {
        vtkGenericWarningMacro("Incorrect maximum.");
        *(args->retVal) = 1;
        }
    }

  // Clean up
  delete [] card_g;
  delete [] min_l;
  delete [] max_l;
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

  // **************************** Parse command line ***************************
  // Set default argument values
  int nVals = 100000;
  double stdev = 50.;
  bool quantize = false;
  int maxHistoSize = 500;
  double absTol = 1.e-6;

  // Initialize command line argument parser
  vtksys::CommandLineArguments clArgs;
  clArgs.Initialize( argc, argv );
  clArgs.StoreUnusedArguments( false );

  // Parse per-process cardinality of each pseudo-random sample
  clArgs.AddArgument("--n-per-proc",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &nVals, "Per-process cardinality of each pseudo-random sample");

  // Parse standard deviation of each pseudo-random sample
  clArgs.AddArgument("--std-dev",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &stdev, "Standard deviation of each pseudo-random sample");

  // Parse maximum histogram size
  clArgs.AddArgument("--max-histo-size",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &maxHistoSize, "Maximum histogram size (when re-quantizing is allowed)");

  // Parse whether quantization should be used (to reduce histogram size)
  clArgs.AddArgument("--quantize",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &quantize, "Allow re-quantizing");

  // Parse absolute tolerance to verify extrema
  clArgs.AddArgument("--abs-tol",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &absTol, "Absolute tolerance to verify extrema");

  // If incorrect arguments were provided, provide some help and terminate in error.
  if ( ! clArgs.Parse() )
    {
    if ( com->GetLocalProcessId() == ioRank )
      {
      cerr << "Usage: " 
           << clArgs.GetHelp()
           << "\n";
      }

    controller->Finalize();
    controller->Delete();

    return 1;
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
  args.nVals = nVals;
  args.stdev = stdev;
  args.quantize = quantize;
  args.maxHistoSize = maxHistoSize;
  args.retVal = &testValue;
  args.ioRank = ioRank;
  args.absTol = absTol;

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
