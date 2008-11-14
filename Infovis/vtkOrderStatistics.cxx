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

vtkCxxRevisionMacro(vtkOrderStatistics, "1.38");
vtkStandardNewMacro(vtkOrderStatistics);

// ----------------------------------------------------------------------
vtkOrderStatistics::vtkOrderStatistics()
{
  this->QuantileDefinition = vtkOrderStatistics::InverseCDFAveragedSteps;
  this->NumberOfIntervals = 4; // By default, calculate 5-points statistics

  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "Quantile" );
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
                                       vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO ); 
  if ( ! outMeta ) 
    { 
    return; 
    } 

  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! this->Internals->Selection.size() )
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
  outMeta->AddColumn( stringCol );
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
    outMeta->AddColumn( variantCol );
    variantCol->Delete();
    }

  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->Selection.begin(); 
        it != this->Internals->Selection.end(); ++ it )
    {
    vtkStdString col = *it;
    if ( ! inData->GetColumnByName( col ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << col.c_str()
                       << ". Ignoring it." );
      continue;
      }

    int isNum = inData->GetColumnByName( col )->IsNumeric();

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
    
    outMeta->InsertNextRow( row );
    
    row->Delete();
    }

  return;
}

// ----------------------------------------------------------------------
void vtkOrderStatistics::ExecuteDerive( vtkDataObject* vtkNotUsed( inMeta ) )
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
  vtkAbstractArray* Data;
  vtkVariantArray* Quantiles;

  TableColumnBucketingFunctor( vtkAbstractArray* vals,
                               vtkVariantArray* quantiles )
  {
    this->Data = vals;
    this->Quantiles = quantiles;
  }
  virtual ~TableColumnBucketingFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    vtkVariant x = this->Data->GetVariantValue( id );
    if ( x < this->Quantiles->GetValue( 1 ) ) // Value #0 is the variable name
      {
      // x is smaller than lower bound
      result->SetNumberOfValues( 1 );
      result->SetValue( 0, 0 );

      return;
      }

    vtkIdType n = this->Quantiles->GetNumberOfValues() + 2;
    vtkIdType q = 2;
    while ( q < n && x > this->Quantiles->GetValue( q ) )
      {
      ++ q;
      }

    result->SetNumberOfValues( 1 );
    result->SetValue( 0, q - 1 ); // -1 offset needed because value #0 in parameter row is the variable name
  }
};

// ----------------------------------------------------------------------
void vtkOrderStatistics::SelectAssessFunctor( vtkTable* inData,
                                              vtkDataObject* inMetaDO,
                                              vtkStringArray* rowNames,
                                              vtkStringArray* vtkNotUsed(columnNames),
                                              AssessFunctor*& dfunc )
{
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO ); 
  if ( ! inMeta ) 
    { 
    return; 
    } 

  vtkStdString varName = rowNames->GetValue( 0 );

  // Loop over parameters table until the requested variable is found
  vtkIdType nRowP = inMeta->GetNumberOfRows();
  for ( int i = 0; i < nRowP; ++ i )
    {
    if ( inMeta->GetValueByName( i, "Variable" ).ToString() == varName )
      {
      // Grab the data for the requested variable
      vtkAbstractArray* vals = inData->GetColumnByName( varName );
      if ( ! vals )
        {
        dfunc = 0;
        return;
        }

      dfunc = new TableColumnBucketingFunctor( vals, inMeta->GetRow( i ) );
      return;
      }
    }

  // The variable of interest was not found in the parameter table
  dfunc = 0;
}
