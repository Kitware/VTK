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
#include "vtkContingencyStatistics.h"

//=============================================================================
int TestContingencyStatistics( int, char *[] )
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
  dataset1Arr->SetName( "X0" );

  vtkDoubleArray* dataset2Arr = vtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "X1" );

  vtkDoubleArray* dataset3Arr = vtkDoubleArray::New();
  dataset3Arr->SetNumberOfComponents( 1 );
  dataset3Arr->SetName( "X2" );

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

  vtkContingencyStatistics* haruspex = vtkContingencyStatistics::New();
  haruspex->SetInput( 0, datasetTable );
  vtkTable* outputTable = haruspex->GetOutput();

  datasetTable->Delete();

// -- Select Column Pair of Interest ( Learn Mode ) -- 
  haruspex->SetX( "X0" );
  haruspex->SetY( "X1" );

// -- Test Learn Mode -- 
  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
  haruspex->Update();
  vtkIdType n = haruspex->GetSampleSize();
  int testIntValue = 0;

  cout << "## Calculated the following statistics ( grand total: "
       << n
       << " ):\n";
  for ( vtkIdType r = 0; r < outputTable->GetNumberOfRows(); ++ r )
    {
    int c = outputTable->GetValue( r, 2 ).ToInt();
    testIntValue += c;

    cout << "   (X, Y) = ("
         << outputTable->GetValue( r, 0 ).ToString()
         << ", "
         << outputTable->GetValue( r, 1 ).ToString()
         << "), "
         << outputTable->GetColumnName( 2 )
         << "="
         << c
         << ", "
         << outputTable->GetColumnName( 3 )
         << "="
         << outputTable->GetValue( r, 3 ).ToDouble()
         << "\n";
    }

  if ( testIntValue != n )
    {
    cerr << "Reported an incorrect number of doublets: "
         << testIntValue
         << " != "
         << n
         << ".\n";
    testStatus = 1;
    }

// -- Test Assess Mode -- 
  vtkContingencyStatistics* haruspex2 = vtkContingencyStatistics::New();
  haruspex2->SetInput( 0, datasetTable );
  haruspex2->SetInput( 1, outputTable );
  vtkTable* outputTable2 = haruspex2->GetOutput();

// -- Select Column Pair of Interest ( Learn Mode ) -- 
  haruspex2->SetX( "X0" );
  haruspex2->SetY( "X1" );

  haruspex2->SetExecutionMode( vtkStatisticsAlgorithm::AssessMode );
  haruspex2->Update();

  cout << "## Calculated the following probabilities:\n";

  int ids[] = { 0, 1, 3, 4, 5 };
  for ( int i = 0; i < 5; ++ i )
    {
    cout << "   "
         << outputTable2->GetColumnName( ids[i] )
         << "  ";
    }
  cout << "\n";

  for ( vtkIdType r = 0; r < outputTable2->GetNumberOfRows(); ++ r )
    {
    for ( int i = 0; i < 5; ++ i )
      {
      cout << "   "
           << outputTable2->GetValue( r, ids[i] ).ToString()
           << "  ";
      }
    cout << "\n";
    }

  haruspex2->Delete();
  haruspex->Delete();

  return testStatus;
}
