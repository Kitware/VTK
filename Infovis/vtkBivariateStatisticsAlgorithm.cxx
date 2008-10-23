/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBivariateStatisticsAlgorithm.cxx

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

#include "vtkBivariateStatisticsAlgorithm.h"
#include "vtkBivariateStatisticsAlgorithmPrivate.h"

#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>
#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkBivariateStatisticsAlgorithm, "1.3");

// ----------------------------------------------------------------------
vtkBivariateStatisticsAlgorithm::vtkBivariateStatisticsAlgorithm()
{
  this->Internals = new vtkBivariateStatisticsAlgorithmPrivate;
}

// ----------------------------------------------------------------------
vtkBivariateStatisticsAlgorithm::~vtkBivariateStatisticsAlgorithm()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::ResetColumnPairs()
{
  this->Internals->ColumnPairs.clear();

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::AddColumnPair( const char* namColX, const char* namColY )
{
  vtkstd::pair<vtkStdString,vtkStdString> namPair( namColX, namColY );
  this->Internals->ColumnPairs.insert( namPair );

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::RemoveColumnPair( const char* namColX, const char* namColY )
{
  vtkstd::pair<vtkStdString,vtkStdString> namPair( namColX, namColY );
  this->Internals->ColumnPairs.erase( namPair );

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::SetColumnStatus( const char* namCol, int status )
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
void vtkBivariateStatisticsAlgorithm::ExecuteAssess( vtkTable* inData,
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

  vtkIdType nColP;
  if ( this->AssessParameters )
    {
    nColP = this->AssessParameters->GetNumberOfValues();
    if ( inMeta->GetNumberOfColumns() < nColP )
      {
      vtkWarningMacro( "Parameter table has " 
                       << inMeta->GetNumberOfColumns()
                       << " < "
                       << nColP
                       << " columns. Doing nothing." );
      return;
      }
    }
  else
    {
    nColP = inMeta->GetNumberOfColumns() - 2;
    }

  if ( nColP < 2 )
    {
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

  // Loop over rows of the parameter table looking for columns that
  // specify the model parameters of some requested pair of table columns.
  for ( int i = 0; i < nRowP; ++ i )
    {
    // Is the parameter row one that is requested?
    vtkStdString colNameX = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString colNameY = inMeta->GetValueByName( i, "Variable Y" ).ToString();
    vtkstd::pair<vtkStdString,vtkStdString> colNamePair( colNameX, colNameY );
    vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> >::iterator it = this->Internals->ColumnPairs.find( colNamePair );
    if ( it == this->Internals->ColumnPairs.end() )
      { // Have parameter values. But user doesn't want it... skip it.
      continue;
      }

    // Does the requested array for X exist in the input inData?
    vtkAbstractArray* arrX;
    if ( ! ( arrX = inData->GetColumnByName( colNameX ) ) )
      { // User requested it. InMeta table has it. But inData doesn't... whine
      vtkWarningMacro( "InData table does not have a column " 
                       << colNameX.c_str() 
                       << ". Ignoring it." );
      continue;
      }

    // Does the requested array for Y exist in the input inData?
    vtkAbstractArray* arrY;
    if ( ! ( arrY = inData->GetColumnByName( colNameY ) ) )
      { // User requested it. InMeta table has it. But inData doesn't... whine
      vtkWarningMacro( "InData table does not have a column "
                       << colNameY.c_str() 
                       << ". Ignoring it." );
      continue;
      }

    vtkVariantArray* row = vtkVariantArray::New();
    row->SetNumberOfValues( nColP );
    
    if ( this->AssessParameters )
      {
      for ( vtkIdType j = 0; j < nColP; ++ j )
        {
        row->SetValue( j, inMeta->GetValueByName( i, this->AssessParameters->GetValue( j ) ) );
        }
      }
    else
      {
      for ( vtkIdType j = 0; j < nColP; ++ j )
        {
        row->SetValue( j, inMeta->GetValue( i, j + 2 ) );
        }
      }

    // Create the outData column
    vtkVariantArray* assessedValues = vtkVariantArray::New();
    vtksys_ios::ostringstream assessColName;
    assessColName << this->AssessmentName
                  << "("
                  << colNameX
                  << ","
                  << colNameY
                  << ")";
    assessedValues->SetName( assessColName.str().c_str() );
    assessedValues->SetNumberOfTuples( nRowD );

    // Select assess functor
    AssessFunctor* dfunc;
    vtkTable* colData = vtkTable::New();
    colData->AddColumn( arrX );
    colData->AddColumn( arrY );
    this->SelectAssessFunctor( colData, row, dfunc );

    // Assess each entry of the column
    vtkVariant assess;
    for ( vtkIdType r = 0; r < nRowD; ++ r )
      {
      assess = (*dfunc)( r );
      assessedValues->SetValue( r, assess );
      }
    delete dfunc;
    row->Delete(); // Do not delete earlier! Otherwise, dfunc will be wrecked

    // Add the column to outData
    outData->AddColumn( assessedValues );

    colData->Delete();
    assessedValues->Delete();
    }
}
