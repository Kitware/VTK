/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubdivideTetra.h
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
// .NAME vtkSubdivideTetra - subdivide one tetrahedron into twelve for every tetra
// .SECTION Description
// This filter subdivides tetrahedra in an unstructured grid into twelve tetrahedra.


#ifndef __vtkSubdivideTetra_h
#define __vtkSubdivideTetra_h

#include "vtkUnstructuredGridToUnstructuredGridFilter.h"

class VTK_GRAPHICS_EXPORT vtkSubdivideTetra : public vtkUnstructuredGridToUnstructuredGridFilter
{
public:
  static vtkSubdivideTetra *New();
  vtkTypeRevisionMacro(vtkSubdivideTetra,vtkUnstructuredGridToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);


protected:
  vtkSubdivideTetra();
  ~vtkSubdivideTetra() {};

  void Execute();

private:
  vtkSubdivideTetra(const vtkSubdivideTetra&);  // Not implemented.
  void operator=(const vtkSubdivideTetra&);  // Not implemented.
};

#endif


