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
#include "vtkTable.h"

#include <vtkstd/set>

vtkCxxRevisionMacro(vtkBivariateStatisticsAlgorithm, "1.1");

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
