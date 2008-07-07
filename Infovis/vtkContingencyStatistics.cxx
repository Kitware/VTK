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

vtkCxxRevisionMacro(vtkContingencyStatistics, "1.13");
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
                                             vtkTable* outMeta,
                                             bool finalize )
{
  if ( ! inData->GetNumberOfColumns() )
    {
    this->SampleSize = 0;
    return;
    }

  this->SampleSize = inData->GetNumberOfRows();
  if ( ! this->SampleSize )
    {
    return;
    }

  vtkStdString colX = this->Internals->ColumnPairs.begin()->first;
  if ( ! inData->GetColumnByName( colX ) )
    {
    vtkWarningMacro( "InData table does not have a column "<<colX.c_str()<<". Doing nothing." );
    return;
    }

  vtkStdString colY = this->Internals->ColumnPairs.begin()->second;
  if ( ! inData->GetColumnByName( colY ) )
    {
    vtkWarningMacro( "InData table does not have a column "<<colY.c_str()<<". Doing nothing." );
    return;
    }
    
  vtkVariantArray* variantCol = vtkVariantArray::New();
  variantCol->SetName( colX );
  outMeta->AddColumn( variantCol );
  variantCol->Delete();

  variantCol = vtkVariantArray::New();
  variantCol->SetName( colY );
  outMeta->AddColumn( variantCol );
  variantCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  outMeta->AddColumn( idTypeCol );
  idTypeCol->Delete();

  if ( finalize )
    {
    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Probability" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();
    }

  typedef vtkstd::map<vtkVariant,vtkIdType,vtkVariantLessThan> Distribution;
  vtkstd::map<vtkVariant,Distribution,vtkVariantLessThan> conTable;
  for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
    {
    ++ conTable[inData->GetValueByName( r, colX )][inData->GetValueByName( r, colY )];
    }

  vtkVariantArray* row = vtkVariantArray::New();
  double n = static_cast<double>( this->SampleSize );
  if ( finalize )
    {
    row->SetNumberOfValues( 4 );

    for ( vtkstd::map<vtkVariant,Distribution,vtkVariantLessThan>::iterator it = conTable.begin(); 
          it != conTable.end(); ++ it )
      {
      row->SetValue( 0, it->first );
      for ( Distribution::iterator dit = it->second.begin(); dit != it->second.end(); ++ dit )
        {
        row->SetValue( 1, dit->first );
        row->SetValue( 2, dit->second );
        row->SetValue( 3, dit->second / n );

        outMeta->InsertNextRow( row );
        }
      }
    }
  else 
    {
    row->SetNumberOfValues( 3 );

    for ( vtkstd::map<vtkVariant,Distribution,vtkVariantLessThan>::iterator it = conTable.begin(); 
          it != conTable.end(); ++ it )
      {
      row->SetValue( 0, it->first );
      for ( Distribution::iterator dit = it->second.begin(); dit != it->second.end(); ++ dit )
        {
        row->SetValue( 1, dit->first );
        row->SetValue( 2, dit->second );

        outMeta->InsertNextRow( row );
        }
      }
    }

  row->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteValidate( vtkTable*,
                                                vtkTable*,
                                                vtkTable* )
{
  // Not implemented for this statistical engine
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

  vtkStdString colX = this->Internals->ColumnPairs.begin()->first;
  if ( ! inData->GetColumnByName( colX ) )
    {
    vtkWarningMacro( "InData table does not have a column "<<colX.c_str()<<". Doing nothing." );
    return;
    }
  
  vtkStdString colY = this->Internals->ColumnPairs.begin()->second;
  if ( ! inData->GetColumnByName( colY ) )
    {
    vtkWarningMacro( "InData table does not have a column "<<colY.c_str()<<". Doing nothing." );
    return;
    }
  
  if ( ! inMeta->GetColumnByName( "Probability" ) )
    {
    vtkWarningMacro( "InData table does not have a column Probability. Doing nothing." );
    return;
    }
  
  typedef vtkstd::map<vtkVariant,double,vtkVariantLessThan> PDF;
  PDF pdfX, pdfY;
  vtkstd::map<vtkVariant,PDF,vtkVariantLessThan> pdfXY;
  vtkVariant x, y;
  double p;
  for ( vtkIdType r = 0; r < nRowP; ++ r )
    {
    x = inMeta->GetValueByName( r, colX );
    y = inMeta->GetValueByName( r, colY );
    p = inMeta->GetValueByName( r, "Probability" ).ToDouble();

    pdfX[x] += p;
    pdfY[y] += p;
    pdfXY[x][y] = p;
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
    << ","  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
  pXY->SetName( pXYName.str().c_str() );
  pXY->SetNumberOfTuples( nRowD );
  outData->AddColumn( pXY );
  pXY->Delete();

  vtkDoubleArray* pYcondX = vtkDoubleArray::New();
  vtksys_ios::ostringstream pYCondXName;
  pYCondXName
    << "p(" << ( colY.size() ? colY.c_str() : "Y" )
    << "|"  << ( colX.size() ? colX.c_str() : "X" ) << ")";
  pYcondX->SetName( pYCondXName.str().c_str() );
  pYcondX->SetNumberOfTuples( nRowD );
  outData->AddColumn( pYcondX );
  pYcondX->Delete();

  vtkDoubleArray* pXcondY = vtkDoubleArray::New();
  vtksys_ios::ostringstream pXCondYName;
  pXCondYName
    << "p(" << ( colX.size() ? colX.c_str() : "X" )
    << "|"  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
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

  // Add outMeta data columns
  vtkDoubleArray* hXY = vtkDoubleArray::New();
  vtksys_ios::ostringstream hXYName;
  hXYName
    << "H(" << ( colX.size() ? colX.c_str() : "X" )
    << ","  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
  hXY->SetName( hXYName.str().c_str() );
  outMeta->AddColumn( hXY );
  hXY->Delete();

  vtkDoubleArray* hX = vtkDoubleArray::New();
  vtksys_ios::ostringstream hXName;
  hXName
    << "H(" << ( colX.size() ? colX.c_str() : "X" ) << ")";
  hX->SetName( hXName.str().c_str() );
  outMeta->AddColumn( hX );
  hX->Delete();

  vtkDoubleArray* hY = vtkDoubleArray::New();
  vtksys_ios::ostringstream hYName;
  hYName
    << "H(" << ( colY.size() ? colY.c_str() : "Y" ) << ")";
  hY->SetName( hYName.str().c_str() );
  outMeta->AddColumn( hY );
  hY->Delete();

  // Insert Meta values
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 3 );
  row->SetValue( 0, HXY );
  row->SetValue( 1, HXY - HYcondX );
  row->SetValue( 2, HXY - HXcondY );
  outMeta->InsertNextRow( row );

  row->Delete();

  return;
}
