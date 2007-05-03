/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostBreadthFirstSearchTree.h

  Copyright 2007 Sandia Corporation.
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
// .NAME vtkBoostBreadthFirstSearchTree - Contructs a BFS tree from a graph

// .SECTION Description

// This vtk class uses the Boost breadth_first_search 
// generic algorithm to perform a breadth first search from a given
// a 'source' vertex on the input graph (a vtkGraph).
// The result is a tree with root node corresponding to the start node
// of the search.

// .SECTION See Also
// vtkGraph vtkGraphToBoostAdapter

#ifndef __vtkBoostBreadthFirstSearchTree_h
#define __vtkBoostBreadthFirstSearchTree_h

#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkBoostBreadthFirstSearchTree : public vtkTreeAlgorithm 
{
public:
  static vtkBoostBreadthFirstSearchTree *New();
  vtkTypeRevisionMacro(vtkBoostBreadthFirstSearchTree, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the index (into the vertex array) of the 
  // breadth first search 'origin' vertex.
  void SetOriginVertex(vtkIdType index);
  
  //BTX

  // Description:
  // Set the breadth first search 'origin' vertex.
  // This method is basically the same as above
  // but allows the application to simply specify
  // an array name and value, instead of having to
  // know the specific index of the vertex.
  void SetOriginVertex(vtkStdString arrayName, vtkVariant value);
  //ETX

protected:
  vtkBoostBreadthFirstSearchTree();
  ~vtkBoostBreadthFirstSearchTree();

  int FillInputPortInformation(int port, vtkInformation* info);

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:

  vtkIdType OriginVertexIndex;
  char* ArrayName;
  //BTX
  vtkVariant OriginValue;
  //ETX
  bool ArrayNameSet;
  
  // Description:
  // Using the convenience function for set strings internally
  vtkSetStringMacro(ArrayName);

  //BTX
  
  // Description:
  // This method is basically a helper function to find
  // the index of a specific value within a specific array
  vtkIdType vtkBoostBreadthFirstSearchTree::GetVertexIndex(
    vtkAbstractArray *abstract,vtkVariant value);
  //ETX

  vtkBoostBreadthFirstSearchTree(const vtkBoostBreadthFirstSearchTree&);  // Not implemented.
  void operator=(const vtkBoostBreadthFirstSearchTree&);  // Not implemented.
};

#endif
