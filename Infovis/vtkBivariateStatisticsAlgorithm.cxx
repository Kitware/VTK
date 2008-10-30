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

vtkCxxRevisionMacro(vtkBivariateStatisticsAlgorithm, "1.4");

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
    if ( inMeta->GetNumberOfColumns() - 2 < nColP )
      {
      vtkWarningMacro( "Parameter table has " 
                       << inMeta->GetNumberOfColumns() - 2
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

  if ( ! this->Internals->ColumnPairs.size() )
    {
    return;
    }


  // Loop over pairs of columns of interest
  for ( vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> >::iterator it = this->Internals->ColumnPairs.begin(); 
        it != this->Internals->ColumnPairs.end(); ++ it )
    {
    vtkStdString varNameX = it->first;
    if ( ! inData->GetColumnByName( varNameX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameX.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    vtkStdString varNameY = it->second;
    if ( ! inData->GetColumnByName( varNameY ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameY.c_str()
                       << ". Ignoring this pair." );
      continue;
      }
    
    vtkStringArray* varNames = vtkStringArray::New();
    varNames->SetNumberOfValues( 2 );
    varNames->SetValue( 0, varNameX );
    varNames->SetValue( 1, varNameY );

    // Create the outData columns
    int nv = this->AssessNames->GetNumberOfValues();
    vtkVariantArray* assessValues;
    vtksys_ios::ostringstream assessColName;
    vtkStdString* names = new vtkStdString[nv];
    for ( int v = 0; v < nv; ++ v )
      {
      assessColName << this->AssessNames->GetValue( v )
                    << "("
                    << varNameX
                    << ","
                    << varNameY
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
      vtkWarningMacro( "AssessFunctors could not be allocated for column pair ("
                       << varNameX.c_str()
                       << ","
                       << varNameY.c_str()
                       << "). Ignoring it." );
      continue;
      }

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

    delete dfunc;
    delete [] names;
    varNames->Delete(); // Do not delete earlier! Otherwise, dfunc will be wrecked
    assessResult->Delete();
    }
}
