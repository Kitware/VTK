/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkViewNode - a node within a VTK scene graph
// .SECTION Description
// This is the superclass for all nodes within a VTK scene graph. It
// contains the API for a node. It supports the essential operations such
// as graph creation, state storage and traversal. Child classes adapt this
// to VTK's major rendering classes. Grandchild classes adapt those to
// for APIs of different rendering libraries.

#ifndef vtkViewNode_h
#define vtkViewNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkWeakPointer.h" //avoid ref loop to parent
#include "vtkObject.h"

class vtkCollection;
class vtkViewNodeFactory;
class vtkViewNodeCollection;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkViewNode :
  public vtkObject
{
public:
  vtkTypeMacro(vtkViewNode, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //This is the VTK class that this node stands in for.
  vtkGetObjectMacro(Renderable, vtkObject);

  //Description:
  //Builds myself.
  virtual void Build(bool /* prepass */) {};

  //Description:
  //Ensures that my state agrees with my Renderable's.
  virtual void Synchronize(bool /* prepass */) {};

  //Description:
  //Makes calls to make self visible.
  virtual void Render(bool /*prepass*/) {};

  //Description:
  //Access the node that owns this one.
  virtual void SetParent(vtkViewNode*);
  virtual vtkViewNode * GetParent();

  //Description:
  //Access nodes that this one owns.
  virtual void SetChildren(vtkViewNodeCollection*);
  vtkGetObjectMacro(Children, vtkViewNodeCollection);

  //Description:
  //A factory that creates particular subclasses for different
  //rendering back ends.
  virtual void SetMyFactory(vtkViewNodeFactory*);
  vtkGetObjectMacro(MyFactory, vtkViewNodeFactory);

  //Description:
  //Returns the view node that corresponding to the provided object
  //Will return NULL if a match is not found in self or descendents
  vtkViewNode* GetViewNodeFor(vtkObject *);

  // Description:
  // Find the first parent/grandparent of the desired type
  vtkViewNode *GetFirstAncestorOfType(const char *type);

  // Description:
  // Alow explicit setting of the renderable for a
  // view node.
  virtual void SetRenderable(vtkObject *);

  // if you want to traverse your children in a specific order
  // or way override this method
  virtual void Traverse(int operation);

  virtual void TraverseAllPasses();

  //Descriptions:
  //Allows smart caching
  unsigned long RenderTime;

  //Description:
  //internal mechanics of graph traversal and actions
  enum operation_type{noop, build, synchronize, render};

protected:
  vtkViewNode();
  ~vtkViewNode();

  static const char* operation_type_strings[];

  void Apply(int operation, bool prepass);

  // Description::
  //convienience method to add node or nodes
  //if missing from our current list
  void AddMissingNode(vtkObject *obj);
  void AddMissingNodes(vtkCollection *col);

  // Description::
  // Called first before adding missing nodes.
  // Keeps track of the nodes that should be in the collection
  void PrepareNodes();
  vtkCollection *PreparedNodes;

  // Description:
  // Called after PrepareNodes and AddMissingNodes
  // removes any extra leftover nodes
  void RemoveUnusedNodes();

  //Description:
  //Create the correct ViewNode subclass for the passed in object.
  virtual vtkViewNode *CreateViewNode(vtkObject *obj);

  vtkObject *Renderable;
  vtkWeakPointer<vtkViewNode> Parent;
  vtkViewNodeCollection *Children;
  vtkViewNodeFactory *MyFactory;

  friend class vtkViewNodeFactory;

private:
  vtkViewNode(const vtkViewNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkViewNode&) VTK_DELETE_FUNCTION;
};

#endif
