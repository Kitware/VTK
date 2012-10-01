/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRandomPKMeansStatisticsMPI.cxx

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
// Thanks to Janine Bennett, Philippe Pebay, and David Thompson from Sandia National Laboratories
// for implementing this test.

#include <mpi.h>

#include "vtkPKMeansStatistics.h"

#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"

#include "vtksys/CommandLineArguments.hxx"

#include <vtksys/ios/sstream>
#include <vtksys/stl/vector>

struct RandomSampleStatisticsArgs
{
  int nObsPerCluster;
  int nProcs;
  int nVariables;
  int nClusters;
  double meanFactor;
  double stdev;
  int* retVal;
  int ioRank;
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

  // Generate column names
  int nVariables = args->nVariables;
  vtksys_stl::vector<vtkStdString> columnNames;
  for ( int v = 0; v < nVariables; ++ v )
    {
    vtksys_ios::ostringstream columnName;
    columnName << "Variable " << v;
    columnNames.push_back( columnName.str() );
    }

  // Generate an input table that contains samples of mutually independent Gaussian random variables
  vtkTable* inputData = vtkTable::New();
  vtkDoubleArray* doubleArray;

  int obsPerCluster = args->nObsPerCluster;
  int nClusters = args->nClusters;
  int nVals = obsPerCluster * nClusters;

  // Generate samples
  for ( int v = 0; v < nVariables; ++ v )
    {
    doubleArray = vtkDoubleArray::New();
    doubleArray->SetNumberOfComponents( 1 );
    doubleArray->SetName( columnNames.at( v ) );

    for ( int c = 0; c < nClusters; ++ c )
      {
      double x;
      for ( int r = 0; r < obsPerCluster; ++ r )
        {
        x = vtkMath::Gaussian( c * args->meanFactor, args->stdev );
        doubleArray->InsertNextValue( x );
        }
      }

    inputData->AddColumn( doubleArray );
    doubleArray->Delete();
    }

  // set up a single set of param data - send out to all and make tables...
  vtkTable* paramData = vtkTable::New();
  vtkIdTypeArray* paramCluster;
  vtkDoubleArray* paramArray;
  paramCluster = vtkIdTypeArray::New();
  paramCluster->SetName( "K" );

  for( int nInRun = 0; nInRun < nClusters; nInRun++ )
    {
    paramCluster->InsertNextValue( nClusters );
    }

  paramData->AddColumn( paramCluster );
  paramCluster->Delete();

  int nClusterCoords = nClusters * nVariables;
  double* clusterCoords = new double[nClusterCoords];

  // generate data on one node only
  if( myRank == args->ioRank )
    {
    int cIndex = 0;
    for ( int v = 0; v < nVariables; ++ v )
      {
      for ( int c = 0; c < nClusters; ++ c )
        {
        double x = inputData->GetValue( ( c % nClusters ) * obsPerCluster, v ).ToDouble();
        clusterCoords[cIndex++] = x;
        }
      }
    }

  // broadcast data to all nodes
  if( !com->Broadcast( clusterCoords, nClusterCoords, args->ioRank) )
    {
    vtkGenericWarningMacro("Could not broadcast initial cluster coordinates.");
    *(args->retVal) = 1;
    return;
    }

  for ( int v = 0; v < nVariables; ++ v )
    {
    paramArray = vtkDoubleArray::New();
    paramArray->SetName( columnNames[v] );
    paramArray->SetNumberOfTuples( nClusters );
    memcpy( paramArray->GetPointer( 0 ), &( clusterCoords[v * ( nClusters )]), nClusters * sizeof( double ) );
    paramData->AddColumn( paramArray );
    paramArray->Delete();
    }

  delete [] clusterCoords;

  // ************************** KMeans Statistics **************************

  // Synchronize and start clock
  com->Barrier();
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  // Instantiate a parallel KMeans statistics engine and set its ports
  vtkPKMeansStatistics* pks = vtkPKMeansStatistics::New();
  pks->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );
  pks->SetMaxNumIterations( 10 );
  pks->SetInputData( vtkStatisticsAlgorithm::LEARN_PARAMETERS, paramData );

  // Select columns for testing
  for ( int v = 0; v < nVariables; ++ v )
    {
    pks->SetColumnStatus( inputData->GetColumnName( v ) , 1 );
    }
  pks->RequestSelectedColumns();

  // Test (in parallel) with Learn, Derive, and Assess options turned on
  pks->SetLearnOption( true );
  pks->SetDeriveOption( true );
  pks->SetAssessOption( true );
  pks->SetTestOption( false );
  pks->Update();

  // Synchronize and stop clock
  com->Barrier();
  timer->StopTimer();

  if ( myRank == args->ioRank )
    {
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pks->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

    cout << "\n## Completed parallel calculation of kmeans statistics (with assessment):\n"
         << "   Wall time: "
         << timer->GetElapsedTime()
         << " sec.\n";
    for ( unsigned int b = 0; b < outputMetaDS->GetNumberOfBlocks(); ++ b )
      {
      vtkTable* outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( b ) );
      if ( ! b )
        {
        vtkIdType testIntValue = 0;
        for( vtkIdType r = 0; r < outputMeta->GetNumberOfRows(); r++ )
          {
          testIntValue += outputMeta->GetValueByName( r, "Cardinality" ).ToInt();
          }

        cout << "\n## Computed clusters (cardinality: "
             << testIntValue
             << " / run):\n";

        if ( testIntValue != nVals * args->nProcs )
          {
          vtkGenericWarningMacro("Sum of cluster cardinalities is incorrect: "
                               << testIntValue
                               << " != "
                               << nVals * args->nProcs
                               << ".");
          *(args->retVal) = 1;
          }
        }
      else
        {
        cout << "   Ranked cluster: " << "\n";
        }
        outputMeta->Dump();
      }
    }
  // Clean up
  pks->Delete();
  inputData->Delete();
  timer->Delete();
  paramData->Delete();

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

  // **************************** Parse command line ***************************
  // Set default argument values
  int nObsPerCluster = 1000;
  int nVariables = 6;
  int nClusters = 8;
  double meanFactor = 7.;
  double stdev = 1.;

  // Initialize command line argument parser
  vtksys::CommandLineArguments clArgs;
  clArgs.Initialize( argc, argv );
  clArgs.StoreUnusedArguments( false );

  // Parse per-process cardinality of each pseudo-random sample
  clArgs.AddArgument("--n-per-proc-per-cluster",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &nObsPerCluster, "Per-process number of observations per cluster");

  // Parse number of variables
  clArgs.AddArgument("--n-variables",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &nVariables, "Number of variables");

  // Parse number of clusters
  clArgs.AddArgument("--n-clusters",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &nClusters, "Number of clusters");

  // Parse mean factor of each pseudo-random sample
  clArgs.AddArgument("--mean-factor",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &meanFactor, "Mean factor of each pseudo-random sample");

  // Parse standard deviation of each pseudo-random sample
  clArgs.AddArgument("--std-dev",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &stdev, "Standard deviation of each pseudo-random sample");

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
  // Parameters for regression test.
  int testValue = 0;
  RandomSampleStatisticsArgs args;
  args.nObsPerCluster = nObsPerCluster;
  args.nProcs = numProcs;
  args.nVariables = nVariables;
  args.nClusters = nClusters;
  args.meanFactor = meanFactor;
  args.stdev = stdev;
  args.retVal = &testValue;
  args.ioRank = ioRank;

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
