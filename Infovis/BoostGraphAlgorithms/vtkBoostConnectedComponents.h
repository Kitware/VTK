/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostConnectedComponents.h

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
 * @class   vtkBoostConnectedComponents
 * @brief   Find the connected components of a graph
 *
 *
 * vtkBoostConnectedComponents discovers the connected regions of a vtkGraph.
 * Each vertex is assigned a component ID in the vertex array "component".
 * If the graph is undirected, this is the natural connected components
 * of the graph.  If the graph is directed, this filter discovers the
 * strongly connected components of the graph (i.e. the maximal sets of
 * vertices where there is a directed path between any pair of vertices
 * within each set).
*/

#ifndef vtkBoostConnectedComponents_h
#define vtkBoostConnectedComponents_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostConnectedComponents : public vtkGraphAlgorithm
{
public:
  static vtkBoostConnectedComponents *New();
  vtkTypeMacro(vtkBoostConnectedComponents, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkBoostConnectedComponents();
  ~vtkBoostConnectedComponents();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

  vtkBoostConnectedComponents(const vtkBoostConnectedComponents&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoostConnectedComponents&) VTK_DELETE_FUNCTION;
};

#endif
