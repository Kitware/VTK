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
#include "vtkTable.h"

#include <vtkstd/set>

vtkCxxRevisionMacro(vtkUnivariateStatisticsAlgorithm, "1.6");

// ----------------------------------------------------------------------
vtkUnivariateStatisticsAlgorithm::vtkUnivariateStatisticsAlgorithm()
{
  this->Internals = new vtkUnivariateStatisticsAlgorithmPrivate;
  this->Internals->ColumnSelectionUsage = false;
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
void vtkUnivariateStatisticsAlgorithm::UseColumnSelection( bool all )
{
  this->Internals->ColumnSelectionUsage = all;
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::ResetColumns()
{
  this->Internals->SelectedColumns.clear();
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::AddColumn( const char* namCol )
{
 this->Internals->SelectedColumns.insert( namCol );
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::RemoveColumn( const char* namCol )
{
 this->Internals->SelectedColumns.erase( namCol );
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::SetColumnSelection( vtkTable* dataset )
{
  if ( this->Internals->ColumnSelectionUsage )
    {
    return;
    }

  this->Internals->SelectedColumns.clear();

  vtkIdType nCol = dataset->GetNumberOfColumns();
  for ( int idxCol = 0; idxCol < nCol; ++ idxCol )
    {
    this->Internals->SelectedColumns.insert( dataset->GetColumnName( idxCol ) );
    }
}
