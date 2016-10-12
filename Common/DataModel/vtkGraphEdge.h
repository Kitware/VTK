/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphEdge.h

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
 * @class   vtkGraphEdge
 * @brief   Representation of a single graph edge.
 *
 *
 * A heavy-weight (vtkObject subclass) graph edge object that may be used
 * instead of the vtkEdgeType struct, for use with wrappers.
 * The edge contains the source and target vertex ids, and the edge id.
 *
 * @sa
 * vtkGraph
*/

#ifndef vtkGraphEdge_h
#define vtkGraphEdge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONDATAMODEL_EXPORT vtkGraphEdge : public vtkObject
{
public:
  static vtkGraphEdge *New();
  vtkTypeMacro(vtkGraphEdge, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The source of the edge.
   */
  vtkSetMacro(Source, vtkIdType);
  vtkGetMacro(Source, vtkIdType);
  //@}

  //@{
  /**
   * The target of the edge.
   */
  vtkSetMacro(Target, vtkIdType);
  vtkGetMacro(Target, vtkIdType);
  //@}

  //@{
  /**
   * The id of the edge.
   */
  vtkSetMacro(Id, vtkIdType);
  vtkGetMacro(Id, vtkIdType);
  //@}

protected:
  vtkGraphEdge();
  ~vtkGraphEdge() VTK_OVERRIDE;

  vtkIdType Source;
  vtkIdType Target;
  vtkIdType Id;

private:
  vtkGraphEdge(const vtkGraphEdge&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphEdge&) VTK_DELETE_FUNCTION;
};

#endif
