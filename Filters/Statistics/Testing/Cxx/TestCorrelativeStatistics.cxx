/*
 * Copyright 2011 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories
// for implementing this test.

#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkStringArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkCorrelativeStatistics.h"

#include <sstream>

//=============================================================================
// When changing this file, change the corresponding file in
// StatisticsGnuR/Testing/Cxx as well.
//=============================================================================

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
  int nVals1 = 32;

  vtkDoubleArray* dataset1Arr = vtkDoubleArray::New();
  dataset1Arr->SetNumberOfComponents( 1 );
  dataset1Arr->SetName( "M0" );

  vtkDoubleArray* dataset2Arr = vtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "M1" );

  vtkDoubleArray* dataset3Arr = vtkDoubleArray::New();
  dataset3Arr->SetNumberOfComponents( 1 );
  dataset3Arr->SetName( "M2" );

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
  int nMetricPairs = 2;
  vtkStdString columnPairs[] =
    {
      "M0", "M1", // First pair
      "M2", "M1"  // Second pair
    };

  // Reference values
  // Means and variances for metrics 0, and 1, respectively
  double meansX1[] = { 49.21875, 49.5 };
  double varsX1[] = { 5.9828629, 7.548397 };

  // Means and variances for metrics 1 and 2, respectively
  double meansY1[] = { 49.5, -1. };
  double varsY1[] = { 7.548397, 0. };

  // Covariance matrix of (metric 0, metric 1) and (metric 1, metric 2) pairs
  double covariances1[] = { 6.14516, 0. };

  // Pearson r for each of the pairs
  double correlations1[] = { 0.914433, vtkMath::Nan() };

  // Thresholds for outlier detection
  double threshold[] = { 4., 1.8, 1.8 };

  // Set correlative statistics algorithm and its input data port
  vtkCorrelativeStatistics* cs1 = vtkCorrelativeStatistics::New();

  // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  cs1->Update();
  cout << "done.\n";

  // Prepare first test with data
  cs1->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable1 );
  datasetTable1->Delete();

  // Select Column Pairs of Interest ( Learn Mode )
  // 1.1: a valid pair
  cs1->AddColumnPair( "M0", "M1" );
  // 1.2: the same valid pair, just reversed -- should thus be ignored
  cs1->AddColumnPair( "M1", "M0" );
  // 2: another valid pair
  cs1->AddColumnPair( "M2", "M1" );
  // 3: an invalid pair
  cs1->AddColumnPair( "M1", "M3" );

  // Test Learn, Derive, Test, and Assess options
  cs1->SetLearnOption( true );
  cs1->SetDeriveOption( true );
  cs1->SetAssessOption( true );
  cs1->SetTestOption( true );
  cs1->Update();

  // Get output data and meta tables
  vtkTable* outputData1 = cs1->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkMultiBlockDataSet* outputMetaDS1 = vtkMultiBlockDataSet::SafeDownCast( cs1->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputPrimary1 = vtkTable::SafeDownCast( outputMetaDS1->GetBlock( 0 ) );
  vtkTable* outputDerived1 = vtkTable::SafeDownCast( outputMetaDS1->GetBlock( 1 ) );
  vtkTable* outputTest1 = cs1->GetOutput( vtkStatisticsAlgorithm::OUTPUT_TEST );

  cout << "## Calculated the following primary statistics for first data set:\n";
  for ( vtkIdType r = 0; r < outputPrimary1->GetNumberOfRows(); ++ r )
  {
    cout << "   ";
    for ( int i = 0; i < outputPrimary1->GetNumberOfColumns(); ++ i )
    {
      cout << outputPrimary1->GetColumnName( i )
           << "="
           << outputPrimary1->GetValue( r, i ).ToString()
           << "  ";
    }

    // Verify some of the calculated primary statistics
    double testMeanX = outputPrimary1->GetValueByName( r, "Mean X" ).ToDouble();
    if ( fabs ( testMeanX - meansX1[r] ) > 1.e-6 )
    {
      vtkGenericWarningMacro("Incorrect mean for X");
      testStatus = 1;
    }

    double testMeanY = outputPrimary1->GetValueByName( r, "Mean Y" ).ToDouble();
    if ( fabs ( testMeanY - meansY1[r] ) > 1.e-6 )
    {
      vtkGenericWarningMacro("Incorrect mean for Y");
      testStatus = 1;
    }
    cout << "\n";
  }

  cout << "\n## Calculated the following derived statistics for first data set:\n";
  for ( vtkIdType r = 0; r < outputDerived1->GetNumberOfRows(); ++ r )
  {
    cout << "   ";
    for ( int i = 0; i < outputDerived1->GetNumberOfColumns(); ++ i )
    {
      cout << outputDerived1->GetColumnName( i )
           << "="
           << outputDerived1->GetValue( r, i ).ToString()
           << "  ";
    }

    // Verify some of the calculated derived statistics
    double testMeanX = outputPrimary1->GetValueByName( r, "Mean X" ).ToDouble();
    double testMeanY = outputPrimary1->GetValueByName( r, "Mean Y" ).ToDouble();

    double testVarX = outputDerived1->GetValueByName( r, "Variance X" ).ToDouble();
    if ( fabs ( testVarX - varsX1[r] ) > 1.e-5 )
    {
      vtkGenericWarningMacro("Incorrect variance for X");
      testStatus = 1;
    }

    double testVarY = outputDerived1->GetValueByName( r, "Variance Y" ).ToDouble();
    if ( fabs ( testVarY - varsY1[r] ) > 1.e-5 )
    {
      vtkGenericWarningMacro("Incorrect variance for Y");
      testStatus = 1;
    }

    double testCovariance = outputDerived1->GetValueByName( r, "Covariance" ).ToDouble();
    if ( fabs ( testCovariance - covariances1[r] ) > 1.e-5 )
    {
      vtkGenericWarningMacro("Incorrect covariance");
      testStatus = 1;
    }

    double testPearsonR = outputDerived1->GetValueByName( r, "Pearson r" ).ToDouble();
    // Special treatment as some values of Pearson r are Nan, resulting in an exception on VS for <
    vtkTypeBool isNanTest = vtkMath::IsNan( testPearsonR );
    vtkTypeBool isNanCorr = vtkMath::IsNan( correlations1[r] );
    if ( isNanTest || isNanCorr )
    {
      if ( isNanTest != isNanCorr )
      {
        vtkGenericWarningMacro("Incorrect correlation coefficient");
        testStatus = 1;
      }
    }
    else if ( fabs ( testPearsonR - correlations1[r] ) > 1.e-6 )
    {
      vtkGenericWarningMacro("Incorrect correlation coefficient");
      testStatus = 1;
    }

    // Test regression lines if linear regression is valid
    if ( outputDerived1->GetValueByName( r, "Linear Correlation").ToString() == "valid" )
    {
      double testSlopeYX = outputDerived1->GetValueByName( r, "Slope Y/X" ).ToDouble();
      double testInterceptYX = outputDerived1->GetValueByName( r, "Intercept Y/X" ).ToDouble();
      if ( fabs ( testSlopeYX * testMeanX + testInterceptYX - testMeanY ) > 1.e-8 )
      {
        vtkGenericWarningMacro("Incorrect linear regression of Y on X");
        testStatus = 1;
      }

      double testSlopeXY = outputDerived1->GetValueByName( r, "Slope X/Y" ).ToDouble();
      double testInterceptXY = outputDerived1->GetValueByName( r, "Intercept X/Y" ).ToDouble();
      if ( fabs ( testSlopeXY * testMeanY + testInterceptXY - testMeanX ) > 1.e-8 )
      {
        vtkGenericWarningMacro("Incorrect linear regression of X on Y");
        testStatus = 1;
      }
    }

    cout << "\n";
  }

  // Check some results of the Test option
  cout << "\n## Calculated the following Jarque-Bera-Srivastava statistics:\n";
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
  cout << "\n## Searching for outliers with respect to various criteria:\n";
  int assessIdx[] = { 3, 4, 5 };
  int testIntValue[] = { 3, 3, 4 };
  for ( int i = 0; i < 3; ++ i )
  {
    cerr << "   For |"
         <<  outputData1->GetColumnName( assessIdx[i] )
         << "| > "
         << threshold[i]
         << ", found the following outliers:\n";

    int nOutliers = 0;

    for ( vtkIdType r = 0; r < outputData1->GetNumberOfRows(); ++ r )
    {
      double assessed = outputData1->GetValue( r, assessIdx[i] ).ToDouble();
      if ( fabs( assessed ) > threshold[i] )
      {
        ++ nOutliers;

        cout << "     ("
             << outputData1->GetValueByName( r, columnPairs[0] ).ToDouble()
             << ","
             << outputData1->GetValueByName( r, columnPairs[1] ).ToDouble()
             << "): "
           << assessed
             << "\n";
      }
    } // r

    // Verify that number of found outliers is correct
    if ( nOutliers != testIntValue[i] )
    {
      vtkGenericWarningMacro("Expected "
                             <<testIntValue[i]
                             <<" outliers, found "
                             << nOutliers
                             << ".");
      testStatus = 1;
    }
  }

  // Test with a slight variation of initial data set (to test model aggregation)
  int nVals2 = 32;

  vtkDoubleArray* dataset4Arr = vtkDoubleArray::New();
  dataset4Arr->SetNumberOfComponents( 1 );
  dataset4Arr->SetName( "M0" );

  vtkDoubleArray* dataset5Arr = vtkDoubleArray::New();
  dataset5Arr->SetNumberOfComponents( 1 );
  dataset5Arr->SetName( "M1" );

  vtkDoubleArray* dataset6Arr = vtkDoubleArray::New();
  dataset6Arr->SetNumberOfComponents( 1 );
  dataset6Arr->SetName( "M2" );

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

  // Set correlative statistics algorithm and its input data port
  vtkCorrelativeStatistics* cs2 = vtkCorrelativeStatistics::New();
  cs2->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable2 );

  // Select all valid column pairs as pairs of interest
  for ( int i = 0; i< nMetricPairs; ++ i )
  {
    cs2->AddColumnPair( columnPairs[2 * i], columnPairs[ 2 * i + 1] );
  }

  // Update with Learn option only
  cs2->SetLearnOption( true );
  cs2->SetDeriveOption( false );
  cs2->SetTestOption( false );
  cs2->SetAssessOption( false );
  cs2->Update();

  // Get output meta tables
  vtkMultiBlockDataSet* outputMetaDS2 = vtkMultiBlockDataSet::SafeDownCast( cs2->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputPrimary2 = vtkTable::SafeDownCast( outputMetaDS2->GetBlock( 0 ) );

  cout << "\n## Calculated the following primary statistics for second data set:\n";
  for ( vtkIdType r = 0; r < outputPrimary2->GetNumberOfRows(); ++ r )
  {
    cout << "   ";
    for ( int i = 0; i < outputPrimary2->GetNumberOfColumns(); ++ i )
    {
      cout << outputPrimary2->GetColumnName( i )
           << "="
           << outputPrimary2->GetValue( r, i ).ToString()
           << "  ";
    }
    cout << "\n";
  }

  // Test model aggregation by adding new data to engine which already has a model
  cs1->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable2 );
  vtkMultiBlockDataSet* model = vtkMultiBlockDataSet::New();
  model->ShallowCopy( outputMetaDS1 );
  cs1->SetInputData( vtkStatisticsAlgorithm::INPUT_MODEL, model );

  // Clean up
  model->Delete();
  datasetTable2->Delete();
  cs2->Delete();

  // Update with Learn and Derive options only
  cs1->SetLearnOption( true );
  cs1->SetDeriveOption( true );
  cs1->SetTestOption( false );
  cs1->SetAssessOption( false );
  cs1->Update();

  // Updated reference values
  // Means and variances for metrics 0 and 1, respectively
  double meansX0[] = { 49.71875 , 49.5 };
  double varsX0[] = { 6.1418651 , 7.548397 * 62. / 63. };

  // Means and variances for metrics 1 and 2, respectively
  double meansY0[] = { 49.5, 0. };
  double varsY0[] = { 7.548397 * 62. / 63., 64. / 63. };

  // Pearson r for each of the two pairs
  double correlations0[] = { 0.895327, 0. };

  // Get output meta tables
  outputMetaDS1 = vtkMultiBlockDataSet::SafeDownCast( cs1->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  outputPrimary1 = vtkTable::SafeDownCast( outputMetaDS1->GetBlock( 0 ) );
  outputDerived1 = vtkTable::SafeDownCast( outputMetaDS1->GetBlock( 1 ) );

  cout << "\n## Calculated the following primary statistics for aggregated (first + second) data set:\n";
  for ( vtkIdType r = 0; r < outputPrimary1->GetNumberOfRows(); ++ r )
  {
    cout << "   ";
    for ( int i = 0; i < outputPrimary1->GetNumberOfColumns(); ++ i )
    {
      cout << outputPrimary1->GetColumnName( i )
           << "="
           << outputPrimary1->GetValue( r, i ).ToString()
           << "  ";
    }

    // Verify some of the calculated primary statistics
    if ( outputPrimary1->GetValueByName( r, "Cardinality" ).ToInt() != nVals1 + nVals2 )
    {
      vtkGenericWarningMacro("Incorrect cardinality");
      testStatus = 1;
    }

    if ( fabs ( outputPrimary1->GetValueByName( r, "Mean X" ).ToDouble() - meansX0[r] ) > 1.e-6 )
    {
      vtkGenericWarningMacro("Incorrect mean for X");
      testStatus = 1;
    }

    if ( fabs ( outputPrimary1->GetValueByName( r, "Mean Y" ).ToDouble() - meansY0[r] ) > 1.e-6 )
    {
      vtkGenericWarningMacro("Incorrect mean for Y");
      testStatus = 1;
    }
    cout << "\n";
  }

  cout << "\n## Calculated the following derived statistics for aggregated (first + second) data set:\n";
  for ( vtkIdType r = 0; r < outputDerived1->GetNumberOfRows(); ++ r )
  {
    cout << "   ";
    for ( int i = 0; i < outputDerived1->GetNumberOfColumns(); ++ i )
    {
      cout << outputDerived1->GetColumnName( i )
           << "="
           << outputDerived1->GetValue( r, i ).ToString()
           << "  ";
    }

    // Verify some of the calculated derived statistics
    if ( fabs ( outputDerived1->GetValueByName( r, "Variance X" ).ToDouble() - varsX0[r] ) > 1.e-5 )
    {
      vtkGenericWarningMacro("Incorrect variance for X");
      testStatus = 1;
    }

    if ( fabs ( outputDerived1->GetValueByName( r, "Variance Y" ).ToDouble() - varsY0[r] ) > 1.e-5 )
    {
      vtkGenericWarningMacro("Incorrect variance for Y");
      testStatus = 1;
    }

    if ( fabs ( outputDerived1->GetValueByName( r, "Pearson r" ).ToDouble() - correlations0[r] ) > 1.e-6 )
    {
      vtkGenericWarningMacro("Incorrect correlation coefficient");
      testStatus = 1;
    }
    cout << "\n";
  }

  // Clean up
  cs1->Delete();

  // ************** Pseudo-random sample to exercise Jarque-Bera-Srivastava test *********
  int nVals = 10000;

  // Pre-set Pearson correlation coefficients
  double rhoXZ1 = .8; // X and Y1 are highly linearly correlated
  double rhoXZ2 = .2; // X and Y2 are weakly linearly correlated
  double rorXZ1 = sqrt( 1. - rhoXZ1 * rhoXZ1 );
  double rorXZ2 = sqrt( 1. - rhoXZ2 * rhoXZ2 );

  vtkDoubleArray* datasetNormalX = vtkDoubleArray::New();
  datasetNormalX->SetNumberOfComponents( 1 );
  datasetNormalX->SetName( "N(0,1)_1" );

  vtkDoubleArray* datasetNormalY = vtkDoubleArray::New();
  datasetNormalY->SetNumberOfComponents( 1 );
  datasetNormalY->SetName( "N(0,1)_2" );

  vtkDoubleArray* datasetNormalZ1 = vtkDoubleArray::New();
  datasetNormalZ1->SetNumberOfComponents( 1 );
  std::ostringstream z1Name;
  z1Name << rhoXZ1
         << " N(0,1)_1 + "
         << rorXZ1
         << " N(0,1)_2";
  datasetNormalZ1->SetName( z1Name.str().c_str() );

  vtkDoubleArray* datasetNormalZ2 = vtkDoubleArray::New();
  datasetNormalZ2->SetNumberOfComponents( 1 );
  std::ostringstream z2Name;
  z2Name << rhoXZ2
         << " N(0,1)_1 + "
         << rorXZ2
         << " N(0,1)_2";
  datasetNormalZ2->SetName( z2Name.str().c_str() );

  vtkDoubleArray* datasetNormalZ3 = vtkDoubleArray::New();
  datasetNormalZ3->SetNumberOfComponents( 1 );
  datasetNormalZ3->SetName( "5 N(0,1)_1 - 2" );

  vtkDoubleArray* datasetUniform = vtkDoubleArray::New();
  datasetUniform->SetNumberOfComponents( 1 );
  datasetUniform->SetName( "Standard Uniform" );

  vtkDoubleArray* datasetLaplace = vtkDoubleArray::New();
  datasetLaplace->SetNumberOfComponents( 1 );
  datasetLaplace->SetName( "Standard Laplace" );

  // Seed random number generator
  vtkMath::RandomSeed( static_cast<int>( vtkTimerLog::GetUniversalTime() ) );

  // Generate pseudo-random vectors
  double x, y, z1, z2, z3;
  for ( int i = 0; i < nVals; ++ i )
  {
    x = vtkMath::Gaussian();
    y = vtkMath::Gaussian();
    z1 = rhoXZ1 * x + rorXZ1 * y;
    z2 = rhoXZ2 * x + rorXZ2 * y;
    z3 = 5. * x - 2.;
    datasetNormalX->InsertNextValue( x );
    datasetNormalY->InsertNextValue( y );
    datasetNormalZ1->InsertNextValue( z1 );
    datasetNormalZ2->InsertNextValue( z2 );
    datasetNormalZ3->InsertNextValue( z3 );
    datasetUniform->InsertNextValue( vtkMath::Random() );
    double u = vtkMath::Random() - .5;
    datasetLaplace->InsertNextValue( ( u < 0. ? 1. : -1. ) * log ( 1. - 2. * fabs( u ) ) );
  }

  vtkTable* testTable = vtkTable::New();
  testTable->AddColumn( datasetNormalX );
  datasetNormalX->Delete();
  testTable->AddColumn( datasetNormalY );
  datasetNormalY->Delete();
  testTable->AddColumn( datasetNormalZ1 );
  datasetNormalZ1->Delete();
  testTable->AddColumn( datasetNormalZ2 );
  datasetNormalZ2->Delete();
  testTable->AddColumn( datasetNormalZ3 );
  datasetNormalZ3->Delete();
  testTable->AddColumn( datasetUniform );
  datasetUniform->Delete();
  testTable->AddColumn( datasetLaplace );
  datasetLaplace->Delete();

  // Set descriptive statistics algorithm and its input data port
  vtkCorrelativeStatistics* cs4 = vtkCorrelativeStatistics::New();
  cs4->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, testTable );
  testTable->Delete();

  // Select Column Pairs of Interest ( Learn Mode )
  cs4->AddColumnPair( "N(0,1)_1", "N(0,1)_2" );
  cs4->AddColumnPair( "N(0,1)_2", "5 N(0,1)_1 - 2" );
  cs4->AddColumnPair( "N(0,1)_1", z1Name.str().c_str() );
  cs4->AddColumnPair( "N(0,1)_1", z2Name.str().c_str() );
  cs4->AddColumnPair( "N(0,1)_1", "Standard Uniform" );
  cs4->AddColumnPair( "Standard Laplace", "N(0,1)_2" );
  cs4->AddColumnPair( "Standard Uniform", "Standard Laplace" );

  // Test Learn, Derive, and Test options only
  cs4->SetLearnOption( true );
  cs4->SetDeriveOption( true );
  cs4->SetTestOption( true );
  cs4->SetAssessOption( false );
  cs4->Update();

  // Get output data and meta tables
  vtkMultiBlockDataSet* outputMetaCS4 = vtkMultiBlockDataSet::SafeDownCast( cs4->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputPrimary4 = vtkTable::SafeDownCast( outputMetaCS4->GetBlock( 0 ) );
  vtkTable* outputDerived4 = vtkTable::SafeDownCast( outputMetaCS4->GetBlock( 1 ) );
  vtkTable* outputTest4 = cs4->GetOutput( vtkStatisticsAlgorithm::OUTPUT_TEST );

  cout << "\n## Calculated the following primary statistics for pseudo-random variables (n="
       << nVals
       << "):\n";
  for ( vtkIdType r = 0; r < outputPrimary4->GetNumberOfRows(); ++ r )
  {
    cout << "   ";
    for ( int i = 0; i < outputPrimary4->GetNumberOfColumns(); ++ i )
    {
      cout << outputPrimary4->GetColumnName( i )
           << "="
           << outputPrimary4->GetValue( r, i ).ToString()
           << "  ";
    }

    cout << "\n";
  }

  cout << "\n## Calculated the following derived statistics for pseudo-random variables (n="
       << nVals
       << "):\n";
  for ( vtkIdType r = 0; r < outputDerived4->GetNumberOfRows(); ++ r )
  {
    cout << "   ";
    for ( int i = 0; i < outputDerived4->GetNumberOfColumns(); ++ i )
    {
      cout << outputDerived4->GetColumnName( i )
           << "="
           << outputDerived4->GetValue( r, i ).ToString()
           << "  ";
    }

    cout << "\n";
  }

  // Check some results of the Test option
  cout << "\n## Calculated the following Jarque-Bera-Srivastava statistics for pseudo-random variables (n="
       << nVals;

#ifdef USE_GNU_R
  int nNonGaussian = 3;
  int nRejected = 0;
  double alpha = .01;

  cout << ", null hypothesis: binormality, significance level="
       << alpha;
#endif // USE_GNU_R

  cout << "):\n";

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

#ifdef USE_GNU_R
    // Check if null hypothesis is rejected at specified significance level
    double p = outputTest4->GetValueByName( r, "P" ).ToDouble();
    // Must verify that p value is valid (it is set to -1 if R has failed)
    if ( p > -1 && p < alpha )
    {
      cout << "N.H. rejected";

      ++ nRejected;
    }
#endif // USE_GNU_R

    cout << "\n";
  }

#ifdef USE_GNU_R
  if ( nRejected < nNonGaussian )
  {
    vtkGenericWarningMacro("Rejected only "
                           << nRejected
                           << " null hypotheses of binormality whereas "
                           << nNonGaussian
                           << " variable pairs are not Gaussian");
    testStatus = 1;
  }
#endif // USE_GNU_R

  // Clean up
  cs4->Delete();

  return testStatus;
}
