/*=========================================================================

Program:   Visualization Toolkit
Module:    TestRandomMomentStatisticsMPI.cxx

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
// Thanks to Philippe Pebay, David Thompson and Janine Bennett from Sandia National Laboratories
// for implementing this test.

#include <mpi.h>

#include "vtkDescriptiveStatistics.h"
#include "vtkPDescriptiveStatistics.h"
#include "vtkCorrelativeStatistics.h"
#include "vtkPCorrelativeStatistics.h"
#include "vtkMultiCorrelativeStatistics.h"
#include "vtkPMultiCorrelativeStatistics.h"
#include "vtkPPCAStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

#include "vtksys/CommandLineArguments.hxx"

struct RandomSampleStatisticsArgs
{
  int nVals;
  bool skipDescriptive;
  bool skipCorrelative;
  bool skipMultiCorrelative;
  bool skipPCA;
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

  // Generate an input table that contains samples of mutually independent random variables
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

  // "68-95-99.7 rule"
  // Actually testing for 1, ..., numRuleVal standard deviations
  int numRuleVal = 6;

  // Reference values
  double sigmaRuleVal[] = { 68.2689492137,
                            95.4499736104,
                            99.7300203937,
                            99.9936657516,
                            99.9999426697,
                            99.9999998027 };

  // Tolerances
  double sigmaRuleTol[] = { 1.,
                            .5,
                            .1,
                            .05,
                            .01,
                            .005 };

  // Create timer to be used by all tests
  vtkTimerLog *timer=vtkTimerLog::New();

  // ************************** Descriptive Statistics **************************

  // Skip descriptive statistics if requested
  if ( ! args->skipDescriptive )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // For verification, instantiate a serial descriptive statistics engine and set its ports
    vtkDescriptiveStatistics* ds = vtkDescriptiveStatistics::New();
    ds->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select all columns
    for ( int c = 0; c < nVariables; ++ c )
      {
      ds->AddColumn( columnNames[c] );
      }

    // Test (serially) with Learn and Derive options only
    ds->SetLearnOption( true );
    ds->SetDeriveOption( true );
    ds->SetAssessOption( false );
    ds->SetTestOption( false );
    ds->Update();

    // Get output data and meta tables
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( ds->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
    vtkTable* outputPrimary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
    vtkTable* outputDerived = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );

    // Collect (local) cardinalities, extrema, and means
    int nRows = outputPrimary->GetNumberOfRows();
    int np = com->GetNumberOfProcesses();
    int n2Rows = 2 * nRows;

    double* extrema_l = new double[n2Rows];
    double* extrema_g = new double[n2Rows];

    double* cardsAndMeans_l = new double[n2Rows];
    double* cardsAndMeans_g = new double[n2Rows];

    double dn;
    for ( vtkIdType r = 0; r < nRows; ++ r )
      {
      dn = outputPrimary->GetValueByName( r, "Cardinality" ).ToDouble();
      cardsAndMeans_l[2 * r] = dn;
      cardsAndMeans_l[2 * r + 1] = dn * outputPrimary->GetValueByName( r, "Mean" ).ToDouble();

      extrema_l[2 * r] = outputPrimary->GetValueByName( r, "Minimum" ).ToDouble();
      // Collect -max instead of max so a single reduce op. (minimum) can process both extrema at a time
      extrema_l[2 * r + 1] = - outputPrimary->GetValueByName( r, "Maximum" ).ToDouble();
      }

    // Reduce all extremal values, and gather all cardinalities and means, on process calcProc
    int calcProc = np - 1;

    com->Reduce( extrema_l,
                 extrema_g,
                 n2Rows,
                 vtkCommunicator::MIN_OP,
                 calcProc );

    com->Reduce( cardsAndMeans_l,
                 cardsAndMeans_g,
                 n2Rows,
                 vtkCommunicator::SUM_OP,
                 calcProc );

    // Have process calcProc calculate global cardinality and mean, and send all results to I/O process
    if ( myRank == calcProc )
      {
      if ( ! com->Send( extrema_g,
                        n2Rows,
                        args->ioRank,
                        65 ) )
        {
        vtkGenericWarningMacro("MPI error: process "<<myRank<< "could not send global results. Serial/parallel sanity check will be meaningless.");
        *(args->retVal) = 1;
        }

      if ( ! com->Send( cardsAndMeans_g,
                        n2Rows,
                        args->ioRank,
                        66 ) )
        {
        vtkGenericWarningMacro("MPI error: process "<<myRank<< "could not send global results. Serial/parallel sanity check will be meaningless.");
        *(args->retVal) = 1;
        }
      }

    // Have I/O process receive results from process calcProc
    if ( myRank == args->ioRank )
      {
      if ( ! com->Receive( extrema_g,
                           n2Rows,
                           calcProc,
                           65 ) )
        {
        vtkGenericWarningMacro("MPI error: I/O process "<<args->ioRank<<" could not receive global results. Serial/parallel sanity check will be meaningless.");
        *(args->retVal) = 1;
        }

      if ( ! com->Receive( cardsAndMeans_g,
                           n2Rows,
                           calcProc,
                           66 ) )
        {
        vtkGenericWarningMacro("MPI error: I/O process "<<args->ioRank<<" could not receive global results. Serial/parallel sanity check will be meaningless.");
        *(args->retVal) = 1;
        }
      }

    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    if ( com->GetLocalProcessId() == args->ioRank )
      {
      cout << "\n## Completed serial calculations of descriptive statistics (with assessment):\n"
           << "   With partial aggregation calculated on process "
           << calcProc
           << "\n"
           << "   Wall time: "
           << timer->GetElapsedTime()
           << " sec.\n";

      for ( vtkIdType r = 0; r < nRows; ++ r )
        {
        cout << "   "
             << outputPrimary->GetColumnName( 0 )
             << "="
             << outputPrimary->GetValue( r, 0 ).ToString()
             << "  "
             << "Cardinality"
             << "="
             << cardsAndMeans_g[2 * r]
             << "  "
             << "Minimum"
             << "="
             << extrema_g[2 * r]
             << "  "
             << "Maximum"
             << "="
             << - extrema_g[2 * r + 1]
             << "  "
             << "Mean"
             << "="
             << cardsAndMeans_g[2 * r + 1] / cardsAndMeans_g[2 * r]
             << "\n";
        }
      }

    // Clean up
    delete [] cardsAndMeans_l;
    delete [] cardsAndMeans_g;

    delete [] extrema_l;
    delete [] extrema_g;

    ds->Delete();

    // Now on to the actual parallel descriptive engine

    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel descriptive statistics engine and set its input data
    vtkPDescriptiveStatistics* pds = vtkPDescriptiveStatistics::New();
    pds->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select all columns
    for ( int c = 0; c < nVariables; ++ c )
      {
      pds->AddColumn( columnNames[c] );
      }

    // Test (in parallel) with Learn, Derive, and Assess options turned on
    pds->SetLearnOption( true );
    pds->SetDeriveOption( true );
    pds->SetAssessOption( true );
    pds->SetTestOption( false );
    pds->SignedDeviationsOff(); // Use unsigned deviations
    pds->Update();

    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    // Get output data and meta tables
    outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pds->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
    outputPrimary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
    outputDerived = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );
    vtkTable* outputData = pds->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );

    if ( com->GetLocalProcessId() == args->ioRank )
      {
      cout << "\n## Completed parallel calculation of descriptive statistics (with assessment):\n"
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

    // Verify that the DISTRIBUTED standard normal samples indeed statisfy the 68-95-99.7 rule
    if ( com->GetLocalProcessId() == args->ioRank )
      {
      cout << "\n## Verifying whether the distributed standard normal samples satisfy the 68-95-99.7 rule:\n";
      }

    vtkDoubleArray* relDev[2];
    relDev[0] = vtkDoubleArray::SafeDownCast(
                                             outputData->GetColumnByName( "d(Standard Normal 0)" ) );
    relDev[1] = vtkDoubleArray::SafeDownCast(
                                             outputData->GetColumnByName( "d(Standard Normal 1)" ) );

    if ( !relDev[0] || ! relDev[1] )
      {
      cout << "*** Error: "
           << "Empty output column(s) on process "
           << com->GetLocalProcessId()
           << ".\n";
      }

    // For each normal variable, count deviations of more than 1, ..., numRuleVal standard deviations from the mean
    for ( int c = 0; c < nNormal; ++ c )
      {
      // Allocate and initialize counters
      int* outsideStdv_l = new int[numRuleVal];
      for ( int i = 0; i < numRuleVal; ++ i )
        {
        outsideStdv_l[i] = 0;
        }

      // Count outliers
      double dev;
      int n = outputData->GetNumberOfRows();
      for ( vtkIdType r = 0; r < n; ++ r )
        {
        dev = relDev[c]->GetValue( r );

        if ( dev >= 1. )
          {
          ++ outsideStdv_l[0];

          if ( dev >= 2. )
            {
            ++ outsideStdv_l[1];

            if ( dev >= 3. )
              {
              ++ outsideStdv_l[2];

              if ( dev >= 4. )
                {
                ++ outsideStdv_l[3];

                if ( dev >= 5. )
                  {
                  ++ outsideStdv_l[4];
                  } // if ( dev >= 5. )
                } // if ( dev >= 4. )
              } // if ( dev >= 3. )
            } // if ( dev >= 2. )
          } // if ( dev >= 1. )
        }

      // Sum all local counters
      int* outsideStdv_g = new int[numRuleVal];
      com->AllReduce( outsideStdv_l,
                      outsideStdv_g,
                      numRuleVal,
                      vtkCommunicator::SUM_OP );

      // Print out percentages of sample points within 1, ..., numRuleVal standard deviations from the mean.
      if ( com->GetLocalProcessId() == args->ioRank )
        {
        cout << "   "
             << outputData->GetColumnName( nUniform + c )
             << ":\n";
        for ( int i = 0; i < numRuleVal; ++ i )
          {
          double testVal = ( 1. - outsideStdv_g[i] / static_cast<double>( outputPrimary->GetValueByName( 0, "Cardinality" ).ToInt() ) ) * 100.;

          cout << "      "
               << testVal
               << "% within "
               << i + 1
               << " standard deviation(s) from the mean.\n";

          // Test some statistics
          if ( fabs ( testVal - sigmaRuleVal[i] ) > sigmaRuleTol[i] )
            {
            vtkGenericWarningMacro("Incorrect value.");
            *(args->retVal) = 1;
            }
          }
        }

      // Clean up
      delete [] outsideStdv_l;
      delete [] outsideStdv_g;
      } // for ( int c = 0; c < nNormal; ++ c )

    // Clean up
    pds->Delete();

    } // if ( ! args->skipDescriptive )

  // ************************** Correlative Statistics **************************

  // Skip correlative statistics if requested
  if ( ! args->skipCorrelative )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel correlative statistics engine and set its input
    vtkPCorrelativeStatistics* pcs = vtkPCorrelativeStatistics::New();
    pcs->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select column pairs (uniform vs. uniform, normal vs. normal)
    pcs->AddColumnPair( columnNames[0], columnNames[1] );
    pcs->AddColumnPair( columnNames[2], columnNames[3] );

    // Test (in parallel) with Learn, Derive options turned on
    pcs->SetLearnOption( true );
    pcs->SetDeriveOption( true );
    pcs->SetAssessOption( false );
    pcs->SetTestOption( false );
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
    } // if ( ! args->skipCorrelative )

  // ************************** Multi-Correlative Statistics **************************

  // Skip multi-correlative statistics if requested
  if ( ! args->skipMultiCorrelative )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel correlative statistics engine and set its ports
    vtkPMultiCorrelativeStatistics* pmcs = vtkPMultiCorrelativeStatistics::New();
    pmcs->SetInput( 0, inputData );

    // Select column pairs (uniform vs. uniform, normal vs. normal)
    pmcs->SetColumnStatus( columnNames[0], true );
    pmcs->SetColumnStatus( columnNames[1], true );
    pmcs->RequestSelectedColumns();

    pmcs->ResetAllColumnStates();
    pmcs->SetColumnStatus( columnNames[2], true );
    pmcs->SetColumnStatus( columnNames[3], true );
    pmcs->RequestSelectedColumns();

    pmcs->ResetAllColumnStates();
    pmcs->SetColumnStatus( columnNames[0], true );
    pmcs->SetColumnStatus( columnNames[1], true );
    pmcs->SetColumnStatus( columnNames[2], true );
    pmcs->SetColumnStatus( columnNames[3], true );
    pmcs->RequestSelectedColumns();

    // Test (in parallel) with Learn, Derive, and Assess options turned on
    pmcs->SetLearnOption( true );
    pmcs->SetDeriveOption( true );
    pmcs->SetAssessOption( true );
    pmcs->Update();

    // Get output meta tables
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pmcs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    if ( com->GetLocalProcessId() == args->ioRank )
      {
      cout << "\n## Completed parallel calculation of multi-correlative statistics (with assessment):\n"
           << "   Total sample size: "
           << vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) )->GetValueByName( 0, "Entries").ToInt()
           << " \n"
           << "   Wall time: "
           << timer->GetElapsedTime()
           << " sec.\n";

      vtkTable* outputMeta;
      for ( unsigned int b = 1; b < outputMetaDS->GetNumberOfBlocks(); ++ b )
        {
        outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( b ) );
        outputMeta->Dump();
        }
      }

    // Clean up
    pmcs->Delete();
    } //   if ( ! args->skipMultiCorrelative )

  // ************************** PCA Statistics **************************

  // Skip PCA statistics if requested
  if ( ! args->skipPCA )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel pca statistics engine and set its ports
    vtkPPCAStatistics* pcas = vtkPPCAStatistics::New();
    pcas->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select column pairs (uniform vs. uniform, normal vs. normal)
    pcas->SetColumnStatus( columnNames[0], true );
    pcas->SetColumnStatus( columnNames[1], true );
    pcas->RequestSelectedColumns();

    pcas->ResetAllColumnStates();
    pcas->SetColumnStatus( columnNames[2], true );
    pcas->SetColumnStatus( columnNames[3], true );
    pcas->RequestSelectedColumns();

    pcas->ResetAllColumnStates();
    pcas->SetColumnStatus( columnNames[0], true );
    pcas->SetColumnStatus( columnNames[1], true );
    pcas->SetColumnStatus( columnNames[2], true );
    pcas->SetColumnStatus( columnNames[3], true );
    pcas->RequestSelectedColumns();

    // Test (in parallel) with Learn, Derive, and Assess options turned on
    pcas->SetLearnOption( true );
    pcas->SetDeriveOption( true );
    pcas->SetAssessOption( true );
    pcas->SetTestOption( true );
    pcas->Update();

    // Get output meta tables
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pcas->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    if ( com->GetLocalProcessId() == args->ioRank )
      {
      cout << "\n## Completed parallel calculation of pca statistics (with assessment):\n"
           << "   Total sample size: "
           << vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) )->GetValueByName( 0, "Entries").ToInt()
           << " \n"
           << "   Wall time: "
           << timer->GetElapsedTime()
           << " sec.\n";

      vtkTable* outputMeta;
      for ( unsigned int b = 1; b < outputMetaDS->GetNumberOfBlocks(); ++ b )
        {
        outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( b ) );
        outputMeta->Dump();
        }
      }

    // Clean up
    pcas->Delete();
    } // if ( ! args->skipPCA )

  // Clean up
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
  int nVals = 100000;
  bool skipDescriptive = false;
  bool skipCorrelative = false;
  bool skipMultiCorrelative = false;
  bool skipPCA = false;

  // Initialize command line argument parser
  vtksys::CommandLineArguments clArgs;
  clArgs.Initialize( argc, argv );
  clArgs.StoreUnusedArguments( false );

  // Parse per-process cardinality of each pseudo-random sample
  clArgs.AddArgument("--n-per-proc",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &nVals, "Per-process cardinality of each pseudo-random sample");

  // Parse whether descriptive statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-descriptive",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipDescriptive, "Skip descriptive statistics");

  // Parse whether correlative statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-correlative",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipCorrelative, "Skip correlative statistics");

  // Parse whether multi-correlative statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-multi-correlative",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipMultiCorrelative, "Skip multi-correlative statistics");

  // Parse whether PCA statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-PCA",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipPCA, "Skip PCA statistics");

  // If incorrect arguments were provided, terminate in error.
  if ( ! clArgs.Parse() )
    {
    vtkGenericWarningMacro("Incorrect input data arguments were provided.");
    return 1;
    }

  // ************************** Initialize test *********************************
  // Parameters for regression test.
  int testValue = 0;
  RandomSampleStatisticsArgs args;
  args.nVals = nVals;
  args.skipDescriptive = skipDescriptive;
  args.skipCorrelative = skipCorrelative;
  args.skipMultiCorrelative = skipMultiCorrelative;
  args.skipPCA = skipPCA;
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
