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

// .NAME vtkGeoTreeNode - 
// .SECTION Description I wanted to hide the normal vtkCamera API

// .SECTION See Also
   
#ifndef __vtkGeoTreeNode_h
#define __vtkGeoTreeNode_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // for SP

class vtkPolyData;

class VTK_GEOVIS_EXPORT vtkGeoTreeNode : public vtkObject
{
public:
  static vtkGeoTreeNode *New();
  vtkTypeRevisionMacro(vtkGeoTreeNode, vtkObject);
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
  void SetParent(vtkGeoTreeNode* node) {this->Parent = node;}

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
//ETX
  

private:
  vtkGeoTreeNode(const vtkGeoTreeNode&);  // Not implemented.
  void operator=(const vtkGeoTreeNode&);  // Not implemented.
};

#endif
