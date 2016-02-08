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
#include "vtkObject.h"

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
  //Call to build self and descendents.
  void Build();

  //Description:
  //Builds myself.
  //Sublcasses must implement.
  virtual void BuildSelf() = 0;

  //Description:
  //Call to ensure that my state (including that of descendents)
  //agrees with Renderable's.
  void Synchronize();

  //Description:
  //Ensures that my state agrees with my Renderable's.
  //Sublcasses must implement.
  virtual void SynchronizeSelf() = 0;

  //Description:
  //Call to make self and descendents visible.
  virtual void Render();

  //Description:
  //Makes calls to make self visible.
  //Sublcasses must implement.
  virtual void RenderSelf() = 0;

  //Description:
  //Access the node that owns this one.
  virtual void SetParent(vtkViewNode*);
  vtkGetObjectMacro(Parent, vtkViewNode);

  //Description:
  //Access nodes that this one owns.
  virtual void SetChildren(vtkViewNodeCollection*);
  vtkGetObjectMacro(Children, vtkViewNodeCollection);

  //Description:
  //A factory that creates particular subclasses for different
  //rendering back ends.
  virtual void SetMyFactory(vtkViewNodeFactory*);
  vtkGetObjectMacro(MyFactory, vtkViewNodeFactory);

protected:
  vtkViewNode();
  ~vtkViewNode();

  //Description:
  //internal mechanics of graph traversal and actions
  enum operation_type{noop, build, synchronize, render};
  static const char* operation_type_strings[];
  void Traverse(int operation);
  void Apply(int operation);

  //Description:
  //Create the correct ViewNode subclass for the passed in object.
  virtual vtkViewNode *CreateViewNode(vtkObject *obj);

  vtkObject *Renderable;
  vtkViewNode *Parent;
  vtkViewNodeCollection *Children;
  vtkViewNodeFactory *MyFactory;

  friend class vtkViewNodeFactory;
  void SetRenderable(vtkObject *);

private:
  vtkViewNode(const vtkViewNode&); // Not implemented.
  void operator=(const vtkViewNode&); // Not implemented.
};

#endif
