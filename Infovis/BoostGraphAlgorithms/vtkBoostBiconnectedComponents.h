/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostBiconnectedComponents.h

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
 * @class   vtkBoostBiconnectedComponents
 * @brief   Find the biconnected components of a graph
 *
 *
 * The biconnected components of a graph are maximal regions of the graph where
 * the removal of any single vertex from the region will not disconnect the
 * graph.  Every edge belongs to exactly one biconnected component.  The
 * biconnected component of each edge is given in the edge array named
 * "biconnected component".  The biconnected component of each vertex is also
 * given in the vertex array named "biconnected component".  Cut vertices (or
 * articulation points) belong to multiple biconnected components, and break
 * the graph apart if removed.  These are indicated by assigning a component
 * value of -1.  To get the biconnected components that a cut vertex belongs
 * to, traverse its edge list and collect the distinct component ids for its
 * incident edges.
 *
 * Self-loop edges that start and end at the same vertex are not
 * assigned a biconnected component, and are given component id -1.
 *
 * @warning
 * The boost graph bindings currently only support boost version 1.33.1.
 * There are apparently backwards-compatibility issues with later versions.
*/

#ifndef vtkBoostBiconnectedComponents_h
#define vtkBoostBiconnectedComponents_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkUndirectedGraphAlgorithm.h"

class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostBiconnectedComponents : public vtkUndirectedGraphAlgorithm
{
public:
  static vtkBoostBiconnectedComponents *New();
  vtkTypeMacro(vtkBoostBiconnectedComponents, vtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the output array name. If no output array name is
   * set then the name "biconnected component" is used.
   */
  vtkSetStringMacro(OutputArrayName);
  //@}

protected:
  vtkBoostBiconnectedComponents();
  ~vtkBoostBiconnectedComponents();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

  char* OutputArrayName;

  vtkBoostBiconnectedComponents(const vtkBoostBiconnectedComponents&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoostBiconnectedComponents&) VTK_DELETE_FUNCTION;
};

#endif
