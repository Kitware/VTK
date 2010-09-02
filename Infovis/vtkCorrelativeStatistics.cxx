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
  Copyright 2008 Sandia Corporation.
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
#ifdef VTK_USE_GNU_R
#include <vtkRInterface.h>
#endif // VTK_USE_GNU_R
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/set>

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkCorrelativeStatistics);

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::vtkCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "d^2" );

  this->AssessParameters = vtkStringArray::New();
  this->AssessParameters->SetNumberOfValues( 5 );
  this->AssessParameters->SetValue( 0, "Mean X" );
  this->AssessParameters->SetValue( 1, "Mean Y" );
  this->AssessParameters->SetValue( 2, "Variance X" );
  this->AssessParameters->SetValue( 3, "Variance Y" );
  this->AssessParameters->SetValue( 4, "Covariance" );
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
      if ( primaryTab->GetValueByName( r, "Variable" ) != aggregatedTab->GetValueByName( r, "Variable" ) )
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

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  primaryTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable X" );
  primaryTab->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable Y" );
  primaryTab->AddColumn( stringCol );
  stringCol->Delete();

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
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin(); 
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
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
    
    row->SetValue( 0, nRow );
    row->SetValue( 1, colX );
    row->SetValue( 2, colY );
    row->SetValue( 3, meanX );
    row->SetValue( 4, meanY );
    row->SetValue( 5, mom2X );
    row->SetValue( 6, mom2Y );
    row->SetValue( 7, momXY );

    primaryTab->InsertNextRow( row );

    row->Delete();
    }
    
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

  int numDoubles = 8;
  vtkStdString doubleNames[] = { "Variance X",
                                 "Variance Y",
                                 "Covariance",
                                 "Slope Y/X", 
                                 "Intersect Y/X", 
                                 "Slope X/Y", 
                                 "Intersect X/Y", 
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

  if ( ! derivedTab->GetColumnByName( "Linear Correlation" ) )
    {  
    vtkStringArray* stringCol = vtkStringArray::New();
    stringCol->SetName( "Linear Correlation" );
    stringCol->SetNumberOfTuples( nRow );
    derivedTab->AddColumn( stringCol );
    stringCol->Delete();
    }

  // Storage for stdv x, stdv y, var x, var y, cov, slope y/x, int. y/x, slope x/y, int. x/y, r
  double* derivedVals = new double[numDoubles]; // 

  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString c1 = primaryTab->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = primaryTab->GetValueByName( i, "Variable Y" ).ToString();
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

    vtkStdString status = "valid";

    double d = varX * varY - covXY * covXY;
    if ( d <= 0. )
      {
      vtkWarningMacro( "Incorrect parameters for column pair ("
                       <<c1.c_str()
                       <<", "
                       <<c2.c_str()
                       <<"): variance/covariance matrix has non-positive determinant." );
      derivedVals[3] = 0.;
      derivedVals[4] = 0.;
      derivedVals[5] = 0.;
      derivedVals[6] = 0.;
      derivedVals[7] = 0.;
      status = "invalid";
      }
    else
      {
      double meanX = primaryTab->GetValueByName( i, "Mean X" ).ToDouble();
      double meanY = primaryTab->GetValueByName( i, "Mean Y" ).ToDouble();

      // variable Y on variable X:
      //   slope
      derivedVals[3] = covXY / varX;
      //   intersect
      derivedVals[4] = meanY - derivedVals[0] * meanX;
      
      //   variable X on variable Y:
      //   slope
      derivedVals[5] = covXY / varY;
      //   intersect
      derivedVals[6] = meanX - derivedVals[2] * meanY;
      
      // correlation coefficient
      derivedVals[7] = covXY / sqrt( varX * varY );
      }

    derivedTab->SetValueByName( i, "Linear Correlation", status );
    for ( int j = 0; j < numDoubles; ++ j )
      {
      derivedTab->SetValueByName( i, doubleNames[j], derivedVals[j] );
      }
    }

  // Finally set second block of output meta port to derived statistics table
  inMeta->SetNumberOfBlocks( 2 );
  inMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Derived Statistics" );
  inMeta->SetBlock( 1, derivedTab );

  // Clean up
  derivedTab->Delete();
  delete [] derivedVals;
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
  vtkStringArray* varsX = vtkStringArray::SafeDownCast( primaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varsY = vtkStringArray::SafeDownCast( primaryTab->GetColumnByName( "Variable Y" ) );

  // Loop over requests
  vtkIdType nRowData = inData->GetNumberOfRows();
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
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

    // Eliminate near degenerate covariance matrices
    double sXY2 = sXY * sXY;
    double detS = sX2 * sY2 - sXY2;
    double invn = 1. / nRowData;
    double halfinvn = .5 * invn;
    if ( detS > 1.e-100
         && sX2 > 0.
         && sY2 > 0. )
      {
      // Calculate trace, discriminant, and eigenvalues of covariance matrix S
      double trS = sX2 + sY2;
      double sqdS = sqrt( trS * trS - 4 * detS );
      double eigS1 = .5 * ( trS + sqdS );
      double eigS2 = .5 * ( trS - sqdS );

      // Calculate transformation matrix H so S = H diag(eigSi) H^t
      double w = .5 * ( sX2 - sY2 - sqdS );
      double f = 1. / sqrt ( sXY2 + w * w );

      double hd = f * sXY; // Diagonal terms of H are identical
      double h21 = f * ( eigS1 - sX2 );
      double h12 = f * ( eigS2 - sY2 );

      // Now iterate over all observations
      double sum3X = 0.;
      double sum3Y = 0.;
      double sum4X = 0.;
      double sum4Y = 0.;
      double x, y, tmp, t1, t2;
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

      // Calculate Srivastava skewness and kurtosis
      bS1 = halfinvn * invn * ( sum3X +  sum3Y );
      bS2 = halfinvn * ( sum4X +  sum4Y );

      // Finally, calculate Jarque-Bera-Srivastava statistic
      tmp = bS2 - 3.;
      jbs = static_cast<double>( nRowData ) * ( bS1 / 3. + ( tmp * tmp ) / 12. );
      }
    else
      {
      bS1 = vtkMath::Nan();
      bS2 = vtkMath::Nan();
      jbs = vtkMath::Nan();
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
  vtkDoubleArray* testCol = 0;
  bool calculatedP = false;

  // If available, use R to obtain the p-values for the Chi square distribution with 2 DOFs
#ifdef VTK_USE_GNU_R
  // Prepare VTK - R interface
  vtkRInterface* ri = vtkRInterface::New();

  // Use the calculated Jarque-Bera-Srivastava statistics as input to the Chi square function
  ri->AssignVTKDataArrayToRVariable( statCol, "jbs" );

  // Calculate the p-values (p+1=3 degrees of freedom)
  ri->EvalRscript( "p=1-pchisq(jbs,3)" );

  // Retrieve the p-values
  testCol = vtkDoubleArray::SafeDownCast( ri->AssignRVariableToVTKDataArray( "p" ) );
  if ( ! testCol || testCol->GetNumberOfTuples() != statCol->GetNumberOfTuples() )
    {
    vtkWarningMacro( "Something went wrong with the R calculations. Reported p-values will be invalid." );
    }
  else
    {
    // Test values have been calculated by R: the test column can be added to the output table
    outMeta->AddColumn( testCol );
    calculatedP = true;
    }

  // Clean up
  ri->Delete();
#endif // VTK_USE_GNU_R

  // Use the invalid value of -1 for p-values if R is absent or there was an R error
  if ( ! calculatedP )
    {
    // A column must be created first
    testCol = vtkDoubleArray::New();

    // Fill this column
    vtkIdType n = statCol->GetNumberOfTuples();
    testCol->SetNumberOfTuples( n );
    for ( vtkIdType r = 0; r < n; ++ r )
      {
      testCol->SetTuple1( r, -1 );
      }

    // Now add the column of invalid values to the output table
    outMeta->AddColumn( testCol );

    // Clean up
    testCol->Delete();
    }

  // The test column name can only be set after the column has been obtained from R
  testCol->SetName( "P" );

  // Clean up
  nameColX->Delete();
  nameColY->Delete();
  bS1Col->Delete();
  bS2Col->Delete();
  statCol->Delete();
}

// ----------------------------------------------------------------------
class TableColumnPairMahlanobisFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkDataArray* DataX;
  vtkDataArray* DataY;
  double MeanX;
  double MeanY;
  double VarX;
  double VarY;
  double CovXY;
  double DInv;

  TableColumnPairMahlanobisFunctor( vtkDataArray* valsX,
                                    vtkDataArray* valsY,
                                    double meanX,
                                    double meanY,
                                    double varX,
                                    double varY,
                                    double covXY,
                                    double dInv )
  {
    this->DataX = valsX;
    this->DataY = valsY;
    this->MeanX = meanX;
    this->MeanY = meanY;
    this->VarX  = varX;
    this->VarY  = varY;
    this->CovXY = covXY;
    this->DInv  = dInv;
  }
  virtual ~TableColumnPairMahlanobisFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    double x = this->DataX->GetTuple1( id ) - this->MeanX;
    double y = this->DataY->GetTuple1( id ) - this->MeanY;

    result->SetNumberOfValues( 1 );
    result->SetValue( 0, ( this->VarY * x * x - 2. * this->CovXY * x * y + this->VarX * y * y ) * this->DInv );
  }
};

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::SelectAssessFunctor( vtkTable* outData,
                                                    vtkDataObject* inMetaDO,
                                                    vtkStringArray* rowNames,
                                                    AssessFunctor*& dfunc )
{
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
  vtkStringArray* varX = vtkStringArray::SafeDownCast( primaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkStringArray::SafeDownCast( primaryTab->GetColumnByName( "Variable Y" ) );
  if ( ! varX || ! varY )
    {
    dfunc = 0;
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
        dfunc = 0;
        return;
        }
      
      // For descriptive statistics, types must be convertible to DataArrays (e.g., StringArrays do not fit here).
      vtkDataArray* valsX = vtkDataArray::SafeDownCast( arrX );
      vtkDataArray* valsY = vtkDataArray::SafeDownCast( arrY );
      if ( ! valsX || ! valsY )
        {
        dfunc = 0;
        return;
        }

      double meanX = primaryTab->GetValueByName( r, this->AssessParameters->GetValue( 0 ) ).ToDouble();
      double meanY = primaryTab->GetValueByName( r, this->AssessParameters->GetValue( 1 ) ).ToDouble();
      double variX = derivedTab->GetValueByName( r, this->AssessParameters->GetValue( 2 ) ).ToDouble();
      double variY = derivedTab->GetValueByName( r, this->AssessParameters->GetValue( 3 ) ).ToDouble();
      double covXY = derivedTab->GetValueByName( r, this->AssessParameters->GetValue( 4 ) ).ToDouble();

      double d = variX * variY - covXY * covXY;
      if ( d <= 0. )
        {
        vtkWarningMacro( "Incorrect parameters for column pair:"
                         << " variance/covariance matrix has non-positive determinant"
                         << " (assessment values will be set to -1)." );
        
        dfunc = 0;
        }
      else
        {
        dfunc = new TableColumnPairMahlanobisFunctor( valsX,
                                                      valsY,
                                                      meanX,
                                                      meanY,
                                                      variX,
                                                      variY,
                                                      covXY,
                                                      1. / d );
        }
      
      return;
      }
    }

  // The pair of variables of interest was not found in the parameter table
  dfunc = 0;
}
