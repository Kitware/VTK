/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContingencyStatistics.cxx

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

#include "vtkContingencyStatistics.h"
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

#include <vtkstd/map>

#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkContingencyStatistics, "1.20");
vtkStandardNewMacro(vtkContingencyStatistics);

// ----------------------------------------------------------------------
vtkContingencyStatistics::vtkContingencyStatistics()
{
}

// ----------------------------------------------------------------------
vtkContingencyStatistics::~vtkContingencyStatistics()
{
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
 }

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteLearn( vtkTable* inData,
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

  vtkVariantArray* variantCol = vtkVariantArray::New();
  variantCol->SetName( "x" );
  outMeta->AddColumn( variantCol );
  variantCol->Delete();

  variantCol = vtkVariantArray::New();
  variantCol->SetName( "y" );
  outMeta->AddColumn( variantCol );
  variantCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  outMeta->AddColumn( idTypeCol );
  idTypeCol->Delete();

  double n = static_cast<double>( this->SampleSize );
  vtkVariantArray* row = vtkVariantArray::New();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Probability" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();
  row->SetNumberOfValues( 6 );

  typedef vtkstd::map<vtkVariant,vtkIdType,vtkVariantLessThan> Distribution;

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

    row->SetValue( 0, colX );
    row->SetValue( 1, colY );

    vtkstd::map<vtkVariant,Distribution,vtkVariantLessThan> conTable;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      ++ conTable[inData->GetValueByName( r, colX )][inData->GetValueByName( r, colY )];
      }

    for ( vtkstd::map<vtkVariant,Distribution,vtkVariantLessThan>::iterator mit = conTable.begin(); 
          mit != conTable.end(); ++ mit )
      {
      row->SetValue( 2, mit->first );
      for ( Distribution::iterator dit = mit->second.begin(); dit != mit->second.end(); ++ dit )
        {
        row->SetValue( 3, dit->first );
        row->SetValue( 4, dit->second );
        row->SetValue( 5, dit->second / n );

        outMeta->InsertNextRow( row );
        }
      }
    }
  row->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteDerive( vtkTable* vtkNotUsed( inMeta ) )
{
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteAssess( vtkTable* inData,
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
  if ( nColP < 3 )
    {
    vtkWarningMacro( "Parameter table has " 
                     << nColP
                     << " < 3 columns. Doing nothing." );
    return;
    }

  vtkIdType nRowP = inMeta->GetNumberOfRows();
  if ( ! nRowP )
    {
    return;
    }

  if ( ! inMeta->GetColumnByName( "Probability" ) )
    {
    vtkWarningMacro( "InData table does not have a column Probability. Doing nothing." );
    return;
    }
  
  // Add outMeta data columns
  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable X" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable Y" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "H(X)" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "H(Y)" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "H(X,Y)" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "H(Y|X)" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "H(X|Y)" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 7 );

  typedef vtkstd::map<vtkVariant,double,vtkVariantLessThan> PDF;

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

    PDF pdfX, pdfY;
    vtkstd::map<vtkVariant,PDF,vtkVariantLessThan> pdfXY;
    vtkVariant x, y;
    double p;
    for ( vtkIdType r = 0; r < nRowP; ++ r )
      {
      vtkStdString c1 = inMeta->GetValueByName( r, "Variable X" ).ToString();
      vtkStdString c2 = inMeta->GetValueByName( r, "Variable Y" ).ToString();
      if ( c1 == it->first && c2 == it->second )
        {
        x = inMeta->GetValueByName( r, "x" );
        y = inMeta->GetValueByName( r, "y" );
        p = inMeta->GetValueByName( r, "Probability" ).ToDouble();
          
        pdfX[x] += p;
        pdfY[y] += p;
        pdfXY[x][y] = p;
        }
      }

    double HYcondX = 0.;
    double HXcondY = 0.;
    double HXY = 0.;
    double pxy;
    vtkstd::map<vtkVariant,PDF,vtkVariantLessThan> pdfYcondX;
    vtkstd::map<vtkVariant,PDF,vtkVariantLessThan> pdfXcondY;
    for ( vtkstd::map<vtkVariant,PDF,vtkVariantLessThan>::iterator xit = pdfXY.begin();
          xit != pdfXY.end(); ++ xit )
      {
      x = xit->first;
      for ( vtkstd::map<vtkVariant,double,vtkVariantLessThan>::iterator yit = xit->second.begin(); 
            yit != xit->second.end(); ++ yit )
        {
        y = yit->first;
        pxy = yit->second;
        pdfYcondX[x][y] = pxy / pdfX[x];
        pdfXcondY[x][y] = pxy / pdfY[y];
        
        HXY -= pxy * log( pxy );
        HYcondX -= pxy * log( pdfYcondX[x][y] );
        HXcondY -= pxy * log( pdfXcondY[x][y] );
        }
      }

    // Add outData data columns
    vtkDoubleArray* pXY = vtkDoubleArray::New();
    vtksys_ios::ostringstream pXYName;
    pXYName
      << "p(" << ( colX.size() ? colX.c_str() : "X" )
      << ", "  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
    pXY->SetName( pXYName.str().c_str() );
    pXY->SetNumberOfTuples( nRowD );
    outData->AddColumn( pXY );
    pXY->Delete();
    
    vtkDoubleArray* pYcondX = vtkDoubleArray::New();
    vtksys_ios::ostringstream pYCondXName;
    pYCondXName
      << "p(" << ( colY.size() ? colY.c_str() : "Y" )
      << " | "  << ( colX.size() ? colX.c_str() : "X" ) << ")";
    pYcondX->SetName( pYCondXName.str().c_str() );
    pYcondX->SetNumberOfTuples( nRowD );
    outData->AddColumn( pYcondX );
    pYcondX->Delete();
    
    vtkDoubleArray* pXcondY = vtkDoubleArray::New();
    vtksys_ios::ostringstream pXCondYName;
    pXCondYName
      << "p(" << ( colX.size() ? colX.c_str() : "X" )
      << " | "  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
    pXcondY->SetName( pXCondYName.str().c_str() );
    pXcondY->SetNumberOfTuples( nRowD );
    outData->AddColumn( pXcondY );
    pXcondY->Delete();

    for ( vtkIdType r = 0; r < nRowD; ++ r )
      {
      x = inData->GetValueByName( r, colX );
      y = inData->GetValueByName( r, colY );
      
      outData->SetValueByName( r, pXYName.str().c_str(), pdfXY[x][y] );
      outData->SetValueByName( r, pYCondXName.str().c_str(), pdfYcondX[x][y] );
      outData->SetValueByName( r, pXCondYName.str().c_str(), pdfXcondY[x][y] );
      }

    // Insert Meta values
    row->SetValue( 0, colX );
    row->SetValue( 1, colY );
    row->SetValue( 2, HXY - HYcondX );
    row->SetValue( 3, HXY - HXcondY );
    row->SetValue( 4, HXY );
    row->SetValue( 5, HYcondX );
    row->SetValue( 6, HXcondY );
    outMeta->InsertNextRow( row );
    }    

  row->Delete();

  return;
}
