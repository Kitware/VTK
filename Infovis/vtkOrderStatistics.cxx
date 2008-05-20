/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOrderStatistics.cxx

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

#include "vtkUnivariateStatisticsAlgorithmPrivate.h"
#include "vtkOrderStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/map>
#include <vtkstd/set>

vtkCxxRevisionMacro(vtkOrderStatistics, "1.1");
vtkStandardNewMacro(vtkOrderStatistics);

// ----------------------------------------------------------------------
vtkOrderStatistics::vtkOrderStatistics()
{
}

// ----------------------------------------------------------------------
vtkOrderStatistics::~vtkOrderStatistics()
{
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::ExecuteLearn( vtkTable* dataset,
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

  if ( finalize )
    {
    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Minimum" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();
    
    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D1" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D2" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Q1" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D3" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D4" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Median" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D6" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D7" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Q3" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D8" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "D9" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Maximum" );
    output->AddColumn( doubleCol ); 
    doubleCol->Delete();

    }
  else
    {
    vtkWarningMacro( "Parallel implementation: not implemented yet." );
    return;
    }

  for ( vtkstd::set<vtkIdType>::iterator it = this->Internals->Columns.begin(); 
        it != this->Internals->Columns.end(); ++ it )
    {
    if ( *it < 0 || *it >= nCol )
      {
      vtkWarningMacro( "Dataset table does not have a column with index "<<*it<<". Ignoring it." );
      continue;
      }

    double q[13] = {
      0.,
      .1,
      .2,
      .25,
      .3,
      .4,
      .5,
      .6,
      .7,
      .75,
      .8,
      .9,
      1. 
    };
    for ( int i = 0; i < 13; ++ i )
      {
      q[i] *= this->SampleSize;
      }

    double quantile[13];
    // quantile[ 0]: minimum
    // quantile[ 1]: 1st decile
    // quantile[ 2]: 2nd decile
    // quantile[ 3]: 1st quartile
    // quantile[ 4]: 3rd decile
    // quantile[ 5]: 4th decile
    // quantile[ 6]: median (= 2nd quartile = 5th decile)
    // quantile[ 7]: 6th decile
    // quantile[ 8]: 7th decile
    // quantile[ 9]: 3rd quartile
    // quantile[10]: 8th decile
    // quantile[11]: 9th decile
    // quantile[12]: maximum

    double val = 0.;
    vtkstd::map<double,vtkIdType> distr;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      val = dataset->GetValue( r, *it ).ToDouble();
      ++ distr[val];
      }
    vtkIdType nVals = distr.size(); 

    double* cumul = new double[nVals];
    double sum = 0.;
    vtkstd::map<double,vtkIdType>::iterator mit = distr.begin();
    double previous = mit->first;
    int j = 0;
    for ( int i = 0; mit != distr.end(); previous = mit->first, ++ mit, ++ i )
      {
      sum += mit->second;
      cumul[i] = sum;

      for ( ; sum > q[j]; ++ j )
        {
        if ( cumul[i - 1] == q[j] )
          {
          quantile[j] = .5 * ( previous + mit->first );
          }
        else
          {
          quantile[j] = mit->first;
          }
        if ( j == 13 )
          {
          // We really should never arrive here. If we do, then we have a problem.
          vtkErrorMacro( "An error occurred while calculating quantiles." );
          delete [] cumul;
          return;
          }
        }
      }

    quantile[12] = previous;

    vtkVariantArray* row = vtkVariantArray::New();
    row->SetNumberOfValues( 14 );

    if ( finalize )
      {

      row->SetValue(  0, *it );
      for ( int i = 0; i < 13; ++ i )
        {
        row->SetValue(  i + 1, quantile[i] );
        }
      }
    else
      {
      vtkWarningMacro( "Parallel implementation: not implemented yet." );
      return;
      }

    output->InsertNextRow( row );

    row->Delete();
    delete [] cumul;
    }

  return;
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::ExecuteValidate( vtkTable*,
                                          vtkTable*,
                                          vtkTable* )
{
  // Not implemented for this statistical engine
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::ExecuteEvince( vtkTable* dataset,
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
  
  for ( vtkstd::set<vtkIdType>::iterator it = this->Internals->Columns.begin(); 
        it != this->Internals->Columns.end(); ++ it )
    {
    if ( *it < 0 || *it >= nColD )
      {
      vtkWarningMacro( "Dataset table does not have a column with index "<<*it<<". Ignoring it." );
      continue;
      }
    
    bool unfound = true;
    for ( int i = 0; i < nRowP; ++ i )
      {
      vtkIdType c = params->GetValue( i, 0 ).ToInt();
      if ( c == *it )
        {
        unfound = false;

        double center = params->GetValue( i, 1 ).ToDouble();
        double radius = params->GetValue( i, 2 ).ToDouble();
        double minimum = center - radius;
        double maximum = center + radius;

        double value;
        for ( vtkIdType r = 0; r < nRowD; ++ r )
          {
          value  = dataset->GetValue( r, c ).ToDouble();
          if ( value < minimum || value > maximum )
            {
            row->SetValue( 0, c );
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
                       <<*it
                       <<". Ignoring it." );
      continue;
      }
    }
  row->Delete();

  return;
}
