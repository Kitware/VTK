/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCorrelativeStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkToolkits.h"

#include "vtkCorrelativeStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <set>
#include <sstream>

vtkObjectFactoryNewMacro(vtkCorrelativeStatistics)

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::vtkCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues( 3 );
  this->AssessNames->SetValue( 0, "d^2" ); // Squared Mahalanobis distance
  this->AssessNames->SetValue( 1, "Residual Y/X" );
  this->AssessNames->SetValue( 2, "Residual X/Y" );
}

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::~vtkCorrelativeStatistics()
{
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::Aggregate( vtkDataObjectCollection* inMetaColl,
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
    // Verify that the model is indeed contained in a table
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
      if ( primaryTab->GetValueByName( r, "Variable X" ) != aggregatedTab->GetValueByName( r, "Variable X" )
           || primaryTab->GetValueByName( r, "Variable Y" ) != aggregatedTab->GetValueByName( r, "Variable Y" ) )
      {
        // Models do not match
        aggregatedTab->Delete();

        return;
      }

      // Get aggregated statistics
      int n = aggregatedTab->GetValueByName( r, "Cardinality" ).ToInt();
      double meanX = aggregatedTab->GetValueByName( r, "Mean X" ).ToDouble();
      double meanY = aggregatedTab->GetValueByName( r, "Mean Y" ).ToDouble();
      double M2X = aggregatedTab->GetValueByName( r, "M2 X" ).ToDouble();
      double M2Y = aggregatedTab->GetValueByName( r, "M2 Y" ).ToDouble();
      double MXY = aggregatedTab->GetValueByName( r, "M XY" ).ToDouble();

      // Get current model statistics
      int n_c = primaryTab->GetValueByName( r, "Cardinality" ).ToInt();
      double meanX_c = primaryTab->GetValueByName( r, "Mean X" ).ToDouble();
      double meanY_c = primaryTab->GetValueByName( r, "Mean Y" ).ToDouble();
      double M2X_c = primaryTab->GetValueByName( r, "M2 X" ).ToDouble();
      double M2Y_c = primaryTab->GetValueByName( r, "M2 Y" ).ToDouble();
      double MXY_c = primaryTab->GetValueByName( r, "M XY" ).ToDouble();

      // Update global statics
      int N = n + n_c;

      double invN = 1. / static_cast<double>( N );

      double deltaX = meanX_c - meanX;
      double deltaX_sur_N = deltaX * invN;

      double deltaY = meanY_c - meanY;
      double deltaY_sur_N = deltaY * invN;

      int prod_n = n * n_c;

      M2X += M2X_c
        + prod_n * deltaX * deltaX_sur_N;

      M2Y += M2Y_c
        + prod_n * deltaY * deltaY_sur_N;

      MXY += MXY_c
        + prod_n * deltaX * deltaY_sur_N;

      meanX += n_c * deltaX_sur_N;

      meanY += n_c * deltaY_sur_N;

      // Store updated model
      aggregatedTab->SetValueByName( r, "Cardinality", N );
      aggregatedTab->SetValueByName( r, "Mean X", meanX );
      aggregatedTab->SetValueByName( r, "Mean Y", meanY );
      aggregatedTab->SetValueByName( r, "M2 X", M2X );
      aggregatedTab->SetValueByName( r, "M2 Y", M2Y );
      aggregatedTab->SetValueByName( r, "M XY", MXY );
    }
  }

  // Finally set first block of aggregated model to primary statistics table
  outMeta->SetNumberOfBlocks( 1 );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Primary Statistics" );
  outMeta->SetBlock( 0, aggregatedTab );

  // Clean up
  aggregatedTab->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::Learn( vtkTable* inData,
                                      vtkTable* vtkNotUsed( inParameters ),
                                      vtkMultiBlockDataSet* outMeta )
{
  if ( ! inData )
  {
    return;
  }

  if ( ! outMeta )
  {
    return;
  }

  // Summary table: assigns a unique key to each (variable X,variable Y) pair
  vtkTable* primaryTab = vtkTable::New();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable X" );
  primaryTab->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable Y" );
  primaryTab->AddColumn( stringCol );
  stringCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  primaryTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Mean X" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Mean Y" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2 X" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2 Y" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M XY" );
  primaryTab->AddColumn( doubleCol );
  doubleCol->Delete();

  // Loop over requests
  vtkIdType nRow = inData->GetNumberOfRows();
  for ( std::set<std::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
  {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    std::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString colX = *it;
    if ( ! inData->GetColumnByName( colX ) )
    {
      vtkWarningMacro( "InData table does not have a column "
                       << colX.c_str()
                       << ". Ignoring this pair." );
      continue;
    }

    ++ it;
    vtkStdString colY = *it;
    if ( ! inData->GetColumnByName( colY ) )
    {
      vtkWarningMacro( "InData table does not have a column "
                       << colY.c_str()
                       << ". Ignoring this pair." );
      continue;
    }

    double meanX = 0.;
    double meanY = 0.;
    double mom2X = 0.;
    double mom2Y = 0.;
    double momXY = 0.;

    double inv_n, x, y, delta, deltaXn;
    for ( vtkIdType r = 0; r < nRow; ++ r )
    {
      inv_n = 1. / ( r + 1. );

      x = inData->GetValueByName( r, colX ).ToDouble();
      delta = x - meanX;
      meanX += delta * inv_n;
      deltaXn = x - meanX;
      mom2X += delta * deltaXn;

      y = inData->GetValueByName( r, colY ).ToDouble();
      delta = y - meanY;
      meanY += delta * inv_n;
      mom2Y += delta * ( y - meanY );

      momXY += delta * deltaXn;
    }

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues( 8 );

    row->SetValue( 0, colX );
    row->SetValue( 1, colY );
    row->SetValue( 2, nRow );
    row->SetValue( 3, meanX );
    row->SetValue( 4, meanY );
    row->SetValue( 5, mom2X );
    row->SetValue( 6, mom2Y );
    row->SetValue( 7, momXY );

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
void vtkCorrelativeStatistics::Derive( vtkMultiBlockDataSet* inMeta )
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
  vtkStdString doubleNames[] = { "Variance X",
                                 "Variance Y",
                                 "Covariance",
                                 "Determinant",
                                 "Slope Y/X",
                                 "Intercept Y/X",
                                 "Slope X/Y",
                                 "Intercept X/Y",
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

  // Storage for stdv x, stdv y, var x, var y, cov, slope y/x, int. y/x, slope x/y, int. x/y, r, D
  double* derivedVals = new double[numDoubles]; //

  for ( int i = 0; i < nRow; ++ i )
  {
    double m2X = primaryTab->GetValueByName( i, "M2 X" ).ToDouble();
    double m2Y = primaryTab->GetValueByName( i, "M2 Y" ).ToDouble();
    double mXY = primaryTab->GetValueByName( i, "M XY" ).ToDouble();

    double varX, varY, covXY;
    int numSamples = primaryTab->GetValueByName(i, "Cardinality" ).ToInt();
    if ( numSamples == 1 )
    {
      varX  = 0.;
      varY  = 0.;
      covXY = 0.;
    }
    else
    {
      double inv_nm1;
      double n = static_cast<double>( numSamples );
      inv_nm1 = 1. / ( n - 1. );
      varX  = m2X * inv_nm1;
      varY  = m2Y * inv_nm1;
      covXY = mXY * inv_nm1;
    }

    derivedVals[0] = varX;
    derivedVals[1] = varY;
    derivedVals[2] = covXY;
    derivedVals[3] = varX * varY - covXY * covXY;

    // There will be NaN values in linear regression if covariance matrix is not positive definite
    double meanX = primaryTab->GetValueByName( i, "Mean X" ).ToDouble();
    double meanY = primaryTab->GetValueByName( i, "Mean Y" ).ToDouble();

    // variable Y on variable X:
    //   slope (explicitly handle degenerate cases)
      if ( varX < VTK_DBL_MIN )
      {
        derivedVals[4] = vtkMath::Nan();
      }
      else
      {
        derivedVals[4] = covXY / varX;
      }
    //   intercept
    derivedVals[5] = meanY - derivedVals[4] * meanX;

    // variable X on variable Y:
    //   slope (explicitly handle degenerate cases)
      if ( varY < VTK_DBL_MIN )
      {
        derivedVals[6] = vtkMath::Nan();
      }
      else
      {
        derivedVals[6] = covXY / varY;
      }
    //   intercept
    derivedVals[7] = meanX - derivedVals[6] * meanY;

    // correlation coefficient (be consistent with degenerate cases detected above)
    if ( varX < VTK_DBL_MIN
         || varY < VTK_DBL_MIN )
    {
      derivedVals[8] = vtkMath::Nan();
    }
    else
    {
      derivedVals[8] = covXY / sqrt( varX * varY );
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
vtkDoubleArray* vtkCorrelativeStatistics::CalculatePValues(vtkDoubleArray* statCol)
{
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
void vtkCorrelativeStatistics::Test( vtkTable* inData,
                                     vtkMultiBlockDataSet* inMeta,
                                     vtkTable* outMeta )
{
  if ( ! inMeta )
  {
    return;
  }

  vtkTable* primaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
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
    vtkErrorMacro( "Inconsistent input: primary model has "
                   << nRowPrim
                   << " rows but derived model has "
                   << derivedTab->GetNumberOfRows()
                   <<". Cannot test." );
    return;
  }

  if ( ! outMeta )
  {
    return;
  }

  // Prepare columns for the test:
  // 0: variable X name
  // 1: variable Y name
  // 2: bivariate Srivastava skewness
  // 3: bivariate Srivastava kurtosis
  // 4: bivariate Jarque-Bera-Srivastava statistic
  // 5: bivariate Jarque-Bera-Srivastava p-value (calculated only if R is available, filled with -1 otherwise)
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  vtkStringArray* nameColX = vtkStringArray::New();
  nameColX->SetName( "Variable X" );

  vtkStringArray* nameColY = vtkStringArray::New();
  nameColY->SetName( "Variable Y" );

  vtkDoubleArray* bS1Col = vtkDoubleArray::New();
  bS1Col->SetName( "Srivastava Skewness" );

  vtkDoubleArray* bS2Col = vtkDoubleArray::New();
  bS2Col->SetName( "Srivastava Kurtosis" );

  vtkDoubleArray* statCol = vtkDoubleArray::New();
  statCol->SetName( "Jarque-Bera-Srivastava" );

  // Downcast columns to string arrays for efficient data access
  vtkStringArray* varsX = vtkArrayDownCast<vtkStringArray>( primaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varsY = vtkArrayDownCast<vtkStringArray>( primaryTab->GetColumnByName( "Variable Y" ) );

  // Loop over requests
  vtkIdType nRowData = inData->GetNumberOfRows();
  for ( std::set<std::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
  {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    std::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString varNameX = *it;
    if ( ! inData->GetColumnByName( varNameX ) )
    {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameX.c_str()
                       << ". Ignoring this pair." );
      continue;
    }

    ++ it;
    vtkStdString varNameY = *it;
    if ( ! inData->GetColumnByName( varNameY ) )
    {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameY.c_str()
                       << ". Ignoring this pair." );
      continue;
    }

    // Find the model row that corresponds to the variable pair of the request
    vtkIdType r = 0;
    while ( r < nRowPrim
            && ( varsX->GetValue( r ) != varNameX
                 || varsY->GetValue( r ) != varNameY ) )
    {
      ++ r;
    }
    if ( r >= nRowPrim )
    {
      vtkWarningMacro( "Incomplete input: model does not have a row for pair"
                     << varNameX.c_str()
                     << ", "
                     << varNameY.c_str()
                     <<". Cannot test." );
      continue;
    }

    // Retrieve model statistics necessary for Jarque-Bera-Srivastava testing
    if ( primaryTab->GetValueByName( r, "Cardinality" ).ToInt() != nRowData )
    {
      vtkWarningMacro( "Inconsistent input: input data has "
                       << nRowData
                       << " rows but primary model has cardinality "
                       << primaryTab->GetValueByName( r, "Cardinality" ).ToDouble()
                       << " for pair "
                       << varNameX.c_str()
                       << ", "
                       << varNameY.c_str()
                       <<". Cannot test." );
      continue;
    }

    double mX = primaryTab->GetValueByName( r, "Mean X" ).ToDouble();
    double mY = primaryTab->GetValueByName( r, "Mean Y" ).ToDouble();
    double sX2 = derivedTab->GetValueByName( r, "Variance X" ).ToDouble();
    double sY2 = derivedTab->GetValueByName( r, "Variance Y" ).ToDouble();
    double sXY = derivedTab->GetValueByName( r, "Covariance" ).ToDouble();

    // Now calculate Jarque-Bera-Srivastava and ancillary statistics
    double bS1;
    double bS2;
    double jbs;

    // Eliminate absurd or degenerate covariance matrices
    double sXY2 = sXY * sXY;
    double detS = sX2 * sY2 - sXY2;
    if ( detS < VTK_DBL_MIN
         || sX2 < 0.
         || sY2 < 0. )
    {
      // Absurd or degenerate inputs result in NaN statistics
      bS1 = vtkMath::Nan();
      bS2 = vtkMath::Nan();
      jbs = vtkMath::Nan();
    }
    else
    {
      // Normalization factors
      double invn = 1. / nRowData;
      double halfinvn = .5 * invn;

      // Initialize third and fourth order sums
      double sum3X = 0.;
      double sum3Y = 0.;
      double sum4X = 0.;
      double sum4Y = 0.;
      double tmp;

      // If covariance matrix is diagonal within machine precision, do not transform
      if ( sXY < sqrt( VTK_DBL_MIN )
           || sXY < ( .5 * sqrt( VTK_DBL_EPSILON ) * fabs( sX2 - sY2 ) ) )
      {
        // Simply iterate over all observations
        double x, y;
        for ( vtkIdType j = 0; j < nRowData; ++ j )
        {
          // Read and center observation
          x = inData->GetValueByName( j, varNameX ).ToDouble() - mX;
          y = inData->GetValueByName( j, varNameY ).ToDouble() - mY;

          // Update third and fourth order sums for each eigencoordinate
          tmp = x * x;
          sum3X += tmp * x;
          sum4X += tmp * tmp;
          tmp = y * y;
          sum3Y += tmp * y;
          sum4Y += tmp * tmp;
        }

        // Normalize all sums with corresponding eigenvalues and powers
        sum3X *= sum3X;
        tmp = sX2 * sX2;
        sum3X /= ( tmp * sX2 );
        sum4X /= tmp;

        sum3Y *= sum3Y;
        tmp = sY2 * sY2;
        sum3Y /= ( tmp * sY2 );
        sum4Y /= tmp;
      } // if ( sXY < 1.e-300 )
      // Otherwise calculated transformation matrix H and apply transformation
      else
      {
        // Calculate trace, discriminant, and eigenvalues of covariance matrix S
        double trS = sX2 + sY2;
        double sqdS = sqrt( trS * trS - 4 * detS );
        double eigS1 = .5 * ( trS + sqdS );
        double eigS2 = .5 * ( trS - sqdS );

        // Calculate transformation matrix H so S = H diag(eigSi) H^t
        double w = .5 * ( sX2 - sY2 - sqdS );
        double f = 1. / sqrt ( sXY2 + w * w );

        // Diagonal terms of H are identical
        double hd = f * sXY;
        double h21 = f * ( eigS1 - sX2 );
        double h12 = f * ( eigS2 - sY2 );

        // Now iterate over all observations
        double x, y, t1, t2;
        for ( vtkIdType j = 0; j < nRowData; ++ j )
        {
          // Read and center observation
          x = inData->GetValueByName( j, varNameX ).ToDouble() - mX;
          y = inData->GetValueByName( j, varNameY ).ToDouble() - mY;

          // Transform coordinates into eigencoordinates
          t1 = hd * x + h21 * y;
          t2 = h12 * x + hd * y;

          // Update third and fourth order sums for each eigencoordinate
          tmp = t1 * t1;
          sum3X += tmp * t1;
          sum4X += tmp * tmp;
          tmp = t2 * t2;
          sum3Y += tmp * t2;
          sum4Y += tmp * tmp;
        }

        // Normalize all sums with corresponding eigenvalues and powers
        sum3X *= sum3X;
        tmp = eigS1 * eigS1;
        sum3X /= ( tmp * eigS1 );
        sum4X /= tmp;

        sum3Y *= sum3Y;
        tmp = eigS2 * eigS2;
        sum3Y /= ( tmp * eigS2 );
        sum4Y /= tmp;
      } // else

      // Calculate Srivastava skewness and kurtosis
      bS1 = halfinvn * invn * ( sum3X +  sum3Y );
      bS2 = halfinvn * ( sum4X +  sum4Y );

      // Finally, calculate Jarque-Bera-Srivastava statistic
      tmp = bS2 - 3.;
      jbs = static_cast<double>( nRowData ) * ( bS1 / 3. + ( tmp * tmp ) / 12. );
    }

    // Insert variable name and calculated Jarque-Bera-Srivastava statistic
    // NB: R will be invoked only once at the end for efficiency
    nameColX->InsertNextValue( varNameX );
    nameColY->InsertNextValue( varNameY );
    bS1Col->InsertNextTuple1( bS1 );
    bS2Col->InsertNextTuple1( bS2 );
    statCol->InsertNextTuple1( jbs );
  } // rit

  // Now, add the already prepared columns to the output table
  outMeta->AddColumn( nameColX );
  outMeta->AddColumn( nameColY );
  outMeta->AddColumn( bS1Col );
  outMeta->AddColumn( bS2Col );
  outMeta->AddColumn( statCol );

  // Last phase: compute the p-values or assign invalid value if they cannot be computed
  // If available, use R to obtain the p-values for the Chi square distribution with 2 DOFs
  vtkDoubleArray* testCol = this->CalculatePValues(statCol);

  // The test column name
  testCol->SetName( "P" );

  // Now add the column of invalid values to the output table
  outMeta->AddColumn( testCol );

  testCol->Delete();

  // Clean up
  nameColX->Delete();
  nameColY->Delete();
  bS1Col->Delete();
  bS2Col->Delete();
  statCol->Delete();
}

// ----------------------------------------------------------------------
class BivariateRegressionDeviationsFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkDataArray* DataX;
  vtkDataArray* DataY;
  double MeanX;
  double MeanY;
  double VarX;
  double VarY;
  double InvDetXY;
  double CovXY;
  double SlopeYX;
  double SlopeXY;
  double InterYX;
  double InterXY;

  BivariateRegressionDeviationsFunctor( vtkDataArray* valsX,
                                        vtkDataArray* valsY,
                                        double meanX,
                                        double meanY,
                                        double varianceX,
                                        double varianceY,
                                        double covXY,
                                        double invDetXY,
                                        double slopeYX,
                                        double slopeXY,
                                        double interYX,
                                        double interXY )
  {
    this->DataX = valsX;
    this->DataY = valsY;
    this->MeanX = meanX;
    this->MeanY = meanY;
    this->VarX  = varianceX;
    this->VarY  = varianceY;
    this->CovXY = covXY;
    this->InvDetXY = invDetXY;
    this->SlopeYX = slopeYX;
    this->SlopeXY = slopeXY;
    this->InterYX = interYX;
    this->InterXY = interXY;
  }
  ~BivariateRegressionDeviationsFunctor() VTK_OVERRIDE { }
  void operator() ( vtkDoubleArray* result,
                            vtkIdType id ) VTK_OVERRIDE
  {
    // First retrieve 2-d observation
    double x = this->DataX->GetTuple1( id );
    double y = this->DataY->GetTuple1( id );

    // Center observation for efficiency
    double x_c = x - this->MeanX;
    double y_c = y - this->MeanY;

    // Calculate 2-d squared Mahalanobis distance (Nan for degenerate cases)
    double smd  =  this->InvDetXY * ( this->VarY * x_c * x_c - 2. * this->CovXY * x_c * y_c + this->VarX * y_c * y_c );

    // Calculate residual from regression of Y into X
    double dYX = y - ( this->SlopeYX * x + this->InterYX );

    // Calculate residual from regression of X into Y
    double dXY = x - ( this->SlopeXY * y + this->InterXY );

    // Store calculated assessments
    result->SetNumberOfValues( 3 );
    result->SetValue( 0, smd );
    result->SetValue( 1, dYX );
    result->SetValue( 2, dXY );
  }
};

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::SelectAssessFunctor( vtkTable* outData,
                                                    vtkDataObject* inMetaDO,
                                                    vtkStringArray* rowNames,
                                                    AssessFunctor*& dfunc )
{
  dfunc = 0;
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta
       || inMeta->GetNumberOfBlocks() < 2 )
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

  vtkStdString varNameX = rowNames->GetValue( 0 );
  vtkStdString varNameY = rowNames->GetValue( 1 );

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* varX = vtkArrayDownCast<vtkStringArray>( primaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkArrayDownCast<vtkStringArray>( primaryTab->GetColumnByName( "Variable Y" ) );
  if ( ! varX || ! varY )
  {
    return;
  }

  // Loop over parameters table until the requested variables are found
  for ( int r = 0; r < nRowPrim; ++ r )
  {
    if ( varX->GetValue( r ) == varNameX
         &&
         varY->GetValue( r ) == varNameY )
    {
      // Grab the data for the requested variables
      vtkAbstractArray* arrX = outData->GetColumnByName( varNameX );
      vtkAbstractArray* arrY = outData->GetColumnByName( varNameY );
      if ( ! arrX || ! arrY )
      {
        return;
      }

      // For descriptive statistics, types must be convertible to DataArrays (e.g., StringArrays do not fit here).
      vtkDataArray* valsX = vtkArrayDownCast<vtkDataArray>( arrX );
      vtkDataArray* valsY = vtkArrayDownCast<vtkDataArray>( arrY );
      if ( ! valsX || ! valsY )
      {
        return;
      }

      // Fetch necessary values from primary model
      double meanX = primaryTab->GetValueByName( r, "Mean X" ).ToDouble();
      double meanY = primaryTab->GetValueByName( r, "Mean Y" ).ToDouble();

      // Fetch necessary values from derived model
      // NB: If derived values were specified (and not calculated by Derive)
      //     and are mutually inconsistent, then incorrect assessments will be produced
      double varianceX = derivedTab->GetValueByName( r, "Variance X" ).ToDouble();
      double varianceY = derivedTab->GetValueByName( r, "Variance Y" ).ToDouble();
      double covXY = derivedTab->GetValueByName( r, "Covariance" ).ToDouble();
      double detXY = derivedTab->GetValueByName( r, "Determinant" ).ToDouble();
      double slopeYX = derivedTab->GetValueByName( r, "Slope Y/X" ).ToDouble();
      double slopeXY = derivedTab->GetValueByName( r, "Slope X/Y" ).ToDouble();
      double interYX = derivedTab->GetValueByName( r, "Intercept Y/X" ).ToDouble();
      double interXY = derivedTab->GetValueByName( r, "Intercept X/Y" ).ToDouble();

      // Mahalanobis distance will always be Nan for degenerate covariance matrices
      double invDetXY;
      if ( detXY < VTK_DBL_MIN
           || varianceX < 0.
           || varianceY < 0. )
      {
        invDetXY = vtkMath::Nan();
      }
      else
      {
        invDetXY = 1. / detXY;
      }

      dfunc = new BivariateRegressionDeviationsFunctor( valsX,
                                                        valsY,
                                                        meanX,
                                                        meanY,
                                                        varianceX,
                                                        varianceY,
                                                        covXY,
                                                        invDetXY,
                                                        slopeYX,
                                                        slopeXY,
                                                        interYX,
                                                        interXY );

      // Parameters of requested column pair were found, no need to continue
      return;
    }
  }

  // If arrived here, it means that the pair of variables of interest was not found in the parameter table
}
