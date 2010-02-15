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
#include "vtkOrderStatistics.h"

#include <vtkstd/map>
//=============================================================================
int TestOrderStatistics( int, char *[] )
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

  vtkOrderStatistics* os = vtkOrderStatistics::New();
  os->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable );
  vtkTable* outputData = os->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkTable* outputMeta = os->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL );

  datasetTable->Delete();

  // Select Columns of Interest
  os->AddColumn( "Metric 3" ); // Include invalid Metric 3
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    os->AddColumn( columns[i] );
    }

  // Test Learn only (Derive does not do anything for order statistics)
  os->SetLearnOption( true );
  os->SetAssessOption( false );
  os->Update();

  // Offset between baseline values for each variable
  int valsOffset = 6;

  double valsTest1 [] = 
    { 0.,
      32., 46., 47., 49., 51.5, 54.,
      32., 45., 47., 49., 52., 54.,
      32., -1., -1., -1., -1., -1.,
    };

  cout << "## Calculated the following 5-points statistics with InverseCDFAveragedSteps quantile definition):\n";
  for ( vtkIdType r = 0; r < outputMeta->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputMeta->GetNumberOfColumns(); ++ i )
      {
      cout << outputMeta->GetColumnName( i )
           << "="
           << outputMeta->GetValue( r, i ).ToString()
           << "  ";

      if ( i && outputMeta->GetValue( r, i ).ToDouble() != valsTest1[r * valsOffset + i] )
        {
        testStatus = 1;
        vtkGenericWarningMacro("Incorrect 5-points statistics: " << valsTest1[r * valsOffset + i] << ".");
        }
      }
    cout << "\n";
    }

  // Test Learn and Assess options for quartiles with InverseCDF quantile definition
  os->SetQuantileDefinition( vtkOrderStatistics::InverseCDF );
  os->SetAssessOption( true );
  os->Update();

  double valsTest2 [] = 
    { 0.,
      32., 46., 47., 49., 51., 54.,
      32., 45., 47., 49., 52., 54.,
      32., -1., -1., -1., -1., -1.,
    };

  cout << "## Calculated the following 5-points statistics with InverseCDF quantile definition:\n";
  for ( vtkIdType r = 0; r < outputMeta->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputMeta->GetNumberOfColumns(); ++ i )
      {
      cout << outputMeta->GetColumnName( i )
           << "="
           << outputMeta->GetValue( r, i ).ToString()
           << "  ";

      if ( i && outputMeta->GetValue( r, i ).ToDouble() != valsTest2[r * valsOffset + i] )
        {
        testStatus = 1;
        vtkGenericWarningMacro("Incorrect 5-points statistics: " << valsTest2[r * valsOffset + i] << ".");
        }
      }
    cout << "\n";
    }

  vtkstd::map<int,int> histoMetric[2];
  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    ++ histoMetric[0][outputData->GetValueByName( r, "Quantile(Metric 0)" ).ToInt()];
    ++ histoMetric[1][outputData->GetValueByName( r, "Quantile(Metric 1)" ).ToInt()];
    }

  int cpt[] = { 0, 0 };
  cout << "## Calculated the following histograms:\n";
  for ( int i = 0; i < 2; ++ i )
    {
    cout << "   "
         << outputData->GetColumnName( i )
         << ":\n";

    for ( vtkstd::map<int,int>::iterator it = histoMetric[i].begin(); 
          it != histoMetric[i].end(); ++ it )
      {
      cpt[i] += it->second;
      cout << "    "
           << it->first 
           << " |-> " 
           << it->second 
           << "\n";
      }

    if ( cpt[i] != outputData->GetNumberOfRows() )
      {
      vtkGenericWarningMacro("Incorrect histogram count: " << cpt[i] << " != " << outputData->GetNumberOfRows() << ".");
      testStatus = 1;
      }
    }

  // Test Learn option for deciles with InverseCDF quantile definition (as with Octave)
  os->SetQuantileDefinition( 0 ); // 0: vtkOrderStatistics::InverseCDF
  os->SetNumberOfIntervals( 10 );
  os->SetAssessOption( false );
  os->Update();

  cout << "## Calculated the following deciles with InverseCDF quantile definition:\n";
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

  // Test Learn option for quartiles with non-numeric ordinal data
  vtkStdString text[] = { 
    "an", 
    "ordinal", 
    "scale", 
    "defines", 
    "a", 
    "total", 
    "preorder", 
    "of", 
    "objects", 
    "the", 
    "scale", 
    "values", 
    "themselves", 
    "have", 
    "a", 
    "total", 
    "order", 
    "names", 
    "may", 
    "be", 
    "used", 
    "like", 
    "bad", 
    "medium", 
    "good", 
    "if", 
    "numbers", 
    "are", 
    "used", 
    "they", 
    "are", 
    "only", 
    "relevant", 
    "up", 
    "to", 
    "strictly", 
    "monotonically", 
    "increasing", 
    "transformations", 
    "order", 
    "isomorphism"
  };
  int textLength = 41;
    
  vtkStringArray* textArr = vtkStringArray::New();
  textArr->SetNumberOfComponents( 1 );
  textArr->SetName( "Text" );

  for ( int i = 0; i < textLength; ++ i )
    {
    textArr->InsertNextValue( text[i] );
    }

  vtkTable* textTable = vtkTable::New();
  textTable->AddColumn( textArr );
  textArr->Delete();

  os->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, textTable );
  textTable->Delete();
  os->ResetAllColumnStates(); // Clear list of columns of interest
  os->AddColumn( "Text" ); // Add column of interest

  // Test Learn and Assess with 4 intervals (use SetParameter method for Learn parameters)
  os->SetParameter( "QuantileDefinition", 0, 0 ); // Does not matter and should be ignored by the engine as the column contains strings
  os->SetParameter( "NumberOfIntervals", 0, 4 );
  os->SetLearnOption( true );
  os->SetAssessOption( true );
  os->Update();

  cout << "## Calculated the following 5-points statistics with non-numerical ordinal data (letters):\n";
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

  vtkstd::map<int,int> histoText;
  for ( vtkIdType r = 0; r < outputData->GetNumberOfRows(); ++ r )
    {
    ++ histoText[outputData->GetValueByName( r, "Quantile(Text)" ).ToInt()];
    }

  int sum = 0;
  cout << "## Calculated the following histogram:\n";
  cout << "   "
       << outputData->GetColumnName( 0 )
       << ":\n";

  for ( vtkstd::map<int,int>::iterator it = histoText.begin(); it != histoText.end(); ++ it )
    {
    sum += it->second;
    cout << "    "
         << it->first 
         << " |-> " 
         << it->second 
         << "\n";
    }
  
  if ( sum != outputData->GetNumberOfRows() )
    {
    vtkGenericWarningMacro("Incorrect histogram count: " << sum << " != " << outputData->GetNumberOfRows() << ".");
    testStatus = 1;
    }

  os->Delete();

  return testStatus;
}
