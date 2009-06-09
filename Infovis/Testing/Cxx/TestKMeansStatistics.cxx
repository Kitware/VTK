/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// .SECTION Thanks
// Thanks to Janine Bennett, Philippe Pebay, and David Thompson from Sandia National Laboratories 
// for implementing this test.

#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStringArray.h"
#include "vtkIdTypeArray.h"
#include "vtkTable.h"
#include "vtkMath.h"
#include "vtkKMeansStatistics.h"
#include "vtkStdString.h"
#include "vtkTimerLog.h"
#include <vtksys/ios/sstream>


//=============================================================================
int TestKMeansStatistics( int, char *[] )
{
  int testStatus = 0;

  const int nDim = 4;
  int nVals = 50;

  // Seed random number generator
  vtkMath::RandomSeed( static_cast<int>( vtkTimerLog::GetUniversalTime() ) );

  // Generate an input table that contains samples of mutually independent random variables over [0, 1]
  vtkTable* inputData = vtkTable::New();
  vtkDoubleArray* doubleArray[nDim];

  for ( int c = 0; c < nDim; ++ c )
    {
    vtksys_ios::ostringstream colName;
    colName << "coord " << c;
    doubleArray[c] = vtkDoubleArray::New();
    doubleArray[c]->SetNumberOfComponents( 1 );
    doubleArray[c]->SetName( colName.str().c_str() );
    doubleArray[c]->SetNumberOfTuples( nVals );

    double x;
    for ( int r = 0; r < nVals; ++ r )
      {
      //x = vtkMath::Gaussian();
      x = vtkMath::Random();
      doubleArray[c]->SetValue( r, x );
      }

    inputData->AddColumn( doubleArray[c] );
    doubleArray[c]->Delete();
    }

  vtkTable* paramData = vtkTable::New();
  vtkIdTypeArray* paramCluster;
  vtkDoubleArray* paramArray[nDim];
  const int numRuns = 5;
  const int numClustersInRun[] = { 5, 2, 3, 4, 5 };
  //const int numRuns = 1;
  //const int numClustersInRun[] = { 3 };
  paramCluster = vtkIdTypeArray::New();
  paramCluster->SetName( "K" );

  for( int curRun = 0; curRun < numRuns; curRun++ )
    {
    for( int nInRun = 0; nInRun < numClustersInRun[curRun]; nInRun++ )
      {
      paramCluster->InsertNextValue( numClustersInRun[curRun] );
      }
    } 
  paramData->AddColumn( paramCluster );
  paramCluster->Delete();
  
  for ( int c = 0; c < 5; ++ c )
    {
    vtksys_ios::ostringstream colName;
    colName << "coord " << c;
    paramArray[c] = vtkDoubleArray::New();
    paramArray[c]->SetNumberOfComponents( 1 );
    paramArray[c]->SetName( colName.str().c_str() );

    double x;
    for( int curRun = 0; curRun < numRuns; curRun++ )
      {
      for( int nInRun = 0; nInRun < numClustersInRun[curRun]; nInRun++ )
        {
        //x = vtkMath::Gaussian();
        x = vtkMath::Random();
        paramArray[c]->InsertNextValue( x );
        }
      } 
    paramData->AddColumn( paramArray[c] );
    paramArray[c]->Delete();
    }


  vtkKMeansStatistics* haruspex = vtkKMeansStatistics::New();
  haruspex->SetColumnStatus( inputData->GetColumnName( 0 ) , 1 );
  haruspex->SetColumnStatus( inputData->GetColumnName( 2 ) , 1 );
  haruspex->SetColumnStatus( "Testing", 1 );
  haruspex->RequestSelectedColumns();

  haruspex->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, inputData );
  cout << "Testing default parameter generation with Default Number of Clusters = 3" << endl;
  haruspex->SetDefaultNumberOfClusters( 3 );

  // -- Test Learn Mode -- 
  haruspex->SetLearn( true );
  haruspex->SetDerive( true );
  haruspex->SetAssess( false );

  haruspex->Update();
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast(  
                        haruspex->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  for ( unsigned int b = 0; b < outputMetaDS->GetNumberOfBlocks(); ++ b )
    {
    vtkTable* outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( b ) );
    if ( b == 0 )
      {
      cout << "Computed clusters:" << "\n";
      }
    else
      {
      cout << "Ranked cluster: " << "\n";
      }

    outputMeta->Dump();
    }


  haruspex->SetInput( vtkStatisticsAlgorithm::LEARN_PARAMETERS, paramData );
  cout << "testing default table:" << endl;
  paramData->Dump();
    
  // -- Test Learn Mode -- 
  haruspex->SetLearn( true );
  haruspex->SetDerive( true );
  haruspex->SetAssess( false );

  haruspex->Update();
  outputMetaDS = vtkMultiBlockDataSet::SafeDownCast( 
                 haruspex->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  for ( unsigned int b = 0; b < outputMetaDS->GetNumberOfBlocks(); ++ b )
    {
    vtkTable* outputMeta = vtkTable::SafeDownCast( outputMetaDS->GetBlock( b ) );
    if ( b == 0 )
      {
      cout << "Computed clusters:" << "\n";
      }
    else
      {
      cout << "Ranked cluster: " << "\n";
      }

    outputMeta->Dump();
    }

  // -- Test Assess Mode -- 
  cout << "=================== ASSESS ==================== " << endl;
  vtkMultiBlockDataSet* paramsTables = vtkMultiBlockDataSet::New();
  paramsTables->ShallowCopy( outputMetaDS );

  haruspex->SetInput( vtkStatisticsAlgorithm::INPUT_MODEL, paramsTables );

  haruspex->SetLearn( false );
  haruspex->SetDerive( false ); // Do not recalculate nor rederive a model
  haruspex->SetAssess( true );
  haruspex->Update();
  vtkTable* outputData = haruspex->GetOutput();
  outputData->Dump();
  cout << "outputData = " << *outputData << endl;
  paramsTables->Delete();

  paramData->Delete();
  inputData->Delete();
  haruspex->Delete();

  return testStatus;
}

