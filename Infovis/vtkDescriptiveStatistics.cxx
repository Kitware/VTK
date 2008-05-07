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

vtkCxxRevisionMacro(vtkDescriptiveStatistics, "1.2");
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

  for ( vtkIdType c = 0; c < nCol; ++ c )
    {
    vtkDoubleArray* outputArr = vtkDoubleArray::New();
    outputArr->SetNumberOfComponents( 1 );
    outputArr->SetName( dataset->GetColumn( c )->GetName() );

    double minVal = dataset->GetValue( 0, c ).ToDouble();
    double maxVal = minVal;

    double val  = 0.;
    double val2 = 0.;
    double sum1 = 0.;
    double sum2 = 0.;
    double sum3 = 0.;
    double sum4 = 0.;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      val  = dataset->GetValue( r, c ).ToDouble();
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

    outputArr->InsertNextValue( minVal );
    outputArr->InsertNextValue( maxVal );

    if ( finalize )
      {
      double G2;
      this->CalculateFromSums( this->SampleSize, sum1, sum2, sum3, sum4, G2 );

      outputArr->InsertNextValue( sum1 );
      outputArr->InsertNextValue( sum2 );
      outputArr->InsertNextValue( sum3 );
      outputArr->InsertNextValue( sum4 );
      outputArr->InsertNextValue( G2 );
      }
    else
      {
      outputArr->InsertNextValue( sum1 );
      outputArr->InsertNextValue( sum2 );
      outputArr->InsertNextValue( sum3 );
      outputArr->InsertNextValue( sum4 );
      }

    output->AddColumn( outputArr );
    outputArr->Delete();
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
  idTypeCol->SetName( "Dataset Column" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Entry Index" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Relative Deviation" );
  output->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 3 );
  
  for ( vtkIdType c = 0; c < nCol; ++ c )
    {
    double nomVal = params->GetValue( 0, c ).ToDouble();
    double accDev = params->GetValue( 1, c ).ToDouble();
    double minVal = nomVal - accDev;
    double maxVal = nomVal + accDev;

    double val;
    for ( vtkIdType r = 0; r < nRow; ++ r )
      {
      val  = dataset->GetValue( r, c ).ToDouble();
      if ( val < minVal || val > maxVal )
        {
        row->SetValue( 0, vtkVariant( c ) );
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

