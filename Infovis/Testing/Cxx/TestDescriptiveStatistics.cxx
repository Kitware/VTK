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

#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkDescriptiveStatistics.h"

//=============================================================================
int TestDescriptiveStatistics( int, char *[] )
{
  int testStatus = 0;

  // ************** Test with 3 columns of input data ************** 

  // Input data
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

  // Test with entire data set
  int nVals1 = 32;

  vtkDoubleArray* dataset1Arr = vtkDoubleArray::New();
  dataset1Arr->SetNumberOfComponents( 1 );
  dataset1Arr->SetName( "Metric 0" );

  vtkDoubleArray* dataset2Arr = vtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "Metric 1" );

  vtkDoubleArray* dataset3Arr = vtkDoubleArray::New();
  dataset3Arr->SetNumberOfComponents( 1 );
  dataset3Arr->SetName( "Metric 2" );

  for ( int i = 0; i < nVals1; ++ i )
    {
    int ti = i << 1;
    dataset1Arr->InsertNextValue( mingledData[ti] );
    dataset2Arr->InsertNextValue( mingledData[ti + 1] );
    dataset3Arr->InsertNextValue( -1. );
    }

  vtkTable* datasetTable1 = vtkTable::New();
  datasetTable1->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable1->AddColumn( dataset2Arr );
  dataset2Arr->Delete();
  datasetTable1->AddColumn( dataset3Arr );
  dataset3Arr->Delete();

  int nMetrics = 3;
  vtkStdString columns[] = { "Metric 1", "Metric 2", "Metric 0" };
  double means[] = { 49.5, -1., 49.2188 };
  double stdevs[] = { sqrt( 7.54839 ), 0., sqrt( 5.98286 ) };

  vtkDescriptiveStatistics* ds1 = vtkDescriptiveStatistics::New();
  ds1->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable1 );
  vtkTable* outputData1 = ds1->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkTable* outputMeta1 = ds1->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );

  datasetTable1->Delete();

  // Select Columns of Interest
  ds1->AddColumn( "Metric 3" ); // Include invalid Metric 3
  ds1->AddColumn( "Metric 4" ); // Include invalid Metric 4
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    ds1->AddColumn( columns[i] );
    }
  ds1->RemoveColumn( "Metric 3" ); // Remove invalid Metric 3 (but keep 4)

  // Run with Learn and Assess options
  ds1->SetLearnOption( true );
  ds1->SetDeriveOption( true );
  ds1->SetAssessOption( true );
  ds1->SignedDeviationsOff();
  ds1->Update();

  for ( vtkIdType r = 0; r < outputMeta1->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputMeta1->GetNumberOfColumns(); ++ i )
      {
      cout << outputMeta1->GetColumnName( i )
           << "="
           << outputMeta1->GetValue( r, i ).ToString()
           << "  ";
      }
    
    if ( fabs ( outputMeta1->GetValueByName( r, "Mean" ).ToDouble() - means[r] ) > 1.e6 )
      {
      vtkGenericWarningMacro("Incorrect mean");
      testStatus = 1;
      }

    if ( fabs ( outputMeta1->GetValueByName( r, "Standard Deviation" ).ToDouble() - stdevs[r] ) > 1.e6 )
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
    outputData1->GetColumnByName( "d(Metric 0)" ) );
  vtkDoubleArray* m1reld = vtkDoubleArray::SafeDownCast(
    outputData1->GetColumnByName( "d(Metric 1)" ) );
  vtkDoubleArray* m0vals = vtkDoubleArray::SafeDownCast(
    outputData1->GetColumnByName( "Metric 0" ) );
  vtkDoubleArray* m1vals = vtkDoubleArray::SafeDownCast(
    outputData1->GetColumnByName( "Metric 1" ) );

  if ( ! m0reld || ! m1reld || ! m0vals || ! m1vals )
    {
    vtkGenericWarningMacro("Empty output column(s).\n");
    testStatus = 1;

    return testStatus;
    }

  double dev;
  double maxdev = 1.5;
  for ( vtkIdType r = 0; r < outputData1->GetNumberOfRows(); ++ r )
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
  for ( vtkIdType r = 0; r < outputData1->GetNumberOfRows(); ++ r )
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

  // Used modified output 1 as input 1 to test 0-deviation
  cout << "Re-running with mean 50 and deviation 0 for metric 1:\n";

  vtkTable* paramsTable = vtkTable::New();
  paramsTable->ShallowCopy( outputMeta1 );
  paramsTable->SetValueByName( 1, "Standard Deviation", 0. );
  paramsTable->SetValueByName( 1, "Mean", 50. );
  
  // Run with Assess option only (do not recalculate nor rederive a model)
  ds1->SetInput( vtkStatisticsAlgorithm::INPUT_MODEL, paramsTable );
  ds1->SetLearnOption( false );
  ds1->SetDeriveOption( false ); 
  ds1->SetAssessOption( true );
  ds1->Update();

  m1vals = vtkDoubleArray::SafeDownCast(
    outputData1->GetColumnByName( "Metric 1" ) );
  m1reld = vtkDoubleArray::SafeDownCast(
    outputData1->GetColumnByName( "d(Metric 1)" ) );

  if ( ! m1reld || ! m1vals )
    {
    vtkGenericWarningMacro("Empty output column(s).\n");
    testStatus = 1;

    return testStatus;
    }

  m1outliers = 0;

  for ( vtkIdType r = 0; r < outputData1->GetNumberOfRows(); ++ r )
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

  // Learn another model with subset (sample size: 20) of initial data set
  int nVals2 = 20;

  vtkDoubleArray* dataset4Arr = vtkDoubleArray::New();
  dataset4Arr->SetNumberOfComponents( 1 );
  dataset4Arr->SetName( "Metric 0" );

  vtkDoubleArray* dataset5Arr = vtkDoubleArray::New();
  dataset5Arr->SetNumberOfComponents( 1 );
  dataset5Arr->SetName( "Metric 1" );

  vtkDoubleArray* dataset6Arr = vtkDoubleArray::New();
  dataset6Arr->SetNumberOfComponents( 1 );
  dataset6Arr->SetName( "Metric 2" );

  for ( int i = 0; i < nVals2; ++ i )
    {
    int ti = i << 1;
    dataset4Arr->InsertNextValue( mingledData[ti] );
    dataset5Arr->InsertNextValue( mingledData[ti + 1] );
    dataset6Arr->InsertNextValue( -1. );
    }

  vtkTable* datasetTable2 = vtkTable::New();
  datasetTable2->AddColumn( dataset4Arr );
  dataset4Arr->Delete();
  datasetTable2->AddColumn( dataset5Arr );
  dataset5Arr->Delete();
  datasetTable2->AddColumn( dataset6Arr );
  dataset6Arr->Delete();

  vtkDescriptiveStatistics* ds2 = vtkDescriptiveStatistics::New();
  ds2->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable2 );
  vtkTable* outputMeta2 = ds2->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );

  datasetTable2->Delete();

  // Select Columns of Interest (all of them)
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    ds2->AddColumn( columns[i] );
    }

  // Update with Learn option only
  ds2->SetLearnOption( true );
  ds2->SetDeriveOption( false );
  ds2->SetAssessOption( false );
  ds2->Update();

  // Now build a data object collection of the two obtained models
  vtkDataObjectCollection* doc = vtkDataObjectCollection::New();
  doc->AddItem( outputMeta1 );
  doc->AddItem( outputMeta2 );

  // And calculate the aggregated statistics of the two models
  vtkDescriptiveStatistics* ds = vtkDescriptiveStatistics::New();
  vtkTable* mod = vtkTable::New();
  ds->LearnAggregate( doc, mod );

  // Clean up
  ds->Delete();
  ds1->Delete();
  ds2->Delete();
  doc->Delete();
  mod->Delete();

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

  ds = vtkDescriptiveStatistics::New();
  ds->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, simpleTable );
  vtkTable* outputSimpleMeta = ds->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );

  simpleTable->Delete();

  // Select Column of Interest
  ds->AddColumn( "Metric" );

  // Test Learn and Derive only
  ds->SetLearnOption( true );
  ds->SetDeriveOption( true );
  ds->SetAssessOption( false );
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
