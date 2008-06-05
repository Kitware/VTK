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
#include "vtkUnivariateStatisticsAlgorithmPrivate.h"

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

vtkCxxRevisionMacro(vtkDescriptiveStatistics, "1.16");
vtkStandardNewMacro(vtkDescriptiveStatistics);

// ----------------------------------------------------------------------
vtkDescriptiveStatistics::vtkDescriptiveStatistics()
{
}

// ----------------------------------------------------------------------
vtkDescriptiveStatistics::~vtkDescriptiveStatistics()
{
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
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

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  output->AddColumn( stringCol );
  stringCol->Delete();

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

    vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName( "Cardinality" );
    output->AddColumn( idTypeCol );
    idTypeCol->Delete();
    }

  this->SetColumnSelection( dataset );
  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->SelectedColumns.begin(); 
        it != this->Internals->SelectedColumns.end(); ++ it )
    {
    vtkStdString col = *it;
    if ( ! dataset->GetColumnByName( col ) )
      {
      vtkWarningMacro( "Dataset table does not have a column "<<col.c_str()<<". Ignoring it." );
      continue;
      }

    double minVal = dataset->GetValueByName( 0, col ).ToDouble();
    double maxVal = minVal;

    double val  = 0.;
    double val2 = 0.;
    double sum1 = 0.;
    double sum2 = 0.;
    double sum3 = 0.;
    double sum4 = 0.;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      val  = dataset->GetValueByName( r, col ).ToDouble();
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
    row->SetNumberOfValues( 8 );
    row->SetValue( 0, col );
    row->SetValue( 1, minVal );
    row->SetValue( 2, maxVal );

    if ( finalize )
      {
      double G2;
      this->CalculateFromSums( this->SampleSize, sum1, sum2, sum3, sum4, G2 );

      row->SetValue( 3, sum1 );
      row->SetValue( 4, sum2 );
      row->SetValue( 5, sum3 );
      row->SetValue( 6, sum4 );
      row->SetValue( 7, G2 );
      }
    else
      {
      row->SetValue( 3, sum1 );
      row->SetValue( 4, sum2 );
      row->SetValue( 5, sum3 );
      row->SetValue( 6, sum4 );
      row->SetValue( 7, this->SampleSize );
      }

    output->InsertNextRow( row );

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
  vtkIdType nColD = dataset->GetNumberOfColumns();
  if ( ! nColD )
    {
    vtkWarningMacro( "Dataset table does not have any columns. Doing nothing." );
    return;
    }

  vtkIdType nRowD = dataset->GetNumberOfRows();
  if ( ! nRowD )
    {
    vtkWarningMacro( "Dataset table does not have any rows. Doing nothing." );
    return;
    }

  vtkIdType nColP = params->GetNumberOfColumns();
  if ( nColP != 3 )
    {
    vtkWarningMacro( "Parameter table has " 
                     << nColP
                     << " != 3 columns. Doing nothing." );
    return;
    }

  vtkIdType nRowP = params->GetNumberOfRows();
  if ( ! nRowP )
    {
    vtkWarningMacro( "Parameter table does not have any rows. Doing nothing." );
    return;
    }

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  output->AddColumn( stringCol );
  stringCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Row" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Relative Deviation" );
  output->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 3 );

  this->SetColumnSelection( dataset );
  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->SelectedColumns.begin(); 
        it != this->Internals->SelectedColumns.end(); ++ it )
    {
    vtkStdString col = *it;
    if ( ! dataset->GetColumnByName( col ) )
      {
      vtkWarningMacro( "Dataset table does not have a column "<<col.c_str()<<". Ignoring it." );
      continue;
      }
    
    bool unfound = true;
    for ( int i = 0; i < nRowP; ++ i )
      {
      if ( params->GetValue( i, 0 ).ToString() == col )
        {
        unfound = false;

        double center = params->GetValue( i, 1 ).ToDouble();
        double radius = params->GetValue( i, 2 ).ToDouble();
        double minimum = center - radius;
        double maximum = center + radius;

        double value;
        for ( vtkIdType r = 0; r < nRowD; ++ r )
          {
          value  = dataset->GetValueByName( r, col ).ToDouble();

          if ( value < minimum || value > maximum )
            {
            row->SetValue( 0, col );
            row->SetValue( 1, r );
            row->SetValue( 2, ( value - center ) / radius );

            output->InsertNextRow( row );
            }
          }

        break;
        }
      }

    if ( unfound )
      {
      vtkWarningMacro( "Parameter table does not have a row for dataset table column "
                       <<col.c_str()
                       <<". Ignoring it." );
      continue;
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
    s2 = 0.;
    s3 = 0.;
    s4 = 0.;
    G2 = 0.;
    return 1;
    }

  // (unbiased) estimation of the variance
  double nm1 = nd - 1.;
  double s1p2 = s1 * s1;
  double var = ( s2 - s1p2 * nd ) / nm1;

  if ( var > 0. )
    {
    // sample estimation of the kurtosis "excess"
    s4 = ( s4 / nd - 4. * s1 * s3 / nd + 6. * s1p2 * s2 / nd - 3. * s1p2 * s1p2 )
      / ( var * var ) - 3.;
    
    // sample estimation of the skewness
    s3 = ( s3 / nd - 3. * s1 * s2 / nd + 2. * s1p2 * s1 ) 
      / pow( var, 1.5 );

    s2 = var;
    }
  else
    {
    s2 = var;
    s3 = 0.;
    s4 = 0.;
    G2 = 0.;
    return 1;
    }

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

