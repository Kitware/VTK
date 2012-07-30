/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAutoCorrelativeStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"

#include "vtkAutoCorrelativeStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkTableFFT.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/set>
#include <vtksys/ios/sstream>
#include <limits>

vtkStandardNewMacro(vtkAutoCorrelativeStatistics);

// ----------------------------------------------------------------------
vtkAutoCorrelativeStatistics::vtkAutoCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "d^2" ); // Squared Mahalanobis distance

  this->SliceCardinality = 0; // Invalid value by default. Correct value must be specified.
}

// ----------------------------------------------------------------------
vtkAutoCorrelativeStatistics::~vtkAutoCorrelativeStatistics()
{
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "SliceCardinality: " << this->SliceCardinality << "\n";
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::Aggregate( vtkDataObjectCollection* inMetaColl,
                                              vtkMultiBlockDataSet* outMeta )
{
  if ( ! outMeta )
    {
    return;
    }

  vtkDataObject *inMetaDO0 = inMetaColl->GetItem( 0 );
  if ( ! inMetaDO0 )
    {
    return;
    }

  // Verify that the first input model is indeed contained in a multiblock data set
  vtkMultiBlockDataSet* inMeta0 = vtkMultiBlockDataSet::SafeDownCast( inMetaDO0 );
  if ( ! inMeta0 )
    {
    return;
    }

  // Iterate over variable blocks
  unsigned int nBlocks = inMeta0->GetNumberOfBlocks();
  for ( unsigned int b = 0; b < nBlocks; ++ b )
    {
    // Get hold of the first model (data object) in the collection
    vtkCollectionSimpleIterator it;
    inMetaColl->InitTraversal( it );
    vtkDataObject *inMetaDO = inMetaColl->GetNextDataObject( it );

    // Verify that the first input model is indeed contained in a multiblock data set
    vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
    if ( ! inMeta )
      {
      continue;
      }

    const char* varName = inMeta->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );
    // Skip FFT block if already present in the model
    if ( ! strcmp( varName, "Autocorrelation FFT" ) )
      {
      continue;
      }

    // Verify that the first model is indeed contained in a table
    vtkTable* currentTab = vtkTable::SafeDownCast( inMeta->GetBlock( b ) );
    if ( ! currentTab )
      {
      continue;
      }

    vtkIdType nRow = currentTab->GetNumberOfRows();
    if ( ! nRow )
      {
      // No statistics were calculated.
      continue;
      }

    // Use this first model to initialize the aggregated one
    vtkTable* aggregatedTab = vtkTable::New();
    aggregatedTab->DeepCopy( currentTab );

    // Now, loop over all remaining models and update aggregated each time
    while ( ( inMetaDO = inMetaColl->GetNextDataObject( it ) ) )
      {
      // Verify that the current model is indeed contained in a multiblock data set
      inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
      if ( ! inMeta )
        {
        aggregatedTab->Delete();

        continue;
        }

      // Verify that the current model is indeed contained in a table
      currentTab = vtkTable::SafeDownCast( inMeta->GetBlock( b ) );
      if ( ! currentTab )
        {
        aggregatedTab->Delete();

        continue;
        }

      if ( currentTab->GetNumberOfRows() != nRow )
        {
        // Models do not match
        aggregatedTab->Delete();

        continue;
        }

      // Iterate over all model rows
      for ( int r = 0; r < nRow; ++ r )
        {
        // Verify that variable names match each other
        if ( currentTab->GetValueByName( r, "Variable" ) != aggregatedTab->GetValueByName( r, "Variable" ) )
          {
          // Models do not match
          aggregatedTab->Delete();

          continue;
          }

        // Get aggregated statistics
        int n = aggregatedTab->GetValueByName( r, "Cardinality" ).ToInt();
        double meanXs = aggregatedTab->GetValueByName( r, "Mean Xs" ).ToDouble();
        double meanXt = aggregatedTab->GetValueByName( r, "Mean Xt" ).ToDouble();
        double M2Xs = aggregatedTab->GetValueByName( r, "M2 Xs" ).ToDouble();
        double M2Xt = aggregatedTab->GetValueByName( r, "M2 Xt" ).ToDouble();
        double MXsXt = aggregatedTab->GetValueByName( r, "M XsXt" ).ToDouble();

        // Get current model statistics
        int n_c = currentTab->GetValueByName( r, "Cardinality" ).ToInt();
        double meanXs_c = currentTab->GetValueByName( r, "Mean Xs" ).ToDouble();
        double meanXt_c = currentTab->GetValueByName( r, "Mean Xt" ).ToDouble();
        double M2Xs_c = currentTab->GetValueByName( r, "M2 Xs" ).ToDouble();
        double M2Xt_c = currentTab->GetValueByName( r, "M2 Xt" ).ToDouble();
        double MXsXt_c = currentTab->GetValueByName( r, "M XsXt" ).ToDouble();

        // Update global statics
        int N = n + n_c;

        double invN = 1. / static_cast<double>( N );

        double deltaXs = meanXs_c - meanXs;
        double deltaXs_sur_N = deltaXs * invN;

        double deltaXt = meanXt_c - meanXt;
        double deltaXt_sur_N = deltaXt * invN;

        int prod_n = n * n_c;

        M2Xs += M2Xs_c
          + prod_n * deltaXs * deltaXs_sur_N;

        M2Xt += M2Xt_c
          + prod_n * deltaXt * deltaXt_sur_N;

        MXsXt += MXsXt_c
          + prod_n * deltaXs * deltaXt_sur_N;

        meanXs += n_c * deltaXs_sur_N;

        meanXt += n_c * deltaXt_sur_N;

        // Store updated model
        aggregatedTab->SetValueByName( r, "Cardinality", N );
        aggregatedTab->SetValueByName( r, "Mean Xs", meanXs );
        aggregatedTab->SetValueByName( r, "Mean Xt", meanXt );
        aggregatedTab->SetValueByName( r, "M2 Xs", M2Xs );
        aggregatedTab->SetValueByName( r, "M2 Xt", M2Xt );
        aggregatedTab->SetValueByName( r, "M XsXt", MXsXt );
        } //r
      } // while ( ( inMetaDO = inMetaColl->GetNextDataObject( it ) ) )

    // Replace initial meta with aggregated table for current variable
//    const char* varName = inMeta->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );
    outMeta->GetMetaData( b )->Set( vtkCompositeDataSet::NAME(), varName );
    outMeta->SetBlock( b, aggregatedTab );

    // Clean up
    aggregatedTab->Delete();
    } // b
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::Learn( vtkTable* inData,
                                          vtkTable* inPara,
                                          vtkMultiBlockDataSet* outMeta )
{
  if ( ! inData )
    {
    return;
    }

  if ( ! inPara )
    {
    return;
    }

  if ( ! outMeta )
    {
    return;
    }

  // Verify that a cardinality was specified for the time slices
  if ( ! this->SliceCardinality )
    {
    vtkErrorMacro( "No time slice cardinality was set. Cannot calculate model." );
    return;
    }

  // Process lparameter table and determine maximum time lag
  vtkIdType nRowPara = inPara->GetNumberOfRows();
  vtkIdType maxLag = 0;
  for ( vtkIdType p = 0; p < nRowPara; ++ p )
    {
    vtkIdType lag = inPara->GetValue( p, 0 ).ToInt();
    if ( lag > maxLag )
      {
      maxLag = lag;
      }
    } // p

  // Verify that a slice cardinality, maximum lag, and data size are consistent
  vtkIdType nRowData = inData->GetNumberOfRows();
  vtkIdType quo = nRowData / this->SliceCardinality;
  if ( maxLag >= quo || nRowData != quo * this->SliceCardinality )
    {
    vtkErrorMacro( "Incorrect specification of time slice cardinality: "
                   << this->SliceCardinality
                   << " with maximum time lag "
                   << maxLag
                   << " and data set cardinality "
                   << nRowData
                   << ". Exiting." );
    return;
    }

  // Rows of the model tables have 6 primary statistics
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 7 );

  // Loop over requests
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString varName = *it;
    if ( ! inData->GetColumnByName( varName ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varName.c_str()
                       << ". Ignoring it." );
      continue;
      }

    // Create primary statistics table for this variable
    vtkTable* modelTab = vtkTable::New();

    vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName( "Time Lag" );
    modelTab->AddColumn( idTypeCol );
    idTypeCol->Delete();

    idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName( "Cardinality" );
    modelTab->AddColumn( idTypeCol );
    idTypeCol->Delete();

    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean Xs" );
    modelTab->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean Xt" );
    modelTab->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "M2 Xs" );
    modelTab->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "M2 Xt" );
    modelTab->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "M XsXt" );
    modelTab->AddColumn( doubleCol );
    doubleCol->Delete();

    // Loop over parameter table
    for ( vtkIdType p = 0; p < nRowPara; ++ p )
      {
      double meanXs = 0.;
      double meanXt = 0.;
      double mom2Xs = 0.;
      double mom2Xt = 0.;
      double momXsXt = 0.;

      // Retrieve current time lag
      vtkIdType lag = inPara->GetValue( p, 0 ).ToInt();

      // Offset into input data table for current time lag
      vtkIdType rowOffset = lag * this->SliceCardinality;

      // Calculate primary statistics
      double inv_n, xs, xt, delta, deltaXsn;
      for ( vtkIdType r = 0; r < this->SliceCardinality; ++ r )
        {
        inv_n = 1. / ( r + 1. );

        xs = inData->GetValueByName( r, varName ).ToDouble();
        delta = xs - meanXs;
        meanXs += delta * inv_n;
        deltaXsn = xs - meanXs;
        mom2Xs += delta * deltaXsn;

        xt = inData->GetValueByName( r + rowOffset, varName ).ToDouble();
        delta = xt - meanXt;
        meanXt += delta * inv_n;
        mom2Xt += delta * ( xt - meanXt );

        momXsXt += delta * deltaXsn;
        }

      // Store primary statistics
      row->SetValue( 0, lag );
      row->SetValue( 1, this->SliceCardinality );
      row->SetValue( 2, meanXs );
      row->SetValue( 3, meanXt );
      row->SetValue( 4, mom2Xs );
      row->SetValue( 5, mom2Xt );
      row->SetValue( 6, momXsXt );
      modelTab->InsertNextRow( row );
      } // p

    // Resize output meta and append model table for current variable
    unsigned int nBlocks = outMeta->GetNumberOfBlocks();
    outMeta->SetNumberOfBlocks( nBlocks + 1 );
    outMeta->GetMetaData( nBlocks )->Set( vtkCompositeDataSet::NAME(), varName );
    outMeta->SetBlock( nBlocks, modelTab );

    // Clean up
    modelTab->Delete();
    } // rit

    // Clean up
    row->Delete();
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::Derive( vtkMultiBlockDataSet* inMeta )
{
  if ( ! inMeta || inMeta->GetNumberOfBlocks() < 1 )
    {
    return;
    }

  // Storage for time series table
  vtkTable* timeTable = vtkTable::New();

  // Iterate over variable blocks
  vtkIdType nLags = 0;
  unsigned int nBlocks = inMeta->GetNumberOfBlocks();
  for ( unsigned int b = 0; b < nBlocks; ++ b )
    {
    vtkTable* modelTab = vtkTable::SafeDownCast( inMeta->GetBlock( b ) );
    if ( ! modelTab  )
      {
      continue;
      }

    // Verify that number of time lags is consistent
    const char* varName = inMeta->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );
    vtkIdType nRow = modelTab->GetNumberOfRows();
    if ( b )
      {
      if ( nRow != nLags )
        {
        vtkErrorMacro( "Variable "
                       << varName
                       << " has "
                       << nRow
                       << " time lags but should have "
                       << nLags
                       << ". Exiting." );
        return;
        }
      }
    else // if ( b )
      {
      nLags = nRow;
      }
    if ( ! nRow  )
      {
      continue;
      }

    int numDerived = 9;
    vtkStdString derivedNames[] = { "Variance Xs",
                                    "Variance Xt",
                                    "Covariance",
                                    "Determinant",
                                    "Slope Xt/Xs",
                                    "Intercept Xt/Xs",
                                    "Slope Xs/Xt",
                                    "Intercept Xs/Xt",
                                    "Autocorrelation" };

    // Find or create columns for derived statistics
    vtkDoubleArray* derivedCol;
    for ( int j = 0; j < numDerived; ++ j )
      {
      if ( ! modelTab->GetColumnByName( derivedNames[j] ) )
        {
        derivedCol = vtkDoubleArray::New();
        derivedCol->SetName( derivedNames[j] );
        derivedCol->SetNumberOfTuples( nRow );
        modelTab->AddColumn( derivedCol );
        derivedCol->Delete();
        }
      }

    // Storage for derived values
    double* derivedVals = new double[numDerived];
    vtkDoubleArray* timeArray = vtkDoubleArray::New();
    timeArray->SetName( varName );

    for ( int i = 0; i < nRow; ++ i )
      {
      double m2Xs = modelTab->GetValueByName( i, "M2 Xs" ).ToDouble();
      double m2Xt = modelTab->GetValueByName( i, "M2 Xt" ).ToDouble();
      double mXsXt = modelTab->GetValueByName( i, "M XsXt" ).ToDouble();

      double varXs, varXt, covXsXt;
      int numSamples = modelTab->GetValueByName(i, "Cardinality" ).ToInt();
      if ( numSamples == 1 )
        {
        varXs  = 0.;
        varXt  = 0.;
        covXsXt = 0.;
        }
      else
        {
        double inv_nm1;
        double n = static_cast<double>( numSamples );
        inv_nm1 = 1. / ( n - 1. );
        varXs  = m2Xs * inv_nm1;
        varXt  = m2Xt * inv_nm1;
        covXsXt = mXsXt * inv_nm1;
        }

      // Store derived values
      derivedVals[0] = varXs;
      derivedVals[1] = varXt;
      derivedVals[2] = covXsXt;
      derivedVals[3] = varXs * varXt - covXsXt * covXsXt;
      
      // There will be NaN values in linear regression if covariance matrix is not positive definite
      double meanXs = modelTab->GetValueByName( i, "Mean Xs" ).ToDouble();
      double meanXt = modelTab->GetValueByName( i, "Mean Xt" ).ToDouble();

      // variable Xt on variable Xs:
      //   slope (explicitly handle degenerate cases)
      if ( varXs < VTK_DBL_MIN )
        {
        derivedVals[4] = vtkMath::Nan();
        }
      else
        {
        derivedVals[4] = covXsXt / varXs;
        }
      //   intercept
      derivedVals[5] = meanXt - derivedVals[4] * meanXs;

      // variable Xs on variable Xt:
      //   slope (explicitly handle degenerate cases)
      if ( varXt < VTK_DBL_MIN )
        {
        derivedVals[6] = vtkMath::Nan();
        }
      else
        {
        derivedVals[6] = covXsXt / varXt;
        }
      //   intercept
      derivedVals[7] = meanXs - derivedVals[6] * meanXt;

      // correlation coefficient (be consistent with degenerate cases detected above)
      if ( varXs < VTK_DBL_MIN
           || varXt < VTK_DBL_MIN )
        {
        derivedVals[8] = vtkMath::Nan();
        }
      else
        {
        derivedVals[8] = covXsXt / sqrt( varXs * varXt );
        }

      // Update time series array
      timeArray->InsertNextValue ( derivedVals[8] );

      for ( int j = 0; j < numDerived; ++ j )
        {
        modelTab->SetValueByName( i, derivedNames[j], derivedVals[j] );
        }
      } // nRow

    // Append correlation coefficient to time series table
    timeTable->AddColumn( timeArray );

    // Clean up
    delete [] derivedVals;
    timeArray->Delete();
    } // for ( unsigned int b = 0; b < nBlocks; ++ b )

  // Now calculate FFT of time series
  vtkTableFFT* fft = vtkTableFFT::New();
  fft->SetInputData( timeTable );
  vtkTable* outt= fft->GetOutput();
  fft->Update();

  // Resize output meta so FFT table can be appended
  inMeta->SetNumberOfBlocks( nBlocks + 1 );

  // Append auto-correlation FFT table at block nBlocks
  inMeta->GetMetaData( nBlocks )->Set( vtkCompositeDataSet::NAME(), "Autocorrelation FFT" );
  inMeta->SetBlock( nBlocks, outt );

  // Clean up
  fft->Delete();
  timeTable->Delete();
}

// ----------------------------------------------------------------------
// Use the invalid value of -1 for p-values if R is absent
vtkDoubleArray* vtkAutoCorrelativeStatistics::CalculatePValues(vtkDoubleArray *statCol)
{
  // A column must be created first
  vtkDoubleArray* testCol = vtkDoubleArray::New();

  // Fill this column
  vtkIdType n = statCol->GetNumberOfTuples();
  testCol->SetNumberOfTuples( n );
  for ( vtkIdType r = 0; r < n; ++ r )
    {
    testCol->SetTuple1( r, -1 );
    }

  return testCol;
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::SelectAssessFunctor( vtkTable* outData,
                                                    vtkDataObject* inMetaDO,
                                                    vtkStringArray* rowNames,
                                                    AssessFunctor*& dfunc )
{
  dfunc = 0;
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta )
    {
    return;
    }

  vtkTable* modelTab= vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! modelTab )
    {
    return;
    }

  vtkTable* derivedTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) );
  if ( ! derivedTab )
    {
    return;
    }

  vtkIdType nRowPrim = modelTab->GetNumberOfRows();
  if ( nRowPrim != derivedTab->GetNumberOfRows() )
    {
    return;
    }

  vtkStdString varName = rowNames->GetValue( 0 );

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* vars = vtkStringArray::SafeDownCast( modelTab->GetColumnByName( "Variable" ) );
  if ( ! vars )
    {
    return;
    }

  // Loop over primary statistics table until the requested variable is found
  for ( int r = 0; r < nRowPrim; ++ r )
    {
    if ( vars->GetValue( r ) == varName )
      {
      // Grab the data for the requested variable
      vtkAbstractArray* arr = outData->GetColumnByName( varName );
      if ( ! arr )
        {
        return;
        }

      // For auto-correlative statistics, type must be convertible to DataArray
      // E.g., StringArrays do not fit here
      vtkDataArray* vals = vtkDataArray::SafeDownCast( arr );
      if ( ! vals )
        {
        return;
        }

      // FIXME: Fetch necessary values here and do something with them

      return;
      }
    }

  // If arrived here it means that the variable of interest was not found in the parameter table
}
