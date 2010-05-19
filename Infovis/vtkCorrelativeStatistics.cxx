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
#include "vtkObjectFactory.h"
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
                                          vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO );
  if ( ! outMeta ) 
    { 
    return; 
    } 

  // Get hold of the first model (data object) in the collection
  vtkCollectionSimpleIterator it;
  inMetaColl->InitTraversal( it );
  vtkDataObject *inMetaDO = inMetaColl->GetNextDataObject( it );

  // Verify that the model is indeed contained in a table
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO );
  if ( ! inMeta ) 
    { 
    return; 
    }

  vtkIdType nRow = inMeta->GetNumberOfRows();
  if ( ! nRow )
    {
    // No statistics were calculated.
    return;
    }

  // use this first model to initialize the aggregated one
  outMeta->DeepCopy( inMeta );

  // Now, loop over all remaining models and update aggregated each time
  while ( ( inMetaDO = inMetaColl->GetNextDataObject( it ) ) )
    {
    // Verify that the model is indeed contained in a table
    inMeta = vtkTable::SafeDownCast( inMetaDO );
    if ( ! inMeta ) 
      { 
      return; 
      }
    
    if ( inMeta->GetNumberOfRows() != nRow )
      {
      // Models do not match
      return;
      }

    // Iterate over all model rows
    for ( int r = 0; r < nRow; ++ r )
      {
      // Verify that variable names match each other
      if ( inMeta->GetValueByName( r, "Variable" ) != outMeta->GetValueByName( r, "Variable" ) )
        {
        // Models do not match
        return;
        }

      // Get aggregated statistics
      int n = outMeta->GetValueByName( r, "Cardinality" ).ToInt();
      double meanX = outMeta->GetValueByName( r, "Mean X" ).ToDouble();
      double meanY = outMeta->GetValueByName( r, "Mean Y" ).ToDouble();
      double M2X = outMeta->GetValueByName( r, "M2 X" ).ToDouble();
      double M2Y = outMeta->GetValueByName( r, "M2 Y" ).ToDouble();
      double MXY = outMeta->GetValueByName( r, "M XY" ).ToDouble();
      
      // Get current model statistics
      int n_c = inMeta->GetValueByName( r, "Cardinality" ).ToInt();
      double meanX_c = inMeta->GetValueByName( r, "Mean X" ).ToDouble();
      double meanY_c = inMeta->GetValueByName( r, "Mean Y" ).ToDouble();
      double M2X_c = outMeta->GetValueByName( r, "M2 X" ).ToDouble();
      double M2Y_c = outMeta->GetValueByName( r, "M2 Y" ).ToDouble();
      double MXY_c = outMeta->GetValueByName( r, "M XY" ).ToDouble();
      
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
      outMeta->SetValueByName( r, "Cardinality", N );
      outMeta->SetValueByName( r, "Mean X", meanX );
      outMeta->SetValueByName( r, "Mean Y", meanY );
      outMeta->SetValueByName( r, "M2 X", M2X );
      outMeta->SetValueByName( r, "M2 Y", M2Y );
      outMeta->SetValueByName( r, "M XY", MXY );
      }
    }

  return;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::Learn( vtkTable* inData,
                                      vtkTable* vtkNotUsed( inParameters ),
                                      vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO ); 
  if ( ! outMeta ) 
    { 
    return; 
    } 

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  outMeta->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable X" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable Y" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Mean X" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();
  
  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Mean Y" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();
  
  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2 X" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2 Y" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M XY" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkIdType nRow = inData->GetNumberOfRows();
  if ( ! nRow )
    {
    return;
    }

  if ( ! inData->GetNumberOfColumns() )
    {
    return;
    }

  // Loop over requests
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

    outMeta->InsertNextRow( row );

    row->Delete();
    }
    
  return;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::Derive( vtkDataObject* inMetaDO )
{
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO ); 
  if ( ! inMeta ) 
    { 
    return; 
    } 

  vtkIdType nCol = inMeta->GetNumberOfColumns();
  if ( nCol < 8 )
    {
    return;
    }

  vtkIdType nRow = inMeta->GetNumberOfRows();
  if ( ! nRow )
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
  
  vtkDoubleArray* doubleCol;
  for ( int j = 0; j < numDoubles; ++ j )
    {
    if ( ! inMeta->GetColumnByName( doubleNames[j] ) )
      {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName( doubleNames[j] );
      doubleCol->SetNumberOfTuples( nRow );
      inMeta->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }

  if ( ! inMeta->GetColumnByName( "Linear Correlation" ) )
    {  
    vtkStringArray* stringCol = vtkStringArray::New();
    stringCol->SetName( "Linear Correlation" );
    stringCol->SetNumberOfTuples( nRow );
    inMeta->AddColumn( stringCol );
    stringCol->Delete();
    }

  double* derivedVals = new double[numDoubles]; // var x, var y, cov, slope y/x, int. y/x, slope x/y, int. x/y, r

  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();
    double m2X = inMeta->GetValueByName( i, "M2 X" ).ToDouble();
    double m2Y = inMeta->GetValueByName( i, "M2 Y" ).ToDouble();
    double mXY = inMeta->GetValueByName( i, "M XY" ).ToDouble();

    double varX, varY, covXY;
    int numSamples = inMeta->GetValueByName(i, "Cardinality" ).ToInt();
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
      double meanX = inMeta->GetValueByName( i, "Mean X" ).ToDouble();
      double meanY = inMeta->GetValueByName( i, "Mean Y" ).ToDouble();

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

    inMeta->SetValueByName( i, "Linear Correlation", status );
    for ( int j = 0; j < numDoubles; ++ j )
      {
      inMeta->SetValueByName( i, doubleNames[j], derivedVals[j] );
      }
    }

  delete [] derivedVals;
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
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO ); 
  if ( ! inMeta ) 
    { 
    return; 
    } 

  vtkStdString varNameX = rowNames->GetValue( 0 );
  vtkStdString varNameY = rowNames->GetValue( 1 );

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* varX = vtkStringArray::SafeDownCast( inMeta->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkStringArray::SafeDownCast( inMeta->GetColumnByName( "Variable Y" ) );
  if ( ! varX || ! varY )
    {
    dfunc = 0;
    return;
    }

  // Loop over parameters table until the requested variables are found 
  vtkIdType nRowP = inMeta->GetNumberOfRows();
  for ( int r = 0; r < nRowP; ++ r )
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

      double meanX = inMeta->GetValueByName( r, this->AssessParameters->GetValue( 0 ) ).ToDouble();
      double meanY = inMeta->GetValueByName( r, this->AssessParameters->GetValue( 1 ) ).ToDouble();
      double variX = inMeta->GetValueByName( r, this->AssessParameters->GetValue( 2 ) ).ToDouble();
      double variY = inMeta->GetValueByName( r, this->AssessParameters->GetValue( 3 ) ).ToDouble();
      double covXY = inMeta->GetValueByName( r, this->AssessParameters->GetValue( 4 ) ).ToDouble();

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
