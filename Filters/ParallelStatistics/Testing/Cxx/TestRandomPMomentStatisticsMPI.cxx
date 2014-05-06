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
// This class was extended to auto-correlative statistics by Philippe Pebay, Kitware 2013

#include <mpi.h>

#include "vtkDescriptiveStatistics.h"
#include "vtkPAutoCorrelativeStatistics.h"
#include "vtkPDescriptiveStatistics.h"
#include "vtkCorrelativeStatistics.h"
#include "vtkPCorrelativeStatistics.h"
#include "vtkMultiCorrelativeStatistics.h"
#include "vtkPMultiCorrelativeStatistics.h"
#include "vtkPPCAStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

#include "vtksys/CommandLineArguments.hxx"

#include <vtksys/ios/sstream>

namespace
{

struct RandomSampleStatisticsArgs
{
  int nVals;
  double absTol;
  bool skipDescriptive;
  bool skipPDescriptive;
  bool skipPAutoCorrelative;
  bool skipPCorrelative;
  bool skipPMultiCorrelative;
  bool skipPPCA;
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

  // Create timer to be used by all tests
  vtkTimerLog *timer = vtkTimerLog::New();

  // Storage for cross-checking between aggregated serial vs. parallel descriptive statistics
  int n2Rows = 2 * nVariables;
  double* extrema_agg = new double[n2Rows];
  double* extrema_par = new double[n2Rows];
  double* cardsAndMeans_agg = new double[n2Rows];
  double* cardsAndMeans_par = new double[n2Rows];

  // ************************** Serial descriptive Statistics **************************

  // Skip serial descriptive statistics if requested
  if ( ! args->skipDescriptive )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // For verification, instantiate a serial descriptive statistics engine and set its ports
    vtkDescriptiveStatistics* ds = vtkDescriptiveStatistics::New();
    ds->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select all columns
    for ( int c = 0; c < nVariables; ++ c )
      {
      ds->AddColumn( columnNames[c] );
      }

    // Test (serially) with Learn operation only (this is only to verify parallel statistics)
    ds->SetLearnOption( true );
    ds->SetDeriveOption( false );
    ds->SetAssessOption( false );
    ds->SetTestOption( false );
    ds->Update();

    // Get output data and meta tables
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( ds->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
    vtkTable* outputPrimary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );

    // Collect and aggregate serial cardinalities, extrema, and means
    int nRows = outputPrimary->GetNumberOfRows();

    // Make sure that the correct number of rows were retrieved
    if ( nRows != nVariables )
      {
      vtkGenericWarningMacro("Incorrect number of retrieved variables: "
                             << nRows
                             << " <> "
                             << nVariables);
      *(args->retVal) = 1;
      }

    // Aggregate serial results
    double* extrema_l = new double[n2Rows];
    double* cardsAndMeans_l = new double[n2Rows];
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

    // Reduce all extremal values, and gather all cardinalities and means, directly on I/O node
    if ( ! com->Reduce( extrema_l,
                        extrema_agg,
                        n2Rows,
                        vtkCommunicator::MIN_OP,
                        args->ioRank ) )
        {
        vtkGenericWarningMacro("MPI error: process "
                               <<myRank
                               << "could not reduce extrema. Serial vs. parallel cross-check will be meaningless.");
        *(args->retVal) = 1;
        }

    if ( ! com->Reduce( cardsAndMeans_l,
                        cardsAndMeans_agg,
                        n2Rows,
                        vtkCommunicator::SUM_OP,
                        args->ioRank ) )
        {
        vtkGenericWarningMacro("MPI error: process "
                               <<myRank
                               << "could not reduce cardinalities and means. Serial vs. parallel cross-check will be meaningless.");
        *(args->retVal) = 1;
        }


    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    if ( myRank == args->ioRank )
      {
      cout << "\n## Completed serial calculations of descriptive statistics:\n"
           << "   With partial aggregation calculated on process "
           << args->ioRank
           << "\n"
           << "   Wall time: "
           << timer->GetElapsedTime()
           << " sec.\n";

      cout << "   Calculated the following primary statistics:\n";
      for ( vtkIdType r = 0; r < nRows; ++ r )
        {
        cout << "   "
             << outputPrimary->GetColumnName( 0 )
             << "="
             << outputPrimary->GetValue( r, 0 ).ToString()
             << "  "
             << "Cardinality"
             << "="
             << cardsAndMeans_agg[2 * r]
             << "  "
             << "Minimum"
             << "="
             << extrema_agg[2 * r]
             << "  "
             << "Maximum"
             << "="
             << - extrema_agg[2 * r + 1]
             << "  "
             << "Mean"
             << "="
             << cardsAndMeans_agg[2 * r + 1] / cardsAndMeans_agg[2 * r]
             << "\n";
        }
      }

    // Clean up
    delete [] cardsAndMeans_l;
    delete [] extrema_l;
    ds->Delete();
    } // if ( ! args->skipDescriptive )
  else if ( myRank == args->ioRank )
    {
    cout << "\n## Skipped serial calculations of descriptive statistics.\n";
    }

  // ************************** Parallel Descriptive Statistics **************************

  // Skip parallel descriptive statistics if requested
  if ( ! args->skipPDescriptive )
    {
    // Now on to the actual parallel descriptive engine
    // "68-95-99.7 rule" for 1 up to numRuleVal standard deviations
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


    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel descriptive statistics engine and set its input data
    vtkPDescriptiveStatistics* pds = vtkPDescriptiveStatistics::New();
    pds->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select all columns
    for ( int c = 0; c < nVariables; ++ c )
      {
      pds->AddColumn( columnNames[c] );
      }

    // Test (in parallel) with Learn, Derive, and Assess operations turned on
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
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pds->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
    vtkTable* outputPrimary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
    vtkTable* outputDerived = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );
    vtkTable* outputData = pds->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );

    if ( myRank == args->ioRank )
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

        // Store cardinalities, extremal and means for cross-verification
        double dn = outputPrimary->GetValueByName( r, "Cardinality" ).ToDouble();

        cardsAndMeans_par[2 * r] = dn;
        cardsAndMeans_par[2 * r + 1] = dn * outputPrimary->GetValueByName( r, "Mean" ).ToDouble();

        extrema_par[2 * r] = outputPrimary->GetValueByName( r, "Minimum" ).ToDouble();
        extrema_par[2 * r + 1] = - outputPrimary->GetValueByName( r, "Maximum" ).ToDouble();
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
    if ( myRank == args->ioRank )
      {
      cout << "\n## Verifying whether the distributed standard normal samples satisfy the 68-95-99.7 rule:\n";
      }

    // For each normal variable, count deviations of more than 1, ..., numRuleVal standard deviations from the mean
    for ( int c = 0; c < nNormal; ++ c )
      {
      // Use assessed values (relative deviations) to check distribution
      vtksys_ios::ostringstream relDevName;
      relDevName << "d(Standard Normal "
                 << c
                 << ")";

      // Verification can be done only if assessed column is present
      vtkAbstractArray* relDevArr = outputData->GetColumnByName( relDevName.str().c_str() );
      if ( relDevArr )
        {
        // Assessed column should be an array of doubles
        vtkDoubleArray* relDev = vtkDoubleArray::SafeDownCast( relDevArr );
        if ( relDev )
          {
          // Allocate and initialize counters
          int* outsideStdv_l = new int[numRuleVal];
          for ( int d = 0; d < numRuleVal; ++ d )
            {
            outsideStdv_l[d] = 0;
            }

          // Count outliers
          double dev;
          int n = outputData->GetNumberOfRows();
          for ( vtkIdType r = 0; r < n; ++ r )
            {
            dev = relDev->GetValue( r );

            // Count for all deviations from 1 to numRuleVal
            for ( int d = 0; d < numRuleVal; ++ d )
              {
              if ( dev >= 1. + d )
                {
                ++ outsideStdv_l[d];
                }
              } //
            } // for ( vtkIdType r = 0; r < n; ++ r )

          // Sum all local counters
          int* outsideStdv_g = new int[numRuleVal];
          com->AllReduce( outsideStdv_l,
                          outsideStdv_g,
                          numRuleVal,
                          vtkCommunicator::SUM_OP );

          // Print out percentages of sample points within 1, ..., numRuleVal standard deviations from the mean.
          if ( myRank == args->ioRank )
            {
            cout << "   "
                 << outputData->GetColumnName( nUniform + c )
                 << ":\n";
            for ( int d = 0; d < numRuleVal; ++ d )
              {
              double testVal = ( 1. - outsideStdv_g[d] / static_cast<double>( outputPrimary->GetValueByName( 0, "Cardinality" ).ToInt() ) ) * 100.;

              cout << "      "
                   << testVal
                   << "% within "
                   << d + 1
                   << " standard deviation(s) from the mean.\n";

              // Test some statistics
              if ( fabs ( testVal - sigmaRuleVal[d] ) > sigmaRuleTol[d] )
                {
                vtkGenericWarningMacro("Incorrect value.");
                *(args->retVal) = 1;
                }
              }
            }

          // Clean up
          delete [] outsideStdv_l;
          delete [] outsideStdv_g;
          } // if ( relDev )
        else
          {
          vtkGenericWarningMacro("Column "
                                 << relDevName.str().c_str()
                                 << " on process "
                                 << myRank
                                 << " is not of type double." );
          *(args->retVal) = 1;
          }

        } // if ( relDevArr )
      else
        {
        vtkGenericWarningMacro("No assessment column called "
                               << relDevName.str().c_str()
                               << " on process "
                               << myRank);
        *(args->retVal) = 1;
        }
      } // for ( int c = 0; c < nNormal; ++ c )

    // Clean up
    pds->Delete();
    } // if ( ! args->skipPDescriptive )
  else if ( myRank == args->ioRank )
    {
    cout << "\n## Skipped calculation of parallel descriptive statistics.\n";
    }

  // Cross-verify aggregated serial vs. parallel results only if both were calculated
  if ( ! args->skipDescriptive && ! args->skipPDescriptive )
    {
    if ( myRank == args->ioRank )
      {
      cout << "\n## Cross-verifying aggregated serial vs. parallel descriptive statistics (within "
           << args->absTol
           << " absolute tolerance).\n";
      for ( int i = 0; i < n2Rows; ++ i )
        {
        if ( fabs( cardsAndMeans_agg[i] - cardsAndMeans_par[i] ) > args->absTol )
          {
          vtkGenericWarningMacro("Incorrect value(s) : "
                                 << cardsAndMeans_agg[i]
                                 << " <> "
                                 << cardsAndMeans_par[i]);
          *(args->retVal) = 1;
          }
        if ( extrema_agg[i] != extrema_par[i] )
          {
          vtkGenericWarningMacro("Incorrect value(s) : "
                                 << extrema_agg[i]
                                 << " <> "
                                 << extrema_par[i]);
          *(args->retVal) = 1;
          }
        } // for ( int i = 0; i < n2Rows; ++ i )
      } // if ( myRank == args->ioRank )
    } // if ( ! args->skipPDescriptive && ! args->skipPDescriptive )
  else if ( myRank == args->ioRank )
    {
    cout << "\n## Skipped cross-verification of aggregated serial vs. parallel descriptive statistics.\n";
    }


  // Clean up
  delete [] cardsAndMeans_agg;
  delete [] cardsAndMeans_par;
  delete [] extrema_agg;
  delete [] extrema_par;

  // ************************** Parallel Auto-Correlative Statistics **************************

  // Skip parallel correlative statistics if requested
  if ( ! args->skipPAutoCorrelative )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel auto-correlative statistics engine and set its input
    vtkPAutoCorrelativeStatistics* pas = vtkPAutoCorrelativeStatistics::New();
    pas->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select all columns
    for ( int c = 0; c < nVariables; ++ c )
      {
      pas->AddColumn( columnNames[c] );
      }

    // Create input parameter table for the stationary case
    vtkIdTypeArray* timeLags = vtkIdTypeArray::New();
    timeLags->SetName( "Time Lags" );
    timeLags->SetNumberOfTuples( 1 );
    timeLags->SetValue( 0, 0 );
    vtkTable* paramTable = vtkTable::New();
    paramTable->AddColumn( timeLags );
    timeLags->Delete();

    // Set spatial cardinality
    pas->SetSliceCardinality( args->nVals ); 

    // Set parameters for autocorrelation of whole data set with respect to itself
    pas->SetInputData( vtkStatisticsAlgorithm::LEARN_PARAMETERS, paramTable );
    paramTable->Delete();

    // Test (in parallel) with Learn, Derive operations turned on
    pas->SetLearnOption( true );
    pas->SetDeriveOption( true );
    pas->SetAssessOption( false );
    pas->SetTestOption( false );
    pas->Update();

    // Get output data and meta tables
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pas->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    if ( myRank == args->ioRank )
      {
      cout << "\n## Completed parallel calculation of auto-correlative statistics (without assessment):\n"
           << "   Total sample size: "
           << vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) )->GetValueByName( 0, "Cardinality").ToInt()
           << " \n"
           << "   Wall time: "
           << timer->GetElapsedTime()
           << " sec.\n";

      cout << "   Calculated the following statistics:\n";
      unsigned int nbm1 = outputMetaDS->GetNumberOfBlocks() - 1;
      for ( unsigned int b = 0; b < nbm1; ++ b )
        {
        const char* tabName = outputMetaDS->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );
        cerr << "   "
             << tabName
             << "\n";
        vtkTable* outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( b ) );
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
        }
      const char* tabName = outputMetaDS->GetMetaData( nbm1 )->Get( vtkCompositeDataSet::NAME() );
      cerr << "   "
           << tabName
           << "\n";
      vtkTable* outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( nbm1 ) );
      outputMeta->Dump();
      }

    // Clean up
    pas->Delete();
    } // if ( ! args->skipPAutoCorrelative )
  else if ( myRank == args->ioRank )
    {
    cout << "\n## Skipped calculation of parallel auto-correlative statistics.\n";
    }

  // ************************** Parallel Correlative Statistics **************************

  // Skip parallel correlative statistics if requested
  if ( ! args->skipPCorrelative )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel correlative statistics engine and set its input
    vtkPCorrelativeStatistics* pcs = vtkPCorrelativeStatistics::New();
    pcs->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

    // Select column pairs (uniform vs. uniform, normal vs. normal)
    pcs->AddColumnPair( columnNames[0], columnNames[1] );
    pcs->AddColumnPair( columnNames[2], columnNames[3] );

    // Test (in parallel) with Learn, Derive operations turned on
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

    if ( myRank == args->ioRank )
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
    } // if ( ! args->skipPCorrelative )
  else if ( myRank == args->ioRank )
    {
    cout << "\n## Skipped calculation of parallel correlative statistics.\n";
    }

  // ************************** Parallel Multi-Correlative Statistics **************************

  // Skip parallel multi-correlative statistics if requested
  if ( ! args->skipPMultiCorrelative )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel correlative statistics engine and set its ports
    vtkPMultiCorrelativeStatistics* pmcs = vtkPMultiCorrelativeStatistics::New();
    pmcs->SetInputData( 0, inputData );

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

    // Test (in parallel) with Learn, Derive, and Assess operations turned on
    // Test is not implemented for multi-correlative
    pmcs->SetLearnOption( true );
    pmcs->SetDeriveOption( true );
    pmcs->SetAssessOption( true );
    pmcs->SetTestOption( true );
    pmcs->Update();

    // Get output meta tables
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pmcs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    if ( myRank == args->ioRank )
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
    } // if ( ! args->skipPMultiCorrelative )
  else if ( myRank == args->ioRank )
    {
    cout << "\n## Skipped calculation of parallel multi-correlative statistics.\n";
    }

  // ************************** Parallel PCA Statistics **************************

  // Skip parallel PCA statistics if requested
  if ( ! args->skipPPCA )
    {
    // Synchronize and start clock
    com->Barrier();
    timer->StartTimer();

    // Instantiate a parallel pca statistics engine and set its ports
    vtkPPCAStatistics* pcas = vtkPPCAStatistics::New();
    pcas->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, inputData );

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

    // Test (in parallel) with all operations except for Test (not implemented in parallel for PCA)
    pcas->SetLearnOption( true );
    pcas->SetDeriveOption( true );
    pcas->SetAssessOption( true );
    pcas->SetTestOption( false );
    pcas->Update();

    // Get output meta tables
    vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( pcas->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

    // Synchronize and stop clock
    com->Barrier();
    timer->StopTimer();

    if ( myRank == args->ioRank )
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
    } // if ( ! args->skipPPCA )
  else if ( myRank == args->ioRank )
    {
    cout << "\n## Skipped calculation of parallel PCA statistics.\n";
    }

  // Clean up
  inputData->Delete();
  timer->Delete();
}

}

//----------------------------------------------------------------------------
int TestRandomPMomentStatisticsMPI( int argc, char* argv[] )
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

  // Get local rank and print out of I/O node
  int myRank = com->GetLocalProcessId();
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
  // Set default argument values
  int nVals = 100000;
  double absTol = 1.e-6;
  bool skipDescriptive = false;
  bool skipPDescriptive = false;
  bool skipPAutoCorrelative = false;
  bool skipPCorrelative = false;
  bool skipPMultiCorrelative = false;
  bool skipPPCA = false;

  // Initialize command line argument parser
  vtksys::CommandLineArguments clArgs;
  clArgs.Initialize( argc, argv );
  clArgs.StoreUnusedArguments( false );

  // Parse per-process cardinality of each pseudo-random sample
  clArgs.AddArgument("--n-per-proc",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &nVals, "Per-process cardinality of each pseudo-random sample");

  // Parse absolute tolerance to cross-verify aggregated serial vs. parallel descriptive stats
  clArgs.AddArgument("--abs-tol",
                     vtksys::CommandLineArguments::SPACE_ARGUMENT,
                     &absTol, "Absolute tolerance to cross-verify aggregated serial and parallel descriptive statistics");

  // Parse whether serial descriptive statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-Descriptive",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipDescriptive, "Skip serial descriptive statistics");

  // Parse whether parallel descriptive statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-PDescriptive",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipPDescriptive, "Skip parallel descriptive statistics");

  // Parse whether parallel auto-correlative statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-PAutoCorrelative",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipPAutoCorrelative, "Skip parallel auto-correlative statistics");

  // Parse whether parallel correlative statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-PCorrelative",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipPCorrelative, "Skip parallel correlative statistics");

  // Parse whether parallel multi-correlative statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-PMultiCorrelative",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipPMultiCorrelative, "Skip parallel multi-correlative statistics");

  // Parse whether parallel PCA statistics should be skipped (for faster testing)
  clArgs.AddArgument("--skip-PPCA",
                     vtksys::CommandLineArguments::NO_ARGUMENT,
                     &skipPPCA, "Skip parallel PCA statistics");

  // If incorrect arguments were provided, provide some help and terminate in error.
  if ( ! clArgs.Parse() )
    {
    if ( myRank == ioRank )
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
  args.nVals = nVals;
  args.absTol = absTol;
  args.skipDescriptive = skipDescriptive;
  args.skipPDescriptive = skipPDescriptive;
  args.skipPAutoCorrelative = skipPAutoCorrelative;
  args.skipPCorrelative = skipPCorrelative;
  args.skipPMultiCorrelative = skipPMultiCorrelative;
  args.skipPPCA = skipPPCA;
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
