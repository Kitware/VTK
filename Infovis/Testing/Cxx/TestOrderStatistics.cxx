/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkOrderStatistics.h"

//=============================================================================
int TestOrderStatistics( int, char *[] )
{
  int testIntValue = 0;

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

  vtkTable* paramsTable = vtkTable::New();
  int nMetrics = 3;
  vtkStdString columns[] = { "Metric 1", "Metric 2", "Metric 0" };
  double centers[] = { 49.5, -1., 49.2188 };
  double radii[] = { 1.5 * sqrt( 7.54839 ), 0., 1.5 * sqrt( 5.98286 ) };

  vtkStringArray* stdStringCol = vtkStringArray::New();
  stdStringCol->SetName( "Column" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    stdStringCol->InsertNextValue( columns[i] );
    }
  paramsTable->AddColumn( stdStringCol );
  stdStringCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Nominal" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    doubleCol->InsertNextValue( centers[i] );
    }
  paramsTable->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Deviation" );
  for ( int i = 0; i < nMetrics; ++ i )
    {
    doubleCol->InsertNextValue( radii[i] );
    }
  paramsTable->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkOrderStatistics* haruspex = vtkOrderStatistics::New();
  haruspex->SetInput( 0, datasetTable );
  haruspex->SetInput( 1, paramsTable );
  vtkTable* outputMeta = haruspex->GetOutput( 1 );

  datasetTable->Delete();
  paramsTable->Delete();

// -- Select Columns of Interest -- 
  haruspex->AddColumn( "Metric 3" ); // Include invalid Metric 3
  haruspex->AddColumn( "Metric 4" ); // Include invalid Metric 4
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    haruspex->AddColumn( columns[i] );
    }
  haruspex->RemoveColumn( "Metric 3" ); // Remove invalid Metric 3 (but retain 4)

// -- Test Learn Mode for quartiles with InverseCDFAveragedSteps quantile definition -- 
  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
  haruspex->Update();

  double valsTest1 [] = { 0.,
    46., 47., 49., 51.5, 54.,
    45., 47., 49., 52., 54.,
    -1., -1., -1., -1., -1.,
  };
  cout << "## Calculated the following 5-points statistics with InverseCDF quantile definition ( "
       << haruspex->GetSampleSize()
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

      if ( i && outputMeta->GetValue( r, i ).ToDouble() != valsTest1[r * 5 + i] )
        {
        testIntValue = 1;
        cout << " !! <> "
             << valsTest1[r * 5 + i]
             << " !!";
        }
      }
    cout << "\n";

    if ( testIntValue )
      {
      vtkGenericWarningMacro("Incorrect 5-points statistics");
      return 1;
      }
    }

// -- Test Learn Mode for quartiles with InverseCDF quantile definition -- 
  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
  haruspex->SetQuantileDefinition( vtkOrderStatistics::InverseCDF );
  haruspex->Update();

  double valsTest2 [] = { 0.,
    46., 47., 49., 51., 54.,
    45., 47., 49., 52., 54.,
    -1., -1., -1., -1., -1.,
  };
  cout << "## Calculated the following 5-points statistics with InverseCDFAveragedSteps quantile definition ( "
       << haruspex->GetSampleSize()
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

      if ( i && outputMeta->GetValue( r, i ).ToDouble() != valsTest2[r * 5 + i] )
        {
        testIntValue = 1;
        cout << " !! <> "
             << valsTest2[r * 5 + i]
             << " !!";
        }
      }
    cout << "\n";
    }

// -- Test Learn Mode for deciles with InverseCDF quantile definition (as with Octave) -- 
  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
  haruspex->SetQuantileDefinition( 0 ); // 0: vtkOrderStatistics::InverseCDF
  haruspex->SetNumberOfIntervals( 10 );
  haruspex->Update();

  cout << "## Calculated the following deciles with InverseCDF quantile definition ( "
       << haruspex->GetSampleSize()
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

  haruspex->Delete();

  return 0;
}
