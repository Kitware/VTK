/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToTreeFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2006 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.
  
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTableToTreeFilter - Filter that converts a vtkTable to a vtkTree
//
// .SECTION Description
//
// vtkTableToTreeFilter is a filter for converting a vtkTable data structure
// into a vtkTree datastructure.  Currently, this will convert the table into
// a star, with each row of the table as a child of a new root node.
// The columns of the table are passed as node fields of the tree.

#ifndef __vtkTableToTreeFilter_h
#define __vtkTableToTreeFilter_h

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTableToTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkTableToTreeFilter* New();
  vtkTypeRevisionMacro(vtkTableToTreeFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTableToTreeFilter();
  ~vtkTableToTreeFilter();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
  int FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation* info);
  int FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info);

private:
  vtkTableToTreeFilter(const vtkTableToTreeFilter&); // Not implemented
  void operator=(const vtkTableToTreeFilter&);   // Not implemented
};

#endif

