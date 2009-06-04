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

// For debugging purposes, output contingency table, which may be huge: it has the size O(span^2).
#define DEBUG_CONTINGENCY_TABLE 0
#define CONTINGENCY_BIG_CASE 1

struct RandomContingencyStatisticsArgs
{
  int nVals;
  double span;
  double absTol;
  int* retVal;
  int ioRank;
  int argc;
  char** argv;
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
  int nVariables = 3;
  vtkIntArray* intArray[3];
  vtkStdString columnNames[] = { "Rounded Normal 0", 
                                 "Rounded Normal 1",
                                 "Rounded Normal 2" };
  
  vtkTable* inputData = vtkTable::New();
  // Discrete rounded normal samples
  for ( int c = 0; c < nVariables; ++ c )
    {
    intArray[c] = vtkIntArray::New();
    intArray[c]->SetNumberOfComponents( 1 );
    intArray[c]->SetName( columnNames[c] );

    for ( int r = 0; r < args->nVals; ++ r )
      {
      intArray[c]->InsertNextValue( static_cast<int>( vtkMath::Round( vtkMath::Gaussian() * args->span ) ) );
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
  pcs->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pcs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  // Select column pairs (uniform vs. uniform, normal vs. normal)
  pcs->AddColumnPair( columnNames[0], columnNames[1] );
#if CONTINGENCY_BIG_CASE
#else // CONTINGENCY_BIG_CASE
  pcs->AddColumnPair( columnNames[0], columnNames[2] );
#endif // CONTINGENCY_BIG_CASE

  // Test (in parallel) with Learn, Derive, and Assess options turned on
  pcs->SetLearn( true );
  pcs->SetDerive( true );
  pcs->SetAssess( true );
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
  int testIntValue;
  double testDoubleValue;
  int numProcs = controller->GetNumberOfProcesses();

  // Verify that all processes have the same grand total
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that all processes have the same grand total.\n";
    }

  // Gather all grand totals
  int GT_l = outputContingency->GetValueByName( 0, "Cardinality" ).ToInt();
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
           << "\n";
      
      if ( GT_g[i] != testIntValue )
        {
        vtkGenericWarningMacro("Incorrect CDF.");
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
          testDoubleValue = H_g[nEntropies * i + 1] + H_g[nEntropies * i + 2]; // H(Y|X)+ H(X|Y)
          
          if ( testDoubleValue > H_g[nEntropies * i] )
            {
            vtkGenericWarningMacro("Reported inconsistent information entropies: H(X,Y) = " 
                                   << H_g[nEntropies * i]
                                   << " < " 
                                   << testDoubleValue 
                                   << " = H(Y|X)+ H(X|Y).");
            *(args->retVal) = 1;
            }
          }
        }

      // Clean up
      delete [] H_l;
      delete [] H_g;
      }
    }

  // Verify that the broadcasted reduced contingency tables all result in a CDF value of 1
  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Verifying that broadcasted CDF sum to 1 on all processes.\n";
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

  // Print out all CDFs
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
        testDoubleValue = cdf_g[i * nRowSumm + k];

        cout << "     On process "
             << i
             << ", CDF = "
             << testDoubleValue
             << "\n";

        // Verify that CDF = 1 (within absTol)
        if ( fabs ( 1. - testDoubleValue ) > args->absTol )
          {
          vtkGenericWarningMacro("Incorrect CDF.");
          *(args->retVal) = 1;
          }
        }
      }
    }
  
#if DEBUG_CONTINGENCY_TABLE
  outputContingency->Dump();
#endif // DEBUG_CONTINGENCY_TABLE

  // Clean up
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
  RandomContingencyStatisticsArgs args;

#if CONTINGENCY_BIG_CASE
  args.nVals = 1000000;
  args.span = 50.;
#else // CONTINGENCY_BIG_CASE
  args.nVals = 10;
  args.span = 3.;
#endif // CONTINGENCY_BIG_CASE

  args.absTol = 1.e-6;
  args.retVal = &testValue;
  args.ioRank = ioRank;
  args.argc = argc;
  args.argv = argv;

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
