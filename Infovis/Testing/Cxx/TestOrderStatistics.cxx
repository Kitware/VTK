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
#include "vtkInformation.h"
#include "vtkStringArray.h"
#include "vtkMath.h"
#include "vtkTable.h"
#include "vtkOrderStatistics.h"
#include "vtkMultiBlockDataSet.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/map>

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
    dataset3Arr->InsertNextValue( static_cast<double>( i ) );
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

  // Set order statistics algorithm and its input data port
  vtkOrderStatistics* os = vtkOrderStatistics::New();

  // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  os->Update();
  cout << "done.\n";

  // Prepare first test with data
  os->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable );
  datasetTable->Delete();

  // Select Columns of Interest
  os->AddColumn( "Metric 3" ); // Include invalid Metric 3
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    os->AddColumn( columns[i] );
    }

  // Test Learn, Derive, and Test operations
  os->SetLearnOption( true );
  os->SetDeriveOption( true );
  os->SetTestOption( true );
  os->SetAssessOption( true );
  os->Update();

  // Offset between baseline values for each variable
  int valsOffset = 5;

  double valsTest1 [] =
    {
      46., 47., 49., 51.5, 54.,
      45., 47., 49., 52., 54.,
      0., 7.5, 15.5, 23.5, 31.,
    };

  // Get output data and meta tables
  vtkTable* outputData = os->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkMultiBlockDataSet* outputModelDS = vtkMultiBlockDataSet::SafeDownCast( os->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  unsigned nbq = outputModelDS->GetNumberOfBlocks() - 1;
  vtkTable* outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( nbq ) );
  vtkTable* outputCard = vtkTable::SafeDownCast( outputModelDS->GetBlock( nbq - 1 ) );
  vtkTable* outputTest = os->GetOutput( vtkStatisticsAlgorithm::OUTPUT_TEST );

  cout << "## Calculated the following quartiles with InverseCDFAveragedSteps quantile definition:\n";
  outputQuantiles->Dump();
  for ( vtkIdType c = 1; c < outputQuantiles->GetNumberOfColumns(); ++ c )
    {
    vtkStdString colName = outputQuantiles->GetColumnName( c );
    cout << "   Variable="
         << colName
         << "\n";

    // Check some results of the Derive operation
    for ( int r = 0; r < outputQuantiles->GetNumberOfRows(); ++ r )
      {
      double Qp = outputQuantiles->GetValue( r, c ).ToDouble();
      int i = ( c - 1 ) * valsOffset + r;

      if ( fabs( Qp - valsTest1[i] ) > 1.e-6 )
        {
        vtkGenericWarningMacro("Incorrect quartiles: "
                               << Qp
                               << " != "
                               << valsTest1[i]
                               << ".");
        testStatus = 1;
        }
      }

    // Check some results of the Assess operation
    vtkStdString quantColName = "Quantile(" + colName + ")";
    vtkAbstractArray* absQuantArr = outputData->GetColumnByName( quantColName );
    if ( ! absQuantArr )
      {
      vtkGenericWarningMacro("Cannot retrieve quartile array for variable: "
                             << colName
                             << ".");
      testStatus = 1;
      }
    else
      {
      vtkDataArray* dataQuantArr = vtkDoubleArray::SafeDownCast( absQuantArr );
      if ( ! dataQuantArr )
        {
        vtkGenericWarningMacro("Quartile array for variable: "
                               << colName
                               << " is not a data array.");
        testStatus = 1;
        }

      vtksys_stl::map<int,int> histoQuantiles;
      for ( vtkIdType r = 0; r < dataQuantArr->GetNumberOfTuples(); ++ r )
        {
        int qIdx = static_cast<int>( vtkMath::Round( dataQuantArr->GetTuple1( r ) ) );
        ++ histoQuantiles[qIdx];
        }

      int totalHist = 0;
      for ( vtksys_stl::map<int,int>::iterator hit = histoQuantiles.begin();
            hit != histoQuantiles.end() ; ++ hit )
        {
        cout << "    IQR "
             << hit->first
             << ": "
             << hit->second
             << " observations\n";

        totalHist += hit->second;
        }
      cout << "    Total: "
           << totalHist
           << " observations\n";

      if ( nVals != totalHist )
        {
        vtkGenericWarningMacro("Quartile-based histogram size "
                               << totalHist
                               << " != "
                               << nVals
                               << ", the data set cardinality.");
        testStatus = 1;
        }
      }

    cout << "\n";
    }

  cout << "## Calculated the following histograms:\n";
  for ( unsigned b = 0; b < outputModelDS->GetNumberOfBlocks() - 2; ++ b )
    {
    vtkStdString varName = outputModelDS->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );
    cout << "   Variable="
         << varName
         << "\n";

    vtkTable* histoTab = vtkTable::SafeDownCast( outputModelDS->GetBlock( b ) );
    histoTab->Dump();
    }

  // Check cardinalities
  cout << "\n## Calculated the following cardinalities:\n";
  for ( vtkIdType r = 0; r < outputCard->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputCard->GetNumberOfColumns(); ++ i )
      {
      cout << outputCard->GetColumnName( i )
           << "="
           << outputCard->GetValue( r, i ).ToString()
           << "  ";
      }

    // Check whether total cardinality is correct
    int testIntValue = outputCard->GetValueByName( r, "Cardinality" ).ToInt();
    if ( testIntValue != outputData->GetNumberOfRows() )
      {
      vtkGenericWarningMacro("Incorrect histogram count: "
                             << testIntValue
                             << " != "
                             << outputData->GetNumberOfRows()
                             << ".");
      testStatus = 1;
      }

    cout << "\n";
    }

  // Check some results of the Test operation
  cout << "\n## Calculated the following Kolmogorov-Smirnov statistics:\n";
  for ( vtkIdType r = 0; r < outputTest->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputTest->GetNumberOfColumns(); ++ i )
      {
      cout << outputTest->GetColumnName( i )
           << "="
           << outputTest->GetValue( r, i ).ToString()
           << "  ";
      }

    cout << "\n";
    }

  // Select Columns of Interest (no more bogus columns)
  os->ResetAllColumnStates();
  os->ResetRequests();
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    os->AddColumn( columns[i] );
    }

  // Test Learn, Derive, and Test operations with InverseCDF quantile definition
  os->SetQuantileDefinition( vtkOrderStatistics::InverseCDF );
  os->SetLearnOption( true );
  os->SetDeriveOption( true );
  os->SetTestOption( true );
  os->SetAssessOption( false );
  os->Update();

  double valsTest2 [] =
    {
      46., 47., 49., 51., 54.,
      45., 47., 49., 52., 54.,
      0., 7., 15., 23., 31.,
    };

  // Get calculated model
  outputModelDS = vtkMultiBlockDataSet::SafeDownCast( os->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  nbq = outputModelDS->GetNumberOfBlocks() - 1;
  outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( nbq ) );

  cout << "\n## Calculated the following quartiles with InverseCDFAveragedSteps quantile definition:\n";
  outputQuantiles->Dump();
  for ( vtkIdType c = 1; c < outputQuantiles->GetNumberOfColumns(); ++ c )
    {
    // Verify some of the calculated quartiles
    for ( int r = 0; r < outputQuantiles->GetNumberOfRows(); ++ r )
      {
      double Qp = outputQuantiles->GetValue( r, c ).ToDouble();
      int i = ( c - 1 ) * valsOffset + r;

      if ( fabs( Qp - valsTest2[i] ) > 1.e-6 )
        {
        vtkGenericWarningMacro("Incorrect quartiles for variable "
                               << outputQuantiles->GetColumnName( c )
                               << ": "
                               << Qp
                               << " != "
                               << valsTest2[i]
                               << ".");
        testStatus = 1;
        }
      }
    }

  // Check some results of the Test operation
  cout << "\n## Calculated the following Kolmogorov-Smirnov statistics:\n";
  for ( vtkIdType r = 0; r < outputTest->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputTest->GetNumberOfColumns(); ++ i )
      {
      cout << outputTest->GetColumnName( i )
           << "="
           << outputTest->GetValue( r, i ).ToString()
           << "  ";
      }

    cout << "\n";
    }

  // Test Learn, Derive, and Test operation for deciles with InverseCDF quantile definition (as with Octave)
  os->SetQuantileDefinition( 0 ); // 0: vtkOrderStatistics::InverseCDF
  os->SetNumberOfIntervals( 10 );
  os->Update();

  // Get calculated model
  outputModelDS = vtkMultiBlockDataSet::SafeDownCast( os->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  nbq = outputModelDS->GetNumberOfBlocks() - 1;
  outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( nbq ) );

  cout << "\n## Calculated the following deciles with InverseCDF quantile definition:\n";
  outputQuantiles->Dump();

  // Check some results of the Test operation
  cout << "\n## Calculated the following Kolmogorov-Smirnov statistics:\n";
  for ( vtkIdType r = 0; r < outputTest->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputTest->GetNumberOfColumns(); ++ i )
      {
      cout << outputTest->GetColumnName( i )
           << "="
           << outputTest->GetValue( r, i ).ToString()
           << "  ";
      }

    cout << "\n";
    }

  // Clean up
  os->Delete();

  // Test Learn operation for quartiles with non-numeric ordinal data
  vtkStdString text(
    "an ordinal scale defines a total preorder of objects the scale values themselves have a total order names may be used like bad medium good if numbers are used they are only relevant up to strictly monotonically increasing transformations also known as order isomorphisms" );
  vtksys_stl::vector<int>::size_type textLength = text.size();

  vtkStringArray* textArr = vtkStringArray::New();
  textArr->SetNumberOfComponents( 1 );
  textArr->SetName( "Text" );

  for ( vtksys_stl::vector<int>::size_type i = 0; i < textLength; ++ i )
    {
    vtkStdString s( "" );
    s += text.at(i);
    textArr->InsertNextValue( s );
    }

  vtkTable* textTable = vtkTable::New();
  textTable->AddColumn( textArr );
  textArr->Delete();

  // Set order statistics engine
  vtkOrderStatistics* os2 = vtkOrderStatistics::New();

  os2->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, textTable );
  textTable->Delete();
  os2->AddColumn( "Text" ); // Add column of interest
  os2->RequestSelectedColumns();

  // Learn, Derive, Test, and Assess with 12 intervals
  os2->SetParameter( "QuantileDefinition", 0, 1 ); // Should be ignored as type is not numeric
  os2->SetParameter( "NumberOfIntervals", 0, 12 );
  os2->SetLearnOption( true );
  os2->SetDeriveOption( true );
  os2->SetTestOption( true );
  os2->SetAssessOption( true );
  os2->Update();

  // Get output data and meta tables
  vtkTable* outputData2 = os2->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkMultiBlockDataSet* outputModelDS2 = vtkMultiBlockDataSet::SafeDownCast( os2->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  nbq = outputModelDS2->GetNumberOfBlocks() - 1;
  vtkTable* outputCard2 = vtkTable::SafeDownCast( outputModelDS2->GetBlock( nbq - 1 ) );
  vtkTable* outputQuantiles2 = vtkTable::SafeDownCast( outputModelDS2->GetBlock( nbq ) );

  cout << "\n## Input text (punctuation omitted):\n   "
       << text
       << "\n";

  cout << "\n## Calculated the following histogram:\n";
  for ( unsigned b = 0; b < outputModelDS2->GetNumberOfBlocks() - 2; ++ b )
    {
    vtkStdString varName = outputModelDS2->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );
    cout << "   Variable="
         << varName
         << "\n";

    vtkTable* histoTab = vtkTable::SafeDownCast( outputModelDS2->GetBlock( b ) );
    histoTab->Dump();

    // Check whether total cardinality is correct
    int testIntValue2 = outputCard2->GetValueByName( 0, "Cardinality" ).ToInt();
    if ( testIntValue2 != outputData2->GetNumberOfRows() )
      {
      vtkGenericWarningMacro("Incorrect histogram count: "
                             << testIntValue2
                             << " != "
                             << outputData2->GetNumberOfRows()
                             << ".");
      testStatus = 1;
      }
    }

  // Calculate quantile-based histogram
  std::map<int,int> histo12Text;
  for ( vtkIdType r = 0; r < outputData2->GetNumberOfRows(); ++ r )
    {
    ++ histo12Text[outputData2->GetValueByName( r, "Quantile(Text)" ).ToInt()];
    }

  int sum12 = 0;
  cout << "\n## Calculated the following quantization from "
       << os2->GetNumberOfIntervals()
       << "-quantiles:\n";

  // Calculate representatives
  std::map<int,char> histo12Repr;
  for ( std::map<int,int>::iterator it = histo12Text.begin(); it != histo12Text.end(); ++ it )
    {
    int quantIdx = it->first;
    int prevqIdx = ( it->first ? it->first - 1 : 0 );
    int frequVal = it->second;
    sum12 += frequVal;

    const char* lowerVal = outputQuantiles2->GetValueByName( prevqIdx, "Text" ).ToString();
    const char* upperVal = outputQuantiles2->GetValueByName( quantIdx, "Text" ).ToString();
    char midVal = ( *lowerVal + *upperVal + 1 ) / 2 ;
    histo12Repr[quantIdx] = midVal;

    cout << "   interval "
         << quantIdx
         << ( quantIdx > 1 ? ": ]" : ": [" )
         << *lowerVal
         << " - "
         << *upperVal
         << "] represented by "
         << midVal
         << " with frequency "
         << frequVal
         << "\n";
    }

  // Verify that we retrieve the total count
  if ( sum12 != outputData2->GetNumberOfRows() )
    {
    vtkGenericWarningMacro("Incorrect histogram count: " << sum12 << " != " << outputData2->GetNumberOfRows() << ".");
    testStatus = 1;
    }

  // Quantize text and print it
  cout << "\n## Quantized text with "
       << histo12Text.size()
       << " quantizers based on "
       << os2->GetNumberOfIntervals()
       << "-quantiles :\n   ";
  for ( vtkIdType r = 0; r < outputData2->GetNumberOfRows(); ++ r )
    {
    cout << histo12Repr[outputData2->GetValueByName( r, "Quantile(Text)" ).ToInt()];
    }
  cout << "\n";

  // Learn, Derive, Assess, and Test again but with with 100 intervals this time
  os2->SetParameter( "QuantileDefinition", 0, 0 ); // Does not matter and should be ignored by the engine as the column contains strings
  os2->SetParameter( "NumberOfIntervals", 0, 100 );
  os2->SetLearnOption( true );
  os2->SetDeriveOption( true );
  os2->SetTestOption( true );
  os2->SetAssessOption( true );
  os2->Update();

  // Get calculated model
  outputModelDS2 = vtkMultiBlockDataSet::SafeDownCast( os2->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  nbq = outputModelDS2->GetNumberOfBlocks() - 1;
  outputQuantiles2 = vtkTable::SafeDownCast( outputModelDS2->GetBlock( nbq ) );

  cout << "\n## Input text (punctuation omitted):\n   "
       << text
       << "\n";

  // Calculate quantile-based histogram
  std::map<int,int> histo100Text;
  for ( vtkIdType r = 0; r < outputData2->GetNumberOfRows(); ++ r )
    {
    ++ histo100Text[outputData2->GetValueByName( r, "Quantile(Text)" ).ToInt()];
    }

  int sum100 = 0;
  cout << "\n## Calculated the following quantization with "
       << os2->GetNumberOfIntervals()
       << "-quantiles:\n";

  // Calculate representatives
  std::map<int,char> histo100Repr;
  for ( std::map<int,int>::iterator it = histo100Text.begin(); it != histo100Text.end(); ++ it )
    {
    int quantIdx = it->first;
    int prevqIdx = ( it->first ? it->first - 1 : 0 );
    int frequVal = it->second;
    sum100 += frequVal;

    const char* lowerVal = outputQuantiles2->GetValueByName( prevqIdx, "Text" ).ToString();
    const char* upperVal = outputQuantiles2->GetValueByName( quantIdx, "Text" ).ToString();
    char midVal = ( *lowerVal + *upperVal + 1 ) / 2 ;
    histo100Repr[quantIdx] = midVal;

    cout << "   interval "
         << quantIdx
         << ( quantIdx > 1 ? ": ]" : ": [" )
         << *lowerVal
         << " - "
         << *upperVal
         << "] represented by "
         << midVal
         << " with frequency "
         << frequVal
         << "\n";
    }

  // Verify that we retrieve the total count
  if ( sum100 != outputData2->GetNumberOfRows() )
    {
    vtkGenericWarningMacro("Incorrect histogram count: " << sum100 << " != " << outputData2->GetNumberOfRows() << ".");
    testStatus = 1;
    }

  // Quantize text and print it
  cout << "\n## Quantized text with "
       << histo100Text.size()
       << " quantizers based on "
       << os2->GetNumberOfIntervals()
       << "-quantiles :\n   ";
  for ( vtkIdType r = 0; r < outputData2->GetNumberOfRows(); ++ r )
    {
    cout << histo100Repr[outputData2->GetValueByName( r, "Quantile(Text)" ).ToInt()];
    }
  cout << "\n";

  // Clean up
  os2->Delete();

  return testStatus;
}
