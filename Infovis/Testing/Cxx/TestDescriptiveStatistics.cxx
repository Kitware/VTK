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
#include "vtkTable.h"
#include "vtkDescriptiveStatistics.h"

//=============================================================================
int TestDescriptiveStatistics( int, char *[] )
{
  int testStatus = 0;

  // ************** More complex example comprising three columns ************** 

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
  haruspex->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable );
  vtkTable* outputData = haruspex->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkTable* outputMeta = haruspex->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );

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
      vtkGenericWarningMacro("Incorrect mean");
      testStatus = 1;
      }

    if ( fabs ( outputMeta->GetValueByName( r, "Standard Deviation" ).ToDouble() - stdevs[r] ) > 1.e6 )
      {
      vtkGenericWarningMacro("Incorrect standard deviation");
      testStatus = 1;
      }
    cout << "\n";
    }

  cout << "## Searching for outliers:\n";

  int m0outliers = 0;
  int m1outliers = 0;
  cout << "Outliers:\n";
  vtkDoubleArray* m0reld = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "d(Metric 0)" ) );
  vtkDoubleArray* m1reld = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "d(Metric 1)" ) );
  vtkDoubleArray* m0vals = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "Metric 0" ) );
  vtkDoubleArray* m1vals = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "Metric 1" ) );

  if ( ! m0reld || ! m1reld || ! m0vals || ! m1vals )
    {
    vtkGenericWarningMacro("Empty output column(s).\n");
    testStatus = 1;

    return testStatus;
    }

  double dev;
  double maxdev = 1.5;
  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    dev = m0reld->GetValue( r );
    if ( dev > maxdev )
      {
      ++ m0outliers;
      cout << "   " 
           << " row " 
           << r
           << ", "
           << m0reld->GetName() 
           << " = " 
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
    dev = m1reld->GetValue( r );
    if ( dev > maxdev )
      {
      ++ m1outliers;
      cout << "   " 
           << " row " 
           << r 
           << ", "
           << m1reld->GetName() 
           << " = " 
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
    vtkGenericWarningMacro("Expected 4 outliers for Metric 0 and 6 outliers for Metric 1.");
    testStatus = 1;
    }

// -- Used modified output 1 as input 1 to test 0-deviation -- 
  cout << "Re-running with mean 50 and deviation 0 for metric 1:\n";

  vtkTable* paramsTable = vtkTable::New();
  paramsTable->ShallowCopy( outputMeta );
  paramsTable->SetValueByName( 1, "Standard Deviation", 0. );
  paramsTable->SetValueByName( 1, "Mean", 50. );
  
  haruspex->SetInput( vtkStatisticsAlgorithm::INPUT_MODEL, paramsTable );
  haruspex->SetLearn( false );
  haruspex->SetDerive( false ); // Do not recalculate nor rederive a model
  haruspex->SetAssess( true );
  haruspex->Update();

  m1vals = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "Metric 1" ) );
  m1reld = vtkDoubleArray::SafeDownCast(
    outputData->GetColumnByName( "d(Metric 1)" ) );

  if ( ! m1reld || ! m1vals )
    {
    vtkGenericWarningMacro("Empty output column(s).\n");
    testStatus = 1;

    return testStatus;
    }

  m1outliers = 0;

  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    dev = m1reld->GetValue( r );
    if ( dev )
      {
      ++ m1outliers;
      cout << "   " 
           << " row " 
           << r
           << ", "
           << m1reld->GetName() 
           << " = " 
           << dev
           << " (value: " 
           << m1vals->GetValue( r ) 
           << ")\n";
      }
    }
  if ( m1outliers != 28 )
    {
    vtkGenericWarningMacro("Expected 28 outliers for Metric 1, found " << m1outliers << ".");
    testStatus = 1;
    }

  paramsTable->Delete();
  haruspex->Delete();

  // ************** Very simple example, for baseline comparison vs. R ********* 
  double simpleData[] = 
    {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    };
  int nSimpleVals = 10;

  vtkDoubleArray* datasetArr = vtkDoubleArray::New();
  datasetArr->SetNumberOfComponents( 1 );
  datasetArr->SetName( "Metric" );

  for ( int i = 0; i < nSimpleVals; ++ i )
    {
    datasetArr->InsertNextValue( simpleData[i] );
    }

  vtkTable* simpleTable = vtkTable::New();
  simpleTable->AddColumn( datasetArr );
  datasetArr->Delete();

  double mean = 4.5;
  double variance = 9.16666666666667;
  double g1 = 0.;
  double g2 = -1.56163636363636;

  vtkDescriptiveStatistics* ds = vtkDescriptiveStatistics::New();
  ds->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, simpleTable );
  vtkTable* outputSimpleMeta = ds->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );

  simpleTable->Delete();

  // -- Select Column of Interest -- 
  ds->AddColumn( "Metric" );

  // -- Test Learn and Derive only -- 
  ds->SetLearn( true );
  ds->SetDerive( true );
  ds->SetAssess( false );
  ds->Update();


  cout << "## Calculated the following statistics ( "
       <<  outputSimpleMeta->GetValueByName( 0, "Cardinality" ).ToInt() 
       << " entries in a single column ):\n"
       << "   ";
  
  for ( int i = 0; i < outputSimpleMeta->GetNumberOfColumns(); ++ i )
    {
    cout << outputSimpleMeta->GetColumnName( i )
         << "="
         << outputSimpleMeta->GetValue( 0, i ).ToString()
         << "  ";
    }
    
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "Mean" ).ToDouble() - mean ) > 1.e6 )
    {
    vtkGenericWarningMacro("Incorrect mean");
    testStatus = 1;
    }
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "Variance" ).ToDouble() - variance ) > 1.e6 )
    {
    vtkGenericWarningMacro("Incorrect variance");
    testStatus = 1;
    }
  cout << "\n";
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "G1 Skewness" ).ToDouble() - g1 ) > 1.e6 )
    {
    vtkGenericWarningMacro("Incorrect G1 skewness");
    testStatus = 1;
    }
  cout << "\n";
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "G2 Kurtosis" ).ToDouble() - g2 ) > 1.e6 )
    {
    vtkGenericWarningMacro("Incorrect G2 kurtosis");
    testStatus = 1;
    }
  cout << "\n";

  ds->Delete();

  return testStatus;
}
