/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvexPointSet.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConvexPointSet.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkConvexPointSet, "1.3");
vtkStandardNewMacro(vtkConvexPointSet);

// Construct the hexahedron with eight points.
vtkConvexPointSet::vtkConvexPointSet()
{
}

vtkConvexPointSet::~vtkConvexPointSet()
{
}

vtkCell *vtkConvexPointSet::MakeObject()
{
  vtkCell *cell = vtkConvexPointSet::New();
  cell->DeepCopy(this);
  return cell;
}



void vtkConvexPointSet::Contour(float value, vtkDataArray *cellScalars, 
                            vtkPointLocator *locator,
                            vtkCellArray *vtkNotUsed(verts), 
                            vtkCellArray *vtkNotUsed(lines), 
                            vtkCellArray *polys, 
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd)
{
}


