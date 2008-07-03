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
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>

#include <vtksys/ios/sstream>

// = Start Private Implementation =======================================
class vtkCorrelativeStatisticsPrivate
{
public:
  vtkCorrelativeStatisticsPrivate();
  ~vtkCorrelativeStatisticsPrivate();

  vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> > ColumnPairs;
  vtkstd::set<vtkStdString> BufferedColumns;
};

vtkCorrelativeStatisticsPrivate::vtkCorrelativeStatisticsPrivate()
{
}

vtkCorrelativeStatisticsPrivate::~vtkCorrelativeStatisticsPrivate()
{
}
// = End Private Implementation =========================================

vtkCxxRevisionMacro(vtkCorrelativeStatistics, "1.20");
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
void vtkCorrelativeStatistics::ResetColumnPairs()
{
  this->Internals->ColumnPairs.clear();

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::AddColumnPair( const char* namColX, const char* namColY, bool reset )
{
  if ( reset )
    {
    this->Internals->ColumnPairs.clear();
    }

  vtkstd::pair<vtkStdString,vtkStdString> namPair( namColX, namColY );
  this->Internals->ColumnPairs.insert( namPair );

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::RemoveColumnPair( const char* namColX, const char* namColY )
{
  vtkstd::pair<vtkStdString,vtkStdString> namPair( namColX, namColY );
  this->Internals->ColumnPairs.erase( namPair );

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::SetColumnStatus( const char* namCol, int status )
{
  if( status )
    {
    this->Internals->BufferedColumns.insert( namCol );
    }
  else
    {
    this->Internals->BufferedColumns.erase( namCol );
    }
  
  this->Internals->ColumnPairs.clear();

  int i = 0;
  for ( vtkstd::set<vtkStdString>::iterator ait = this->Internals->BufferedColumns.begin(); 
        ait != this->Internals->BufferedColumns.end(); ++ ait, ++ i )
    {
    int j = 0;
    for ( vtkstd::set<vtkStdString>::iterator bit = this->Internals->BufferedColumns.begin(); 
          j < i ; ++ bit, ++ j )
      {
      vtkstd::pair<vtkStdString,vtkStdString> namPair( *bit, *ait );
      this->Internals->ColumnPairs.insert( namPair );
      }
    }

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteLearn( vtkTable* inData,
                                             vtkTable* outMeta,
                                             bool finalize )
{
  vtkIdType nCol = inData->GetNumberOfColumns();
  if ( ! nCol )
    {
    this->SampleSize = 0;
    return;
    }

  this->SampleSize = inData->GetNumberOfRows();
  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! this->Internals->ColumnPairs.size() )
    {
    return;
    }

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable X" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable Y" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  if ( finalize )
    {
    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean X" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean Y" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Var(X)" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Var(Y)" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Cov(X,Y)" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    stringCol = vtkStringArray::New();
    stringCol->SetName( "Linear Correlation" );
    outMeta->AddColumn( stringCol );
    stringCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Slope Y/X" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Intersect Y/X" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Slope X/Y" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Intersect X/Y" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Corr. Coeff." );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();
    }
  else
    {
    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum y" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x2" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum y2" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum xy" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName( "Cardinality" );
    outMeta->AddColumn( idTypeCol );
    idTypeCol->Delete();
    }

  for ( vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> >::iterator it = this->Internals->ColumnPairs.begin(); 
        it != this->Internals->ColumnPairs.end(); ++ it )
    {
    vtkStdString colX = it->first;
    if ( ! inData->GetColumnByName( colX ) )
      {
      vtkWarningMacro( "InData table does not have a column "<<colX.c_str()<<". Ignoring this pair." );
      continue;
      }

    vtkStdString colY = it->second;
    if ( ! inData->GetColumnByName( colY ) )
      {
      vtkWarningMacro( "InData table does not have a column "<<colY.c_str()<<". Ignoring this pair." );
      continue;
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
      x = inData->GetValueByName( r, colX ).ToDouble();
      y = inData->GetValueByName( r, colY ).ToDouble();
      
      sx  += x;
      sy  += y;
      sx2 += x * x;
      sy2 += y * y;
      sxy += x * y;
      }

    vtkVariantArray* row = vtkVariantArray::New();

    if ( finalize )
      {
      row->SetNumberOfValues( 13 );

      double correlations[5];
      int res = this->CalculateFromSums( this->SampleSize, sx, sy, sx2, sy2, sxy, correlations );
      
      row->SetValue( 0, colX );
      row->SetValue( 1, colY );
      row->SetValue( 2, sx );
      row->SetValue( 3, sy );
      row->SetValue( 4, sx2 );
      row->SetValue( 5, sy2 );
      row->SetValue( 6, sxy );
      row->SetValue( 7, ( res ? "invalid" : "valid" ) );
      for ( int i = 0; i < 5; ++ i )
        {
        row->SetValue( i + 8, correlations[i] );
        }
      }
    else 
      {
      row->SetNumberOfValues( 8 );

      row->SetValue( 0, colX );
      row->SetValue( 1, colY );
      row->SetValue( 2, sx );
      row->SetValue( 3, sy );
      row->SetValue( 4, sx2 );
      row->SetValue( 5, sy2 );
      row->SetValue( 6, sxy );
      row->SetValue( 7, this->SampleSize );
      }

    outMeta->InsertNextRow( row );

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
void vtkCorrelativeStatistics::ExecuteAssess( vtkTable* inData,
                                              vtkTable* inMeta,
                                              vtkTable* outData,
                                              vtkTable* outMeta )
{
  vtkIdType nColD = inData->GetNumberOfColumns();
  if ( ! nColD )
    {
    return;
    }

  vtkIdType nRowD = inData->GetNumberOfRows();
  if ( ! nRowD )
    {
    return;
    }

  vtkIdType nColP = inMeta->GetNumberOfColumns();
  if ( nColP < 7 )
    {
    vtkWarningMacro( "Parameter table has " 
                     << nColP
                     << " < 7 columns. Doing nothing." );
    return;
    }

  vtkIdType nRowP = inMeta->GetNumberOfRows();
  if ( ! nRowP )
    {
    return;
    }

  if ( ! this->Internals->ColumnPairs.size() )
    {
    return;
    }

  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 4 );
  
  for ( vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> >::iterator it = this->Internals->ColumnPairs.begin(); 
        it != this->Internals->ColumnPairs.end(); ++ it )
    {
    vtkStdString colX = it->first;
    if ( ! inData->GetColumnByName( colX ) )
      {
      vtkWarningMacro( "InData table does not have a column "<<colX.c_str()<<". Ignoring this pair." );
      continue;
      }

    vtkStdString colY = it->second;
    if ( ! inData->GetColumnByName( colY ) )
      {
      vtkWarningMacro( "InData table does not have a column "<<colY.c_str()<<". Ignoring this pair." );
      continue;
      }

    bool unfound = true;
    for ( int i = 0; i < nRowP; ++ i )
      {
      vtkStdString c1 = inMeta->GetValue( i, 0 ).ToString();
      vtkStdString c2 = inMeta->GetValue( i, 1 ).ToString();
      if ( ( c1 == it->first && c2 == it->second ) ||  ( c2 == it->first && c1 == it->second ) )
        {
        unfound = false;

        double varX = inMeta->GetValue( i, 4 ).ToDouble();
        double varY = inMeta->GetValue( i, 5 ).ToDouble();
        double covr = inMeta->GetValue( i, 6 ).ToDouble();
        
        double d = varX * varY - covr * covr;
        if ( d <= 0. )
          {
          vtkWarningMacro( "Incorrect parameters for column pair ("
                           <<c1.c_str()
                           <<", "
                           <<c2.c_str()
                           <<"): variance/covariance matrix has non-positive determinant." );
          continue;
          }
        
        double nominalX = inMeta->GetValue( i, 2 ).ToDouble();
        double nominalY = inMeta->GetValue( i, 3 ).ToDouble();
        double eFac = -.5 / d;
        covr *= 2.;
        
        if ( c2 == it->first )
          {
          vtkStdString tmp( colX );
          colX = colY;
          colY = tmp;
          }

        // Add a column to outData
        vtkDoubleArray* pXY = vtkDoubleArray::New();
        vtksys_ios::ostringstream pXYName;
        pXYName
          << "p(" << ( colX.size() ? colX.c_str() : "X" )
          << ","  << ( colY.size() ? colY.c_str() : "Y" ) << ")/p_max";
        pXY->SetName( pXYName.str().c_str() );
        pXY->SetNumberOfTuples( nRowD );
        outData->AddColumn( pXY );
        pXY->Delete();

        double x, y;
        for ( vtkIdType r = 0; r < nRowD; ++ r )
          {
          x = inData->GetValueByName( r, colX ).ToDouble() - nominalX;
          y = inData->GetValueByName( r, colY ).ToDouble() - nominalY;
          
          outData->SetValueByName( r, pXYName.str().c_str(), 
                                  exp( eFac * ( varY * x * x - covr * x * y + varX * y * y ) ) );
          }

        break;
        }
      }

    if ( unfound )
      {
      vtkWarningMacro( "Parameter table does not have a row for inData table column pair ("
                       << it->first.c_str()
                       << ", "
                       << it->second.c_str()
                       << "). Ignoring it." );
      continue;
      }
    }
  row->Delete();

  outMeta->ShallowCopy( inMeta );

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

  // linear regression

  if ( sx2 > 0. && sy2 > 0. )
    {
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

    // correlation coefficient
    double d = sx2 * sy2;
    if ( d > 0 )
      {
      correlations[4] = sxy / sqrt( d );
      return 0;
      }
    else
      {
      correlations[4] = 0.;
      return 1;
      }
    }
  else
    {
    correlations[0] = 0.;
    correlations[1] = 0.;
    correlations[2] = 0.;
    correlations[3] = 0.;
    correlations[4] = 0.;
    return 1;
    }
}
