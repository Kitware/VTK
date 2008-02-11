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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkQtAbstractModelAdapter.h"

int vtkQtAbstractModelAdapter::ModelColumnToFieldDataColumn(int col) const
{
  int result = -1;
  int numDataColumns = this->DataEndColumn - this->DataStartColumn + 1;
  int key = -1;
  switch (this->ViewType)
    {
    case FULL_VIEW:
      result = col;
      break;
    case DATA_VIEW:
      result = this->DataStartColumn + col;
      break;
    case METADATA_VIEW:
      if (this->KeyColumn >= 0)
        {
        if (this->KeyColumn < this->DataStartColumn)
          {
          key = this->KeyColumn;
          }
        else
          {
          key = this->KeyColumn - numDataColumns;
          }
        result = (col == 0 ? key : (col == key ? 0 : col));
        }
      if (result >= this->DataStartColumn)
        {
        result += numDataColumns;
        }
      break;
    default:
      vtkGenericWarningMacro("vtkQtAbstractModelAdapter: Bad view type.");
      break;
    };
  return result;
}
