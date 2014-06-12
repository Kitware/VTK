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

#include "vtkMultiBlockDataSet.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtkContingencyStatistics.h"

//=============================================================================
// When changing this file, change the corresponding file in
// StatisticsGnuR/Testing/Cxx as well.
//=============================================================================

//=============================================================================
int TestContingencyStatistics( int, char *[] )
{
  int testStatus = 0;

  vtkVariant mingledData[] =
    {
      123,456,80,"HTTP",
      123,789,80,"HTTP",
      123,789,80,"HTTP",
      123,456,80,"HTTP",
      456,123,80,"HTTP",
      456,123,80,"HTTP",
      456,123,8080,"HTTP",
      789,123,1122,"HTTP",
      456,789,80,"HTTP",
      456,789,25,"SMTP",
      456,789,25,"SMTP",
      456,789,25,"SMTP",
      456,789,25,"SMTP",
      123,789,25,"SMTP",
      789,123,80,"SMTP",
      123,456,20,"FTP",
      789,456,20,"FTP",
      789,123,20,"FTP",
      789,123,122,"FTP",
      789,456,20,"FTP",
      789,456,20,"FTP",
    };
  int nVals = 21;

  vtkVariantArray* dataset0Arr = vtkVariantArray::New();
  dataset0Arr->SetNumberOfComponents( 1 );
  dataset0Arr->SetName( "Source" );

  vtkVariantArray* dataset1Arr = vtkVariantArray::New();
  dataset1Arr->SetNumberOfComponents( 1 );
  dataset1Arr->SetName( "Destination" );

  vtkVariantArray* dataset2Arr = vtkVariantArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "Port" );

  vtkVariantArray* dataset3Arr = vtkVariantArray::New();
  dataset3Arr->SetNumberOfComponents( 1 );
  dataset3Arr->SetName( "Protocol" );

  for ( int i = 0; i < nVals; ++ i )
    {
    int ti = i << 2;
    dataset0Arr->InsertNextValue( mingledData[ti] );
    dataset1Arr->InsertNextValue( mingledData[ti + 1] );
    dataset2Arr->InsertNextValue( mingledData[ti + 2] );
    dataset3Arr->InsertNextValue( mingledData[ti + 3] );
    }

  vtkTable* datasetTable = vtkTable::New();
  datasetTable->AddColumn( dataset0Arr );
  dataset0Arr->Delete();
  datasetTable->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable->AddColumn( dataset2Arr );
  dataset2Arr->Delete();
  datasetTable->AddColumn( dataset3Arr );
  dataset3Arr->Delete();

  int nMetricPairs = 2;

  // Entropies in the summary table should normally be retrieved as follows:
  //   column 2: H(X,Y)
  //   column 3: H(Y|X)
  //   column 4: H(X|Y)
  int iEntropies[] = { 2,
                       3,
                       4 };
  int nEntropies = 3; // correct number of entropies reported in the summary table
  double* H = new double[nEntropies];

  // Set contingency statistics algorithm and its input data port
  vtkContingencyStatistics* cs = vtkContingencyStatistics::New();

  // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  cs->Update();
  cout << "done.\n";

  // Prepare first test with data
  cs->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable );
  vtkTable* outputData = cs->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );

  datasetTable->Delete();

  // Select Column Pair of Interest ( Learn Option )
  // 1.1: a valid pair
  cs->AddColumnPair( "Port", "Protocol" );
  // 1.2: the same valid pair, just reversed -- should thus be ignored
  cs->AddColumnPair( "Protocol", "Port" );
  // 2: another valid pair
  cs->AddColumnPair( "Source", "Port" );
  // 3: an invalid pair
  cs->AddColumnPair( "Source", "Dummy" );

  // Test Learn, Derive, Assess, and Test options
  cs->SetLearnOption( true );
  cs->SetDeriveOption( true );
  cs->SetAssessOption( true );
  cs->SetTestOption( true );
  cs->Update();

  vtkMultiBlockDataSet* outputModelDS = vtkMultiBlockDataSet::SafeDownCast( cs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputSummary = vtkTable::SafeDownCast( outputModelDS->GetBlock( 0 ) );
  vtkTable* outputContingency = vtkTable::SafeDownCast( outputModelDS->GetBlock( 1 ) );
  vtkTable* outputTest = vtkTable::SafeDownCast( cs->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_TEST ) );

  int testIntValue = 0;
  double testDoubleValue = 0;

  vtkIdType n = outputContingency->GetValueByName( 0, "Cardinality" ).ToInt();
  cout << "## Calculated the following information entropies (grand total: "
       << n
       << "):\n";

  testIntValue = outputSummary->GetNumberOfColumns();

  if ( testIntValue != nEntropies + 2 )
    {
    vtkGenericWarningMacro("Reported an incorrect number of columns in the summary table: "
                           << testIntValue
                           << " != "
                           << nEntropies + 2
                           << ".");
    testStatus = 1;
    }
  else
    {
    // For each row in the summary table, fetch variable names and information entropies
    for ( vtkIdType r = 0; r < outputSummary->GetNumberOfRows(); ++ r )
      {
      // Variable names
      cout << "   (X,Y) = ("
           << outputSummary->GetValue( r, 0 ).ToString()
           << ", "
           << outputSummary->GetValue( r, 1 ).ToString()
           << ")";

      // Information entropies
      for ( vtkIdType c = 0; c < nEntropies; ++ c )
        {
        H[c] = outputSummary->GetValue( r, iEntropies[c] ).ToDouble();

        cout << ", "
             << outputSummary->GetColumnName( iEntropies[c] )
             << "="
             << H[c];
        }
      cout << "\n";

      // Make sure that H(X,Y) > H(Y|X)+ H(X|Y)
      testDoubleValue = H[1] + H[2]; // H(Y|X)+ H(X|Y)

      if ( testDoubleValue > H[0] )
        {
        vtkGenericWarningMacro("Reported inconsistent information entropies: H(X,Y) = "
                               << H[0]
                               << " < "
                               << testDoubleValue
                               << " = H(Y|X)+ H(X|Y).");
        testStatus = 1;
        }
      }
    }
  cout << "   where H(X,Y) = - Sum_{x,y} p(x,y) log p(x,y) and H(X|Y) = - Sum_{x,y} p(x,y) log p(x|y).\n";

  cout << "\n";

  cout << "## Calculated the following joint and conditional probabilities and mutual informations:\n";
  testIntValue = 0;

  // Skip first row which contains data set cardinality
  vtkIdType key;
  for ( vtkIdType r = 1; r < outputContingency->GetNumberOfRows(); ++ r )
    {
    key = outputContingency->GetValue( r, 0 ).ToInt();
    cout << "   ("
         << outputSummary->GetValue( key, 0 ).ToString()
         << ","
         << outputSummary->GetValue( key, 1 ).ToString()
         << ") = ("
         << outputContingency->GetValue( r, 1 ).ToString()
         << ","
         << outputContingency->GetValue( r, 2 ).ToString()
         << ")";

    for ( vtkIdType c = 3; c < outputContingency->GetNumberOfColumns(); ++ c )
      {
      cout << ", "
           << outputContingency->GetColumnName( c )
           << "="
           << outputContingency->GetValue( r, c ).ToDouble();
      }

    cout << "\n";

    // Update total cardinality
    testIntValue += outputContingency->GetValueByName( r, "Cardinality" ).ToInt();
    }

  if ( testIntValue != nVals * nMetricPairs )
    {
    vtkGenericWarningMacro("Reported an incorrect total cardinality: "
                           << testIntValue
                           << " != "
                           << nVals * nMetricPairs
                           << ".");
    testStatus = 1;
    }
  cout << "\n";

  cout << "## Calculated the following marginal probabilities:\n";
  testIntValue = 0;

  for ( unsigned int b = 2; b < outputModelDS->GetNumberOfBlocks(); ++ b )
    {
    outputContingency = vtkTable::SafeDownCast( outputModelDS->GetBlock( b ) );

    for ( vtkIdType r = 0; r < outputContingency->GetNumberOfRows(); ++ r )
      {
      cout << "   "
           << outputContingency->GetColumnName( 0 )
           << " = "
           << outputContingency->GetValue( r, 0 ).ToString()
           << ", "
           << outputContingency->GetColumnName( 1 )
           << "="
           << outputContingency->GetValue( r, 1 ).ToDouble()
           << ", "
           << outputContingency->GetColumnName( 2 )
           << "="
           << outputContingency->GetValue( r, 2 ).ToDouble()
           << "\n";
        }
    cout << "\n";

    // Update total cardinality
    testIntValue += 0;//outputContingency->GetValueByName( r, "Cardinality" ).ToInt();
    }

  // Now inspect results of the Assess option by looking for outliers
  key = 0;
  vtkStdString varX = outputSummary->GetValue( key, 0 ).ToString();
  vtkStdString varY = outputSummary->GetValue( key, 1 ).ToString();

  // List of columns used for outlier detection
  vtkStdString outlierColumn[] = { "P",
                                   "Px|y",
                                   "PMI" };
  // Corresponding threshold (low) values
  double threshold[] = { .2,
                         .2,
                         .0 };

  // Corresponding known number of outliers
  int nOutliers[] = { 4,
                      4,
                      1 };

  int nOutlierTypes = 3;
  for ( int i = 0; i < nOutlierTypes; ++ i )
    {
    vtkStdString colName = outlierColumn[i] + "(" + varX + "," + varY + ")";

    cout << "## Found the following outliers such that "
         << colName
         << " < "
         << threshold[i]
         << ":\n";

    double val;
    testIntValue = 0;
    for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
      {
      val = outputData->GetValueByName( r, colName ).ToDouble();
      if ( val >= threshold[i] )
        {
        continue;
        }

      ++ testIntValue;

      cout << "   "
           << outlierColumn[i]
           << "("
           << outputData->GetValueByName( r, varX ).ToString()
           << ","
           << outputData->GetValueByName( r, varY ).ToString()
           << ") = "
           << val
           << "\n";
      }

    if ( testIntValue != nOutliers[i] )
      {
      vtkGenericWarningMacro("Reported an incorrect number of outliers: "
                             << testIntValue
                             << " != "
                             << nOutliers[i]
                             << ".");
      testStatus = 1;
      }
    cout << "\n";
    }

  // Last, check some results of the Test option
  cout << "## Chi square statistics:\n";

  // Reference values
  double testValues[] = {
    // (Port,Protocol)
    10.,       // number of degrees of freedom
    36.896,    // Chi square statistic
    22.35,     // Chi square statistic with Yates correction
#ifdef USE_GNU_R
    .00005899, // p-valued of Chi square statistic
    .01341754, // p-value of Chi square statistic with Yates correction
#endif // USE_GNU_R
    // (Port,Source)
    10.,       // number of degrees of freedom
    17.353,    // Chi square statistic
    7.279,     // Chi square statistic with Yates correction
#ifdef USE_GNU_R
    .06690889, // p-valued of Chi square statistic
    .69886917  // p-value of Chi square statistic with Yates correction
#endif // USE_GNU_R
  };

#ifdef USE_GNU_R
  double alpha = .05;
  vtkIdType nv = 5;
#else // USE_GNU_R
  vtkIdType nv = 3;
#endif // USE_GNU_R

  // Loop over Test table
  for ( vtkIdType r = 0; r < outputTest->GetNumberOfRows(); ++ r )
    {
    cout << "   ("
         << outputSummary->GetValue( r, 0 ).ToString()
         << ","
         << outputSummary->GetValue( r, 1 ).ToString()
         << ")";

    for ( vtkIdType c = 0; c < nv; ++ c )
      {
      double x =  outputTest->GetValue( r, c ).ToDouble();
      cout << ", "
           << outputTest->GetColumnName( c )
           << "="
           << x;

      // Verify calculated results
      if ( fabs ( x - testValues[r * nv + c] ) > 1.e-4 * x )
        {
        vtkGenericWarningMacro("Incorrect "
                               << outputTest->GetColumnName( c )
                               << ": "
                               << x
                               << " != "
                               << testValues[r * nv + c]);
        testStatus = 1;
        }
      }

#ifdef USE_GNU_R
    // Check if null hypothesis is rejected at specified significance level
    double p = outputTest->GetValueByName( r, "P Yates" ).ToDouble();
    // Must verify that p value is valid (it is set to -1 if R has failed)
    if ( p > -1 && p < alpha )
      {
      cout << ", Null hypothesis (independence) rejected at "
           << alpha
           << " significance level";
      }
#endif // USE_GNU_R

    cout << "\n";
    }

  // Clean up
  delete [] H;
  cs->Delete();

  return testStatus;
}
