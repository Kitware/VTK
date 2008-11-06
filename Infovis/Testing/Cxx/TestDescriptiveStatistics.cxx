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

#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"
#include "vtkTable.h"
#include "vtkDescriptiveStatistics.h"

//=============================================================================
int TestDescriptiveStatistics( int, char *[] )
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

  int nMetrics = 3;
  vtkStdString columns[] = { "Metric 1", "Metric 2", "Metric 0" };
  double means[] = { 49.5, -1., 49.2188 };
  double stdevs[] = { sqrt( 7.54839 ), 0., sqrt( 5.98286 ) };

  vtkDescriptiveStatistics* haruspex = vtkDescriptiveStatistics::New();
  haruspex->SetInput( 0, datasetTable );
  vtkTable* outputData = haruspex->GetOutput( 0 );
  vtkTable* outputMeta = haruspex->GetOutput( 1 );

  datasetTable->Delete();

// -- Select Columns of Interest -- 
  haruspex->AddColumn( "Metric 3" ); // Include invalid Metric 3
  haruspex->AddColumn( "Metric 4" ); // Include invalid Metric 4
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    haruspex->AddColumn( columns[i] );
    }
  haruspex->RemoveColumn( "Metric 3" ); // Remove invalid Metric 3 (but keep 4)

// -- Test Learn and Assess Modes -- 
  haruspex->SetLearn( true );
  haruspex->SetDerive( true );
  haruspex->SetAssess( true );
  haruspex->SignedDeviationsOff();
  haruspex->Update();
  vtkIdType n = haruspex->GetSampleSize();

  cout << "## Calculated the following statistics ( "
       << n
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
    
    if ( fabs ( outputMeta->GetValueByName( r, "Mean" ).ToDouble() - means[r] ) > 1.e6 )
      {
      cout << "** Incorrect mean ** ";
      testStatus = 1;
      }

    if ( fabs ( outputMeta->GetValueByName( r, "Standard Deviation" ).ToDouble() - stdevs[r] ) > 1.e6 )
      {
      cout << "** Incorrect standard deviation **";
      testStatus = 1;
      }
    cout << "\n";
    }

  cout << "## Searching for outliers:\n";

  int m0outliers = 0;
  int m1outliers = 0;
  cout << "Outliers:\n";
  vtkVariantArray* m0reld = vtkVariantArray::SafeDownCast(
    outputData->GetColumnByName( "Relative Deviation(Metric 0)" ) );
  vtkVariantArray* m1reld = vtkVariantArray::SafeDownCast(
    outputData->GetColumnByName( "Relative Deviation(Metric 1)" ) );
  vtkDoubleArray* m0vals = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "Metric 0" ) );
  vtkDoubleArray* m1vals = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "Metric 1" ) );

  if ( ! m0reld || ! m1reld || ! m0vals || ! m1vals )
    {
    cout << "Error: "
         << "Empty output column(s).\n";
    testStatus = 1;

    return testStatus;
    }

  double dev;
  double maxdev = 1.5;
  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    dev = m0reld->GetValue( r ).ToDouble();
    if ( dev > maxdev )
      {
      ++ m0outliers;
      cout << "   " 
           << m0reld->GetName() 
           << " row " 
           << r 
           << " deviation " 
           << dev 
           << " > " 
           << maxdev
           << " (value: " 
           << m0vals->GetValue( r ) 
           << ")\n";
      }
    }
  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    dev = m1reld->GetValue( r ).ToDouble();
    if ( dev > maxdev )
      {
      ++ m1outliers;
      cout << "   " 
           << m1reld->GetName() 
           << " row " 
           << r 
           << " deviation " 
           << dev 
           << " > " 
           << maxdev
           << " (value: " 
           << m1vals->GetValue( r ) 
           << ")\n";
      }
    }
  cout
    << "Found " << m0outliers << " outliers for Metric 0"
    << " and " << m1outliers << " outliers for Metric 1.\n";
  if ( m0outliers != 4 || m1outliers != 6 )
    {
    cout
      << "Error: "
      << "Expected 4 outliers for Metric 0"
      << " and 6 outliers for Metric 1.\n";
    testStatus = 1;
    }

// -- Used modified output 1 as input 1 to test 0-deviation -- 
  cout << "Re-running with mean 50 and deviation 0 for metric 1:\n";

  vtkTable* paramsTable = vtkTable::New();
  paramsTable->ShallowCopy( outputMeta );
  paramsTable->SetValueByName( 1, "Standard Deviation", 0. );
  paramsTable->SetValueByName( 1, "Mean", 50. );
  
  haruspex->SetInput( 1, paramsTable );
  haruspex->SetLearn( false );
  haruspex->SetDerive( false ); // Do not recalculate nor rederive a model
  haruspex->SetAssess( true );
  haruspex->Update();

  m1vals = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "Metric 1" ) );
  m1reld = vtkVariantArray::SafeDownCast(
    outputData->GetColumnByName( "Relative Deviation(Metric 1)" ) );

  if ( ! m1reld || ! m1vals )
    {
    cout << "Error: "
         << "Empty output column(s).\n";
    testStatus = 1;

    return testStatus;
    }

  m1outliers = 0;

  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    dev = m1reld->GetValue( r ).ToDouble();
    if ( dev )
      {
      ++ m1outliers;
      cout << "   " 
           << m1reld->GetName() 
           << " row " 
           << r 
           << ": " 
           << dev
           << " value " 
           << m1vals->GetValue( r ) 
           << "\n";
      }
    }
  if ( m1outliers != 28 )
    {
    cout << "Error: Expected 28 outliers for Metric 1, found " << m1outliers << ".\n";
    testStatus = 1;
    }

  paramsTable->Delete();
  haruspex->Delete();

  return testStatus;
}
