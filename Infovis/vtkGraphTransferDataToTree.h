/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphTransferDataToTree.h

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
// .NAME vtkGraphTransferDataToTree - transfer data from a graph representation
// to a tree representation using direct mapping or pedigree ids.
//
// .SECTION Description
// The filter requires
// both a vtkGraph and vtkTree as input.  The tree vertices must be a
// superset of the graph vertices.  A common example is when the graph vertices
// correspond to the leaves of the tree, but the internal vertices of the tree
// represent groupings of graph vertices.  The algorithm matches the vertices
// using the array "PedigreeId".  The user may alternately set the
// DirectMapping flag to indicate that the two structures must have directly
// corresponding offsets (i.e. node i in the graph must correspond to node i in
// the tree).
//
// .SECTION Thanks

#ifndef __vtkGraphTransferDataToTree_h
#define __vtkGraphTransferDataToTree_h

#include "vtkTreeAlgorithm.h"
#include "vtkVariant.h"

class VTK_INFOVIS_EXPORT vtkGraphTransferDataToTree : public vtkTreeAlgorithm 
{
public:
  static vtkGraphTransferDataToTree *New();

  vtkTypeRevisionMacro(vtkGraphTransferDataToTree,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If on, uses direct mapping from tree to graph vertices.
  // If off, both the graph and tree must contain PedigreeId arrays
  // which are used to match graph and tree vertices.
  // Default is off.
  vtkSetMacro(DirectMapping, bool);
  vtkGetMacro(DirectMapping, bool);
  vtkBooleanMacro(DirectMapping, bool);

  // Description:
  // The field name to use for storing the source array.
  vtkGetStringMacro(SourceArrayName);
  vtkSetStringMacro(SourceArrayName);

  // Description:
  // The field name to use for storing the source array.
  vtkGetStringMacro(TargetArrayName);
  vtkSetStringMacro(TargetArrayName);


  // Description:
  // Method to get/set the default value.
  //BTX
  vtkVariant GetDefaultValue();
  void SetDefaultValue(vtkVariant value);
  //ETX

  // Description:
  // Set the input type of the algorithm to vtkGraph.
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkGraphTransferDataToTree();
  ~vtkGraphTransferDataToTree();

  bool DirectMapping;
  char* SourceArrayName;
  char* TargetArrayName;

  //BTX
  vtkVariant DefaultValue;
  //ETX

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkGraphTransferDataToTree(const vtkGraphTransferDataToTree&);  // Not implemented.
  void operator=(const vtkGraphTransferDataToTree&);  // Not implemented.
};

#endif
