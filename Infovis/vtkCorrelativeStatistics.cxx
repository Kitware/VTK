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
#include "vtkBivariateStatisticsAlgorithmPrivate.h"

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

vtkCxxRevisionMacro(vtkCorrelativeStatistics, "1.26");
vtkStandardNewMacro(vtkCorrelativeStatistics);

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::vtkCorrelativeStatistics()
{
}

// ----------------------------------------------------------------------
vtkCorrelativeStatistics::~vtkCorrelativeStatistics()
{
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteLearn( vtkTable* inData,
                                             vtkTable* outMeta )
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
    
    double meanX = 0.;
    double meanY = 0.;
    double mom2X = 0.;
    double mom2Y = 0.;
    double momXY = 0.;

    double inv_n, x, y, delta, deltaXn;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      inv_n = 1. / ( r + 1. );

      x = inData->GetValueByName( r, colX ).ToDouble();
      delta = x - meanX;
      meanX += delta * inv_n;
      deltaXn = x - meanX;
      mom2X += delta * deltaXn;

      y = inData->GetValueByName( r, colY ).ToDouble();
      delta = y - meanY;
      meanY += delta * inv_n;
      mom2Y += delta * ( y - meanY );

      momXY += delta * deltaXn;
      }

    double inv_nm1, varX, varY, covXY;
    if ( this->SampleSize == 1 )
      {
      varX  = 0.;
      varY  = 0.;
      covXY = 0.;
      }
    else
      {
      inv_nm1 = 1. / ( this->SampleSize - 1. );
      varX  = mom2X * inv_nm1;
      varY  = mom2Y * inv_nm1;
      covXY = momXY * inv_nm1;
      }

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues( 13 );
    
    row->SetValue( 0, colX );
    row->SetValue( 1, colY );
    row->SetValue( 2, meanX );
    row->SetValue( 3, meanY );
    row->SetValue( 4, varX );
    row->SetValue( 5, varY );
    row->SetValue( 6, covXY );

    double correlations[5];
    bool validity = true;
    if ( varX > 0. && varY > 0. )
      {
      // variable Y on variable X:
      //   slope
      correlations[0] = covXY / varX;
      //   intersect
      correlations[1] = meanY - correlations[0] * meanX;

      //   variable X on variable Y:
      //   slope
      correlations[2] = covXY / varY;
      //   intersect
      correlations[3] = meanX - correlations[2] * meanY;
      
      // correlation coefficient
      correlations[4] = covXY / sqrt( varX * varY );
      }
    else
      {
      correlations[0] = 0.;
      correlations[1] = 0.;
      correlations[2] = 0.;
      correlations[3] = 0.;
      correlations[4] = 0.;
      validity = false;
      }

    row->SetValue( 7, ( validity ? "valid" : "invalid" ) );
    for ( int i = 0; i < 5; ++ i )
      {
      row->SetValue( i + 8, correlations[i] );
      }

    outMeta->InsertNextRow( row );

    row->Delete();
    }
    
  return;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteAssess( vtkTable* inData,
                                              vtkTable* inMeta,
                                              vtkTable* outData,
                                              vtkTable* vtkNotUsed( outMeta ) )
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

  return;
}
