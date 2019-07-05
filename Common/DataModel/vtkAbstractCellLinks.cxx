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
  this->SequentialProcessing = false;
  this->Type = vtkAbstractCellLinks::LINKS_NOT_DEFINED;
}

//----------------------------------------------------------------------------
vtkAbstractCellLinks::~vtkAbstractCellLinks() = default;

//----------------------------------------------------------------------------
int vtkAbstractCellLinks::
ComputeType(vtkIdType maxPtId, vtkIdType maxCellId, vtkCellArray *ca)
{
  vtkIdType numEntries = ca->GetNumberOfConnectivityEntries();
  vtkIdType max = maxPtId;
  max = (maxCellId > max ? maxCellId : max);
  max = (numEntries > max ? numEntries : max);

  if ( max >= VTK_UNSIGNED_INT_MAX )
  {
    return vtkAbstractCellLinks::STATIC_CELL_LINKS_IDTYPE;
  }
  else if ( max >= VTK_UNSIGNED_SHORT_MAX )
  {
    return vtkAbstractCellLinks::STATIC_CELL_LINKS_UINT;
  }
  else
  {
    return vtkAbstractCellLinks::STATIC_CELL_LINKS_USHORT;
  }
}

//----------------------------------------------------------------------------
void vtkAbstractCellLinks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sequential Processing: "
     << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Type: " << this->Type << "\n";
}
