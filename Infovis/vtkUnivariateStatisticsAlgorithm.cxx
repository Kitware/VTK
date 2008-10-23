/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnivariateStatisticsAlgorithm.cxx

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

#include "vtkUnivariateStatisticsAlgorithm.h"
#include "vtkUnivariateStatisticsAlgorithmPrivate.h"

#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>
#include <vtksys/ios/sstream>

vtkCxxRevisionMacro(vtkUnivariateStatisticsAlgorithm, "1.15");

// ----------------------------------------------------------------------
vtkUnivariateStatisticsAlgorithm::vtkUnivariateStatisticsAlgorithm()
{
  this->Internals = new vtkUnivariateStatisticsAlgorithmPrivate;
}

// ----------------------------------------------------------------------
vtkUnivariateStatisticsAlgorithm::~vtkUnivariateStatisticsAlgorithm()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::ResetColumns()
{
  this->Internals->SelectedColumns.clear();

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::AddColumn( const char* namCol )
{
 this->Internals->SelectedColumns.insert( namCol );

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::RemoveColumn( const char* namCol )
{
 this->Internals->SelectedColumns.erase( namCol );

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::SetColumnStatus( const char* namCol, int status )
{
  if( status )
    {
    this->Internals->SelectedColumns.insert( namCol );
    }
  else
    {
    this->Internals->SelectedColumns.erase( namCol );
    }

  this->Modified();
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::ExecuteAssess( vtkTable* inData,
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
    nColP = inMeta->GetNumberOfColumns() - 1;
    }

  if ( nColP < 1 )
    {
    return;
    }

  vtkIdType nRowP = inMeta->GetNumberOfRows();
  if ( ! nRowP )
    {
    return;
    }

  if ( ! this->Internals->SelectedColumns.size() )
    {
    return;
    }

  // Loop over rows of the parameter table looking for columns that
  // specify the model parameters of some requested table column.
  for ( int i = 0; i < nRowP; ++ i )
    {
    // Is the parameter row one that is requested?
    vtkStdString colName = inMeta->GetValueByName( i, "Variable" ).ToString();
    vtkstd::set<vtkStdString>::iterator it = this->Internals->SelectedColumns.find( colName );
    if ( it == this->Internals->SelectedColumns.end() )
      { // Have parameter values. But user doesn't want it... skip it.
      continue;
      }

    // Does the requested array exist in the input inData?
    vtkAbstractArray* arr;
    if ( ! ( arr = inData->GetColumnByName( colName ) ) )
      { // User requested it. InMeta table has it. But inData doesn't... whine
      vtkWarningMacro( "InData table does not have a column "
                       << colName.c_str() 
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
        row->SetValue( j, inMeta->GetValue( i, j + 1 ) );
        }
      }

    // Create the outData column
    vtkVariantArray* assessedValues = vtkVariantArray::New();
    vtksys_ios::ostringstream assessColName;
    assessColName << this->AssessmentName
                  << "("
                  << colName
                  << ")";
    assessedValues->SetName( assessColName.str().c_str() );
    assessedValues->SetNumberOfTuples( nRowD );

    // Select assess functor
    AssessFunctor* dfunc;
    vtkTable* colData = vtkTable::New();
    colData->AddColumn( arr );
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
