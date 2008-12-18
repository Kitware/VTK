/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories 
// for implementing this test.

#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkCorrelativeStatistics.h"

//=============================================================================
int TestCorrelativeStatistics( int, char *[] )
{
  int testStatus = 0;

  double mingledData[] = 
    {
    46,
    45,
    47,
    49,
    46,
    47,
    46,
    46,
    47,
    46,
    47,
    49,
    49,
    49,
    47,
    45,
    50,
    50,
    46,
    46,
    51,
    50,
    48,
    48,
    52,
    54,
    48,
    47,
    52,
    52,
    49,
    49,
    53,
    54,
    50,
    50,
    53,
    54,
    50,
    52,
    53,
    53,
    50,
    51,
    54,
    54,
    49,
    49,
    52,
    52,
    50,
    51,
    52,
    52,
    49,
    47,
    48,
    48,
    48,
    50,
    46,
    48,
    47,
    47,
    };
  int nVals = 32;

  vtkDoubleArray* dataset1Arr = vtkDoubleArray::New();
  dataset1Arr->SetNumberOfComponents( 1 );
  dataset1Arr->SetName( "Metric 0" );

  vtkDoubleArray* dataset2Arr = vtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "Metric 1" );

  vtkDoubleArray* dataset3Arr = vtkDoubleArray::New();
  dataset3Arr->SetNumberOfComponents( 1 );
  dataset3Arr->SetName( "Metric 2" );

  for ( int i = 0; i < nVals; ++ i )
    {
    int ti = i << 1;
    dataset1Arr->InsertNextValue( mingledData[ti] );
    dataset2Arr->InsertNextValue( mingledData[ti + 1] );
    dataset3Arr->InsertNextValue( -1. );
    }

  vtkTable* datasetTable = vtkTable::New();
  datasetTable->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable->AddColumn( dataset2Arr );
  dataset2Arr->Delete();
  datasetTable->AddColumn( dataset3Arr );
  dataset3Arr->Delete();

  int nMetricPairs = 3;
  vtkStdString columnPairs[] = { "Metric 0", "Metric 1", "Metric 1", "Metric 0", "Metric 2", "Metric 1" };
  double centers[] = { 49.2188, 49.5 };
  double covariance[] = { 5.98286, 7.54839, 6.14516 };
  double threshold = 4.;

  vtkCorrelativeStatistics* haruspex = vtkCorrelativeStatistics::New();
  haruspex->SetInput( 0, datasetTable );
  vtkTable* outputData = haruspex->GetOutput( 0 );
  vtkTable* outputMeta = haruspex->GetOutput( 1 );

  datasetTable->Delete();

// -- Select Column Pairs of Interest ( Learn Mode ) -- 
  haruspex->AddColumnPair( "Metric 0", "Metric 1" ); // A valid pair
  haruspex->AddColumnPair( "Metric 1", "Metric 0" ); // The same valid pair, just reversed
  haruspex->AddColumnPair( "Metric 2", "Metric 1" ); // Another valid pair
  for ( int i = 0; i< nMetricPairs; i += 2 )
    {  // Try to add all valid pairs once more
    haruspex->AddColumnPair( columnPairs[i], columnPairs[i+1] );
    }
  haruspex->AddColumnPair( "Metric 1", "Metric 3" ); // An invalid pair

// -- Test Learn Mode -- 
  haruspex->SetLearn( true );
  haruspex->SetDerive( true );
  haruspex->SetAssess( false );
  haruspex->Update();
  vtkIdType n = haruspex->GetSampleSize();

  cout << "## Calculated the following statistics ( "
       << n
       << " entries per column ):\n";
  for ( vtkIdType r = 0; r < outputMeta->GetNumberOfRows(); ++ r )
    {
    cout << "   (X, Y) = ("
         << outputMeta->GetValue( r, 0 ).ToString()
         << ", "
         << outputMeta->GetValue( r, 1 ).ToString()
         << ")";

    for ( int i = 2; i < 7; ++ i )
      {
      cout << ", "
           << outputMeta->GetColumnName( i )
           << "="
           << outputMeta->GetValue( r, i ).ToDouble();
      }

    if ( outputMeta->GetValueByName( r,  "Linear Correlation" ).ToString() == vtkStdString( "valid" ) )
      {
      cout << "\n   Y = "
           << outputMeta->GetValueByName( r, "Slope Y/X" ).ToDouble()
           << " * X + "
           << outputMeta->GetValueByName( r, "Intersect Y/X" ).ToDouble()
           << ", X = "
           << outputMeta->GetValueByName( r, "Slope X/Y" ).ToDouble()
           << " * Y + "
           << outputMeta->GetValueByName( r, "Intersect X/Y" ).ToDouble()
           << ", corr. coeff.: "
           << outputMeta->GetValueByName( r, "Pearson r" ).ToDouble()
           << "\n";
      }
    else
      {
      cout << "\n   Degenerate input, linear correlation was not calculated.\n";
      }
    }

// -- Select Column Pairs of Interest ( Assess Mode ) -- 
  haruspex->ResetColumnPairs(); // Clear existing pairs
  haruspex->AddColumnPair( columnPairs[0], columnPairs[1] ); // A valid pair

// -- Test Assess Mode -- 
  cout << "## Searching for outliers with respect to this bivariate Gaussian distribution:\n"
       << "   (X, Y) = ("
       << columnPairs[0]
       << ", "
       << columnPairs[1]
       << "), mean=("
       << centers[0]
       << ", "
       << centers[1]
       << "), covariance=["
       << covariance[0]
       << ", "
       << covariance[2]
       << " ; "
       << covariance[2]
       << ", "
       << covariance[1]
       << "], Squared Mahalanobis > "
       << threshold
       << "\n";

  vtkTable* paramsTable = vtkTable::New();
  paramsTable->ShallowCopy( outputMeta );
  paramsTable->SetValueByName( 0, "Mean X", centers[0] );
  paramsTable->SetValueByName( 0, "Mean Y", centers[1] );
  paramsTable->SetValueByName( 0, "Variance X", covariance[0] );
  paramsTable->SetValueByName( 0, "Variance Y", covariance[1] );
  paramsTable->SetValueByName( 0, "Covariance", covariance[2] );
  
  haruspex->SetInput( 1, paramsTable );
  haruspex->SetLearn( false );
  haruspex->SetDerive( false ); // Do not recalculate nor rederive a model
  haruspex->SetAssess( true );
  haruspex->Update();

  int nOutliers = 0;
  int tableIdx[] = { 0, 1, 3 };
  cout << "   Found the following outliers:\n";
  for ( int i = 0; i < 3; ++ i )
    {
    cout << "   "
         << outputData->GetColumnName( tableIdx[i] );
    }
  cout << "\n";

  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    if ( outputData->GetValue( r, tableIdx[2] ).ToDouble() > threshold )
      {
      ++ nOutliers;

      for ( int i = 0; i < 3; ++ i )
        {
        cout << "     "
             << outputData->GetValue( r,  tableIdx[i] ).ToDouble()
             << "    ";
        }
      cout << "\n";
      }
    }

  if ( nOutliers != 3 )
    {
    vtkGenericWarningMacro("Expected 3 outliers, found " << nOutliers << ".");
    testStatus = 1;
    }

  paramsTable->Delete();
  haruspex->Delete();

  return testStatus;
}
