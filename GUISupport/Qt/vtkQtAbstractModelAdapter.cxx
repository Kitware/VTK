/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtAbstractModelAdapter.cxx

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

#include "vtkObject.h" // For vtkGenericWarningMacro
#include "vtkQtAbstractModelAdapter.h"

int vtkQtAbstractModelAdapter::ModelColumnToFieldDataColumn(int col) const
{
  int result = -1;
  switch (this->ViewType)
  {
    case FULL_VIEW:
      result = col;
      break;
    case DATA_VIEW:
      result = this->DataStartColumn + col;
      break;
    default:
      vtkGenericWarningMacro("vtkQtAbstractModelAdapter: Bad view type.");
      break;
  };
  return result;
}
