/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexLinks.h

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
// .NAME vtkVertexLinks - object represents pointers from vertices to a list of edges
//
// .SECTION Description
// vtkVertexLinks is a supplemental object to vtkGraph and vtkTree, 
// enabling access from vertices to the incident arcs. vtkVertexLinks is
// a list of vertices, each vertex represents a dynamic list of arc id's incident to
// the vertex. The information provided by this object can be used to determine 
// neighbors and construct other local topological information.
//
// For vtkGraph, arc ids are stored, but for vtkTree, vertex ids are stored.
//
// .SECTION Thanks
// Thanks to Ken Moreland for his suggestions for this class.
// Thanks also to David Thompson for creating the freerange class used to
// efficiently allocate / deallocate the adjacency arrays.


#ifndef __vtkVertexLinks_h
#define __vtkVertexLinks_h

#include "vtkObject.h"

//BTX
struct vtkVertexLinksInternals;
//ETX

class VTK_FILTERING_EXPORT vtkVertexLinks : public vtkObject 
{
public:

  static vtkVertexLinks *New();
  vtkTypeRevisionMacro(vtkVertexLinks,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set a pointer to the adjacent objects for a specific vertex,
  // along with the number of adjacent objects.
  void GetAdjacent(vtkIdType vertex, vtkIdType& nadj, const vtkIdType*& adj);
  void GetInAdjacent(vtkIdType vertex, vtkIdType& nadj, const vtkIdType*& adj);
  void GetOutAdjacent(vtkIdType vertex, vtkIdType& nadj, const vtkIdType*& adj);

  // Description:
  // Return the number of objects adjacent to a vertex.
  vtkIdType GetDegree(vtkIdType vertex);
  vtkIdType GetInDegree(vtkIdType vertex);
  vtkIdType GetOutDegree(vtkIdType vertex);

  // Description:
  // The number of vertices stored in this structure.
  vtkIdType GetNumberOfVertices();

  // Description:
  // Add a vertex and return its ID.
  vtkIdType AddVertex();

  // Description:
  // Remove a vertex by copying the last vertex (with ID NumberOfVertices - 1) over
  // the deleted vertex.  Returns the old ID of the moved vertex (i.e. NumberOfVertices - 1).
  vtkIdType RemoveVertex(vtkIdType vertex);

  // Description:
  // Add an adjacent ID to the vertex's incoming adjacency list.
  void AddInAdjacent(vtkIdType vertex, vtkIdType adj);

  // Description:
  // Add an adjacent ID to the vertex's outgoing adjacency list.
  void AddOutAdjacent(vtkIdType vertex, vtkIdType adj);

  // Description:
  // Remove an adjacent ID from the vertex's incoming adjacency list.
  void RemoveInAdjacent(vtkIdType vertex, vtkIdType adj);

  // Description:
  // Remove an adjacent ID from the vertex's outgoing adjacency list.
  // Moves the final out ID into the position of the removed ID.
  void RemoveOutAdjacent(vtkIdType vertex, vtkIdType adj);

  // Description:
  // Remove an adjacent ID from the vertex's outgoing adjacency list.
  // Shifts out IDs after the removed ID back one position.
  // This is slower than RemoveOutAdjacent but preserves order.
  void RemoveOutAdjacentShift(vtkIdType vertex, vtkIdType adj);

  // Description:
  // Get the adjacent ID from the vertex's incoming adjacency list at an index.
  vtkIdType GetInAdjacent(vtkIdType vertex, vtkIdType index);

  // Description:
  // Get the adjacent ID from the vertex's outgoing adjacency list at an index.
  vtkIdType GetOutAdjacent(vtkIdType vertex, vtkIdType index);

  // Description:
  // Set the adjacent ID from the vertex's incoming adjacency list at an index.
  void SetInAdjacent(vtkIdType vertex, vtkIdType index, vtkIdType value);

  // Description:
  // Set the adjacent ID from the vertex's outgoing adjacency list at an index.
  void SetOutAdjacent(vtkIdType vertex, vtkIdType index, vtkIdType value);

  // Description:
  // Reset to a state of no entries without freeing the memory.
  void Reset();

  // Description:
  // Return the memory in kilobytes consumed by this cell links array. 
  // Used to support streaming and reading/writing data. The value 
  // returned is guaranteed to be greater than or equal to the memory 
  // required to actually represent the data represented by this object. 
  // The information returned is valid only after the pipeline has 
  // been updated.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Standard DeepCopy method.  Since this object contains no reference
  // to other objects, there is no ShallowCopy.
  void DeepCopy(vtkVertexLinks *src);

protected:
  vtkVertexLinks();
  ~vtkVertexLinks();

  vtkVertexLinksInternals* Internals;

  // Description:
  // Change the length of a vertex's adjacency list by the size specified.
  void ResizeVertexList(vtkIdType vertex, vtkIdType size);

private:
  vtkVertexLinks(const vtkVertexLinks&);  // Not implemented.
  void operator=(const vtkVertexLinks&);  // Not implemented.
};

#endif

