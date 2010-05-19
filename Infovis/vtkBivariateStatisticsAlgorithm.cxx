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
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/set>
#include <vtksys/ios/sstream>

#define VTK_STATISTICS_NUMBER_OF_VARIABLES 2


// ----------------------------------------------------------------------
vtkBivariateStatisticsAlgorithm::vtkBivariateStatisticsAlgorithm()
{
}

// ----------------------------------------------------------------------
vtkBivariateStatisticsAlgorithm::~vtkBivariateStatisticsAlgorithm()
{
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::AddColumnPair( const char* namColX, const char* namColY )
{
  if ( this->Internals->AddColumnPairToRequests( namColX, namColY ) )
    {
    this->Modified();
    }
}

// ----------------------------------------------------------------------
int vtkBivariateStatisticsAlgorithm::RequestSelectedColumns()
{
  return this->Internals->AddBufferEntryPairsToRequests();
}

// ----------------------------------------------------------------------
void vtkBivariateStatisticsAlgorithm::Assess( vtkTable* inData,
                                              vtkDataObject* inMetaDO,
                                              vtkTable* outData )
{
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO );
  if ( ! inMeta )
    {
    return;
    }

  if ( ! inData || inData->GetNumberOfColumns() <= 0 )
    {
    return;
    }

  vtkIdType nRowD = inData->GetNumberOfRows();
  if ( nRowD <= 0 )
    {
    return;
    }

  vtkIdType nColP;
  if ( this->AssessParameters )
    {
    nColP = this->AssessParameters->GetNumberOfValues();
    if ( inMeta->GetNumberOfColumns() - VTK_STATISTICS_NUMBER_OF_VARIABLES < nColP )
      {
      vtkWarningMacro( "Parameter table has " 
                       << inMeta->GetNumberOfColumns() - VTK_STATISTICS_NUMBER_OF_VARIABLES
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

  // Loop over requests
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin(); 
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString varNameX = *it;
    if ( ! inData->GetColumnByName( varNameX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameX.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    ++ it;
    vtkStdString varNameY = *it;
    if ( ! inData->GetColumnByName( varNameY ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameY.c_str()
                       << ". Ignoring this pair." );
      continue;
      }
    
    vtkStringArray* varNames = vtkStringArray::New();
    varNames->SetNumberOfValues( VTK_STATISTICS_NUMBER_OF_VARIABLES );
    varNames->SetValue( 0, varNameX );
    varNames->SetValue( 1, varNameY );

    // Store names to be able to use SetValueByName, and create the outData columns
    int nv = this->AssessNames->GetNumberOfValues();
    vtkStdString* names = new vtkStdString[nv];
    for ( int v = 0; v < nv; ++ v )
      {
      vtksys_ios::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue( v )
                    << "("
                    << varNameX
                    << ","
                    << varNameY
                    << ")";

      names[v] = assessColName.str().c_str();

      vtkDoubleArray* assessValues = vtkDoubleArray::New(); 
      assessValues->SetName( names[v] ); 
      assessValues->SetNumberOfTuples( nRowD  ); 
      outData->AddColumn( assessValues ); 
      assessValues->Delete(); 
      }

    // Select assess functor
    AssessFunctor* dfunc;
    this->SelectAssessFunctor( outData,
                               inMeta,
                               varNames,
                               dfunc );

    if ( ! dfunc )
      {
      // Functor selection did not work. Do nothing.
      vtkWarningMacro( "AssessFunctors could not be allocated for column pair ("
                       << varNameX.c_str()
                       << ","
                       << varNameY.c_str()
                       << "). Ignoring it." );
      delete [] names;
      continue;
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
