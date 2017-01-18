/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToTreeFilter.h

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
/**
 * @class   vtkTableToTreeFilter
 * @brief   Filter that converts a vtkTable to a vtkTree
 *
 *
 *
 * vtkTableToTreeFilter is a filter for converting a vtkTable data structure
 * into a vtkTree datastructure.  Currently, this will convert the table into
 * a star, with each row of the table as a child of a new root node.
 * The columns of the table are passed as node fields of the tree.
*/

#ifndef vtkTableToTreeFilter_h
#define vtkTableToTreeFilter_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkTableToTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkTableToTreeFilter* New();
  vtkTypeMacro(vtkTableToTreeFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkTableToTreeFilter();
  ~vtkTableToTreeFilter() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  int FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation* info) VTK_OVERRIDE;
  int FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info) VTK_OVERRIDE;

private:
  vtkTableToTreeFilter(const vtkTableToTreeFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTableToTreeFilter&) VTK_DELETE_FUNCTION;
};

#endif

