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

#include "vtkOrderStatistics.h"
#include "vtkUnivariateStatisticsAlgorithmPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtkstd/set>

vtkCxxRevisionMacro(vtkOrderStatistics, "1.22");
vtkStandardNewMacro(vtkOrderStatistics);

// ----------------------------------------------------------------------
vtkOrderStatistics::vtkOrderStatistics()
{
  this->QuantileDefinition = vtkOrderStatistics::InverseCDFAveragedSteps;
  this->NumberOfIntervals = 4; // By default, calculate 5-points statistics
}

// ----------------------------------------------------------------------
vtkOrderStatistics::~vtkOrderStatistics()
{
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "NumberOfIntervals: " << this->NumberOfIntervals << endl;
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
    this->SampleSize = 0;
    return;
    }

  this->SampleSize = dataset->GetNumberOfRows();
  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! this->Internals->SelectedColumns.size() )
    {
    return;
    }

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  output->AddColumn( stringCol );
  stringCol->Delete();

  if ( finalize )
    {
    if ( this->NumberOfIntervals < 1 )
      {
      return;
      }
    
    vtkDoubleArray* doubleCol;
    double dq = 1. / static_cast<double>( this->NumberOfIntervals );
    for ( int i = 0; i <= this->NumberOfIntervals; ++ i )
      {
      doubleCol = vtkDoubleArray::New();
      div_t q = div( i << 2, this->NumberOfIntervals );

      if ( q.rem )
        {
        doubleCol->SetName( vtkStdString( vtkVariant( i * dq ).ToString() + "-quantile" ).c_str() );
        }
      else
        {
        switch ( q.quot )
          {
          case 0:
            doubleCol->SetName( "Minimum" );
            break;
          case 1:
            doubleCol->SetName( "First Quartile" );
            break;
          case 2:
            doubleCol->SetName( "Median" );
            break;
          case 3:
            doubleCol->SetName( "Third Quartile" );
            break;
          case 4:
            doubleCol->SetName( "Maximum" );
            break;
          default:
            doubleCol->SetName( vtkStdString( vtkVariant( i * dq ).ToString() + "-quantile" ).c_str() );
            break;
          }
        }
      output->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }
  else
    {
    vtkWarningMacro( "Parallel implementation: not implemented yet." );
    return;
    }

  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->SelectedColumns.begin(); 
        it != this->Internals->SelectedColumns.end(); ++ it )
    {
    vtkStdString col = *it;
    if ( ! dataset->GetColumnByName( col ) )
      {
      vtkWarningMacro( "Dataset table does not have a column "<<col.c_str()<<". Ignoring it." );
      continue;
      }

    vtkstd::map<double,vtkIdType> distr;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      ++ distr[dataset->GetValueByName( r, col ).ToDouble()];
      }

    vtkVariantArray* row = vtkVariantArray::New();

    if ( finalize )
      {
      row->SetNumberOfValues( this->NumberOfIntervals + 2 );

      int i = 0;
      row->SetValue( i ++, col );

      vtkstd::vector<double> quantileThresholds;
      double dh = this->SampleSize / static_cast<double>( this->NumberOfIntervals );
      for ( int j = 0; j < this->NumberOfIntervals; ++ j )
        {
        quantileThresholds.push_back( j * dh );
        }

      double sum = 0;
      vtkstd::vector<double>::iterator qit = quantileThresholds.begin();
      for ( vtkstd::map<double,vtkIdType>::iterator mit = distr.begin();
            mit != distr.end(); ++ mit  )
        {
        for ( sum += mit->second; qit != quantileThresholds.end() && sum >= *qit; ++ qit )
          {
          if ( sum == *qit
               && this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps )
            {
            vtkstd::map<double,vtkIdType>::iterator nit = mit;
            row->SetValue( i ++, ( (++ nit)->first + mit->first ) * .5 );
            }
          else
            {
            row->SetValue( i ++, mit->first );
            }
          }
        }
    
      row->SetValue( i, distr.rbegin()->first );
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
    return;
    }

  vtkIdType nRowD = dataset->GetNumberOfRows();
  if ( ! nRowD )
    {
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
    return;
    }

  if ( ! this->Internals->SelectedColumns.size() )
    {
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
void vtkOrderStatistics::SetQuantileDefinition( int qd )
{
  switch ( qd )
    {
    case vtkOrderStatistics::InverseCDF:
      break;
    case vtkOrderStatistics::InverseCDFAveragedSteps:
      break;
    default:
      vtkWarningMacro( "Incorrect type of quantile definition: "
                       <<qd
                       <<". Ignoring it." );
      return;
    }
  
  this->QuantileDefinition =  static_cast<vtkOrderStatistics::QuantileDefinitionType>( qd );
  this->Modified();

  return;
}
