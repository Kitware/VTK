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
  os->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable );
  datasetTable->Delete();

  // Select Columns of Interest
  os->AddColumn( "Metric 3" ); // Include invalid Metric 3
  for ( int i = 0; i< nMetrics; ++ i )
    {  // Try to add all valid indices once more
    os->AddColumn( columns[i] );
    }

  // Test Learn, Derive, and Test options
  os->SetLearnOption( true );
  os->SetDeriveOption( true );
  os->SetTestOption( true );
  os->SetAssessOption( true );
  os->SetNumericType ( true ); // using numeric data
  os->Update();

  // Offset between baseline values for each variable
  int valsOffset = 6;

  double valsTest1 [] =
    { 0.,
      32., 46., 47., 49., 51.5, 54.,
      32., 45., 47., 49., 52., 54.,
      32., 0., 7.5, 15.5, 23.5, 31.,
    };

  // Get output data and meta tables
  vtkTable* outputData = os->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkMultiBlockDataSet* outputModelDS = vtkMultiBlockDataSet::SafeDownCast( os->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputSummary = vtkTable::SafeDownCast( outputModelDS->GetBlock( 0 ) );
  vtkTable* outputHistogram = vtkTable::SafeDownCast( outputModelDS->GetBlock( 1 ) );
  vtkTable* outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( 2 ) );
  vtkTable* outputTest = os->GetOutput( vtkStatisticsAlgorithm::OUTPUT_TEST );

  cout << "## Calculated the following 5-points statistics with InverseCDFAveragedSteps quantile definition):\n";
  for ( vtkIdType r = 0; r < outputQuantiles->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputQuantiles->GetNumberOfColumns(); ++ i )
      {
      cout << outputQuantiles->GetColumnName( i )
           << "="
           << outputQuantiles->GetValue( r, i ).ToString()
           << "  ";

      // Verify some of the calculated primary statistics
      if ( i && outputQuantiles->GetValue( r, i ).ToDouble() - valsTest1[r * valsOffset + i] > 1.e-6 )
        {
        vtkGenericWarningMacro("Incorrect 5-points statistics: " << valsTest1[r * valsOffset + i] << ".");
        testStatus = 1;
        }
      }
    cout << "\n";
    }

  cout << "\n## Calculated the following histogram:\n";

  // Skip first row which contains data set cardinality
  vtkIdType key;
  vtksys_stl::map<vtkIdType,vtkIdType> cpt1;
  for ( vtkIdType r = 1; r < outputHistogram->GetNumberOfRows(); ++ r )
    {
    key = outputHistogram->GetValue( r, 0 ).ToInt();
    cout << "   "
         << outputSummary->GetValue( key, 0 ).ToString()
         << " = "
         << outputHistogram->GetValue( r, 1 ).ToString();

    for ( vtkIdType c = 2; c < outputHistogram->GetNumberOfColumns(); ++ c )
      {
      cout << ", "
           << outputHistogram->GetColumnName( c )
           << "="
           << outputHistogram->GetValue( r, c ).ToDouble();
      }

    cout << "\n";

    // Update total cardinality
    cpt1[outputHistogram->GetValueByName( r, "Key" ).ToInt()]
      += outputHistogram->GetValueByName( r, "Cardinality" ).ToInt();
    }

  // Check whether total cardinalities are correct
  for ( vtksys_stl::map<vtkIdType,vtkIdType>::iterator it = cpt1.begin();
          it != cpt1.end(); ++ it )
    {
    if ( it->second != outputData->GetNumberOfRows() )
      {
      vtkGenericWarningMacro("Incorrect histogram count: "
                             << it->second
                             << " != "
                             << outputData->GetNumberOfRows()
                             << ".");
      testStatus = 1;
      }
    }

  // Check some results of the Test option
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

  // Test Learn, Derive, and Test options with InverseCDF quantile definition
  os->SetQuantileDefinition( vtkOrderStatistics::InverseCDF );
  os->SetLearnOption( true );
  os->SetDeriveOption( true );
  os->SetTestOption( true );
  os->SetAssessOption( false );
  os->Update();

  double valsTest2 [] =
    { 0.,
      32., 46., 47., 49., 51., 54.,
      32., 45., 47., 49., 52., 54.,
      32., 0., 7., 15., 23., 31.,
    };

  // Get calculated model
  outputModelDS = vtkMultiBlockDataSet::SafeDownCast( os->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  outputSummary = vtkTable::SafeDownCast( outputModelDS->GetBlock( 0 ) );
  outputHistogram = vtkTable::SafeDownCast( outputModelDS->GetBlock( 1 ) );
  outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( 2 ) );

  cout << "\n## Calculated the following 5-points statistics with InverseCDF quantile definition:\n";
  for ( vtkIdType r = 0; r < outputQuantiles->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputQuantiles->GetNumberOfColumns(); ++ i )
      {
      cout << outputQuantiles->GetColumnName( i )
           << "="
           << outputQuantiles->GetValue( r, i ).ToString()
           << "  ";

      // Verify some of the calculated primary statistics
      if ( i && outputQuantiles->GetValue( r, i ).ToDouble() - valsTest1[r * valsOffset + i] > 1.e-6 )
        {
        vtkGenericWarningMacro("Incorrect 5-points statistics: " << valsTest2[r * valsOffset + i] << ".");
        testStatus = 1;
        }
      }
    cout << "\n";
    }

  // Check some results of the Test option
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

  // Test Learn, Derive, and Test option for deciles with InverseCDF quantile definition (as with Octave)
  os->SetQuantileDefinition( 0 ); // 0: vtkOrderStatistics::InverseCDF
  os->SetNumberOfIntervals( 10 );
  os->Update();

  // Get calculated model
  outputModelDS = vtkMultiBlockDataSet::SafeDownCast( os->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  outputQuantiles = vtkTable::SafeDownCast( outputModelDS->GetBlock( 2 ) );

  cout << "\n## Calculated the following deciles with InverseCDF quantile definition:\n";
  for ( vtkIdType r = 0; r < outputQuantiles->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputQuantiles->GetNumberOfColumns(); ++ i )
      {
      cout << outputQuantiles->GetColumnName( i )
           << "="
           << outputQuantiles->GetValue( r, i ).ToString()
           << "  ";
      }
    cout << "\n";
    }

  // Check some results of the Test option
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

  // Test Learn option for quartiles with non-numeric ordinal data
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

  os2->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, textTable );
  textTable->Delete();
  os2->AddColumn( "Text" ); // Add column of interest
  os2->RequestSelectedColumns();

  // Learn, Derive, Test, and Assess with 12 intervals
  os2->SetParameter( "QuantileDefinition", 0, 1 ); // Should be ignored as type is not numeric
  os2->SetParameter( "NumberOfIntervals", 0, 12 );
  os2->SetParameter( "NumericType", 0, 0 ); // Not numeric values
  os2->SetLearnOption( true );
  os2->SetDeriveOption( true );
  os2->SetTestOption( true );
  os2->SetAssessOption( true );
  os2->Update();

  // Get output data and meta tables
  vtkTable* outputData2 = os2->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA );
  vtkMultiBlockDataSet* outputModelDS2 = vtkMultiBlockDataSet::SafeDownCast( os2->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputSummary2 = vtkTable::SafeDownCast( outputModelDS2->GetBlock( 0 ) );
  vtkTable* outputHistogram2 = vtkTable::SafeDownCast( outputModelDS2->GetBlock( 1 ) );
  vtkTable* outputQuantiles2 = vtkTable::SafeDownCast( outputModelDS2->GetBlock( 2 ) );

  cout << "\n## Input text (punctuation omitted):\n   "
       << text
       << "\n";

  cout << "\n## Calculated the following histogram:\n";

  // Skip first row which contains data set cardinality
  vtksys_stl::map<vtkIdType,vtkIdType> cpt2;
  for ( vtkIdType r = 1; r < outputHistogram2->GetNumberOfRows(); ++ r )
    {
    key = outputHistogram2->GetValue( r, 0 ).ToInt();
    cout << "   "
         << outputSummary2->GetValue( key, 0 ).ToString()
         << " = "
         << outputHistogram2->GetValue( r, 1 ).ToString();

    for ( vtkIdType c = 2; c < outputHistogram2->GetNumberOfColumns(); ++ c )
      {
      cout << ", "
           << outputHistogram2->GetColumnName( c )
           << "="
           << outputHistogram2->GetValue( r, c ).ToString();
      }

    cout << "\n";

    // Update total cardinality
    cpt2[outputHistogram2->GetValueByName( r, "Key" ).ToInt()]
      += outputHistogram2->GetValueByName( r, "Cardinality" ).ToInt();
    }

  // Check whether total cardinalities are correct
  for ( vtksys_stl::map<vtkIdType,vtkIdType>::iterator it = cpt2.begin();
          it != cpt2.end(); ++ it )
    {
    if ( it->second != outputData2->GetNumberOfRows() )
      {
      vtkGenericWarningMacro("Incorrect histogram count: "
                             << it->second
                             << " != "
                             << outputData2->GetNumberOfRows()
                             << ".");
      testStatus = 1;
      }
    }

  // Calculate quantile-based histogram
  vtkstd::map<int,int> histo12Text;
  for ( vtkIdType r = 0; r < outputData2->GetNumberOfRows(); ++ r )
    {
    ++ histo12Text[outputData2->GetValueByName( r, "Quantile(Text)" ).ToInt()];
    }

  int sum12 = 0;
  cout << "\n## Calculated the following histogram from "
       << os2->GetNumberOfIntervals()
       << "-quantiles:\n";

  // Calculate representatives
  vtkstd::map<int,char> histo12Repr;
  for ( vtkstd::map<int,int>::iterator it = histo12Text.begin(); it != histo12Text.end(); ++ it )
    {
    int lowerBnd = it->first;
    int upperBnd = it->second;
    sum12 += upperBnd;

    const char* lowerVal = outputQuantiles2->GetValue( 0, lowerBnd + 1 ).ToString();
    const char* upperVal = outputQuantiles2->GetValue( 0, lowerBnd + 2 ).ToString();
    char midVal = ( *lowerVal + *upperVal + 1 ) / 2 ;
    histo12Repr[lowerBnd] = midVal;

    cout << "   interval "
         << lowerBnd
         << ( lowerBnd > 1 ? ": ]" : ": [" )
         << *lowerVal
         << " - "
         << *upperVal
         << "] represented by "
         << midVal
         << " with frequency "
         << upperBnd
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
  outputModelDS = vtkMultiBlockDataSet::SafeDownCast( os2->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  outputQuantiles2 = vtkTable::SafeDownCast( outputModelDS->GetBlock( 2 ) );

  cout << "\n## Input text (punctuation omitted):\n   "
       << text
       << "\n";

  // Calculate quantile-based histogram
  vtkstd::map<int,int> histo100Text;
  for ( vtkIdType r = 0; r < outputData2->GetNumberOfRows(); ++ r )
    {
    ++ histo100Text[outputData2->GetValueByName( r, "Quantile(Text)" ).ToInt()];
    }

  int sum100 = 0;
  cout << "\n## Calculated the following histogram with "
       << os2->GetNumberOfIntervals()
       << "-quantiles:\n";

  // Calculate representatives
  vtkstd::map<int,char> histo100Repr;
  for ( vtkstd::map<int,int>::iterator it = histo100Text.begin(); it != histo100Text.end(); ++ it )
    {
    int lowerBnd = it->first;
    int upperBnd = it->second;
    sum100 += upperBnd;

    const char* lowerVal = outputQuantiles2->GetValue( 0, lowerBnd + 1 ).ToString();
    const char* upperVal = outputQuantiles2->GetValue( 0, lowerBnd + 2 ).ToString();
    char midVal = ( *lowerVal + *upperVal + 1 ) / 2 ;
    histo100Repr[lowerBnd] = midVal;

    cout << "   interval "
         << lowerBnd
         << ( lowerBnd > 1 ? ": ]" : ": [" )
         << *lowerVal
         << " - "
         << *upperVal
         << "] represented by "
         << midVal
         << " with frequency "
         << upperBnd
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
