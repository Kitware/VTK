/*=========================================================================

Program:   Visualization Toolkit
Module:    TestRandomPContingencyStatisticsMPI.cxx

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

#include "vtkContingencyStatistics.h"
#include "vtkPContingencyStatistics.h"

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

#include "vtksys/CommandLineArguments.hxx"

struct RandomContingencyStatisticsArgs
{
  int nVals;
  double stdev;
  double absTol;
  int* retVal;
  int ioRank;
};

// This will be called by all processes
void RandomContingencyStatistics( vtkMultiProcessController* controller, void* arg )
{
  // Get test parameters
  RandomContingencyStatisticsArgs* args = reinterpret_cast<RandomContingencyStatisticsArgs*>( arg );
  *(args->retVal) = 0;

  // Get MPI communicator
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast( controller->GetCommunicator() );

  // Get local rank
  int myRank = com->GetLocalProcessId();

  // Seed random number generator
  vtkMath::RandomSeed( static_cast<int>( vtkTimerLog::GetUniversalTime() ) * ( myRank + 1 ) );

  // Generate an input table that contains samples of mutually independent discrete random variables
  int nVariables = 2;
  vtkIntArray* intArray[2];
  vtkStdString columnNames[] = { "Rounded Normal 0",
                                 "Rounded Normal 1" };

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

  // Entropies in the summary table should normally be retrieved as follows:
  //   column 2: H(X,Y)
  //   column 3: H(Y|X)
  //   column 4: H(X|Y)
  int iEntropies[] = { 2,
                       3,
                       4 };
  int nEntropies = 3; // correct number of entropies reported in the summary table

  // ************************** Contingency Statistics **************************

  // Synchronize and start clock
  com->Barrier();
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  // Instantiate a parallel contingency statistics engine and set its ports
  vtkPContingencyStatistics* pcs = vtkPContingencyStatistics::New();
  pcs->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pcs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  // Select column pairs (uniform vs. uniform, normal vs. normal)
  pcs->AddColumnPair( columnNames[0], columnNames[1] );

  // Test (in parallel) with Learn, Derive, and Assess options turned on
  pcs->SetLearnOption( true );
  pcs->SetDeriveOption( true );
  pcs->SetAssessOption( true );
  pcs->Update();

  // Synchronize and stop clock
  com->Barrier();
  timer->StopTimer();

  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Completed parallel calculation of contingency statistics (with assessment):\n"
         << "   Wall time: "
         << timer->GetElapsedTime()
         << " sec.\n";
    }

  // Now perform verifications
  vtkTable* outputSummary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
  vtkTable* outputContingency = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );

  vtkIdType nRowSumm = outputSummary->GetNumberOfRows();
  double testDoubleValue1;
  double testDoubleValue2;
  int numProcs = controller->GetNumberOfProcesses();

  // Verify that all processes have the same grand total and contingency tables size
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that all processes have the same grand total and contingency tables size.\n";
    }

  // Gather all grand totals
  int GT_l = outputContingency->GetValueByName( 0, "Cardinality" ).ToInt();
  int* GT_g = new int[numProcs];
  com->AllGather( &GT_l,
                  GT_g,
                  1 );

  // Known global grand total
  int testIntValue = args->nVals * numProcs;

  // Print out all grand totals
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    for ( int i = 0; i < numProcs; ++ i )
      {
      cout << "     On process "
           << i
           << ", grand total = "
           << GT_g[i]
           << ", contingency table size = "
           << outputContingency->GetNumberOfRows()
           << "\n";

      if ( GT_g[i] != testIntValue )
        {
        vtkGenericWarningMacro("Incorrect grand total:"
                               << GT_g[i]
                               << " <> "
                               << testIntValue
                               << ")");
        *(args->retVal) = 1;
        }
      }
    }

  // Verify that information entropies on all processes make sense
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that information entropies are consistent on all processes.\n";
    }

  testIntValue = outputSummary->GetNumberOfColumns();

  if ( testIntValue != nEntropies + 2 )
    {
    vtkGenericWarningMacro("Reported an incorrect number of columns in the summary table: "
                           << testIntValue
                           << " != "
                           << nEntropies + 2
                           << ".");
    *(args->retVal) = 1;
    }
  else
    {
    // For each row in the summary table, fetch variable names and information entropies
    for ( vtkIdType k = 0; k < nRowSumm; ++ k )
      {
      // Get local information entropies from summary table
      double* H_l = new double[nEntropies];
      for ( vtkIdType c = 0; c < nEntropies; ++ c )
        {
        H_l[c] = outputSummary->GetValue( k, iEntropies[c] ).ToDouble();
        }

      // Gather all local entropies
      double* H_g = new double[nEntropies * numProcs];
      com->AllGather( H_l,
                      H_g,
                      nEntropies );

      // Print out all entropies
      if ( com->GetLocalProcessId() == args->ioRank )
        {
        // Get variable names
        cout << "   (X,Y) = ("
             << outputSummary->GetValue( k, 0 ).ToString()
             << ", "
             << outputSummary->GetValue( k, 1 ).ToString()
             << "):\n";

        for ( int i = 0; i < numProcs; ++ i )
          {
          cout << "     On process "
               << i;

          for ( vtkIdType c = 0; c < nEntropies; ++ c )
            {
            cout << ", "
                 << outputSummary->GetColumnName( iEntropies[c] )
                 << " = "
                 << H_g[nEntropies * i + c];
            }

          cout << "\n";

          // Make sure that H(X,Y) >= H(Y|X)+ H(X|Y)
          testDoubleValue1 = H_g[nEntropies * i + 1] + H_g[nEntropies * i + 2]; // H(Y|X)+ H(X|Y)

          if ( testDoubleValue1 > H_g[nEntropies * i] )
            {
            vtkGenericWarningMacro("Reported inconsistent information entropies: H(X,Y) = "
                                   << H_g[nEntropies * i]
                                   << " < "
                                   << testDoubleValue1
                                   << " = H(Y|X)+ H(X|Y).");
            *(args->retVal) = 1;
            }
          }
        cout << "   where H(X,Y) = - Sum_{x,y} p(x,y) log p(x,y) and H(X|Y) = - Sum_{x,y} p(x,y) log p(x|y).\n";
        }

      // Clean up
      delete [] H_l;
      delete [] H_g;
      }
    }

  // Verify that the local and global CDFs sum to 1 within presribed relative tolerance
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that local and global CDFs sum to 1 (within "
         << args->absTol
         << " absolute tolerance).\n";
    }

  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( outputContingency->GetColumnByName( "Key" ) );
  if ( ! keys )
    {
    cout << "*** Error: "
         << "Empty contingency table column 'Key' on process "
         << com->GetLocalProcessId()
         << ".\n";
    }

  vtkStdString proName = "P";
  vtkDoubleArray* prob = vtkDoubleArray::SafeDownCast( outputContingency->GetColumnByName( proName ) );
  if ( ! prob )
    {
    cout << "*** Error: "
         << "Empty contingency table column '"
         << proName
         << "' on process "
         << com->GetLocalProcessId()
         << ".\n";
    }

  // Calculate local CDFs
  double* cdf_l = new double[nRowSumm];
  for ( vtkIdType k = 0; k < nRowSumm; ++ k )
    {
    cdf_l[k] = 0;
    }

  int n = outputContingency->GetNumberOfRows();

  // Skip first entry which is reserved for the cardinality
  for ( vtkIdType r = 1; r < n; ++ r )
    {
    cdf_l[keys->GetValue( r )] += prob->GetValue( r );
    }

  // Gather all local CDFs
  double* cdf_g = new double[nRowSumm * numProcs];
  com->AllGather( cdf_l,
                  cdf_g,
                  nRowSumm );

  // Print out all local and global CDFs
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    for ( vtkIdType k = 0; k < nRowSumm; ++ k )
      {
      // Get variable names
      cout << "   (X,Y) = ("
           << outputSummary->GetValue( k, 0 ).ToString()
           << ", "
           << outputSummary->GetValue( k, 1 ).ToString()
           << "):\n";

      for ( int i = 0; i < numProcs; ++ i )
        {
        testDoubleValue1 = cdf_l[k];
        testDoubleValue2 = cdf_g[i * nRowSumm + k];

        cout << "     On process "
             << i
             << ", local CDF = "
             << testDoubleValue1
             << ", global CDF = "
             << testDoubleValue2
             << "\n";

        // Verify that local CDF = 1 (within absTol)
        if ( fabs ( 1. - testDoubleValue1 ) > args->absTol )
          {
          vtkGenericWarningMacro("Incorrect local CDF.");
          *(args->retVal) = 1;
          }

        // Verify that global CDF = 1 (within absTol)
        if ( fabs ( 1. - testDoubleValue2 ) > args->absTol )
          {
          vtkGenericWarningMacro("Incorrect global CDF.");
          *(args->retVal) = 1;
          }
        }
      }
    }

  // Clean up
  delete [] GT_g;
  delete [] cdf_l;
  delete [] cdf_g;
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

  MPI_Comm_get_attr( MPI_COMM_WORLD,
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
  double stdev = 5.;
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

  // Parse absolute tolerance to verify that final CDF is 1
  clArgs.AddArgument("--abs-tol",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &absTol, "Absolute tolerance to verify that final CDF is 1");

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
  RandomContingencyStatisticsArgs args;
  args.nVals = nVals;
  args.stdev = stdev;
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
  controller->SetSingleMethod( RandomContingencyStatistics, &args );
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
