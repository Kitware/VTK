/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGroupLeafVertices.h

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
// .NAME vtkGroupLeafVertices - Filter that expands a tree, categorizing leaf vertices
//
// .SECTION Description
// Use SetInputArrayToProcess(0, ...) to set the array to group on.
// Currently this array must be a vtkStringArray.

#ifndef __vtkGroupLeafVertices_h
#define __vtkGroupLeafVertices_h

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkGroupLeafVertices : public vtkTreeAlgorithm
{
public:
  static vtkGroupLeafVertices* New();
  vtkTypeRevisionMacro(vtkGroupLeafVertices,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkGroupLeafVertices();
  ~vtkGroupLeafVertices();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
private:
  vtkGroupLeafVertices(const vtkGroupLeafVertices&); // Not implemented
  void operator=(const vtkGroupLeafVertices&);   // Not implemented
};

#endif

