// .SECTION Thanks
// This test was implemented by Philippe Pebay, Kitware SAS 2012

#include "vtkAutoCorrelativeStatistics.h"
#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"

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

  // Create input data table
  vtkTable* datasetTable1 = vtkTable::New();
  datasetTable1->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable1->AddColumn( dataset2Arr );
  dataset2Arr->Delete();

  // Create input parameter table for the stationary case
  vtkIdTypeArray* timeLags = vtkIdTypeArray::New();
  timeLags->SetName( "Time Lags" );
  timeLags->SetNumberOfTuples( 1 );
  timeLags->SetValue( 0, 0 );
  vtkTable* paramTable = vtkTable::New();
  paramTable->AddColumn( timeLags );
  timeLags->Delete();

  // Columns of interest
  int nMetrics1 = 2;
  vtkStdString columns1[] =
    {
      "Metric 1",
      "Metric 0"
    };

  // Reference values
  // Means for metrics 0 and 1 respectively
  double meansXs1[] = { 49.21875 , 49.5 };

  // Standard deviations for metrics 0 and 1, respectively
  double varsXs1[] = { 5.9828629, 7.548397 };

  // Set autocorrelative statistics algorithm and its input data port
  vtkAutoCorrelativeStatistics* as1 = vtkAutoCorrelativeStatistics::New();

  // First verify that absence of input does not cause trouble
  cout << "\n## Verifying that absence of input does not cause trouble... ";
  as1->Update();
  cout << "done.\n";

  // Prepare first test with data
  as1->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable1 );
  datasetTable1->Delete();

  // Select columns of interest
  for ( int i = 0; i < nMetrics1; ++ i )
    {
    as1->AddColumn( columns1[i] );
    }

  // Set spatial cardinality
  as1->SetSliceCardinality( nVals1 ); 

  // Set parameters for autocorrelation of whole data set with respect to itself
  as1->SetInputData( vtkStatisticsAlgorithm::LEARN_PARAMETERS, paramTable );

  // Test Learn and Derive options
  as1->SetLearnOption( true );
  as1->SetDeriveOption( true );
  as1->SetAssessOption( false );
  as1->SetTestOption( false );
  as1->Update();

  // Get output model tables
  vtkMultiBlockDataSet* outputModelAS1 = vtkMultiBlockDataSet::SafeDownCast( as1->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  cout << "\n## Calculated the following statistics for first data set:\n";
  for ( unsigned b = 0; b < outputModelAS1->GetNumberOfBlocks(); ++ b )
    {
    vtkStdString varName = outputModelAS1->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );

    vtkTable* modelTab = vtkTable::SafeDownCast( outputModelAS1->GetBlock( b ) );
    if ( varName == "Autocorrelation FFT" )
      {
      if ( modelTab->GetNumberOfRows() )
        {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
        }
      }

    cout << "   Variable="
         << varName
         << "\n";

    cout << "   ";
    for ( int i = 0; i < modelTab->GetNumberOfColumns(); ++ i )
      {
      cout << modelTab->GetColumnName( i )
           << "="
           << modelTab->GetValue( 0, i ).ToString()
           << "  ";
      }

    // Verify some of the calculated statistics
    if ( fabs ( modelTab->GetValueByName( 0, "Mean Xs" ).ToDouble() - meansXs1[b] ) > 1.e-6 )
      {
      vtkGenericWarningMacro("Incorrect mean for Xs");
      testStatus = 1;
      }

    if ( fabs ( modelTab->GetValueByName( 0, "Variance Xs" ).ToDouble() - varsXs1[b] ) > 1.e-5 )
      {
      vtkGenericWarningMacro("Incorrect variance for Xs");
      testStatus = 1;
      }

    if ( fabs ( modelTab->GetValueByName( 0, "Autocorrelation" ).ToDouble() - 1. ) > 1.e-6 )
      {
      vtkGenericWarningMacro("Incorrect autocorrelation");
      testStatus = 1;
      }

    cout << "\n";
    }

  // Test with a slight variation of initial data set (to test model aggregation)
  int nVals2 = 32;

  vtkDoubleArray* dataset4Arr = vtkDoubleArray::New();
  dataset4Arr->SetNumberOfComponents( 1 );
  dataset4Arr->SetName( "Metric 0" );

  vtkDoubleArray* dataset5Arr = vtkDoubleArray::New();
  dataset5Arr->SetNumberOfComponents( 1 );
  dataset5Arr->SetName( "Metric 1" );

  for ( int i = 0; i < nVals2; ++ i )
    {
    int ti = i << 1;
    dataset4Arr->InsertNextValue( mingledData[ti] + 1. );
    dataset5Arr->InsertNextValue( mingledData[ti + 1] );
    }

  vtkTable* datasetTable2 = vtkTable::New();
  datasetTable2->AddColumn( dataset4Arr );
  dataset4Arr->Delete();
  datasetTable2->AddColumn( dataset5Arr );
  dataset5Arr->Delete();

  // Set auto-correlative statistics algorithm and its input data port
  vtkAutoCorrelativeStatistics* as2 = vtkAutoCorrelativeStatistics::New();
  as2->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable2 );

  // Select columns of interest
  for ( int i = 0; i < nMetrics1; ++ i )
    {
    as2->AddColumn( columns1[i] );
    }

  // Set spatial cardinality
  as2->SetSliceCardinality( nVals2 ); 

  // Set parameters for autocorrelation of whole data set with respect to itself
  as2->SetInputData( vtkStatisticsAlgorithm::LEARN_PARAMETERS, paramTable );

  // Update with Learn option only
  as2->SetLearnOption( true );
  as2->SetDeriveOption( false );
  as2->SetTestOption( false );
  as2->SetAssessOption( false );
  as2->Update();

  // Get output meta tables
  vtkMultiBlockDataSet* outputModelAS2 = vtkMultiBlockDataSet::SafeDownCast( as2->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  cout << "\n## Calculated the following statistics for second data set:\n";
  for ( unsigned b = 0; b < outputModelAS2->GetNumberOfBlocks(); ++ b )
    {
    vtkStdString varName = outputModelAS2->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );

    vtkTable* modelTab = vtkTable::SafeDownCast( outputModelAS2->GetBlock( b ) );
    if ( varName == "Autocorrelation FFT" )
      {
      if ( modelTab->GetNumberOfRows() )
        {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
        }
      }

    cout << "\n   Variable="
         << varName
         << "\n";

    cout << "   ";
    for ( int i = 0; i < modelTab->GetNumberOfColumns(); ++ i )
      {
      cout << modelTab->GetColumnName( i )
           << "="
           << modelTab->GetValue( 0, i ).ToString()
           << "  ";
      }

    cout << "\n";
    }

  // Test model aggregation by adding new data to engine which already has a model
  as1->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable2 );
  vtkMultiBlockDataSet* model = vtkMultiBlockDataSet::New();
  model->ShallowCopy( outputModelAS1 );
  as1->SetInputData( vtkStatisticsAlgorithm::INPUT_MODEL, model );

  // Clean up
  model->Delete();
  datasetTable2->Delete();
  as2->Delete();

  // Update with Learn and Derive options only
  as1->SetLearnOption( true );
  as1->SetDeriveOption( true );
  as1->SetTestOption( false );
  as1->SetAssessOption( false );
  as1->Update();

  // Updated reference values
  // Means and variances for metrics 0 and 1, respectively
  double meansXs0[] = { 49.71875 , 49.5 };
  double varsXs0[] = { 6.1418651 , 7.548397 * 62. / 63. };

  // Get output meta tables
  outputModelAS1 = vtkMultiBlockDataSet::SafeDownCast( as1->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  cout << "\n## Calculated the following statistics for aggregated (first + second) data set:\n";
  for ( unsigned b = 0; b < outputModelAS1->GetNumberOfBlocks(); ++ b )
    {
    vtkStdString varName = outputModelAS1->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );

    vtkTable* modelTab = vtkTable::SafeDownCast( outputModelAS1->GetBlock( b ) );
    if ( varName == "Autocorrelation FFT" )
      {
      if ( modelTab->GetNumberOfRows() )
        {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
        }
      }

    cout << "\n   Variable="
         << varName
         << "\n";

    cout << "   ";
    for ( int i = 0; i < modelTab->GetNumberOfColumns(); ++ i )
      {
      cout << modelTab->GetColumnName( i )
           << "="
           << modelTab->GetValue( 0, i ).ToString()
           << "  ";
      }

    // Verify some of the calculated statistics
    if ( fabs ( modelTab->GetValueByName( 0, "Mean Xs" ).ToDouble() - meansXs0[b] ) > 1.e-6 )
      {
      vtkGenericWarningMacro("Incorrect mean for Xs");
      testStatus = 1;
      }

    if ( fabs ( modelTab->GetValueByName( 0, "Variance Xs" ).ToDouble() - varsXs0[b] ) > 1.e-5 )
      {
      vtkGenericWarningMacro("Incorrect variance for Xs");
      testStatus = 1;
      }

    cout << "\n";
    }

  // Clean up
  as1->Delete();

  // ************** Test with 2 columns of synthetic data **************

  // Space and time parameters
  vtkIdType nSteps = 2;
  vtkIdType cardSlice = 1000;
  vtkIdType cardTotal = nSteps * cardSlice;

  // Expand parameter table to contain all steps
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 1 );
  for ( vtkIdType p = 1; p < nSteps; ++ p )
    {
    row->SetValue( 0, p );
    paramTable->InsertNextRow( row );
    }
  row->Delete();

  vtkDoubleArray* lineArr = vtkDoubleArray::New();
  lineArr->SetNumberOfComponents( 1 );
  lineArr->SetName( "Line" );

  vtkDoubleArray* vArr = vtkDoubleArray::New();
  vArr->SetNumberOfComponents( 1 );
  vArr->SetName( "V" );

  vtkDoubleArray* circleArr = vtkDoubleArray::New();
  circleArr->SetNumberOfComponents( 1 );
  circleArr->SetName( "Circle" );

  // Fill data columns
  vtkIdType midPoint = cardTotal >> 1;
  double dAlpha = ( 2.0 * vtkMath::Pi() ) / cardSlice;
  for ( int i = 0; i < cardTotal; ++ i )
    {
    lineArr->InsertNextValue( i );
    if ( i < midPoint )
      {
      vArr->InsertNextValue( cardTotal - i );
      circleArr->InsertNextValue( cos( i * dAlpha ) );
      }
    else
      {
      vArr->InsertNextValue( i );
      circleArr->InsertNextValue( sin( i * dAlpha ) );
      }
    }


  // Create input data table
  vtkTable* datasetTable3 = vtkTable::New();
  datasetTable3->AddColumn( lineArr );
  lineArr->Delete();
  datasetTable3->AddColumn( vArr );
  vArr->Delete();
  datasetTable3->AddColumn( circleArr );
  circleArr->Delete();

  // Columns of interest
  int nMetrics2 = 3;
  vtkStdString columns2[] =
    {
      "Line",
      "V",
      "Circle"
    };

  // Reference values
  double halfNm1 = .5 * ( cardSlice - 1 );

  // Means of Xs for circle, line, and v-shaped variables respectively
  double meansXt3[] = { 0., 0.,
                        halfNm1, halfNm1 + cardSlice,
                        cardTotal - halfNm1, cardTotal - halfNm1 - 1. };

  // Autocorrelation values for circle, line, and v-shaped variables respectively
  double autocorr3[] = { 1., 0.,
                        1., 1.,
                        1., -1. };

  // Prepare autocorrelative statistics algorithm and its input data port
  vtkAutoCorrelativeStatistics* as3 = vtkAutoCorrelativeStatistics::New();
  as3->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA, datasetTable3 );
  datasetTable3->Delete();

  // Select Columns of Interest
  for ( int i = 0; i < nMetrics2; ++ i )
    {
    as3->AddColumn( columns2[i] );
    }

  // Set spatial cardinality
  as3->SetSliceCardinality( cardSlice );

  // Set autocorrelation parameters for first slice against slice following midpoint
  as3->SetInputData( vtkStatisticsAlgorithm::LEARN_PARAMETERS, paramTable );

  // Test Learn, and Derive options
  as3->SetLearnOption( true );
  as3->SetDeriveOption( true );
  as3->SetAssessOption( false );
  as3->SetTestOption( false );
  as3->Update();

  // Get output data and meta tables
  vtkMultiBlockDataSet* outputModelAS3 = vtkMultiBlockDataSet::SafeDownCast( as3->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  cout << "\n## Calculated the following statistics for third data set:\n";
  for ( unsigned b = 0; b < outputModelAS3->GetNumberOfBlocks(); ++ b )
    {
    vtkStdString varName = outputModelAS3->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );

    vtkTable* modelTab = vtkTable::SafeDownCast( outputModelAS3->GetBlock( b ) );
    if ( varName == "Autocorrelation FFT" )
      {
      if ( modelTab->GetNumberOfRows() )
        {
        cout << "\n   Autocorrelation FFT:\n";
        modelTab->Dump();
        continue;
        }
      }

    cout << "\n   Variable="
         << varName
         << "\n";

    for ( int r = 0; r < modelTab->GetNumberOfRows(); ++ r )
      {
      cout << "   ";
      for ( int i = 0; i < modelTab->GetNumberOfColumns(); ++ i )
        {
        cout << modelTab->GetColumnName( i )
             << "="
             << modelTab->GetValue( r, i ).ToString()
             << "  ";
        }

      // Verify some of the calculated statistics
      int idx = nSteps * b + r; 
      if ( fabs ( modelTab->GetValueByName( r, "Mean Xt" ).ToDouble() - meansXt3[idx] ) > 1.e-6 )
        {
        vtkGenericWarningMacro("Incorrect mean for Xt");
        testStatus = 1;
        }
      
      if ( fabs ( modelTab->GetValueByName( r, "Autocorrelation" ).ToDouble() - autocorr3[idx] ) > 1.e-6 )
        {
        vtkGenericWarningMacro("Incorrect autocorrelation "<<autocorr3[idx]);
        testStatus = 1;
        }

      cout << "\n";
      } // i
    } // r

  // Clean up
  as3->Delete();
  paramTable->Delete();

  return testStatus;
}
