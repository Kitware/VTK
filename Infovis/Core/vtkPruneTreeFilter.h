/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPruneTreeFilter.h

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
 * @class   vtkPruneTreeFilter
 * @brief   prune a subtree out of a vtkTree
 *
 *
 * Removes a subtree rooted at a particular vertex in a vtkTree.
 *
*/

#ifndef vtkPruneTreeFilter_h
#define vtkPruneTreeFilter_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class vtkTree;
class vtkPVXMLElement;

class VTKINFOVISCORE_EXPORT vtkPruneTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkPruneTreeFilter* New();
  vtkTypeMacro(vtkPruneTreeFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the parent vertex of the subtree to remove.
   */
  vtkGetMacro(ParentVertex, vtkIdType);
  vtkSetMacro(ParentVertex, vtkIdType);
  //@}

  //@{
  /**
   * Should we remove the parent vertex, or just its descendants?
   * Default behavior is to remove the parent vertex.
   */
  vtkGetMacro(ShouldPruneParentVertex, bool);
  vtkSetMacro(ShouldPruneParentVertex, bool);
  //@}

protected:
  vtkPruneTreeFilter();
  ~vtkPruneTreeFilter() VTK_OVERRIDE;

  vtkIdType ParentVertex;
  bool ShouldPruneParentVertex;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkPruneTreeFilter(const vtkPruneTreeFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPruneTreeFilter&) VTK_DELETE_FUNCTION;
};

#endif

