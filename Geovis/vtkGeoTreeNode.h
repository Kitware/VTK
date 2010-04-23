/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTreeNode.h

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

// .NAME vtkGeoTreeNode - Stores data for a patch of the globe.
//
// .SECTION Description
// A self-referential data structure for storing geometry or imagery for
// the geospatial views. The data is organized in a quadtree. Each node
// contains a pointer to its parent and owns references to its four
// child nodes. The ID of each node is unique in its level, and encodes
// the path from the root node in its bits.

// .SECTION See Also
// vtkGeoView vtkGeoView2D vtkGeoTerrain vtkGeoAlignedImageRepresentation
   
#ifndef __vtkGeoTreeNode_h
#define __vtkGeoTreeNode_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // for SP

class vtkPolyData;

class VTK_GEOVIS_EXPORT vtkGeoTreeNode : public vtkObject
{
public:
  static vtkGeoTreeNode *New();
  vtkTypeMacro(vtkGeoTreeNode, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The id uniquely specified this node.
  // For this implementation I am going to store the branch path
  // in the bits.
  vtkSetMacro(Id,unsigned long);
  vtkGetMacro(Id,unsigned long);
  
  // Description;
  // Knowing the level simplifies encoding the branch trace in the Id.
  vtkSetMacro(Level, int);
  vtkGetMacro(Level, int);
  
  // Description:
  // Longitude and latitude range of the terrain model.
  vtkSetVector2Macro(LongitudeRange,double);
  vtkGetVector2Macro(LongitudeRange,double);
  vtkSetVector2Macro(LatitudeRange,double);
  vtkGetVector2Macro(LatitudeRange,double);
  
  // Description:
  // Get a child of this node. If one is set, then they all should
  // set.  No not mix subclasses.
  void SetChild(vtkGeoTreeNode* node, int idx);

  // Description:
  // When we merge children to a lower resolution parent, we need
  // this reference.  It is not referenced counted to avoid reference loops.
  // A child should never exist when the parent is destructed anyway.
  void SetParent(vtkGeoTreeNode* node)
    { this->Parent = node; }

  // Description:
  // Manage links to older and newer tree nodes.
  // These are used to periodically delete unused patches.
  void SetOlder(vtkGeoTreeNode* node)
    { this->Older = node; }
  vtkGeoTreeNode* GetOlder()
    { return this->Older; }
  void SetNewer(vtkGeoTreeNode* node)
    { this->Newer = node; }
  vtkGeoTreeNode* GetNewer()
    { return this->Newer; }

  // Description:
  // Returns whether this node has valid data associated
  // with it, or if it is an "empty" node.
  virtual bool HasData()
    { return false; }

  // Description:
  // Deletes the data associated with the node to make this
  // an "empty" node. This is performed when the node has
  // been unused for a certain amount of time.
  virtual void DeleteData()
    { }

  // Decription:
  // Get this nodes child index in node's parent.
  int GetWhichChildAreYou();

  // Description:
  // This method returns true if this node descends from the
  // elder node.  The descision is made from the node ids, so the nodes do
  // not have to be in the same tree!
  bool IsDescendantOf(vtkGeoTreeNode* elder);

  // Decription:
  // Create children of the same type as parent.
  // Id, level and Latitude-Longitude ranges are set.
  // Returns VTK_ERROR if level gets too deep to create children.
  int CreateChildren();

  // Description:
  // Get the child as a vtkGeoTreeNode.
  // Subclasses also implement GetChild() which returns the child
  // as the appropriate subclass type.
  vtkGeoTreeNode* GetChildTreeNode(int idx)
    { return this->Children[idx]; }

  // Description:
  // Get the parent as a vtkGeoTreeNode.
  // Subclasses also implement GetParent() which returns the parent
  // as the appropriate subclass type.
  vtkGeoTreeNode* GetParentTreeNode()
    { return this->Parent; }

//BTX
  enum NodeStatus
    {
    NONE,
    PROCESSING
    };

  NodeStatus GetStatus();
  void SetStatus(NodeStatus status);
//ETX

  // Description:
  // Shallow and Deep copy. Deep copy performs a shallow copy
  // of the Child nodes.
  virtual void ShallowCopy(vtkGeoTreeNode *src);  
  virtual void DeepCopy(vtkGeoTreeNode *src);

protected:
  vtkGeoTreeNode();
  ~vtkGeoTreeNode();

  int Level;
  unsigned long Id;

  double LongitudeRange[2];
  double LatitudeRange[2];

  //BTX
  vtkSmartPointer<vtkGeoTreeNode> Children[4];
  vtkGeoTreeNode * Parent;
  NodeStatus Status;
  vtkGeoTreeNode* Older;
  vtkGeoTreeNode* Newer;
  //ETX

private:
  vtkGeoTreeNode(const vtkGeoTreeNode&);  // Not implemented.
  void operator=(const vtkGeoTreeNode&);  // Not implemented.
};

#endif
