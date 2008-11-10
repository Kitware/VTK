/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPDescriptiveStatistics.cxx

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
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this test.

#include <mpi.h>
#include <time.h>

#include "vtkDescriptiveStatistics.h"
#include "vtkPDescriptiveStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkStdString.h"
#include "vtkTable.h"

double nVals = 10000;

// This will be called by all processes
void RandomSampleStatistics(vtkMultiProcessController* controller, void* vtkNotUsed(arg) )
{
  // Get local rank
  int myRank = controller->GetLocalProcessId();

  // Seed random number generator
  vtkMath::RandomSeed( static_cast<int>( time( NULL ) ) * ( myRank + 1 ) );

  // Generate a input table that contains samples of 2 independent uniform random variables over [0, 1]
  int nMetrics = 2;
  vtkTable* inputData = vtkTable::New();
  vtkDoubleArray* doubleArray[2];
  vtkStdString columnNames[] = { "Uniform 0", 
                                 "Uniform 1" };

  
  double s[2] = { 0., 0. };
  for ( int c = 0; c < nMetrics; ++ c )
    {
    doubleArray[c] = vtkDoubleArray::New();
    doubleArray[c]->SetNumberOfComponents( 1 );
    doubleArray[c]->SetName( columnNames[c] );

    double x;
    for ( int r = 0; r < nVals; ++ r )
      {
      x = vtkMath::Random();
      doubleArray[c]->InsertNextValue( x );
      s[c] += x;
      }
    
    inputData->AddColumn( doubleArray[c] );
    doubleArray[c]->Delete();
    }

  // Instantiate a (serial) descriptive statistics engine and set its ports
  vtkDescriptiveStatistics* ds = vtkDescriptiveStatistics::New();
  ds->SetInput( 0, inputData );
  vtkTable* outputMeta = ds->GetOutput( 1 );

  // Select all columns
  for ( int c = 0; c < nMetrics; ++ c )
    {
    ds->AddColumn( columnNames[c] );
    } // Include invalid Metric 3

  // Test with Learn and Derive options only
  ds->SetLearn( true );
  ds->SetDerive( true );
  ds->SetAssess( false );
  ds->Update();

  cout << "\n## Proc "
       << myRank
       << " calculated the following statistics ( "
       << ds->GetSampleSize()
       << " entries per column ):\n";
  for ( vtkIdType r = 0; r < outputMeta->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputMeta->GetNumberOfColumns(); ++ i )
      {
      cout << outputMeta->GetColumnName( i )
           << "="
           << outputMeta->GetValue( r, i ).ToString()
           << "  ";
      }
    cout << "\n";
    }
  
  // Clean up
  ds->Delete();

  // Instantiate a parallel descriptive statistics engine and set its ports
  vtkPDescriptiveStatistics* pds = vtkPDescriptiveStatistics::New();
  pds->SetInput( 0, inputData );
  inputData->Delete();
  vtkTable* poutputMeta = pds->GetOutput( 1 );

  // Select all columns
  for ( int c = 0; c < nMetrics; ++ c )
    {
    pds->AddColumn( columnNames[c] );
    } // Include invalid Metric 3

  // Test with Learn and Derive options only
  pds->SetLearn( true );
  pds->SetDerive( true );
  pds->SetAssess( false );
  pds->Update();

  controller->Barrier();

  if ( ! controller->GetLocalProcessId() )
    {
    cout << "\n# Calculated the following parallel statistics ( total sample size: "
         << pds->GetSampleSize()
         << " ):\n";
    for ( vtkIdType r = 0; r < poutputMeta->GetNumberOfRows(); ++ r )
      {
      cout << "   ";
      for ( int i = 0; i < poutputMeta->GetNumberOfColumns(); ++ i )
        {
        cout << poutputMeta->GetColumnName( i )
             << "="
             << poutputMeta->GetValue( r, i ).ToString()
             << "  ";
        }
      cout << "\n";
      }
    }
  
  // Clean up
  pds->Delete();
}

//----------------------------------------------------------------------------
int main( int argc, char** argv )
{
  int testValue = 0;

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize( &argc, &argv );

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if ( controller->IsA( "vtkThreadedController" ) )
    {
    // Set the number of processes to 2 for this example.
    controller->SetNumberOfProcesses( 2 );
    } 

  // Check how many processes have been made available
  int numProcs = controller->GetNumberOfProcesses();
  if ( ! controller->GetLocalProcessId() )
    {
    cout << "# Running test with "
         << numProcs
         << " processes...\n";
    }
  controller->Barrier();

  // Execute the function named "process" on both processes
  controller->SetSingleMethod( RandomSampleStatistics, 0 );
  controller->SingleMethodExecute();
  
  // Clean-up and exit
  controller->Finalize();
  controller->Delete();
  
  return testValue;
}
