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
 * Copyright 2011 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Philippe Pebay and Ajith Mascarenhas from Sandia National Laboratories
// for implementing this test.

#include <mpi.h>

#include "vtkPDescriptiveStatistics.h"

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

#include "vtksys/CommandLineArguments.hxx"
#include <vector>

namespace
{

struct RealDataDescriptiveStatisticsArgs
{
  int* retVal;
  int ioRank;
  vtkStdString fileName;
  int* dataDim;
  int* procDim;
};

// Calculate the processor id (integer triple), given its rank
void CalculateProcessorId( int *procDim, int rank, int *procId )
{
  int procXY = procDim[0] * procDim[1];

  procId[2] = rank / procXY;
  procId[1] = ( rank - procId[2] * procXY ) / procDim[0];
  procId[0] = ( rank - procId[2] * procXY ) % procDim[0];
}

// Calculate the processor rank given its id(integer triple)
int CalculateProcessorRank( int *procDim, int *procId )
{
  int rank = procId[2] * procDim[0] * procDim[1] +
    procId[1] * procDim[0] + procId[0];

  return rank;
}

// Read a block of data bounded by [low, high] from file into buffer.
// The entire data has dimensions dim
int ReadFloatDataBlockFromFile( ifstream& ifs,
                                int *dim,
                                int *low,
                                int *high,
                                float *buffer )
{
  vtkIdType dimXY = dim[0] * dim[1];
  vtkIdType dimX  = dim[0];

  float *pbuffer = buffer;

  // Set bounds
  vtkIdType bounds[2][3];
  bounds[0][0] = ( low[0] < 0 ? 0 : low[0] );
  bounds[0][1] = ( low[1] < 0 ? 0 : low[1] );
  bounds[0][2] = ( low[2] < 0 ? 0 : low[2] );

  bounds[1][0] = ( high[0] >= dim[0] ? dim[0] - 1 : high[0] );
  bounds[1][1] = ( high[1] >= dim[1] ? dim[1] - 1 : high[1] );
  bounds[1][2] = ( high[2] >= dim[2] ? dim[2] - 1 : high[2] );

  vtkIdType rangeX = bounds[1][0] - bounds[0][0] + 1;
  vtkIdType sizeX = high[0] - low[0] + 1;
  vtkIdType sizeY = high[1] - low[1] + 1;
  vtkIdType sizeXY = sizeX * sizeY;

  // Next position to start writing
  pbuffer += ( bounds[0][0] - low[0] );

  // Iterate over 'z'
  for ( int z = low[2]; z <= high[2]; z++ )
  {
    if ( z >= bounds[0][2] && z <= bounds[1][2] )
    {
      vtkIdType offsetZ = z * dimXY;

      // Iterate over 'y'.
      for (int y = low[1]; y <= high[1]; y++)
      {
        if (y >= bounds[0][1] && y <= bounds[1][1] )
        {
          vtkIdType offsetY = y * dimX;
          long long offset = offsetZ + offsetY + bounds[0][0];

          // Seek to point
          ifs.seekg( offset * sizeof(*pbuffer), ios::beg );

          // Get a block of rangeX values
          ifs.read( reinterpret_cast<char *>( pbuffer ),
                    rangeX * sizeof(*pbuffer) );

          // Proceed to next write position
          pbuffer += sizeX;

          if ( ifs.fail() || ifs.eof() )
          {
            return 1;
          }
        }
        else
        {
          // Skip one line
          pbuffer += sizeX;
        }
      }
    }
    else
    {
      // Skip one plane
      pbuffer += sizeXY;
    }
  }
  return 0;
}

// Given the data dimensions dataDim, the process dimensions procDim, my
// process id myProcId, set the block bounding box myBlockBounds for my data.
// Also open the data file as filestream ifs.
int SetDataParameters( int *dataDim,
                       int *procDim,
                       int *myProcId,
                       const char* fileName,
                       ifstream& ifs,
                       int myBlockBounds[2][3] )
{
  vtkIdType myDim[3];
  myDim[0] = static_cast<int>( ceil( dataDim[0] / ( 1. * procDim[0] ) ) );
  myDim[1] = static_cast<int>( ceil( dataDim[1] / ( 1. * procDim[1] ) ) );
  myDim[2] = static_cast<int>( ceil( dataDim[2] / ( 1. * procDim[2] ) ) );

  // Determine data bounds
  myBlockBounds[0][0] = myProcId[0] * myDim[0];
  myBlockBounds[0][1] = myProcId[1] * myDim[1];
  myBlockBounds[0][2] = myProcId[2] * myDim[2];

  vtkIdType mybb0 = myBlockBounds[0][0] + myDim[0] - 1;
  vtkIdType cast0 = static_cast<int>( dataDim[0] - 1 );
  myBlockBounds[1][0] = ( mybb0 < cast0 ? mybb0 : cast0 );

  vtkIdType mybb1 = myBlockBounds[0][1] + myDim[1] - 1;
  vtkIdType cast1 = static_cast<int>( dataDim[1] - 1 );
  myBlockBounds[1][1] = ( mybb1 < cast1 ? mybb1 : cast1 );

  vtkIdType mybb2 = myBlockBounds[0][2] + myDim[2] - 1;
  vtkIdType cast2 = static_cast<int>( dataDim[2] - 1 );
  myBlockBounds[1][2] = ( mybb2 < cast2 ? mybb2 : cast2 );

  // Open file
  ifs.open( fileName, ios::in | ios::binary );

  return ( ifs.fail() );
}

// This will be called by all processes
void RealDataDescriptiveStatistics( vtkMultiProcessController* controller, void* arg )
{
  // Get test parameters
  RealDataDescriptiveStatisticsArgs* args = reinterpret_cast<RealDataDescriptiveStatisticsArgs*>( arg );
  *(args->retVal) = 0;

  // Get MPI communicator
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast( controller->GetCommunicator() );

  // Get local rank
  int myRank = com->GetLocalProcessId();
  int myProcId[3];
  CalculateProcessorId( args->procDim, myRank, myProcId );

  // Synchronize and start clock
  com->Barrier();
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();

  // ************************** Read input data file ****************************
  ifstream ifs;
  int myBlockBounds[2][3];
  if ( SetDataParameters( args->dataDim,
                          args->procDim,
                          myProcId,
                          args->fileName,
                          ifs,
                          myBlockBounds ) )
  {
    // If failed to open file with given name, exit in error
    vtkGenericWarningMacro("Process "
                           << myRank
                           << " could not open file with name "
                           << args->fileName
                           <<", exiting.");

    // Exit cleanly
    timer->Delete();
    *(args->retVal) = 1;
    return;
  }

  vtkIdType myDataDim[3];
  myDataDim[0] = myBlockBounds[1][0] - myBlockBounds[0][0] + 1;
  myDataDim[1] = myBlockBounds[1][1] - myBlockBounds[0][1] + 1;
  myDataDim[2] = myBlockBounds[1][2] - myBlockBounds[0][2] + 1;
  vtkIdType card_l = myDataDim[0] * myDataDim[1] * myDataDim[2];
  float* buffer = new float[card_l];

  if (   ReadFloatDataBlockFromFile( ifs,
                                     args->dataDim,
                                     myBlockBounds[0],
                                     myBlockBounds[1],
                                     buffer ) )
  {
    // If failed to read data block from file, exit in error
    vtkGenericWarningMacro("Process "
                           << myRank
                           << " failed to read data or reached EOF in file "
                           << args->fileName
                           <<", exiting.");

    // Exit cleanly
    timer->Delete();
    *(args->retVal) = 1;
    return;
  }

  // ************************** Create input data table *************************
  vtkStdString varName( "Chi" );
  vtkFloatArray* floatArr = vtkFloatArray::New();
  floatArr->SetNumberOfComponents( 1 );
  floatArr->SetName( varName );

  for ( vtkIdType i = 0; i < card_l; ++ i )
  {
    floatArr->InsertNextValue( buffer[i] );
  }

  vtkTable* inputData = vtkTable::New();
  inputData->AddColumn( floatArr );

  floatArr->Delete();
  delete [] buffer;

  // ************************** Descriptive Statistics **************************

  // Instantiate a parallel descriptive statistics engine and set its input
  vtkPDescriptiveStatistics* pcs = vtkPDescriptiveStatistics::New();
  pcs->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

  // Select column of interest
  pcs->AddColumn( varName );

  // Test (in parallel) with Learn and Derive options turned on
  pcs->SetLearnOption( true );
  pcs->SetDeriveOption( true );
  pcs->SetTestOption( false );
  pcs->SetAssessOption( false );
  pcs->Update();

  // Get output data and meta tables
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pcs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputPrimary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
  vtkTable* outputDerived = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );

  // Synchronize and stop clock
  com->Barrier();
  timer->StopTimer();

  if ( com->GetLocalProcessId() == args->ioRank )
  {
    cout << "\n## Completed parallel calculation of descriptive statistics (without assessment):\n"
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

  // Verify that sizes of read data sets sums up to the calculated global cardinality
  if ( com->GetLocalProcessId() == args->ioRank )
  {
    cout << "\n## Verifying that sizes of read data sets sums up to the calculated global cardinality.\n";
  }

  // Gather all cardinalities
  int numProcs = controller->GetNumberOfProcesses();
  vtkIdType* card_g = new vtkIdType[numProcs];
  com->AllGather( &card_l,
                  card_g,
                  1 );

  // Calculated global cardinality
  vtkIdType testIntValue = outputPrimary->GetValueByName( 0, "Cardinality" ).ToInt();

  // Print and verify some results
  if ( com->GetLocalProcessId() == args->ioRank )
  {
    vtkIdType sumCards = 0;
    for ( int i = 0; i < numProcs; ++ i )
    {
      cout << "   Cardinality of data set read on process "
           << i
           << ": "
           << card_g[i]
           << "\n";

      sumCards += card_g[i];
    }

    cout << "   Cardinality of global data set: "
         << sumCards
         << " \n";

    if ( sumCards != testIntValue )
    {
      vtkGenericWarningMacro("Incorrect calculated global cardinality:"
                             << testIntValue
                             << " <> "
                             << sumCards
                             << ")");
      *(args->retVal) = 1;
    }
  }

  // Clean up
  pcs->Delete();
  inputData->Delete();
  timer->Delete();

}

}

//----------------------------------------------------------------------------
int TestRealDataPDescriptiveStatisticsMPI( int argc, char* argv[] )
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

  // Get local rank
  int myRank = com->GetLocalProcessId();

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

  if ( myRank == ioRank )
  {
    cout << "\n# Process "
         << ioRank
         << " will be the I/O node.\n";
  }

  // Check how many processes have been made available
  int numProcs = controller->GetNumberOfProcesses();
  if ( myRank == ioRank )
  {
    cout << "\n# Running test with "
         << numProcs
         << " processes...\n";
  }

  // **************************** Parse command line ***************************
  // If no arguments were provided, terminate in error.
  if ( argc < 2 )
  {
    vtkGenericWarningMacro("No input data arguments were provided.");
    controller->Delete();
    return 1;
  }

  // Set default argument values (some of which are invalid, for mandatory parameters)
  vtkStdString fileName= "";
  std::vector<int> dataDim;
  std::vector<int> procDim;

  // Initialize command line argument parser
  vtksys::CommandLineArguments clArgs;
  clArgs.Initialize( argc, argv );
  clArgs.StoreUnusedArguments( false );

  // Parse input data file name
  clArgs.AddArgument("--file-name",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &fileName, "Name of input data file");

  // Parse input data dimensions
  clArgs.AddArgument("--data-dim",
                     vtksys::CommandLineArguments::MULTI_ARGUMENT,
                     &dataDim, "Dimensions of the input data");

  // Parse process array dimensions
  clArgs.AddArgument("--proc-dim",
                     vtksys::CommandLineArguments::MULTI_ARGUMENT,
                     &procDim, "Dimensions of the input data");

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

  // If no file name was provided, terminate in error.
  if ( ! strcmp( fileName.c_str(), "" ) )
  {
    if ( myRank == ioRank )
    {
      vtkGenericWarningMacro("No input data file name was provided.");
    }

    // Terminate cleanly
    controller->Finalize();
    controller->Delete();
    return 1;
  }
  else
  {
    if ( myRank == ioRank )
    {
      cout << "\n# Input data file name: "
           << fileName
           << "\n";
    }
  }

  // If no or insufficient data dimensionality information, terminate in error.
  if ( dataDim.size() < 3 )
  {
    if ( myRank == ioRank )
    {
      vtkGenericWarningMacro("Only "
                             << dataDim.size()
                             << "data dimension(s) provided (3 needed).");
    }

    // Terminate cleanly
    controller->Finalize();
    controller->Delete();
    return 1;
  }
  else
  {
    if ( myRank == ioRank )
    {
      cout << "\n# Data dimensionality: "
           << dataDim.at( 0 )
           << " "
           << dataDim.at( 1 )
           << " "
           << dataDim.at( 2 )
           << "\n";
    }
  }

  // Fill process dimensionality with ones if not provided or incomplete
  int missingDim = 3 - static_cast<int>( procDim.size() );
  for ( int d = 0; d < missingDim; ++ d )
  {
    procDim.push_back( 1 );
  }

  // If process dimensionality is inconsistent with total number of processes, terminate in error.
  if ( procDim.at( 0 ) * procDim.at( 1 ) * procDim.at( 2 ) != numProcs )
  {
    if ( myRank == ioRank )
    {
      vtkGenericWarningMacro("Number of processes: "
                             << numProcs
                             << " <> "
                             << procDim.at( 0 )
                             << " * "
                             << procDim.at( 1 )
                             << " * "
                             << procDim.at( 2 )
                             << ".");
    }

    // Terminate cleanly
    controller->Finalize();
    controller->Delete();
    return 1;
  }
  else
  {
    if ( myRank == ioRank )
    {
      cout << "\n# Process dimensionality: "
           << procDim.at( 0 )
           << " "
           << procDim.at( 1 )
           << " "
           << procDim.at( 2 )
           << "\n";
    }
  }

  // ************************** Initialize test *********************************

  // Parameters for regression test.
  int testValue = 0;
  RealDataDescriptiveStatisticsArgs args;

  args.retVal = &testValue;
  args.ioRank = ioRank;
  args.fileName = fileName;
  int dataDimPtr[3];
  dataDimPtr[0] = dataDim.at( 0 );
  dataDimPtr[1] = dataDim.at( 1 );
  dataDimPtr[2] = dataDim.at( 2 );
  args.dataDim = dataDimPtr;

  int procDimPtr[3];
  procDimPtr[0] = procDim.at( 0 );
  procDimPtr[1] = procDim.at( 1 );
  procDimPtr[2] = procDim.at( 2 );
  args.procDim = procDimPtr;

  // Execute the function named "process" on both processes
  controller->SetSingleMethod( RealDataDescriptiveStatistics, &args );
  controller->SingleMethodExecute();

  // Clean up and exit
  if ( myRank == ioRank )
  {
    cout << "\n# Test completed.\n\n";
  }

  controller->Finalize();
  controller->Delete();

  return testValue;
}
