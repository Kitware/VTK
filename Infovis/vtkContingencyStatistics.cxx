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
#include <vtkstd/set>

#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkContingencyStatistics, "1.6");
vtkStandardNewMacro(vtkContingencyStatistics);

// ----------------------------------------------------------------------
vtkContingencyStatistics::vtkContingencyStatistics()
{
  this->X = 0;
  this->Y = 0;
}

// ----------------------------------------------------------------------
vtkContingencyStatistics::~vtkContingencyStatistics()
{
  this->SetX( 0 );
  this->SetY( 0 );
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "X: " << (this->X ? this->X : "(none)") << endl;
  os << indent << "Y: " << (this->Y ? this->Y : "(none)") << endl;
 }

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteLearn( vtkTable* dataset,
                                             vtkTable* output,
                                             bool finalize )
{
  if ( ! dataset->GetNumberOfColumns() )
    {
    this->SampleSize = 0;
    return;
    }

  this->SampleSize = dataset->GetNumberOfRows();
  if ( ! this->SampleSize )
    {
    return;
    }

  vtkStdString colX = this->X;
  if ( ! dataset->GetColumnByName( colX ) )
    {
    vtkWarningMacro( "Dataset table does not have a column "<<colX.c_str()<<". Doing nothing." );
    return;
    }

  vtkStdString colY = this->Y;
  if ( ! dataset->GetColumnByName( colY ) )
    {
    vtkWarningMacro( "Dataset table does not have a column "<<colY.c_str()<<". Doing nothing." );
    return;
    }
    
  vtkVariantArray* variantCol = vtkVariantArray::New();
  variantCol->SetName( X );
  output->AddColumn( variantCol );
  variantCol->Delete();

  variantCol = vtkVariantArray::New();
  variantCol->SetName( Y );
  output->AddColumn( variantCol );
  variantCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  output->AddColumn( idTypeCol );
  idTypeCol->Delete();

  if ( finalize )
    {
    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Probability" );
    output->AddColumn( doubleCol );
    doubleCol->Delete();
    }

  typedef vtkstd::map<double,vtkIdType> Distribution;
  vtkstd::map<double,Distribution> conTable;
  for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
    {
    ++ conTable[dataset->GetValueByName( r, colX ).ToDouble()][dataset->GetValueByName( r, colY ).ToDouble()];
    }

  vtkVariantArray* row = vtkVariantArray::New();
  double n = static_cast<double>( this->SampleSize );
  if ( finalize )
    {
    row->SetNumberOfValues( 4 );

    for ( vtkstd::map<double,Distribution>::iterator it = conTable.begin(); 
          it != conTable.end(); ++ it )
      {
      row->SetValue( 0, it->first );
      for ( Distribution::iterator dit = it->second.begin(); dit != it->second.end(); ++ dit )
        {
        row->SetValue( 1, dit->first );
        row->SetValue( 2, dit->second );
        row->SetValue( 3, dit->second / n );

        output->InsertNextRow( row );
        }
      }
    }
  else 
    {
    row->SetNumberOfValues( 3 );

    for ( vtkstd::map<double,Distribution>::iterator it = conTable.begin(); 
          it != conTable.end(); ++ it )
      {
      row->SetValue( 0, it->first );
      for ( Distribution::iterator dit = it->second.begin(); dit != it->second.end(); ++ dit )
        {
        row->SetValue( 1, dit->first );
        row->SetValue( 2, dit->second );

        output->InsertNextRow( row );
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
void vtkContingencyStatistics::ExecuteAssess( vtkTable* dataset,
                                              vtkTable* params,
                                              vtkTable* output)
{
  output->ShallowCopy( dataset );

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
  if ( nColP < 3 )
    {
    vtkWarningMacro( "Parameter table has " 
                     << nColP
                     << " < 3 columns. Doing nothing." );
    return;
    }

  vtkIdType nRowP = params->GetNumberOfRows();
  if ( ! nRowP )
    {
    return;
    }

  vtkStdString colX = this->X;
  if ( ! dataset->GetColumnByName( colX ) )
    {
    vtkWarningMacro( "Dataset table does not have a column "<<colX.c_str()<<". Doing nothing." );
    return;
    }
  
  vtkStdString colY = this->Y;
  if ( ! dataset->GetColumnByName( colY ) )
    {
    vtkWarningMacro( "Dataset table does not have a column "<<colY.c_str()<<". Doing nothing." );
    return;
    }
  
  if ( ! params->GetColumnByName( "Probability" ) )
    {
    vtkWarningMacro( "Dataset table does not have a column Probability. Doing nothing." );
    return;
    }
  
  // Create the output columns
  vtkDoubleArray* pXY = vtkDoubleArray::New();
  vtksys_ios::ostringstream pXYName;
  pXYName
    << "p(" << ( colX.size() ? colX.c_str() : "X" )
    << ","  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
  pXY->SetName( pXYName.str().c_str() );
  pXY->SetNumberOfTuples( nRowD );
  output->AddColumn( pXY );
  pXY->Delete();

  vtkDoubleArray* pYcondX = vtkDoubleArray::New();
  vtksys_ios::ostringstream pYCondXName;
  pYCondXName
    << "p(" << ( colY.size() ? colY.c_str() : "Y" )
    << "|"  << ( colX.size() ? colX.c_str() : "X" ) << ")";
  pYcondX->SetName( pYCondXName.str().c_str() );
  pYcondX->SetNumberOfTuples( nRowD );
  output->AddColumn( pYcondX );
  pYcondX->Delete();

  vtkDoubleArray* pXcondY = vtkDoubleArray::New();
  vtksys_ios::ostringstream pXCondYName;
  pXCondYName
    << "p(" << ( colX.size() ? colX.c_str() : "X" )
    << "|"  << ( colY.size() ? colY.c_str() : "Y" ) << ")";
  pXcondY->SetName( pXCondYName.str().c_str() );
  pXcondY->SetNumberOfTuples( nRowD );
  output->AddColumn( pXcondY );
  pXcondY->Delete();

  vtkstd::map<double,double> pdfX, pdfY;
  vtkstd::map<double,vtkstd::map<double,double> > pdfXY;
  double x, y, p;
  for ( vtkIdType r = 0; r < nRowP; ++ r )
    {
    x = params->GetValueByName( r, colX ).ToDouble();
    y = params->GetValueByName( r, colY ).ToDouble();
    p = params->GetValueByName( r, "Probability" ).ToDouble();

    pdfX[x] += p;
    pdfY[y] += p;
    pdfXY[x][y] = p;
    }

  vtkstd::map<double,vtkstd::map<double,double> > pdfYcondX;
  vtkstd::map<double,vtkstd::map<double,double> > pdfXcondY;
  for ( vtkstd::map<double,vtkstd::map<double,double> >::iterator xit = pdfXY.begin();
        xit != pdfXY.end(); ++ xit )
    {
    x = xit->first;
    for ( vtkstd::map<double,double>::iterator yit = xit->second.begin(); 
          yit != xit->second.end(); ++ yit )
      {
      y = yit->first;
      pdfYcondX[x][y] = yit->second / pdfX[x];
      pdfXcondY[x][y] = yit->second / pdfY[y];
      }
    }

  for ( vtkIdType r = 0; r < nRowD; ++ r )
    {
    x = dataset->GetValueByName( r, colX ).ToDouble();
    y = dataset->GetValueByName( r, colY ).ToDouble();

    output->SetValueByName( r, pXYName.str().c_str(), pdfXY[x][y] );
    output->SetValueByName( r, pYCondXName.str().c_str(), pdfYcondX[x][y] );
    output->SetValueByName( r, pXCondYName.str().c_str(), pdfXcondY[x][y] );
    }

  return;
}
