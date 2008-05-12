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

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>

// = Start Private Implementation =======================================
class vtkCorrelativeStatisticsPrivate
{
public:
  vtkCorrelativeStatisticsPrivate();
  ~vtkCorrelativeStatisticsPrivate();

  vtkstd::set<vtkstd::pair<vtkIdType,vtkIdType> > ColumnPairs;
};

vtkCorrelativeStatisticsPrivate::vtkCorrelativeStatisticsPrivate()
{
}

vtkCorrelativeStatisticsPrivate::~vtkCorrelativeStatisticsPrivate()
{
}

// = End Private Implementation =========================================

vtkCxxRevisionMacro(vtkCorrelativeStatistics, "1.2");
vtkStandardNewMacro(vtkCorrelativeStatistics);

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::vtkCorrelativeStatistics()
{
  this->Internals = new vtkCorrelativeStatisticsPrivate;
}

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::~vtkCorrelativeStatistics()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ResetColumns()
{
  this->Internals->ColumnPairs.clear();
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::AddColumnPair( vtkIdType idxColX, vtkIdType idxColY )
{
  vtkstd::pair<vtkIdType,vtkIdType> idxPair( idxColX, idxColY );
  this->Internals->ColumnPairs.insert( idxPair );
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteLearn( vtkTable* dataset,
                                             vtkTable* output,
                                             bool finalize )
{
  vtkIdType nCol = dataset->GetNumberOfColumns();
  if ( ! nCol )
    {
    vtkWarningMacro( "Dataset table does not have any columns. Doing nothing." );
    this->SampleSize = 0;
    return;
    }

  this->SampleSize = dataset->GetNumberOfRows();
  if ( ! this->SampleSize )
    {
    vtkWarningMacro( "Dataset table does not have any rows. Doing nothing." );
    return;
    }

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Column X" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Column Y" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  if ( finalize )
    {
    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean X" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean Y" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Variance X" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Variance Y" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Covariance" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "SlopeYX" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Intersect YX" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Slope XY" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Intersect XY" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Correlation" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();
    }
  else
    {
    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum y" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x2" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum y2" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum xy" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName( "Cardinality" );
    output->AddColumn( idTypeCol );
    idTypeCol->Delete();
    }


  for ( vtkstd::set<vtkstd::pair<vtkIdType,vtkIdType> >::iterator it = this->Internals->ColumnPairs.begin(); 
        it != this->Internals->ColumnPairs.end(); ++ it )
    {
    vtkIdType idX = it->first;
    if ( idX >= nCol )
      {
      vtkWarningMacro( "Dataset table does not have a column with index "<<idX<<". Doing nothing." );
      this->SampleSize = 0;
      return;
      }

    vtkIdType idY = it->second;
    if ( idY >= nCol )
      {
      vtkWarningMacro( "Dataset table does not have a column with index "<<idY<<". Doing nothing." );
      this->SampleSize = 0;
      return;
      }
    
    double x   = 0.;
    double y   = 0.;
    double sx  = 0.;
    double sy  = 0.;
    double sx2 = 0.;
    double sy2 = 0.;
    double sxy = 0.;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      x = dataset->GetValue( r, idX ).ToDouble();
      y = dataset->GetValue( r, idY ).ToDouble();
      
      sx  += x;
      sy  += y;
      sx2 += x * x;
      sy2 += y * y;
      sxy += x * y;
      }

    vtkVariantArray* row = vtkVariantArray::New();

    if ( finalize )
      {
      row->SetNumberOfValues( 12 );

      double correlations[5];
      this->CalculateFromSums( this->SampleSize, sx, sy, sx2, sy2, sxy, correlations );
      
      row->SetValue( 0, idX );
      row->SetValue( 1, idY );
      row->SetValue( 2, sx );
      row->SetValue( 3, sy );
      row->SetValue( 4, sx2 );
      row->SetValue( 5, sy2 );
      row->SetValue( 6, sxy );
      for ( int i = 0; i < 5; ++ i )
        {
        row->SetValue( i + 7, correlations[i] );
        }
      }
    else 
      {
      row->SetNumberOfValues( 8 );

      row->SetValue( 0, idX );
      row->SetValue( 1, idY );
      row->SetValue( 2, sx );
      row->SetValue( 3, sy );
      row->SetValue( 4, sx2 );
      row->SetValue( 5, sy2 );
      row->SetValue( 6, sxy );
      row->SetValue( 7, this->SampleSize );
      }

    output->InsertNextRow( row );

    row->Delete();
    }
    
  return;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteValidate( vtkTable*,
                                                vtkTable*,
                                                vtkTable* )
{
  // Not implemented for this statistical engine
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteEvince( vtkTable* dataset,
                                              vtkTable* params,
                                              vtkTable* output)
{
  vtkIdType nCol = dataset->GetNumberOfColumns();
  if ( ! nCol )
    {
    vtkWarningMacro( "Dataset table does not have any columns. Doing nothing." );
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
  
  for ( vtkstd::set<vtkstd::pair<vtkIdType,vtkIdType> >::iterator it = this->Internals->ColumnPairs.begin(); 
        it != this->Internals->ColumnPairs.end(); ++ it )
    {
    vtkIdType idX = it->first;
    if ( idX >= nCol )
      {
      vtkWarningMacro( "Dataset table does not have a column with index "<<idX<<". Doing nothing." );
      this->SampleSize = 0;
      return;
      }

    vtkIdType idY = it->second;
    if ( idY >= nCol )
      {
      vtkWarningMacro( "Dataset table does not have a column with index "<<idY<<". Doing nothing." );
      this->SampleSize = 0;
      return;
      }

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
      x = dataset->GetValue( r, idX ).ToDouble() - nomValX;
      y = dataset->GetValue( r, idY ).ToDouble() - nomValY;

      rPDF = exp( eFac * ( accVarY * x * x - accCovr * x * y + accVarX * y * y ) );
      if ( rPDF < mRelPDF )
        {
        row->SetValue( 0, vtkVariant( r ) );
        row->SetValue( 1, vtkVariant( rPDF ) );
        
        output->InsertNextRow( row );
        }
      }
    }
  row->Delete();

  return;
}

// ----------------------------------------------------------------------
int vtkCorrelativeStatistics::CalculateFromSums( int n,
                                                 double& sx,
                                                 double& sy,
                                                 double& sx2,
                                                 double& sy2,
                                                 double& sxy,
                                                 double* correlations )
{
  if ( n < 2 ) 
    {
    return -1;
    }

  double nd = static_cast<double>( n );

  // (unbiased) estimation of the means
  sx /= nd;
  sy /= nd;

  if ( n == 1 )
    {
    sx2 = sy2 = sxy = 0.;
    return 0;
    }

  // (unbiased) estimation of the variances and covariance
  double f = 1. / ( nd - 1. );
  sx2 = ( sx2 - sx * sx * nd ) * f;
  sy2 = ( sy2 - sy * sy * nd ) * f;
  sxy = ( sxy - sx * sy * nd ) * f;


  // linear regression coefficients

  // variable Y on variable X:
  //   slope
  correlations[0] = sxy / sx2;
  //   intersect
  correlations[1] = sy - correlations[0] * sx;

  //   variable X on variable Y:
  //   slope
  correlations[2] = sxy / sy2;
  //   intersect
  correlations[3] = sx - correlations[2] * sy;

  // linear correlation coefficient
  double d = sx2 * sy2;
  if ( d > 0 )
    {
    correlations[4] = sxy / sqrt( d );
    return 0;
    }

  correlations[4] = 0.;
  return 1;
}
