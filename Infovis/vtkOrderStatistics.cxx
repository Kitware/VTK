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

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtkstd/set>

vtkCxxRevisionMacro(vtkOrderStatistics, "1.34");
vtkStandardNewMacro(vtkOrderStatistics);

// ----------------------------------------------------------------------
vtkOrderStatistics::vtkOrderStatistics()
{
  this->QuantileDefinition = vtkOrderStatistics::InverseCDFAveragedSteps;
  this->NumberOfIntervals = 4; // By default, calculate 5-points statistics
  this->SetAssessmentName( "Quantile" );
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
void vtkOrderStatistics::ExecuteLearn( vtkTable* inData,
                                       vtkTable* output )
{
  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! this->Internals->SelectedColumns.size() )
    {
    return;
    }

  vtkIdType nCol = inData->GetNumberOfColumns();
  if ( ! nCol )
    {
    this->SampleSize = 0;
    return;
    }

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  output->AddColumn( stringCol );
  stringCol->Delete();

  if ( this->NumberOfIntervals < 1 )
    {
    return;
    }
  
  vtkVariantArray* variantCol;
  double dq = 1. / static_cast<double>( this->NumberOfIntervals );
  for ( int i = 0; i <= this->NumberOfIntervals; ++ i )
    {
    variantCol = vtkVariantArray::New();
    div_t q = div( i << 2, this->NumberOfIntervals );
    
    if ( q.rem )
      {
      variantCol->SetName( vtkStdString( vtkVariant( i * dq ).ToString() + "-quantile" ).c_str() );
      }
    else
      {
      switch ( q.quot )
        {
        case 0:
          variantCol->SetName( "Minimum" );
          break;
        case 1:
          variantCol->SetName( "First Quartile" );
          break;
        case 2:
          variantCol->SetName( "Median" );
          break;
        case 3:
          variantCol->SetName( "Third Quartile" );
          break;
        case 4:
          variantCol->SetName( "Maximum" );
          break;
        default:
          variantCol->SetName( vtkStdString( vtkVariant( i * dq ).ToString() + "-quantile" ).c_str() );
          break;
        }
      }
    output->AddColumn( variantCol );
    variantCol->Delete();
    }

  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->SelectedColumns.begin(); 
        it != this->Internals->SelectedColumns.end(); ++ it )
    {
    vtkStdString col = *it;
    if ( ! inData->GetColumnByName( col ) )
      {
      vtkWarningMacro( "InData table does not have a column "<<col.c_str()<<". Ignoring it." );
      continue;
      }

    bool isNum = inData->GetColumnByName( col )->IsNumeric();

    vtkstd::map<vtkVariant,vtkIdType> distr;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      ++ distr[inData->GetValueByName( r, col )];
      }

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues( this->NumberOfIntervals + 2 );
    
    int i = 0;
    row->SetValue( i ++, col );
    
    vtkstd::vector<double> quantileThresholds;
    double dh = this->SampleSize / static_cast<double>( this->NumberOfIntervals );
    for ( int j = 0; j < this->NumberOfIntervals; ++ j )
      {
      quantileThresholds.push_back( j * dh );
      }
    
    vtkIdType sum = 0;
    vtkstd::vector<double>::iterator qit = quantileThresholds.begin();
    for ( vtkstd::map<vtkVariant,vtkIdType>::iterator mit = distr.begin();
          mit != distr.end(); ++ mit  )
      {
      for ( sum += mit->second; qit != quantileThresholds.end() && sum >= *qit; ++ qit )
        {
        if ( isNum
             && sum == *qit
             && this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps )
          {
          vtkstd::map<vtkVariant,vtkIdType>::iterator nit = mit;
          row->SetValue( i ++, ( (++ nit)->first.ToDouble() + mit->first.ToDouble() ) * .5 );
          }
        else
          {
          row->SetValue( i ++, mit->first );
          }
        }
      }
    
    row->SetValue( i, distr.rbegin()->first );
    
    output->InsertNextRow( row );
    
    row->Delete();
    }

  return;
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::ExecuteDerive( vtkTable* vtkNotUsed( inMeta ) )
{
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

// ----------------------------------------------------------------------
class TableColumnBucketingFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkTable* Data;
  vtkVariantArray* Quantiles;

  TableColumnBucketingFunctor( vtkTable* inData, vtkVariantArray* quantiles )
  {
    this->Data = inData;
    this->Quantiles = quantiles;
  }
  virtual ~TableColumnBucketingFunctor() { }
  virtual vtkVariant operator() ( vtkIdType id )
  {
    vtkVariant x = this->Data->GetValue( id, 0 );
    if ( x < this->Quantiles->GetValue( 0 ) )
      {
      // x is smaller than lower bound
      return 0;
      }

    vtkIdType n = this->Quantiles->GetNumberOfValues() + 1;
    vtkIdType q = 1;
    while ( q < n && x > this->Quantiles->GetValue( q ) )
      {
      ++ q;
      }
    return q; // if x is greater than upper bound, then n + 1 is returned
  }
};

// ----------------------------------------------------------------------
void vtkOrderStatistics::SelectAssessFunctor( vtkTable* inData,
                                              vtkVariantArray* row,
                                              AssessFunctor*& dfunc )
{
  dfunc = new TableColumnBucketingFunctor( inData, row );
}
