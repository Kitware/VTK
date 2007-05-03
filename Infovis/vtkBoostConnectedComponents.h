/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostConnectedComponents.h

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
// .NAME vtkBoostConnectedComponents - Find the connected components of a graph
//
// .SECTION Description
// vtkBoostConnectedComponents discovers the connected regions of a vtkGraph.
// Each vertex is assigned a component ID in the vertex array "components".
// If the graph is undirected, this is the natural connected components
// of the graph.  If the graph is directed, this filter discovers the
// strongly connected components of the graph (i.e. the maximal sets of
// vertices where there is a directed path between any pair of vertices
// within each set).

#ifndef __vtkBoostConnectedComponents_h
#define __vtkBoostConnectedComponents_h

#include "vtkGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkBoostConnectedComponents : public vtkGraphAlgorithm 
{
public:
  static vtkBoostConnectedComponents *New();
  vtkTypeRevisionMacro(vtkBoostConnectedComponents, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkBoostConnectedComponents();
  ~vtkBoostConnectedComponents();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:

  vtkBoostConnectedComponents(const vtkBoostConnectedComponents&);  // Not implemented.
  void operator=(const vtkBoostConnectedComponents&);  // Not implemented.
};

#endif
