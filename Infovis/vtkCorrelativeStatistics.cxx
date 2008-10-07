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

vtkCxxRevisionMacro(vtkCorrelativeStatistics, "1.35");
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
  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! inData->GetNumberOfColumns() )
    {
    this->SampleSize = 0;
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
  doubleCol->SetName( "M2 X" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2 Y" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M XY" );
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

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues( 7 );
    
    row->SetValue( 0, colX );
    row->SetValue( 1, colY );
    row->SetValue( 2, meanX );
    row->SetValue( 3, meanY );
    row->SetValue( 4, mom2X );
    row->SetValue( 5, mom2Y );
    row->SetValue( 6, momXY );

    outMeta->InsertNextRow( row );

    row->Delete();
    }
    
  return;
}

// ----------------------------------------------------------------------
void vtkCorrelativeStatistics::ExecuteDerive( vtkTable* inMeta )
{
  vtkIdType nCol = inMeta->GetNumberOfColumns();
  if ( nCol < 7 )
    {
    return;
    }

  vtkIdType nRow = inMeta->GetNumberOfRows();
  if ( ! nRow )
    {
    return;
    }

  int numDoubles = 5;
  vtkStdString doubleNames[] = { "Slope Y/X", 
                               "Intersect Y/X", 
                               "Slope X/Y", 
                               "Intersect X/Y", 
                               "Pearson r" };

  vtkDoubleArray* doubleCol;
  for ( int j = 0; j < numDoubles; ++ j )
    {
    if ( ! inMeta->GetColumnByName( doubleNames[j] ) )
      {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName( doubleNames[j] );
      doubleCol->SetNumberOfTuples( nRow );
      inMeta->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }

  if ( ! inMeta->GetColumnByName( "Linear Correlation" ) )
    {  
    vtkStringArray* stringCol = vtkStringArray::New();
    stringCol->SetName( "Linear Correlation" );
    stringCol->SetNumberOfTuples( nRow );
    inMeta->AddColumn( stringCol );
    stringCol->Delete();
    }

  double* doubleVals = new double[numDoubles]; // slope y/x, int. y/x, slope x/y, int. x/y, r

  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();
    double m2X = inMeta->GetValueByName( i, "M2 X" ).ToDouble();
    double m2Y = inMeta->GetValueByName( i, "M2 Y" ).ToDouble();
    double mXY = inMeta->GetValueByName( i, "M XY" ).ToDouble();

    double varX, varY, covXY;
    if ( this->SampleSize == 1 )
      {
      varX  = 0.;
      varY  = 0.;
      covXY = 0.;
      }
    else
      {
      double inv_nm1;
      inv_nm1 = 1. / ( this->SampleSize - 1. );
      varX  = m2X * inv_nm1;
      varY  = m2Y * inv_nm1;
      covXY = mXY * inv_nm1;
      }
    
    vtkStdString status = "valid";

    double d = varX * varY - covXY * covXY;
    if ( d <= 0. )
      {
      vtkWarningMacro( "Incorrect parameters for column pair ("
                       <<c1.c_str()
                       <<", "
                       <<c2.c_str()
                       <<"): variance/covariance matrix has non-positive determinant." );
      doubleVals[0] = 0.;
      doubleVals[1] = 0.;
      doubleVals[2] = 0.;
      doubleVals[3] = 0.;
      doubleVals[4] = 0.;
      status = "invalid";
      }
    else
      {
      double meanX = inMeta->GetValueByName( i, "Mean X" ).ToDouble();
      double meanY = inMeta->GetValueByName( i, "Mean Y" ).ToDouble();

      // variable Y on variable X:
      //   slope
      doubleVals[0] = covXY / varX;
      //   intersect
      doubleVals[1] = meanY - doubleVals[0] * meanX;
      
      //   variable X on variable Y:
      //   slope
      doubleVals[2] = covXY / varY;
      //   intersect
      doubleVals[3] = meanX - doubleVals[2] * meanY;
      
      // correlation coefficient
      doubleVals[4] = covXY / sqrt( varX * varY );
      }

    inMeta->SetValueByName( i, "Linear Correlation", status );
    for ( int j = 0; j < numDoubles; ++ j )
      {
      inMeta->SetValueByName( i, doubleNames[j], doubleVals[j] );
      }
    }

  delete [] doubleVals;
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
        double cov  = inMeta->GetValue( i, 6 ).ToDouble();
        
        double d = varX * varY - cov * cov;
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
        
        if ( c2 == it->first )
          {
          vtkStdString tmp( colX );
          colX = colY;
          colY = tmp;
          }

        // Add a column to outData
        vtkDoubleArray* dXY = vtkDoubleArray::New();
        vtksys_ios::ostringstream dXYName;
        dXYName
          << "d2_Mahalanobis(" << ( colX.size() ? colX.c_str() : "X" )
          << ","  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
        dXY->SetName( dXYName.str().c_str() );
        dXY->SetNumberOfTuples( nRowD );
        outData->AddColumn( dXY );
        dXY->Delete();

        double x, y;
        for ( vtkIdType r = 0; r < nRowD; ++ r )
          {
          x = inData->GetValueByName( r, colX ).ToDouble() - nominalX;
          y = inData->GetValueByName( r, colY ).ToDouble() - nominalY;
          
          outData->SetValueByName( r, dXYName.str().c_str(), ( varY * x * x - 2. * cov * x * y + varX * y * y ) / d );
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
