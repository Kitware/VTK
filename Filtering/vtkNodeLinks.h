/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNodeLinks.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNodeLinks - object represents pointers from nodes to a list of edges
//
// .SECTION Description
// vtkNodeLinks is a supplemental object to vtkGraph and vtkTree, 
// enabling access from nodes to the incident arcs. vtkNodeLinks is
// a list of nodes, each node represents a dynamic list of arc id's incident to
// the node. The information provided by this object can be used to determine 
// neighbors and construct other local topological information.
//
// For vtkGraph, arc ids are stored, but for vtkTree, node ids are stored.
//
// .SECTION Thanks
// Thanks to Ken Moreland for his suggestions for this class.
// Thanks also to David Thompson for creating the freerange class used to
// efficiently allocate / deallocate the adjacency arrays.


#ifndef __vtkNodeLinks_h
#define __vtkNodeLinks_h

#include "vtkObject.h"

//BTX
struct vtkNodeLinksInternals;
//ETX

class VTK_FILTERING_EXPORT vtkNodeLinks : public vtkObject 
{
public:

  static vtkNodeLinks *New();
  vtkTypeRevisionMacro(vtkNodeLinks,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set a pointer to the adjacent objects for a specific node,
  // along with the number of adjacent objects.
  void GetAdjacent(vtkIdType node, vtkIdType& nadj, const vtkIdType*& adj);
  void GetInAdjacent(vtkIdType node, vtkIdType& nadj, const vtkIdType*& adj);
  void GetOutAdjacent(vtkIdType node, vtkIdType& nadj, const vtkIdType*& adj);

  // Description:
  // Return the number of objects adjacent to a node.
  vtkIdType GetDegree(vtkIdType node);
  vtkIdType GetInDegree(vtkIdType node);
  vtkIdType GetOutDegree(vtkIdType node);

  // Description:
  // The number of nodes stored in this structure.
  vtkIdType GetNumberOfNodes();

  // Description:
  // Add a node and return its ID.
  vtkIdType AddNode();

  // Description:
  // Remove a node by copying the last node (with ID NumberOfNodes - 1) over
  // the deleted node.  Returns the old ID of the moved node (i.e. NumberOfNodes - 1).
  vtkIdType RemoveNode(vtkIdType node);

  // Description:
  // Add an adjacent ID to the node's incoming adjacency list.
  void AddInAdjacent(vtkIdType node, vtkIdType adj);

  // Description:
  // Add an adjacent ID to the node's outgoing adjacency list.
  void AddOutAdjacent(vtkIdType node, vtkIdType adj);

  // Description:
  // Remove an adjacent ID from the node's incoming adjacency list.
  void RemoveInAdjacent(vtkIdType node, vtkIdType adj);

  // Description:
  // Remove an adjacent ID from the node's outgoing adjacency list.
  // Moves the final out ID into the position of the removed ID.
  void RemoveOutAdjacent(vtkIdType node, vtkIdType adj);

  // Description:
  // Remove an adjacent ID from the node's outgoing adjacency list.
  // Shifts out IDs after the removed ID back one position.
  // This is slower than RemoveOutAdjacent but preserves order.
  void RemoveOutAdjacentShift(vtkIdType node, vtkIdType adj);

  // Description:
  // Get the adjacent ID from the node's incoming adjacency list at an index.
  vtkIdType GetInAdjacent(vtkIdType node, vtkIdType index);

  // Description:
  // Get the adjacent ID from the node's outgoing adjacency list at an index.
  vtkIdType GetOutAdjacent(vtkIdType node, vtkIdType index);

  // Description:
  // Set the adjacent ID from the node's incoming adjacency list at an index.
  void SetInAdjacent(vtkIdType node, vtkIdType index, vtkIdType value);

  // Description:
  // Set the adjacent ID from the node's outgoing adjacency list at an index.
  void SetOutAdjacent(vtkIdType node, vtkIdType index, vtkIdType value);

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
  void DeepCopy(vtkNodeLinks *src);

protected:
  vtkNodeLinks();
  ~vtkNodeLinks();

  vtkNodeLinksInternals* Internals;

  // Description:
  // Change the length of a node's adjacency list by the size specified.
  void ResizeNodeList(vtkIdType node, vtkIdType size);

private:
  vtkNodeLinks(const vtkNodeLinks&);  // Not implemented.
  void operator=(const vtkNodeLinks&);  // Not implemented.
};

#endif

