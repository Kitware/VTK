/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkGraphInternals - Internal representation of vtkGraph
//
// .SECTION Description
// This is the internal representation of vtkGraph, used only in rare cases 
// where one must modify that representation.

#ifndef __vtkGraphInternals_h
#define __vtkGraphInternals_h

#include "vtkGraph.h"
#include "vtkObject.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/map>

//----------------------------------------------------------------------------
// class vtkVertexAdjacencyList
//----------------------------------------------------------------------------
//BTX
class vtkVertexAdjacencyList
{
public:
  vtksys_stl::vector<vtkInEdgeType> InEdges;
  vtksys_stl::vector<vtkOutEdgeType> OutEdges;
};
//ETX

//----------------------------------------------------------------------------
// class vtkGraphInternals
//----------------------------------------------------------------------------
class vtkGraphInternals : public vtkObject
{
public:
  static vtkGraphInternals *New();
  //BTX
  vtkTypeRevisionMacro(vtkGraphInternals, vtkObject);
  vtksys_stl::vector<vtkVertexAdjacencyList> Adjacency;
  //ETX
  vtkIdType NumberOfEdges;
  vtkDistributedGraphHelper *DistributedHelper;

protected:
  vtkGraphInternals();
  ~vtkGraphInternals();

private:
  vtkGraphInternals(const vtkGraphInternals&);  // Not implemented.
  void operator=(const vtkGraphInternals&);  // Not implemented.
};

#endif // __vtkGraphInternals_h

