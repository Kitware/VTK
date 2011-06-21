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
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkUnivariateStatisticsAlgorithm.h"

#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/set>
#include <vtksys/ios/sstream>

// ----------------------------------------------------------------------
vtkUnivariateStatisticsAlgorithm::vtkUnivariateStatisticsAlgorithm()
{
}

// ----------------------------------------------------------------------
vtkUnivariateStatisticsAlgorithm::~vtkUnivariateStatisticsAlgorithm()
{
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkUnivariateStatisticsAlgorithm::AddColumn( const char* namCol )
{
  if ( this->Internals->SetBufferColumnStatus( namCol, 1 ) )
    {
    this->Modified();
    }
}

// ----------------------------------------------------------------------
int vtkUnivariateStatisticsAlgorithm::RequestSelectedColumns()
{
  return this->Internals->AddBufferEntriesToRequests();
}
