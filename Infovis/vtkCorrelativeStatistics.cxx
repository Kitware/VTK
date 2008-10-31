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
#include "vtkBivariateStatisticsAlgorithmPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>

#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkCorrelativeStatistics, "1.41");
vtkStandardNewMacro(vtkCorrelativeStatistics);

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::vtkCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "Squared Mahalanobis" );

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
  this->AssessParameters->Delete();
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteLearn( vtkTable* inData,
                                             vtkTable* outMeta )
{
  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! inData->GetNumberOfColumns() )
    {
    this->SampleSize = 0;
    return;
    }

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

  for ( vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> >::iterator it = this->Internals->Selection.begin(); 
        it != this->Internals->Selection.end(); ++ it )
    {
    vtkStdString colX = it->first;
    if ( ! inData->GetColumnByName( colX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << colX.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    vtkStdString colY = it->second;
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
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
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

    row->SetNumberOfValues( 7 );
    
    row->SetValue( 0, colX );
    row->SetValue( 1, colY );
    row->SetValue( 2, meanX );
    row->SetValue( 3, meanY );
    row->SetValue( 4, mom2X );
    row->SetValue( 5, mom2Y );
    row->SetValue( 6, momXY );

    outMeta->InsertNextRow( row );

    row->Delete();
    }
    
  return;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteDerive( vtkTable* inMeta )
{
  vtkIdType nCol = inMeta->GetNumberOfColumns();
  if ( nCol < 7 )
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

  double* doubleVals = new double[numDoubles]; // var x, var y, cov, slope y/x, int. y/x, slope x/y, int. x/y, r

  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();
    double m2X = inMeta->GetValueByName( i, "M2 X" ).ToDouble();
    double m2Y = inMeta->GetValueByName( i, "M2 Y" ).ToDouble();
    double mXY = inMeta->GetValueByName( i, "M XY" ).ToDouble();

    double varX, varY, covXY;
    if ( this->SampleSize == 1 )
      {
      varX  = 0.;
      varY  = 0.;
      covXY = 0.;
      }
    else
      {
      double inv_nm1;
      inv_nm1 = 1. / ( this->SampleSize - 1. );
      varX  = m2X * inv_nm1;
      varY  = m2Y * inv_nm1;
      covXY = mXY * inv_nm1;
      }
    
    doubleVals[0] = varX;
    doubleVals[1] = varY;
    doubleVals[2] = covXY;

    vtkStdString status = "valid";

    double d = varX * varY - covXY * covXY;
    if ( d <= 0. )
      {
      vtkWarningMacro( "Incorrect parameters for column pair ("
                       <<c1.c_str()
                       <<", "
                       <<c2.c_str()
                       <<"): variance/covariance matrix has non-positive determinant." );
      doubleVals[3] = 0.;
      doubleVals[4] = 0.;
      doubleVals[5] = 0.;
      doubleVals[6] = 0.;
      doubleVals[7] = 0.;
      status = "invalid";
      }
    else
      {
      double meanX = inMeta->GetValueByName( i, "Mean X" ).ToDouble();
      double meanY = inMeta->GetValueByName( i, "Mean Y" ).ToDouble();

      // variable Y on variable X:
      //   slope
      doubleVals[3] = covXY / varX;
      //   intersect
      doubleVals[4] = meanY - doubleVals[0] * meanX;
      
      //   variable X on variable Y:
      //   slope
      doubleVals[5] = covXY / varY;
      //   intersect
      doubleVals[6] = meanX - doubleVals[2] * meanY;
      
      // correlation coefficient
      doubleVals[7] = covXY / sqrt( varX * varY );
      }

    inMeta->SetValueByName( i, "Linear Correlation", status );
    for ( int j = 0; j < numDoubles; ++ j )
      {
      inMeta->SetValueByName( i, doubleNames[j], doubleVals[j] );
      }
    }

  delete [] doubleVals;
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
void vtkCorrelativeStatistics::SelectAssessFunctor( vtkTable* inData,
                                                    vtkTable* inMeta,
                                                    vtkStringArray* rowNames,
                                                    vtkStringArray* columnNames,
                                                    AssessFunctor*& dfunc )
{
  vtkStdString varNameX = rowNames->GetValue( 0 );
  vtkStdString varNameY = rowNames->GetValue( 1 );

  vtkIdType nRowP = inMeta->GetNumberOfRows();
  for ( int i = 0; i < nRowP; ++ i )
    {
    if ( inMeta->GetValueByName( i, "Variable X" ).ToString() == varNameX
         && inMeta->GetValueByName( i, "Variable Y" ).ToString() == varNameY )
      { 
      // Grab the data for the requested variables
      vtkAbstractArray* arrX = inData->GetColumnByName( varNameX );
      vtkAbstractArray* arrY = inData->GetColumnByName( varNameY );
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

      double meanX = inMeta->GetValueByName( i, columnNames->GetValue( 0 ) ).ToDouble();
      double meanY = inMeta->GetValueByName( i, columnNames->GetValue( 1 ) ).ToDouble();
      double varX  = inMeta->GetValueByName( i, columnNames->GetValue( 2 ) ).ToDouble();
      double varY  = inMeta->GetValueByName( i, columnNames->GetValue( 3 ) ).ToDouble();
      double covXY = inMeta->GetValueByName( i, columnNames->GetValue( 4 ) ).ToDouble();

      double d = varX * varY - covXY * covXY;
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
                                                      varX,
                                                      varY,
                                                      covXY,
                                                      1. / d );
        }
      
      return;
      }
    }

  // The pair of variables of interest was not found in the parameter table
  dfunc = 0;
}
