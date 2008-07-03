/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

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

  vtkVariantArray* dataset1Arr = vtkVariantArray::New();
  dataset1Arr->SetNumberOfComponents( 1 );
  dataset1Arr->SetName( "X" );

  vtkVariantArray* dataset2Arr = vtkVariantArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "Y" );

  for ( int i = 0; i < nVals; ++ i )
    {
    int ti = i << 2;
    dataset1Arr->InsertNextValue( mingledData[ti + 2] );
    dataset2Arr->InsertNextValue( mingledData[ti + 3] );
    }

  vtkTable* datasetTable = vtkTable::New();
  datasetTable->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable->AddColumn( dataset2Arr );
  dataset2Arr->Delete();

  vtkContingencyStatistics* haruspex = vtkContingencyStatistics::New();
  haruspex->SetInput( 0, datasetTable );
  vtkTable* outputMeta = haruspex->GetOutput( 1 );

  datasetTable->Delete();

// -- Select Column Pair of Interest ( Learn Mode ) -- 
  haruspex->SetX( "X" );
  haruspex->SetY( "Y" );

// -- Test Learn Mode -- 
  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
  haruspex->Update();
  vtkIdType n = haruspex->GetSampleSize();
  int testIntValue = 0;

  cout << "## Calculated the following statistics ( grand total: "
       << n
       << " ):\n";
  for ( vtkIdType r = 0; r < outputMeta->GetNumberOfRows(); ++ r )
    {
    int c = outputMeta->GetValue( r, 2 ).ToInt();
    testIntValue += c;

    cout << "   (X, Y) = ("
         << outputMeta->GetValue( r, 0 ).ToString()
         << ", "
         << outputMeta->GetValue( r, 1 ).ToString()
         << "), "
         << outputMeta->GetColumnName( 2 )
         << "="
         << c
         << ", "
         << outputMeta->GetColumnName( 3 )
         << "="
         << outputMeta->GetValue( r, 3 ).ToDouble()
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
  haruspex2->SetInput( 1, outputMeta );
  vtkTable* outData2 = haruspex2->GetOutput( 0 );
  vtkTable* outMeta2 = haruspex2->GetOutput( 1 );

// -- Select Column Pair of Interest ( Learn Mode ) -- 
  haruspex2->SetX( "X" );
  haruspex2->SetY( "Y" );

  haruspex2->SetExecutionMode( vtkStatisticsAlgorithm::AssessMode );
  haruspex2->Update();

  cout << "## Calculated the following information entropies:\n";
  for ( int i = 0; i < outMeta2->GetNumberOfColumns(); ++ i )
    {
    cout << "   "
         << outMeta2->GetColumnName( i )
         << " = "
         << outMeta2->GetValue( 0, i ).ToString()
         << "\n";
    }

  cout << "## Calculated the following probabilities:\n";
  for ( int i = 0; i < 5; ++ i )
    {
    cout << "   "
         << outData2->GetColumnName( i )
         << "  ";
    }
  cout << "\n";

  for ( vtkIdType r = 0; r < outData2->GetNumberOfRows(); ++ r )
    {
    for ( int i = 0; i < 5; ++ i )
      {
      cout << "   "
           << outData2->GetValue( r, i ).ToString()
           << "  ";
      }
    cout << "\n";
    }

  haruspex2->Delete();
  haruspex->Delete();

  return testStatus;
}
