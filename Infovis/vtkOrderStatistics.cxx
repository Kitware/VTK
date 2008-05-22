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

vtkCxxRevisionMacro(vtkOrderStatistics, "1.3");
vtkStandardNewMacro(vtkOrderStatistics);

static const double quantileRatios[] = { 0., .1, .2, .25, .3, .4, .5, .6, .7, .75, .8, .9 };

// ----------------------------------------------------------------------
vtkOrderStatistics::vtkOrderStatistics()
{
  this->QuantileDefinition = vtkOrderStatistics::InverseCDFAveragedSteps;
}

// ----------------------------------------------------------------------
vtkOrderStatistics::~vtkOrderStatistics()
{
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "QuantileDefinition: " << this->QuantileDefinition << endl;
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

    vtkstd::map<double,vtkIdType> distr;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      ++ distr[dataset->GetValue( r, *it ).ToDouble()];
      }

    vtkVariantArray* row = vtkVariantArray::New();

    if ( finalize )
      {
      double quantileThresholds[12];
      for ( int i = 0; i < 12; ++ i )
        {
        quantileThresholds[i] = quantileRatios[i] * this->SampleSize;
        }

      double quantileValues[13];
      // quantileValues[ 0]: minimum
      // quantileValues[ 1]: 1st decile
      // quantileValues[ 2]: 2nd decile
      // quantileValues[ 3]: 1st quartile
      // quantileValues[ 4]: 3rd decile
      // quantileValues[ 5]: 4th decile
      // quantileValues[ 6]: median (= 2nd quartile = 5th decile)
      // quantileValues[ 7]: 6th decile
      // quantileValues[ 8]: 7th decile
      // quantileValues[ 9]: 3rd quartile
      // quantileValues[10]: 8th decile
      // quantileValues[11]: 9th decile
      // quantileValues[12]: maximum

      double sum = 0;
      int j = 0;
      for ( vtkstd::map<double,vtkIdType>::iterator mit = distr.begin();
            mit != distr.end(); ++ mit  )
        {
        for ( sum += mit->second; sum >= quantileThresholds[j] && j < 12; ++ j )
          {
          if ( sum == quantileThresholds[j] 
               && this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps )
            {
            vtkstd::map<double,vtkIdType>::iterator nit = mit;
            quantileValues[j] = ( (++nit)->first + mit->first ) * .5;
            }
          else
            {
            quantileValues[j] = mit->first;
            }
          }
        }
    
      quantileValues[12] = distr.rbegin()->first;

      row->SetNumberOfValues( 14 );

      row->SetValue(  0, *it );
      for ( int i = 0; i < 13; ++ i )
        {
        row->SetValue(  i + 1, quantileValues[i] );
        }
      }
    else
      {
      vtkWarningMacro( "Parallel implementation: not implemented yet." );
      return;
      }

    output->InsertNextRow( row );

    row->Delete();
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
