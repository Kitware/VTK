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
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
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
  this->TimeLag = 0; // By default, autocorrelation matrix only contains var(X)
}

// ----------------------------------------------------------------------
vtkAutoCorrelativeStatistics::~vtkAutoCorrelativeStatistics()
{
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "TimeLag: " << this->TimeLag << "\n";
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::Aggregate( vtkDataObjectCollection* inMetaColl,
                                              vtkMultiBlockDataSet* outMeta )
{
  if ( ! outMeta )
    {
    return;
    }

  // Get hold of the first model (data object) in the collection
  vtkCollectionSimpleIterator it;
  inMetaColl->InitTraversal( it );
  vtkDataObject *inMetaDO = inMetaColl->GetNextDataObject( it );

  // Verify that the first input model is indeed contained in a multiblock data set
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta )
    {
    return;
    }

  // Verify that the first primary statistics are indeed contained in a table
  vtkTable* primaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! primaryTab )
    {
    return;
    }

  vtkIdType nRow = primaryTab->GetNumberOfRows();
  if ( ! nRow )
    {
    // No statistics were calculated.
    return;
    }

  // Use this first model to initialize the aggregated one
  vtkTable* aggregatedTab = vtkTable::New();
  aggregatedTab->DeepCopy( primaryTab );

  // Now, loop over all remaining models and update aggregated each time
  while ( ( inMetaDO = inMetaColl->GetNextDataObject( it ) ) )
    {
    // Verify that the current model is indeed contained in a multiblock data set
    inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
    if ( ! inMeta )
      {
      aggregatedTab->Delete();

      return;
      }

    // Verify that the current primary statistics are indeed contained in a table
    primaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
    if ( ! primaryTab )
      {
      aggregatedTab->Delete();

      return;
      }

    if ( primaryTab->GetNumberOfRows() != nRow )
      {
      // Models do not match
      aggregatedTab->Delete();

      return;
      }

    // Iterate over all model rows
    for ( int r = 0; r < nRow; ++ r )
      {
      // Verify that variable names match each other
      if ( primaryTab->GetValueByName( r, "Variable" ) != aggregatedTab->GetValueByName( r, "Variable" ) )
        {
        // Models do not match
        aggregatedTab->Delete();

        return;
        }

      // Get aggregated statistics
      int n = aggregatedTab->GetValueByName( r, "Cardinality" ).ToInt();
      double meanXs = aggregatedTab->GetValueByName( r, "Mean Xs" ).ToDouble();
      double meanXt = aggregatedTab->GetValueByName( r, "Mean Xt" ).ToDouble();
      double M2Xs = aggregatedTab->GetValueByName( r, "M2 Xs" ).ToDouble();
      double M2Xt = aggregatedTab->GetValueByName( r, "M2 Xt" ).ToDouble();
      double MXsXt = aggregatedTab->GetValueByName( r, "M XsXt" ).ToDouble();

      // Get current model statistics
      int n_c = primaryTab->GetValueByName( r, "Cardinality" ).ToInt();
      double meanXs_c = primaryTab->GetValueByName( r, "Mean Xs" ).ToDouble();
      double meanXt_c = primaryTab->GetValueByName( r, "Mean Xt" ).ToDouble();
      double M2Xs_c = primaryTab->GetValueByName( r, "M2 Xs" ).ToDouble();
      double M2Xt_c = primaryTab->GetValueByName( r, "M2 Xt" ).ToDouble();
      double MXsXt_c = primaryTab->GetValueByName( r, "M XsXt" ).ToDouble();

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
      }
    }

  // Finally set first block of aggregated model to primary statistics table
  outMeta->SetNumberOfBlocks( 1 );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Primary Statistics" );
  outMeta->SetBlock( 0, aggregatedTab );

  // Clean up
  aggregatedTab->Delete();
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

  // The primary statistics table
  vtkTable* primaryTab = vtkTable::New();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  primaryTab->AddColumn( stringCol );
  stringCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  primaryTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Mean Xs" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Mean Xt" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2 Xs" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2 Xt" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M XsXt" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  // Verify that a cardinality was specified for the time slices
  if ( ! this->SliceCardinality )
    {
    vtkErrorMacro( "No time slice cardinality was set. Cannot calculate model." );
    return;
    }

  // Process list of time lags given by parameter table and determine maximum
  vtkIdType nRowPara = inPara->GetNumberOfRows();
  vtkIdType maxLag = 0;
  for ( vtkIdType r = 0; r < nRowPara; ++ r )
    {
    vtkIdType lag = inPara->GetValue( r, 0 ).ToInt();
    if ( lag > maxLag )
      {
      maxLag = lag;
      }
    }

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

  // Store offset into input data table, which will remain constant across variables
  vtkIdType rowOffset = this->TimeLag * this->SliceCardinality;

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

    double meanXs = 0.;
    double meanXt = 0.;
    double mom2Xs = 0.;
    double mom2Xt = 0.;
    double momXsXt = 0.;

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

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues( 7 );

    row->SetValue( 0, varName );
    row->SetValue( 1, this->SliceCardinality );
    row->SetValue( 2, meanXs );
    row->SetValue( 3, meanXt );
    row->SetValue( 4, mom2Xs );
    row->SetValue( 5, mom2Xt );
    row->SetValue( 6, momXsXt );

    primaryTab->InsertNextRow( row );

    row->Delete();
    } // rit

  // Finally set first block of output meta port to primary statistics table
  outMeta->SetNumberOfBlocks( 1 );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Primary Statistics" );
  outMeta->SetBlock( 0, primaryTab );

  // Clean up
  primaryTab->Delete();
}

// ----------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::Derive( vtkMultiBlockDataSet* inMeta )
{
  if ( ! inMeta || inMeta->GetNumberOfBlocks() < 1 )
    {
    return;
    }

  vtkTable* primaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! primaryTab  )
    {
    return;
    }

  int numDoubles = 9;
  vtkStdString doubleNames[] = { "Variance Xs",
                                 "Variance Xt",
                                 "Covariance",
                                 "Determinant",
                                 "Slope Xt/Xs",
                                 "Intercept Xt/Xs",
                                 "Slope Xs/Xt",
                                 "Intercept Xs/Xt",
                                 "Pearson r" };

  // Create table for derived statistics
  vtkIdType nRow = primaryTab->GetNumberOfRows();
  vtkTable* derivedTab = vtkTable::New();
  vtkDoubleArray* doubleCol;
  for ( int j = 0; j < numDoubles; ++ j )
    {
    if ( ! derivedTab->GetColumnByName( doubleNames[j] ) )
      {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName( doubleNames[j] );
      doubleCol->SetNumberOfTuples( nRow );
      derivedTab->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }

  // Storage for derived values
  double* derivedVals = new double[numDoubles]; //

  for ( int i = 0; i < nRow; ++ i )
    {
    double m2Xs = primaryTab->GetValueByName( i, "M2 Xs" ).ToDouble();
    double m2Xt = primaryTab->GetValueByName( i, "M2 Xt" ).ToDouble();
    double mXsXt = primaryTab->GetValueByName( i, "M XsXt" ).ToDouble();

    double varXs, varXt, covXsXt;
    int numSamples = primaryTab->GetValueByName(i, "Cardinality" ).ToInt();
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

    derivedVals[0] = varXs;
    derivedVals[1] = varXt;
    derivedVals[2] = covXsXt;
    derivedVals[3] = varXs * varXt - covXsXt * covXsXt;

    // There will be NaN values in linear regression if covariance matrix is not positive definite
    double meanXs = primaryTab->GetValueByName( i, "Mean Xs" ).ToDouble();
    double meanXt = primaryTab->GetValueByName( i, "Mean Xt" ).ToDouble();

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
    //   intersect
    derivedVals[5] = meanXt - derivedVals[4] * meanXs;

    //   variable Xs on variable Xt:
    //   slope (explicitly handle degenerate cases)
      if ( varXt < VTK_DBL_MIN )
        {
        derivedVals[6] = vtkMath::Nan();
        }
      else
        {
        derivedVals[6] = covXsXt / varXt;
        }
    //   intersect
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

    for ( int j = 0; j < numDoubles; ++ j )
      {
      derivedTab->SetValueByName( i, doubleNames[j], derivedVals[j] );
      }
    } // nRow

  // Finally set second block of output meta port to derived statistics table
  inMeta->SetNumberOfBlocks( 2 );
  inMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Derived Statistics" );
  inMeta->SetBlock( 1, derivedTab );

  // Clean up
  derivedTab->Delete();
  delete [] derivedVals;
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

  vtkTable* primaryTab= vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! primaryTab )
    {
    return;
    }

  vtkTable* derivedTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) );
  if ( ! derivedTab )
    {
    return;
    }

  vtkIdType nRowPrim = primaryTab->GetNumberOfRows();
  if ( nRowPrim != derivedTab->GetNumberOfRows() )
    {
    return;
    }

  vtkStdString varName = rowNames->GetValue( 0 );

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* vars = vtkStringArray::SafeDownCast( primaryTab->GetColumnByName( "Variable" ) );
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
