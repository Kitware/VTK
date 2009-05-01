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

  int nMetricPairs = 3;

  // Entropies in the summary table should normally be retrieved as follows:
  //   column 2: H(X,Y)
  //   column 3: H(Y|X)
  //   column 4: H(X|Y)
  int iEntropies[] = { 2,
                       3,
                       4 }; 
  int nEntropies = 3; // correct number of entropies reported in the summary table
  double* H = new double[nEntropies];
  
  vtkContingencyStatistics* haruspex = vtkContingencyStatistics::New();
  haruspex->SetInput( 0, datasetTable );
  vtkTable* outputData = haruspex->GetOutput( 0 );

  datasetTable->Delete();

// -- Select Column Pair of Interest ( Learn Mode ) -- 
  haruspex->AddColumnPair( "Port", "Protocol" ); // A valid pair
  haruspex->AddColumnPair( "Protocol", "Port" ); // The same valid pair, just reversed
  haruspex->AddColumnPair( "Source", "Port" ); // Another valid pair
  haruspex->AddColumnPair( "Source", "Dummy" ); // An invalid pair

// -- Test Learn Mode -- 
  haruspex->SetLearn( true );
  haruspex->SetAssess( true );
  haruspex->Update();

  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( haruspex->GetOutputDataObject( 1 ) );
  vtkTable* outputSummary = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 0 ) );
  vtkTable* outputContingency = vtkTable::SafeDownCast( outputMetaDS->GetBlock( 1 ) );

  int testIntValue = 0;

  vtkIdType n = outputContingency->GetValueByName( 0, "Cardinality" ).ToInt();
  cout << "## Calculated the following information entropies( grand total: "
       << n
       << " ):\n";

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
      cout << "   ("
           << outputSummary->GetValue( r, 0 ).ToString()
           << ", "
           << outputSummary->GetValue( r, 1 ).ToString()
           << "):";
      
      
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
      }
    cout << "\n";
    }

  cout << "## Calculated the following probabilities:\n";
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

  key = 0;
  vtkStdString varX = outputSummary->GetValue( key, 0 ).ToString();
  vtkStdString varY = outputSummary->GetValue( key, 1 ).ToString();
  vtkStdString proName = "Px|y";
  vtkStdString colName = proName + "(" + varX + "," + varY + ")";
  double threshold = .2;

  cout << "## Found the following outliers such that "
       << colName
       << " < "
       << threshold
       << ":\n";

  double p;
  testIntValue = 0;
  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    p = outputData->GetValueByName( r, colName ).ToDouble();
    if ( p >= threshold )
      {
      continue;
      }

    ++ testIntValue;

    cout << "   ("
         << outputData->GetValueByName( r, varX ).ToString()
         << ","
         << outputData->GetValueByName( r, varY ).ToString()
         << "):  "
         << proName
         << " = "
         << p
         << "\n";
    }

  double nOutliers = 4;
  if ( testIntValue != nOutliers )
    {
    vtkGenericWarningMacro("Reported an incorrect number of outliers: " 
                           << testIntValue 
                           << " != " 
                           << nOutliers
                           << ".");
    testStatus = 1;
    }
  cout << "\n";

  // Clean up
  delete [] H;
  haruspex->Delete();

  return testStatus;
}
