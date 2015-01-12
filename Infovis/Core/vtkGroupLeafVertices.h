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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkGroupLeafVertices - Filter that expands a tree, categorizing leaf vertices
//
// .SECTION Description
// Use SetInputArrayToProcess(0, ...) to set the array to group on.
// Currently this array must be a vtkStringArray.

#ifndef vtkGroupLeafVertices_h
#define vtkGroupLeafVertices_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkGroupLeafVertices : public vtkTreeAlgorithm
{
public:
  static vtkGroupLeafVertices* New();
  vtkTypeMacro(vtkGroupLeafVertices,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The name of the domain that non-leaf vertices will be assigned to.
  // If the input graph already contains vertices in this domain:
  // - If the ids for this domain are numeric, starts assignment with max id
  // - If the ids for this domain are strings, starts assignment with "group X"
  //   where "X" is the max id.
  // Default is "group_vertex".
  vtkSetStringMacro(GroupDomain);
  vtkGetStringMacro(GroupDomain);

protected:
  vtkGroupLeafVertices();
  ~vtkGroupLeafVertices();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  char* GroupDomain;

private:
  vtkGroupLeafVertices(const vtkGroupLeafVertices&); // Not implemented
  void operator=(const vtkGroupLeafVertices&);   // Not implemented
};

#endif

