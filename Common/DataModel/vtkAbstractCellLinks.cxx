/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractCellLinks.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractCellLinks.h"

#include "vtkObjectFactory.h"
#include "vtkCellArray.h"

//----------------------------------------------------------------------------
vtkAbstractCellLinks::vtkAbstractCellLinks()
{
}

//----------------------------------------------------------------------------
vtkAbstractCellLinks::~vtkAbstractCellLinks()
{
}

//----------------------------------------------------------------------------
int vtkAbstractCellLinks::
GetIdType(vtkIdType maxPtId, vtkIdType maxCellId, vtkCellArray *ca)
{
  vtkIdType numEntries = ca->GetNumberOfConnectivityEntries();
  vtkIdType max = maxPtId;
  max = (maxCellId > max ? maxCellId : max);
  max = (numEntries > max ? numEntries : max);

  if ( max >= VTK_INT_MAX )
  {
    return VTK_ID_TYPE;
  }
  else if ( max >= VTK_SHORT_MAX )
  {
    return VTK_INT;
  }
  else
  {
    return VTK_SHORT;
  }
}

//----------------------------------------------------------------------------
void vtkAbstractCellLinks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
