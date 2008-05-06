/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStatisticsLinearCorrelation.cxx

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

#include "vtkStatisticsLinearCorrelation.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkStatisticsLinearCorrelation, "1.1");
vtkStandardNewMacro(vtkStatisticsLinearCorrelation);

// ----------------------------------------------------------------------
vtkStatisticsLinearCorrelation::vtkStatisticsLinearCorrelation()
{
  // Default indices of X and Y variables
  this->IdX = 0;
  this->IdY = 1;
}

// ----------------------------------------------------------------------
vtkStatisticsLinearCorrelation::~vtkStatisticsLinearCorrelation()
{
}

// ----------------------------------------------------------------------
void vtkStatisticsLinearCorrelation::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkStatisticsLinearCorrelation::ExecuteLearn( vtkTable* dataset,
                                                   vtkTable* output )
{
  vtkIdType nCol = dataset->GetNumberOfColumns();
  if ( ! nCol )
    {
    vtkWarningMacro( "Dataset table does not have any columns. Doing nothing." );
    this->SampleSize = 0;
    return;
    }

  vtkIdType ix = this->IdX;
  if ( ix >= nCol )
    {
    vtkWarningMacro( "Dataset table does not have a column with index "<<ix<<". Doing nothing." );
    this->SampleSize = 0;
    return;
    }

  vtkIdType iy = this->IdY;
  if ( iy >= nCol )
    {
    vtkWarningMacro( "Dataset table does not have a column with index "<<iy<<". Doing nothing." );
    this->SampleSize = 0;
    return;
    }

  this->SampleSize = dataset->GetNumberOfRows();
  if ( ! this->SampleSize )
    {
    vtkWarningMacro( "Dataset table does not have any rows. Doing nothing." );
    return;
    }

  vtkDoubleArray* outputArr = vtkDoubleArray::New();
  outputArr->SetNumberOfComponents( 1 );
  outputArr->SetName( "Raw moments" );

  double x   = 0.;
  double y   = 0.;
  double sx  = 0.;
  double sy  = 0.;
  double sx2 = 0.;
  double sy2 = 0.;
  double sxy = 0.;
  for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
    {
    x = dataset->GetValue( r, ix ).ToDouble();
    y = dataset->GetValue( r, iy ).ToDouble();

    sx  += x;
    sy  += y;
    sx2 += x * x;
    sy2 += y * y;
    sxy += x * y;
    }

  outputArr->InsertNextValue( sx );
  outputArr->InsertNextValue( sy );
  outputArr->InsertNextValue( sx2 );
  outputArr->InsertNextValue( sy2 );
  outputArr->InsertNextValue( sxy );
  
  output->AddColumn( outputArr );
  outputArr->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkStatisticsLinearCorrelation::ExecuteValidate( vtkTable*,
                                                      vtkTable*,
                                                      vtkTable* )
{
  // Not implemented for this statistical engine
}

// ----------------------------------------------------------------------
void vtkStatisticsLinearCorrelation::ExecuteEvince( vtkTable* dataset,
                                                    vtkTable* params,
                                                    vtkTable* output)
{
  vtkIdType nCol = dataset->GetNumberOfColumns();
  if ( ! nCol )
    {
    vtkWarningMacro( "Dataset table does not have any columns. Doing nothing." );
    return;
    }

  vtkIdType ix = this->IdX;
  if ( ix >= nCol )
    {
    vtkWarningMacro( "Dataset table does not have a column with index "<<ix<<". Doing nothing." );
    this->SampleSize = 0;
    return;
    }

  vtkIdType iy = this->IdY;
  if ( iy >= nCol )
    {
    vtkWarningMacro( "Dataset table does not have a column with index "<<iy<<". Doing nothing." );
    this->SampleSize = 0;
    return;
    }

  vtkIdType nRow = dataset->GetNumberOfRows();
  if ( ! nRow )
    {
    vtkWarningMacro( "Dataset table does not have any rows. Doing nothing." );
    return;
    }

  if ( params->GetNumberOfRows() < 6 )
    {
    vtkWarningMacro( "Parameter table has " 
                     << params->GetNumberOfRows() 
                     << " != 6 rows. Doing nothing." );
    return;
    }

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Entry Index" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Relative PDF" );
  output->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 2 );
  
  double accVarX = params->GetValue( 2, 0 ).ToDouble();
  double accVarY = params->GetValue( 3, 0 ).ToDouble();
  double accCovr = params->GetValue( 4, 0 ).ToDouble();

  double d = accVarX * accVarY - accCovr * accCovr;
  if ( d <= 0. )
    {
    vtkWarningMacro( "Cannot calculate statistics: variance/covariance matrix has non-positive determinant." );
    return;
    }

  double nomValX = params->GetValue( 0, 0 ).ToDouble();
  double nomValY = params->GetValue( 1, 0 ).ToDouble();
  double mRelPDF = params->GetValue( 5, 0 ).ToDouble();
  double eFac = -.5 / d;
  accCovr *= 2.;

  double x = 0.;
  double y = 0.;
  double rPDF;
  for ( vtkIdType r = 0; r < nRow; ++ r )
    {
    x = dataset->GetValue( r, ix ).ToDouble() - nomValX;
    y = dataset->GetValue( r, iy ).ToDouble() - nomValY;

    rPDF = exp( eFac * ( accVarY * x * x - accCovr * x * y + accVarX * y * y ) );
    if ( rPDF < mRelPDF )
      {
      row->SetValue( 0, vtkVariant( r ) );
      row->SetValue( 1, vtkVariant( rPDF ) );
      
      output->InsertNextRow( row );
      }
    }
  row->Delete();

  return;
}

// ----------------------------------------------------------------------
int vtkStatisticsLinearCorrelation::CalculateFromRawMoments( int n, double* s, double* r )
{
  if ( n < 2 ) 
    {
    return -1;
    }

  double nd = static_cast<double>( n );

  // (unbiased) estimation of the means
  s[0] /= nd;
  s[1] /= nd;

  if ( n == 1 )
    {
    s[2] = s[3] = s[4] = 0.;
    return 0;
    }

  // (unbiased) estimation of the variances and covariance
  double f = 1. / ( nd - 1. );
  s[2] = ( s[2] - s[0] * s[0] * nd ) * f;
  s[3] = ( s[3] - s[1] * s[1] * nd ) * f;
  s[4] = ( s[4] - s[0] * s[1] * nd ) * f;


  // linear regression coefficients

  // variable Y on variable X:
  //   slope
  r[0] = s[4] / s[2];
  //   intersect
  r[1] = s[1] - r[0] * s[0];

  //   variable X on variable Y:
  //   slope
  r[2] = s[4] / s[3];
  //   intersect
  r[3] = s[0] - r[2] * s[1];

  // linear correlation coefficient
  double d = s[2] * s[3];
  if ( d > 0 )
    {
    r[4] = s[4] / sqrt( d );
    return 0;
    }

  r[4] = 0.;
  return 1;
}
