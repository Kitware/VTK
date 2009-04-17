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
#include <time.h>

#include "vtkContingencyStatistics.h"
#include "vtkPContingencyStatistics.h"

#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

// For debugging purposes, output results of serial engines ran on each slice of the distributed data set
#define PRINT_ALL_SERIAL_STATS 0 

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
  vtkMath::RandomSeed( static_cast<int>( time( NULL ) ) * ( myRank + 1 ) );

  // Generate an input table that contains samples of mutually independent discrete random variables
  int nVariables = 2;
  vtkIntArray* intArray[2];
  vtkStdString columnNames[] = { "Uniform 0", 
                                 "Uniform 1" };
  
  vtkTable* inputData = vtkTable::New();
  // Discrete uniform samples
  for ( int c = 0; c < nVariables; ++ c )
    {
    intArray[c] = vtkIntArray::New();
    intArray[c]->SetNumberOfComponents( 1 );
    intArray[c]->SetName( columnNames[c] );

    int x;
    for ( int r = 0; r < args->nVals; ++ r )
      {
      x = static_cast<int>( floor( vtkMath::Random() * 100. ) ) + 5;
      intArray[c]->InsertNextValue( x );
      }
    
    inputData->AddColumn( intArray[c] );
    intArray[c]->Delete();
    }

  // ************************** Contingency Statistics ************************** 

  // Synchronize and start clock
  com->Barrier();
  time_t t0;
  time ( &t0 );

  // Instantiate a parallel contingency statistics engine and set its ports
  vtkPContingencyStatistics* pcs = vtkPContingencyStatistics::New();
  pcs->SetInput( 0, inputData );
  vtkTable* outputData = pcs->GetOutput( 0 );
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pcs->GetOutputDataObject( 1 ) );

  // Select column pairs (uniform vs. uniform, normal vs. normal)
  pcs->AddColumnPair( columnNames[0], columnNames[1] );

  // Test (in parallel) with Learn, Derive, and Assess options turned on
  pcs->SetLearn( true );
  pcs->SetDerive( true );
  pcs->SetAssess( true );
  pcs->Update();

    // Synchronize and stop clock
  com->Barrier();
  time_t t1;
  time ( &t1 );

  if ( com->GetLocalProcessId() == args->ioRank )
    {
    cout << "\n## Completed parallel calculation of contingency statistics (with assessment):\n"
         << " \n"
         << "   Wall time: "
         << difftime( t1, t0 )
         << " sec.\n";

//    for ( unsigned int b = 0; b < outputMetaDS->GetNumberOfBlocks(); ++ b )
    for ( unsigned int b = 0; b < 2; ++ b )
      {
      vtkTable* outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( b ) );
      outputMeta->Dump();
      }
    }

  // Clean up
  pcs->Delete();
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
    cout << "\n# Houston, this is process "
         << ioRank
         << " speaking. I'll be the I/O node.\n";
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
  args.nVals = 200000;
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
