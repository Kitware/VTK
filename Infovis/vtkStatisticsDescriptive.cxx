/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStatisticsDescriptive.cxx

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

#include "vtkStatisticsDescriptive.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkStatisticsDescriptive, "");
vtkStandardNewMacro(vtkStatisticsDescriptive);

// ----------------------------------------------------------------------
vtkStatisticsDescriptive::vtkStatisticsDescriptive()
{
}

// ----------------------------------------------------------------------
vtkStatisticsDescriptive::~vtkStatisticsDescriptive()
{
}

// ----------------------------------------------------------------------
void vtkStatisticsDescriptive::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkStatisticsDescriptive::ExecuteLearn( vtkTable* dataset,
                                             vtkTable* output )
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
    outputArr->InsertNextValue( sum1 );
    outputArr->InsertNextValue( sum2 );
    outputArr->InsertNextValue( sum3 );
    outputArr->InsertNextValue( sum4 );

    output->AddColumn( outputArr );
    outputArr->Delete();
    }

  return;
}

// ----------------------------------------------------------------------
void vtkStatisticsDescriptive::ExecuteValidate( vtkTable*,
                                                vtkTable*,
                                                vtkTable* )
{
  // Not implemented for this statistical engine
}

// ----------------------------------------------------------------------
void vtkStatisticsDescriptive::ExecuteEvince( vtkTable* dataset,
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
    vtkWarningMacro( "Parameter table does has " 
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
int vtkStatisticsDescriptive::CalculateCenteredMoments( int n, double* s )
{
  if ( n < 1 ) 
    {
    return -1;
    }

  double nd = static_cast<double>( n );

  // (unbiased) estimation of the mean
  s[0] /= nd;

  if ( n == 1 )
    {
    s[1] = s[2] = s[3] = 0.;
    return 0;
    }

  // (unbiased) estimation of the variance
  double nm1 = nd - 1.;
  double s0p2 = s[0] * s[0];
  double var = ( s[1] - s0p2 * nd ) / nm1;

  // sample estimation of the kurtosis "excess"
  s[3] = ( s[3] / nd - 4. * s[0] * s[2] / nd + 6. * s0p2 * s[1] / nd - 3. * s0p2 * s0p2 )
    / ( var * var ) - 3.;

  // sample estimation of the skewness
  s[2] = ( s[2] / nd - 3. * s[0] * s[1] / nd + 2. * s0p2 * s[0] ) 
    / pow( var, 1.5 );

  s[1] = var;

  // s[4] estimation of the kurtosis "excess"
  if ( n > 3 )
    {
    s[4] = ( ( nd + 1. ) * s[3] + 6. ) * nm1 / ( ( nd - 2. ) * ( nd - 3. ) );
    return 0;
    }
  else
    {
    s[4] = s[3];
    return 1;
    }
}

