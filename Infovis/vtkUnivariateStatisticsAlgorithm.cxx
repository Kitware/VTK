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

vtkCxxRevisionMacro(vtkUnivariateStatisticsAlgorithm, "1.10");

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

