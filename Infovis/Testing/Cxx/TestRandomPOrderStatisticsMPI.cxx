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
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

#include "vtksys/CommandLineArguments.hxx"

struct RandomOrderStatisticsArgs
{
  int nVals;
  double stdev;
  bool skipInt;
  bool skipString;
  bool quantize;
  int maxHistoSize;
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

  // Generate an input table that contains samples of:
  // 1. A truncated Gaussian pseudo-random variable (vtkIntArray)
  // 2. A uniform pseudo-random variable of characters (vtkStringArray)
  vtkStdString columnNames[] = { "Rounded Normal Integer", "Uniform Character" };

  // Infer number and type of generated variables based on command line options
  int nVariables = 0;
  vtksys_stl::map<int,bool> isVariableAString;
  if ( ! args->skipInt )
    {
    isVariableAString[nVariables] = false;
    ++ nVariables;
    }
  if ( ! args->skipString )
    {
    isVariableAString[nVariables] = true;
    ++ nVariables;
    }

  // Prepare column of integers
  vtkIntArray* intArray = vtkIntArray::New();
  intArray->SetNumberOfComponents( 1 );
  intArray->SetName( columnNames[0] );
  
  // Prepare column of strings
  vtkStringArray* strArray = vtkStringArray::New();
  strArray->SetNumberOfComponents( 1 );
  strArray->SetName( columnNames[1] );

  // Storage for pseudo-random values and local extrema
  int* v = new int[nVariables];
  int* min_l = new int[nVariables];
  int* max_l = new int[nVariables];
  
  // Initial current variable index
  int idx = 0;

  // Store first integer value
  if ( ! args->skipInt )
    {
    v[idx] = static_cast<int>( vtkMath::Round( vtkMath::Gaussian() * args->stdev ) );
    intArray->InsertNextValue( v[idx] );
    ++ idx;
    }
  
  // Store first string value
  if ( ! args->skipString )
    {
    v[idx] = 96 + vtkMath::Ceil( vtkMath::Random() * 26 );
    char c = static_cast<char>( v[idx] );
    vtkStdString s( &c, 1 );
    strArray->InsertNextValue( s );
    }

  // Initialize local extrema
  for ( int i = 0; i < nVariables; ++ i )
    {
    min_l[i] = v[i];
    max_l[i] = v[i];
    }
  
  // Continue up to nVals values have been generated
  for ( int r = 1; r < args->nVals; ++ r )
    {
    // Initial current variable index
    idx = 0;

    // Store current integer value
    if ( ! args->skipInt )
      {
      v[idx] = static_cast<int>( vtkMath::Round( vtkMath::Gaussian() * args->stdev ) );
      intArray->InsertNextValue( v[idx] );
      ++ idx;
      }

    // Store current string value
    if ( ! args->skipString )
      {
      v[idx] = 96 + vtkMath::Ceil( vtkMath::Random() * 26 );
      char c = static_cast<char>( v[idx] );
      vtkStdString s( &c, 1 );
      strArray->InsertNextValue( s );
      }

    // Update local extrema
    for ( int i = 0; i < nVariables; ++ i )
      {
      if ( v[i] < min_l[i] )
        {
        min_l[i] = v[i];
        }
      else if ( v[i] > max_l[i] )
        {
        max_l[i] = v[i];
        }
      } // i
    } // r

  // Create input table
  vtkTable* inputData = vtkTable::New();
  if ( ! args->skipInt )
    {
    inputData->AddColumn( intArray );
    }
  if ( ! args->skipString )
    {
    inputData->AddColumn( strArray );
    }

  // Storage for global extrema
  int* min_g = new int[nVariables];
  int* max_g = new int[nVariables];

  // Reduce extrema for all variables
  com->AllReduce( min_l,
                  min_g,
                  nVariables,
                  vtkCommunicator::MIN_OP );
  
  com->AllReduce( max_l,
                  max_g,
                  nVariables,
                  vtkCommunicator::MAX_OP );

  if ( myRank == args->ioRank )
    {
    cout << "\n## Generated pseudo-random samples with following ranges:\n";
    for ( int i = 0; i < nVariables; ++ i )
      {
      cout << "   "
           << columnNames[i]
           << ": ";
      if ( isVariableAString[i] )
        {
        cout << static_cast<char>( min_g[i] )
             << " to "
             << static_cast<char>( max_g[i] );
        }
      else
        {
        cout <<  min_g[i]
             << " to "
             << max_g[i];
        }
      cout << "\n";
      } // i
    } // if ( myRank == args->ioRank )

  // Clean up
  delete [] v;  
  delete [] min_l;
  delete [] max_l;
  intArray->Delete();
  strArray->Delete();


  // ************************** Order Statistics **************************

  // Synchronize and start clock
  com->Barrier();
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  // Instantiate a parallel order statistics engine and set its ports
  vtkPOrderStatistics* pos = vtkPOrderStatistics::New();
  pos->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );
  vtkMultiBlockDataSet* outputModelDS = vtkMultiBlockDataSet::SafeDownCast( pos->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  // Select columns of interest depending on command line choices
  if ( ! args->skipInt )
    {
    pos->AddColumn( columnNames[0] );
    }
  if ( ! args->skipString )
    {
    pos->AddColumn( columnNames[1] );
    }

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

  if ( myRank == args->ioRank )
    {
    cout << "\n## Completed parallel calculation of order statistics (with assessment):\n"
         << "   Wall time: "
         << timer->GetElapsedTime()
         << " sec.\n";
    }

  // If no variables were requested, terminate here (only made sure that empty input worked)
  if ( ! nVariables )
    {
    pos->Delete();
    inputData->Delete();
    timer->Delete();

    return;
    }

  // Now perform verifications
  vtkTable* outputCard = vtkTable::SafeDownCast( outputModelDS->GetBlock( nVariables ) );

  // Verify that all processes have the same grand total and histograms size
  if ( myRank == args->ioRank )
    {
    cout << "\n## Verifying that all processes have the same grand total and histograms size.\n";
    }

  // Gather all cardinalities
  int numProcs = controller->GetNumberOfProcesses();
  int card_l = outputCard->GetValueByName( 0, "Cardinality" ).ToInt();
  int* card_g = new int[numProcs];
  com->AllGather( &card_l, card_g, 1 );

  // Known global cardinality
  int testIntValue = args->nVals * numProcs;

  // Verify histogram cardinalities for each variable
  for ( int i = 0; i < nVariables; ++ i )
    {
    if ( myRank == args->ioRank )
      {
      cout << "   "
           << columnNames[i]
           << ":\n";
      }  // if ( myRank == args->ioRank )

    vtkTable* outputHistogram = vtkTable::SafeDownCast( outputModelDS->GetBlock( i ) );
    // Print out and verify all cardinalities
    if ( myRank == args->ioRank )
      {
      for ( int p = 0; p < numProcs; ++ p )
        {
        cout << "     On process "
             << p
             << ", cardinality = "
             << card_g[p]
             << ", histogram size = "
             << outputHistogram->GetNumberOfRows()
             << "\n";
        
        if ( card_g[p] != testIntValue )
          {
          vtkGenericWarningMacro("Incorrect cardinality:"
                                 << card_g[p]
                                 << " <> "
                                 << testIntValue
                                 << ")");
          *(args->retVal) = 1;
          }
        } // p
      } // if ( myRank == args->ioRank )
    } // i

  // Print out and verify global extrema
  vtkTable* outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( nVariables + 1 ) );
  if ( myRank == args->ioRank )
    {
    cout << "\n## Verifying that calculated global ranges are correct:\n";
    for ( int i = 0; i < nVariables; ++ i )
      {
      vtkVariant min_c = outputQuantiles->GetValue( 0, 
                                                    i + 1 );
      
      vtkVariant max_c = outputQuantiles->GetValue( outputQuantiles->GetNumberOfRows() - 1 ,
                                                    i + 1 );
      
      // Print out computed range
      cout << "   "
           << columnNames[i]
           << ": "
           << min_c
           << " to "
           << max_c
           << "\n";

      // Check minimum
      if ( min_c.IsString() )
        {
        char c = static_cast<char>( min_g[i] );
        if ( min_c.ToString() != vtkStdString( &c, 1 ) )
          {
          vtkGenericWarningMacro("Incorrect calculated minimum for variable "
                                 << columnNames[i]
                                 << ": "
                                 << min_c.ToString()
                                 << " <> "
                                 << vtkStdString( &c, 1 ) );
          *(args->retVal) = 1;
          }
        } // if ( min_c.IsString() )
      else
        {
        if ( min_c != min_g[i] )
          {
          vtkGenericWarningMacro("Incorrect calculated minimum for variable "
                                 << columnNames[i]
                                 << ": "
                                 << min_c
                                 << " <> "
                                 << min_g[i]);
          *(args->retVal) = 1;
          }
        } // else
      
      // Check maximum
      if ( max_c.IsString() )
        {
        char c = static_cast<char>( max_g[i] );
        if ( max_c.ToString() != vtkStdString( &c, 1 ) )
          {
          vtkGenericWarningMacro("Incorrect calculated maximum for variable "
                                 << columnNames[i]
                                 << ": "
                                 << max_c.ToString()
                                 << " <> "
                                 << vtkStdString( &c, 1 ) );
          *(args->retVal) = 1;
          }
        }
      else
        {
        if ( max_c != max_g[i] )
          {
          vtkGenericWarningMacro("Incorrect calculated maximum for variable "
                                 << columnNames[i]
                                 << ": "
                                 << max_c
                                 << " <> "
                                 << max_g[i]);
          *(args->retVal) = 1;
          } //  ( max_c.IsString() )
        } // else
      } // i
    } // if ( myRank == args->ioRank )

  // Clean up
  delete [] card_g;
  delete [] min_g;
  delete [] max_g;
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
  bool skipInt = false;
  bool skipString = false;
  double stdev = 50.;
  bool quantize = false;
  int maxHistoSize = 500;

  // Initialize command line argument parser
  vtksys::CommandLineArguments clArgs;
  clArgs.Initialize( argc, argv );
  clArgs.StoreUnusedArguments( false );

  // Parse per-process cardinality of each pseudo-random sample
  clArgs.AddArgument("--n-per-proc",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &nVals, "Per-process cardinality of each pseudo-random sample");

  // Parse whether integer variable should be skipped
  clArgs.AddArgument("--skip-int",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipInt, "Skip integer variable");

  // Parse whether string variable should be skipped
  clArgs.AddArgument("--skip-string",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipString, "Skip string variable");

  // Parse standard deviation of pseudo-random Gaussian sample
  clArgs.AddArgument("--std-dev",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &stdev, "Standard deviation of pseudo-random Gaussian sample");

  // Parse maximum histogram size
  clArgs.AddArgument("--max-histo-size",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &maxHistoSize, "Maximum histogram size (when re-quantizing is allowed)");

  // Parse whether quantization should be used (to reduce histogram size)
  clArgs.AddArgument("--quantize",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &quantize, "Allow re-quantizing");


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
  args.skipInt = skipInt;
  args.skipString = skipString;
  args.quantize = quantize;
  args.maxHistoSize = maxHistoSize;
  args.retVal = &testValue;
  args.ioRank = ioRank;

  // Check how many processes have been made available
  int numProcs = controller->GetNumberOfProcesses();
  if ( controller->GetLocalProcessId() == ioRank )
    {
    cout << "\n# Running test with "
         << numProcs
         << " processes and standard deviation = "
         << args.stdev
         << " for rounded Gaussian variable.\n";
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
