// .SECTION Thanks
// This test was implemented by Philippe Pebay, Kitware SAS 2012

#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkAutoCorrelativeStatistics.h"

//=============================================================================
int TestAutoCorrelativeStatistics( int, char *[] )
{
  int testStatus = 0;

  // ************** Test with 2 columns of input data **************

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

  for ( int i = 0; i < nVals1; ++ i )
    {
    int ti = i << 1;
    dataset1Arr->InsertNextValue( mingledData[ti] );
    dataset2Arr->InsertNextValue( mingledData[ti + 1] );
    }

  vtkTable* datasetTable1 = vtkTable::New();
  datasetTable1->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable1->AddColumn( dataset2Arr );
  dataset2Arr->Delete();

  // Columns of interest
  int nMetrics = 2;
  vtkStdString columns[] =
    {
      "Metric 1",
      "Metric 0"
    };

  // Reference values
  // Means for metrics 0 and 1 respectively
  double means1[] = { 49.21875 , 49.5 };

  // Standard deviations for metrics 0 and 1, respectively
  double vars1[] = { 5.9828629, 7.548397 };

  // Set autocorrelative statistics algorithm and its input data port
  vtkAutoCorrelativeStatistics* as1 = vtkAutoCorrelativeStatistics::New();

  // First verify that absence of input does not cause trouble
  cout << "\n## Verifying that absence of input does not cause trouble... ";
  as1->Update();
  cout << "done.\n";

  // Prepare first test with data
  as1->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable1 );
  datasetTable1->Delete();

  // Select Columns of Interest
  for ( int i = 0; i < nMetrics; ++ i )
    {
    as1->AddColumn( columns[i] );
    }

  // Take autocorrelation of whole data set with respect to itself
  // NB: Not setting the time lag on purpose, as 0 should be the default
  as1->SetSliceCardinality( 32 ); 

  // Test Learn, and Derive options
  as1->SetLearnOption( true );
  as1->SetDeriveOption( true );
  as1->SetAssessOption( false );
  as1->SetTestOption( false );
  as1->Update();

  // Get output data and meta tables
  vtkMultiBlockDataSet* outputMetaAS1 = vtkMultiBlockDataSet::SafeDownCast( as1->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputPrimary1 = vtkTable::SafeDownCast( outputMetaAS1->GetBlock( 0 ) );
  vtkTable* outputDerived1 = vtkTable::SafeDownCast( outputMetaAS1->GetBlock( 1 ) );

  cout << "\n## Calculated the following primary statistics for first data set:\n";
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
    if ( fabs ( outputPrimary1->GetValueByName( r, "Mean Xs" ).ToDouble() - means1[r] ) > 1.e-6 )
      {
      vtkGenericWarningMacro("Incorrect mean");
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
    if ( fabs ( outputDerived1->GetValueByName( r, "Variance Xs" ).ToDouble() - vars1[r] ) > 1.e-5 )
      {
      vtkGenericWarningMacro("Incorrect variance");
      testStatus = 1;
      }
    cout << "\n";
    }

  // Clean up
  as1->Delete();

  // ************** Test with 2 columns of synthetic data **************

  // Data set and slice size
  vtkIdType cardTotal = 100;
  vtkIdType cardSlice = 10;

  vtkDoubleArray* dataset3Arr = vtkDoubleArray::New();
  dataset3Arr->SetNumberOfComponents( 1 );
  dataset3Arr->SetName( "Linear" );

  vtkDoubleArray* dataset4Arr = vtkDoubleArray::New();
  dataset4Arr->SetNumberOfComponents( 1 );
  dataset4Arr->SetName( "V Shaped" );

  vtkIdType halfCard = cardTotal >> 1;
  for ( int i = 0; i < cardTotal; ++ i )
    {
    dataset3Arr->InsertNextValue( i );
    if ( i < halfCard )
      {
      dataset4Arr->InsertNextValue( -i );
      }
    else
      {
      dataset4Arr->InsertNextValue( i - cardTotal );
      }
    }

  vtkTable* datasetTable2 = vtkTable::New();
  datasetTable2->AddColumn( dataset3Arr );
  dataset3Arr->Delete();
  datasetTable2->AddColumn( dataset4Arr );
  dataset4Arr->Delete();

  // Columns of interest
  int nMetrics2 = 2;
  vtkStdString columns2[] =
    {
      "Linear",
      "V Shaped"
    };

  // Prepare autocorrelative statistics algorithm and its input data port
  vtkAutoCorrelativeStatistics* as2 = vtkAutoCorrelativeStatistics::New();
  as2->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable2 );
  datasetTable2->Delete();

  // Select Columns of Interest
  for ( int i = 0; i < nMetrics2; ++ i )
    {
    as2->AddColumn( columns2[i] );
    }

  // Set autocorrelation parameters for first slice against slice following midpoint
  as2->SetSliceCardinality( cardSlice ); 
  as2->SetTimeLag( cardTotal / ( 2 * cardSlice ) ); 

  // Test Learn, and Derive options
  as2->SetLearnOption( true );
  as2->SetDeriveOption( true );
  as2->SetAssessOption( false );
  as2->SetTestOption( false );
  as2->Update();

  // Get output data and meta tables
  vtkMultiBlockDataSet* outputMetaAS2 = vtkMultiBlockDataSet::SafeDownCast( as2->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  vtkTable* outputPrimary2 = vtkTable::SafeDownCast( outputMetaAS2->GetBlock( 0 ) );
  vtkTable* outputDerived2 = vtkTable::SafeDownCast( outputMetaAS2->GetBlock( 1 ) );

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

    // Verify some of the calculated primary statistics
//     if ( fabs ( outputPrimary2->GetValueByName( r, "Mean Xs" ).ToDouble() - means1[r] ) > 1.e-6 )
//       {
//       vtkGenericWarningMacro("Incorrect mean");
//       testStatus = 1;
//       }
    cout << "\n";
    }

  cout << "\n## Calculated the following derived statistics for second data set:\n";
  for ( vtkIdType r = 0; r < outputDerived2->GetNumberOfRows(); ++ r )
    {
    cout << "   ";
    for ( int i = 0; i < outputDerived2->GetNumberOfColumns(); ++ i )
      {
      cout << outputDerived2->GetColumnName( i )
           << "="
           << outputDerived2->GetValue( r, i ).ToString()
           << "  ";
      }

    // Verify some of the calculated derived statistics
//     if ( fabs ( outputDerived2->GetValueByName( r, "Variance Xs" ).ToDouble() - vars1[r] ) > 1.e-5 )
//       {
//       vtkGenericWarningMacro("Incorrect variance");
//       testStatus = 1;
//       }
    cout << "\n";
    }

  // Clean up
  as2->Delete();

  return testStatus;
}
