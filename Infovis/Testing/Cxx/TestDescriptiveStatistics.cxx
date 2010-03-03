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
#include "vtkMath.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
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

  // Pairs of interest
  int nMetrics = 3;
  vtkStdString columns[] = 
    { 
      "Metric 1", 
      "Metric 2", 
      "Metric 0" 
    };

  // Reference values
  // Means for metrics 0, 1, and 2, respectively
  double means1[] = { 49.21875 , 49.5, -1. };

  // Standard deviations for metrics 0, 1, and 2, respectively
  double stdevs1[] = { sqrt( 5.9828629 ), sqrt( 7.548397 ), 0. };

  vtkDescriptiveStatistics* ds1 = vtkDescriptiveStatistics::New();
  ds1->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable1 );
  vtkTable* outputData1 = ds1->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkTable* outputMeta1 = ds1->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );
  vtkTable* outputTest1 = ds1->GetOutput( vtkStatisticsAlgorithm::OUTPUT_TEST );

  datasetTable1->Delete();

  // Select Columns of Interest
  ds1->AddColumn( "Metric 3" ); // Include invalid Metric 3
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    ds1->AddColumn( columns[i] );
    }

  // Use default Assess parameter names to make sure this is covered as well
  ds1->SetNominalParameter( "Mean");
  ds1->SetDeviationParameter( "Standard Deviation");

  // Test Learn, Derive, Test, and Assess options
  ds1->SetLearnOption( true );
  ds1->SetDeriveOption( true );
  ds1->SetAssessOption( true );
  ds1->SetTestOption( true );
  ds1->SignedDeviationsOff();
  ds1->Update();

  cout << "## Calculated the following statistics for first data set:\n";

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

    // Verify some of the calculated statistics
    if ( fabs ( outputMeta1->GetValueByName( r, "Mean" ).ToDouble() - means1[r] ) > 1.e-6 )
      {
      vtkGenericWarningMacro("Incorrect mean");
      testStatus = 1;
      }

    if ( fabs ( outputMeta1->GetValueByName( r, "Standard Deviation" ).ToDouble() - stdevs1[r] ) > 1.e-5 )
      {
      vtkGenericWarningMacro("Incorrect standard deviation");
      testStatus = 1;
      }
    cout << "\n";
    }

  // Check some results of the Test option
  cout << "\n## Calculated the following Jarque-Bera statistics:\n";
  for ( vtkIdType r = 0; r < outputTest1->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputTest1->GetNumberOfColumns(); ++ i )
      {
      cout << outputTest1->GetColumnName( i )
           << "="
           << outputTest1->GetValue( r, i ).ToString()
           << "  ";
      }

    cout << "\n";
    }

  // Search for outliers to check results of Assess option
  double maxdev = 1.5;
  cout << "\n## Searching for outliers from mean with relative deviation > "
       << maxdev
       << " for metric 1:\n";

  vtkDoubleArray* vals0 = vtkDoubleArray::SafeDownCast( outputData1->GetColumnByName( "Metric 0" ) );
  vtkDoubleArray* vals1 = vtkDoubleArray::SafeDownCast( outputData1->GetColumnByName( "Metric 1" ) );
  vtkDoubleArray* devs0 = vtkDoubleArray::SafeDownCast( outputData1->GetColumnByName( "d(Metric 0)" ) );
  vtkDoubleArray* devs1 = vtkDoubleArray::SafeDownCast( outputData1->GetColumnByName( "d(Metric 1)" ) );

  if ( ! devs0 || ! devs1 || ! vals0 || ! vals1 )
    {
    vtkGenericWarningMacro("Empty output column(s).\n");
    testStatus = 1;

    return testStatus;
    }

  double dev;
  int m0outliers = 0;
  int m1outliers = 0;
  for ( vtkIdType r = 0; r < outputData1->GetNumberOfRows(); ++ r )
    {
    dev = devs0->GetValue( r );
    if ( dev > maxdev )
      {
      ++ m0outliers;
      cout << "   " 
           << " row " 
           << r
           << ", "
           << devs0->GetName() 
           << " = " 
           << dev 
           << " > " 
           << maxdev
           << " (value: " 
           << vals0->GetValue( r ) 
           << ")\n";
      }
    }
  for ( vtkIdType r = 0; r < outputData1->GetNumberOfRows(); ++ r )
    {
    dev = devs1->GetValue( r );
    if ( dev > maxdev )
      {
      ++ m1outliers;
      cout << "   " 
           << " row " 
           << r 
           << ", "
           << devs1->GetName() 
           << " = " 
           << dev 
           << " > " 
           << maxdev
           << " (value: " 
           << vals1->GetValue( r ) 
           << ")\n";
      }
    }

  cout << "  Found " 
       << m0outliers 
       << " outliers for Metric 0"
       << " and " 
       << m1outliers 
       << " outliers for Metric 1.\n";

  if ( m0outliers != 4 || m1outliers != 6 )
    {
    vtkGenericWarningMacro("Expected 4 outliers for Metric 0 and 6 outliers for Metric 1.");
    testStatus = 1;
    }

  // Now, used modified output 1 as input 1 to test 0-deviation
  cout << "\n## Searching for outliers from mean with relative deviation > 0 from 50 for metric 1:\n";

  vtkTable* paramsTable = vtkTable::New();
  paramsTable->ShallowCopy( outputMeta1 );
  paramsTable->SetValueByName( 1, "Mean", 50. );
  paramsTable->SetValueByName( 1, "Standard Deviation", 0. );
  
  // Run with Assess option only (do not recalculate nor rederive a model)
  ds1->SetInput( vtkStatisticsAlgorithm::INPUT_MODEL, paramsTable );
  ds1->SetLearnOption( false );
  ds1->SetDeriveOption( false ); 
  ds1->SetTestOption( true );
  ds1->SetAssessOption( true );
  ds1->Update();

  vals1 = vtkDoubleArray::SafeDownCast( outputData1->GetColumnByName( "Metric 1" ) );
  devs1 = vtkDoubleArray::SafeDownCast( outputData1->GetColumnByName( "d(Metric 1)" ) );

  if ( ! devs1 || ! vals1 )
    {
    vtkGenericWarningMacro("Empty output column(s).\n");
    testStatus = 1;

    return testStatus;
    }

  m1outliers = 0;

  for ( vtkIdType r = 0; r < outputData1->GetNumberOfRows(); ++ r )
    {
    dev = devs1->GetValue( r );
    if ( dev )
      {
      ++ m1outliers;
      cout << "   " 
           << " row " 
           << r
           << ", "
           << devs1->GetName() 
           << " = " 
           << dev
           << " (value: " 
           << vals1->GetValue( r ) 
           << ")\n";
      }
    }
  if ( m1outliers != 28 )
    {
    vtkGenericWarningMacro("Expected 28 outliers for Metric 1, found " << m1outliers << ".");
    testStatus = 1;
    }

  // Clean up (which implies resetting parameters table values which were modified to their initial values)
  paramsTable->SetValueByName( 1, "Mean", means1[1] );
  paramsTable->SetValueByName( 1, "Standard Deviation", stdevs1[1] );
  paramsTable->Delete();

  // Test with a slight variation of initial data set (to test model aggregation)
  int nVals2 = 32;

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
    dataset4Arr->InsertNextValue( mingledData[ti] + 1. );
    dataset5Arr->InsertNextValue( mingledData[ti + 1] );
    dataset6Arr->InsertNextValue( 1. );
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
  ds2->SetTestOption( false );
  ds2->SetAssessOption( false );
  ds2->Update();

  cout << "\n## Calculated the following statistics for second data set:\n";

  for ( vtkIdType r = 0; r < outputMeta2->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputMeta2->GetNumberOfColumns(); ++ i )
      {
      cout << outputMeta2->GetColumnName( i )
           << "="
           << outputMeta2->GetValue( r, i ).ToString()
           << "  ";
      }
    cout << "\n";
    }

  // Now build a data object collection of the two obtained models
  vtkDataObjectCollection* doc = vtkDataObjectCollection::New();
  doc->AddItem( outputMeta1 );
  doc->AddItem( outputMeta2 );

  // And calculate the aggregated minimal statistics of the two models
  vtkDescriptiveStatistics* ds = vtkDescriptiveStatistics::New();
  vtkTable* aggregated = vtkTable::New();
  ds->Aggregate( doc, aggregated );

  // Finally, calculate the derived statistics of the aggregated model
  ds2->SetInput( vtkStatisticsAlgorithm::INPUT_MODEL, aggregated );
  ds2->SetLearnOption( false );
  ds2->SetDeriveOption( true ); 
  ds2->SetTestOption( false );
  ds2->SetAssessOption( false );
  ds2->Update();

  // Reference values
  // Means deviations for metrics 0, 1, and 2, respectively
  double means2[] = { 49.71875 , 49.5, 0. };

  // Standard deviations for metrics 0, 1, and 2, respectively
  double stdevs2[] = { sqrt( 6.1418651 ), sqrt( 7.548397 * 62. / 63. ), sqrt( 64. / 63. ) };

  cout << "\n## Calculated the following statistics for aggregated (first + second) data set:\n";

  for ( vtkIdType r = 0; r < outputMeta2->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputMeta2->GetNumberOfColumns(); ++ i )
      {
      cout << outputMeta2->GetColumnName( i )
           << "="
           << outputMeta2->GetValue( r, i ).ToString()
           << "  ";
      }

    // Verify some of the calculated statistics
    if ( fabs ( outputMeta2->GetValueByName( r, "Mean" ).ToDouble() - means2[r] ) > 1.e-6 )
      {
      vtkGenericWarningMacro("Incorrect mean");
      testStatus = 1;
      }

    if ( fabs ( outputMeta2->GetValueByName( r, "Standard Deviation" ).ToDouble() - stdevs2[r] ) > 1.e-5 )
      {
      vtkGenericWarningMacro("Incorrect standard deviation");
      testStatus = 1;
      }
    cout << "\n";
    }

  // Clean up
  ds->Delete();
  ds1->Delete();
  ds2->Delete();
  doc->Delete();
  aggregated->Delete();

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
  datasetArr->SetName( "Digits" );

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

  vtkDescriptiveStatistics* ds3 = vtkDescriptiveStatistics::New();
  ds3->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, simpleTable );
  vtkTable* outputSimpleMeta = ds3->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );

  simpleTable->Delete();

  // Select Column of Interest
  ds3->AddColumn( "Digits" );

  // Test Learn and Derive options only
  ds3->SetLearnOption( true );
  ds3->SetDeriveOption( true );
  ds3->SetTestOption( false );
  ds3->SetAssessOption( false );
  ds3->Update();

  cout << "\n## Calculated the following statistics for {0,...9} sequence:\n";
  
  for ( int i = 0; i < outputSimpleMeta->GetNumberOfColumns(); ++ i )
    {
    cout << outputSimpleMeta->GetColumnName( i )
         << "="
         << outputSimpleMeta->GetValue( 0, i ).ToString()
         << "  ";
    }
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "Mean" ).ToDouble() - mean ) > 1.e-6 )
    {
    vtkGenericWarningMacro("Incorrect mean");
    testStatus = 1;
    }
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "Variance" ).ToDouble() - variance ) > 1.e-6 )
    {
    vtkGenericWarningMacro("Incorrect variance");
    testStatus = 1;
    }
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "g1 Skewness" ).ToDouble() - g1 ) > 1.e-6 )
    {
    vtkGenericWarningMacro("Incorrect g1 skewness");
    testStatus = 1;
    }
  
  if ( fabs ( outputSimpleMeta->GetValueByName( 0, "g2 Kurtosis" ).ToDouble() - g2 ) > 1.e-6 )
    {
    vtkGenericWarningMacro("Incorrect g2 kurtosis");
    testStatus = 1;
    }
  cout << "\n";

  ds3->Delete();

  // ************** Pseudo-random sample to exercise Jarque-Bera test ********* 
  int nVals = 10000;

  vtkDoubleArray* datasetNormal = vtkDoubleArray::New();
  datasetNormal->SetNumberOfComponents( 1 );
  datasetNormal->SetName( "Standard Normal" );

  vtkDoubleArray* datasetUniform = vtkDoubleArray::New();
  datasetUniform->SetNumberOfComponents( 1 );
  datasetUniform->SetName( "Standard Uniform" );

  vtkDoubleArray* datasetLogNormal = vtkDoubleArray::New();
  datasetLogNormal->SetNumberOfComponents( 1 );
  datasetLogNormal->SetName( "Standard Log-Normal" );

  vtkDoubleArray* datasetExponential = vtkDoubleArray::New();
  datasetExponential->SetNumberOfComponents( 1 );
  datasetExponential->SetName( "Standard Exponential" );

  vtkDoubleArray* datasetLaplace = vtkDoubleArray::New();
  datasetLaplace->SetNumberOfComponents( 1 );
  datasetLaplace->SetName( "Standard Laplace" );

  // Seed random number generator
  vtkMath::RandomSeed( static_cast<int>( vtkTimerLog::GetUniversalTime() ) );

  for ( int i = 0; i < nVals; ++ i )
    {
    datasetNormal->InsertNextValue( vtkMath::Gaussian() );
    datasetUniform->InsertNextValue( vtkMath::Random() );
    datasetLogNormal->InsertNextValue( exp( vtkMath::Gaussian() ) );
    datasetExponential->InsertNextValue( -log ( vtkMath::Random() ) );
    double u = vtkMath::Random() - .5;
    datasetLaplace->InsertNextValue( ( u < 0. ? 1. : -1. ) * log ( 1. - 2. * fabs( u ) ) );
    }

  vtkTable* gaussianTable = vtkTable::New();
  gaussianTable->AddColumn( datasetNormal );
  datasetNormal->Delete();
  gaussianTable->AddColumn( datasetUniform );
  datasetUniform->Delete();
  gaussianTable->AddColumn( datasetLogNormal );
  datasetLogNormal->Delete();
  gaussianTable->AddColumn( datasetExponential );
  datasetExponential->Delete();
  gaussianTable->AddColumn( datasetLaplace );
  datasetLaplace->Delete();

  vtkDescriptiveStatistics* ds4 = vtkDescriptiveStatistics::New();
  ds4->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, gaussianTable );
  vtkTable* outputMeta4 = ds4->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );
  vtkTable* outputTest4 = ds4->GetOutput( vtkStatisticsAlgorithm::OUTPUT_TEST );

  gaussianTable->Delete();

  // Select Column of Interest
  ds4->AddColumn( "Standard Normal" );
  ds4->AddColumn( "Standard Uniform" );
  ds4->AddColumn( "Standard Log-Normal" );
  ds4->AddColumn( "Standard Exponential" );
  ds4->AddColumn( "Standard Laplace" );

  // Test Learn, Derive, and Test options only
  ds4->SetLearnOption( true );
  ds4->SetDeriveOption( true );
  ds4->SetTestOption( true );
  ds4->SetAssessOption( false );
  ds4->Update();

  // Print some calculated statistics of the Learn and Derive options
  cout << "\n## Some calculated descriptive statistics for pseudo-random variables (n="
       << nVals
       << "):\n";

  // Statistics of interest
  int soi[] = 
    { 
      0,  // variable name
      2,  // minimum
      3,  // maximum
      4,  // mean
      9,  // variance
      10, // g1 skewness
      12  // g2 kurtosis
    };
  int nsoi = 7;

  for ( vtkIdType r = 0; r < outputMeta4->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < nsoi; ++ i )
      {
      cout << outputMeta4->GetColumnName( soi[i] )
           << "="
           << outputMeta4->GetValue( r, soi[i] ).ToString()
           << "  ";
      }

    cout << "\n";
    }

  // Check some results of the Test option
  cout << "\n## Calculated the following Jarque-Bera statistics for pseudo-random variables (n="
       << nVals
       << "):\n";
  
#ifdef VTK_USE_GNU_R
  int nNonGaussian = 3;
  int nRejected = 0;
  double alpha = .01;
#endif // VTK_USE_GNU_R

  // Loop over Test table
  for ( vtkIdType r = 0; r < outputTest4->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int c = 0; c < outputTest4->GetNumberOfColumns(); ++ c )
      {
      cout << outputTest4->GetColumnName( c )
           << "="
           << outputTest4->GetValue( r, c ).ToString()
           << "  ";
      }

#ifdef VTK_USE_GNU_R
    // Check if null hypothesis is rejected at specified significance level
    double p = outputTest4->GetValueByName( r, "P" ).ToDouble();
    // Must verify that p value is valid (it is set to -1 if R has failed)
    if ( p > -1 && p < alpha )
      {
      cout << "Null hypothesis (normality) rejected at "
           << alpha
           << " significance level";

      ++ nRejected;
      }
#endif // VTK_USE_GNU_R

    cout << "\n";
    }

#ifdef VTK_USE_GNU_R
  if ( nRejected < nNonGaussian )
    {
    vtkGenericWarningMacro("Rejected only "
                           << nRejected
                           << " null hypotheses of normality whereas "
                           << nNonGaussian
                           << " variables are not Gaussian");
    testStatus = 1;
    }
#endif // VTK_USE_GNU_R
  
  ds4->Delete();

  return testStatus;
}
