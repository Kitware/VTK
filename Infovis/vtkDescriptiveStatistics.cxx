/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDescriptiveStatistics.cxx

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

#include "vtkDescriptiveStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>

// = Start Private Implementation =======================================
class vtkDescriptiveStatisticsPrivate
{
public:
  vtkDescriptiveStatisticsPrivate();
  ~vtkDescriptiveStatisticsPrivate();

  vtkstd::set<vtkIdType> Columns;
};

vtkDescriptiveStatisticsPrivate::vtkDescriptiveStatisticsPrivate()
{
}

vtkDescriptiveStatisticsPrivate::~vtkDescriptiveStatisticsPrivate()
{
}

// = End Private Implementation =========================================

vtkCxxRevisionMacro(vtkDescriptiveStatistics, "1.4");
vtkStandardNewMacro(vtkDescriptiveStatistics);

// ----------------------------------------------------------------------
vtkDescriptiveStatistics::vtkDescriptiveStatistics()
{
  this->Internals = new vtkDescriptiveStatisticsPrivate;
}

// ----------------------------------------------------------------------
vtkDescriptiveStatistics::~vtkDescriptiveStatistics()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::ResetColumns()
{
  this->Internals->Columns.clear();
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::AddColumn( vtkIdType idxCol )
{
 this->Internals->Columns.insert( idxCol );
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::RemoveColumn( vtkIdType idxCol )
{
 this->Internals->Columns.erase( idxCol );
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::AddColumnRange( vtkIdType idxColBegin, vtkIdType idxColEnd )
{
  for ( int idxCol = idxColBegin; idxCol < idxColEnd; ++ idxCol )
    {
    this->Internals->Columns.insert( idxCol );
    }
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::RemoveColumnRange( vtkIdType idxColBegin, vtkIdType idxColEnd )
{
  for ( int idxCol = idxColBegin; idxCol < idxColEnd; ++ idxCol )
    {
    this->Internals->Columns.erase( idxCol );
    }
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::ExecuteLearn( vtkTable* dataset,
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
  idTypeCol->SetName( "Column" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Minimum" );
  output->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Maximum" );
  output->AddColumn( doubleCol );
  doubleCol->Delete();

  if ( finalize )
    {
    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Variance" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Skewness" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sample Kurtosis" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "G2 Kurtosis" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();
    }
  else
    {
    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x2" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x3" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x4" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();
    }

  for ( vtkstd::set<vtkIdType>::iterator it = this->Internals->Columns.begin(); it != this->Internals->Columns.end(); ++ it )
    {
    if ( *it < 0 || *it >= nCol )
      {
      vtkWarningMacro( "Dataset table does not have a column with index "<<*it<<". Ignoring it." );
      this->SampleSize = 0;
      return;
    }

    double minVal = dataset->GetValue( 0, *it ).ToDouble();
    double maxVal = minVal;

    double val  = 0.;
    double val2 = 0.;
    double sum1 = 0.;
    double sum2 = 0.;
    double sum3 = 0.;
    double sum4 = 0.;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      val  = dataset->GetValue( r, *it ).ToDouble();
      val2 = val * val;
      sum1 += val;
      sum2 += val2;
      sum3 += val2 * val;
      sum4 += val2 * val2;
      if ( val < minVal )
        {
        minVal = val;
        }
      else if ( val > maxVal )
        {
        maxVal = val;
        }
      }

    vtkVariantArray* row = vtkVariantArray::New();
    if ( finalize )
      {
      row->SetNumberOfValues( 8 );
  
      double G2;
      this->CalculateFromSums( this->SampleSize, sum1, sum2, sum3, sum4, G2 );

      row->SetValue( 0, *it );
      row->SetValue( 1, minVal );
      row->SetValue( 2, maxVal );
      row->SetValue( 3, sum1 );
      row->SetValue( 4, sum2 );
      row->SetValue( 5, sum3 );
      row->SetValue( 6, sum4 );
      row->SetValue( 7, G2 );

      output->InsertNextRow( row );
      }
    else
      {
      row->SetNumberOfValues( 7 );
  
      row->SetValue( 0, *it );
      row->SetValue( 1, minVal );
      row->SetValue( 2, maxVal );
      row->SetValue( 3, sum1 );
      row->SetValue( 4, sum2 );
      row->SetValue( 5, sum3 );
      row->SetValue( 6, sum4 );

      output->InsertNextRow( row );
      }

    row->Delete();
    }

  return;
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::ExecuteValidate( vtkTable*,
                                                vtkTable*,
                                                vtkTable* )
{
  // Not implemented for this statistical engine
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::ExecuteEvince( vtkTable* dataset,
                                              vtkTable* params,
                                              vtkTable* output)
{
  vtkIdType nCol = dataset->GetNumberOfColumns();
  if ( ! nCol )
    {
    vtkWarningMacro( "Dataset table does not have any columns. Doing nothing." );
    return;
    }

  if ( params->GetNumberOfColumns() != nCol )
    {
    vtkWarningMacro( "Dataset and parameter tables do not have the same number of columns. Doing nothing." );
    return;
    }

  vtkIdType nRow = dataset->GetNumberOfRows();
  if ( ! nRow )
    {
    vtkWarningMacro( "Dataset table does not have any rows. Doing nothing." );
    return;
    }

  if ( params->GetNumberOfRows() < 2 )
    {
    vtkWarningMacro( "Parameter table has " 
                     << params->GetNumberOfRows() 
                     << " != 2 rows. Doing nothing." );
    return;
    }

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Column" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Row" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Relative Deviation" );
  output->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 3 );
  
  for ( vtkstd::set<vtkIdType>::iterator it = this->Internals->Columns.begin(); it != this->Internals->Columns.end(); ++ it )
    {
    double nomVal = params->GetValue( 0, *it ).ToDouble();
    double accDev = params->GetValue( 1, *it ).ToDouble();
    double minVal = nomVal - accDev;
    double maxVal = nomVal + accDev;

    double val;
    for ( vtkIdType r = 0; r < nRow; ++ r )
      {
      val  = dataset->GetValue( r, *it ).ToDouble();
      if ( val < minVal || val > maxVal )
        {
        row->SetValue( 0, vtkVariant( *it ) );
        row->SetValue( 1, vtkVariant( r ) );
        row->SetValue( 2, vtkVariant( ( val - nomVal ) / accDev ) );

        output->InsertNextRow( row );
        }
      }
    }
  row->Delete();

  return;
}

// ----------------------------------------------------------------------
int vtkDescriptiveStatistics::CalculateFromSums( int n, 
                                                 double& s1,
                                                 double& s2,
                                                 double& s3,
                                                 double& s4,
                                                 double& G2 )
{
  if ( n < 1 ) 
    {
    return -1;
    }

  double nd = static_cast<double>( n );

  // (unbiased) estimation of the mean
  s1 /= nd;

  if ( n == 1 )
    {
    s2 = s3 = s4 = 0.;
    return 0;
    }

  // (unbiased) estimation of the variance
  double nm1 = nd - 1.;
  double s0p2 = s1 * s1;
  double var = ( s2 - s0p2 * nd ) / nm1;

  // sample estimation of the kurtosis "excess"
  s4 = ( s4 / nd - 4. * s1 * s3 / nd + 6. * s0p2 * s2 / nd - 3. * s0p2 * s0p2 )
    / ( var * var ) - 3.;

  // sample estimation of the skewness
  s3 = ( s3 / nd - 3. * s1 * s2 / nd + 2. * s0p2 * s1 ) 
    / pow( var, 1.5 );

  s2 = var;

  // G2 estimation of the kurtosis "excess"
  if ( n > 3 )
    {
    G2 = ( ( nd + 1. ) * s4 + 6. ) * nm1 / ( ( nd - 2. ) * ( nd - 3. ) );
    return 0;
    }
  else
    {
    G2 = s4;
    return 1;
    }
}

