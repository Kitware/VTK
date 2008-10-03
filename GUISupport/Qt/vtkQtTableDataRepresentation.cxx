/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableDataRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkQtTableDataRepresentation.h"
#include "vtkQtTableModelAdapter.h"

#include <vtkDoubleArray.h>
#include <vtkObjectFactory.h>

#include <QColor>
#include <QModelIndex>

#include <assert.h>

vtkCxxRevisionMacro(vtkQtTableDataRepresentation, "1.1");
vtkStandardNewMacro(vtkQtTableDataRepresentation);

// ----------------------------------------------------------------------

vtkQtTableDataRepresentation::vtkQtTableDataRepresentation()
{
  // nothing to do -- all handled in the superclass
}

// ----------------------------------------------------------------------

vtkQtTableDataRepresentation::~vtkQtTableDataRepresentation()
{
  // nothing to do -- all handled in the superclass
}

// ----------------------------------------------------------------------

void
vtkQtTableDataRepresentation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------

void
vtkQtTableDataRepresentation::SetModelType()
{
  this->ModelAdapter->SetViewType(vtkQtAbstractModelAdapter::DATA_VIEW);
}
