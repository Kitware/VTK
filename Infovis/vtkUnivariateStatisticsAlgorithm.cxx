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

vtkCxxRevisionMacro(vtkUnivariateStatisticsAlgorithm, "1.17");

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
  if ( ! inData->GetNumberOfColumns() )
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
    if ( inMeta->GetNumberOfColumns() - 1 < nColP )
      {
      vtkWarningMacro( "Parameter table has " 
                       << inMeta->GetNumberOfColumns() - 1
                       << " parameters < "
                       << nColP
                       << " columns. Doing nothing." );
      return;
      }
    }

  if ( ! inMeta->GetNumberOfRows() )
    {
    return;
    }

  if ( ! this->Internals->SelectedColumns.size() )
    {
    return;
    }

  // Loop over columns of interest
  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->SelectedColumns.begin(); 
        it != this->Internals->SelectedColumns.end(); ++ it )
    {
    vtkStdString varName = *it;
    if ( ! inData->GetColumnByName( varName ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varName.c_str()
                       << ". Ignoring it." );
      continue;
      }

    vtkStringArray* varNames = vtkStringArray::New();
    varNames->SetNumberOfValues( 1 );
    varNames->SetValue( 0, varName );

    // Create the outData columns
    int nv = this->AssessNames->GetNumberOfValues();
    vtkVariantArray* assessValues;
    vtksys_ios::ostringstream assessColName;
    vtkStdString* names = new vtkStdString[nv];
    for ( int v = 0; v < nv; ++ v )
      {
      assessColName << this->AssessNames->GetValue( v )
                    << "("
                    << varName
                    << ")";

      assessValues = vtkVariantArray::New();
      names[v] = assessColName.str().c_str(); // Storing names to be able to use SetValueByName which is faster than SetValue
      assessValues->SetName( names[v] );
      assessValues->SetNumberOfTuples( nRowD );
      outData->AddColumn( assessValues );
      assessValues->Delete();
      }

    // Select assess functor
    AssessFunctor* dfunc;
    this->SelectAssessFunctor( inData,
                               inMeta,
                               varNames,
                               this->AssessParameters,
                               dfunc );

    if ( ! dfunc )
      {
      // Functor selection did not work. Do nothing.
      vtkWarningMacro( "AssessFunctors could not be allocated for column "
                       << varName.c_str()
                       << ". Ignoring it." );
      }
    else
      {
      // Assess each entry of the column
      vtkVariantArray* assessResult = vtkVariantArray::New();
      for ( vtkIdType r = 0; r < nRowD; ++ r )
        {
        (*dfunc)( assessResult, r );
        for ( int v = 0; v < nv; ++ v )
          {
          outData->SetValueByName( r, names[v], assessResult->GetValue( v ) );
          }
        }

      assessResult->Delete();
      }

    delete dfunc;
    delete [] names;
    varNames->Delete(); // Do not delete earlier! Otherwise, dfunc will be wrecked
    }
}
