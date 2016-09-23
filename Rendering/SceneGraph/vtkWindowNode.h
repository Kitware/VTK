/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindowNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWindowNode
 * @brief   vtkViewNode specialized for vtkRenderWindows
 *
 * State storage and graph traversal for vtkRenderWindow
*/

#ifndef vtkWindowNode_h
#define vtkWindowNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

class vtkUnsignedCharArray;
class vtkFloatArray;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkWindowNode :
  public vtkViewNode
{
public:
  static vtkWindowNode* New();
  vtkTypeMacro(vtkWindowNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Build containers for our child nodes.
   */
  virtual void Build(bool prepass);

  /**
   * Get state of my renderable.
   */
  virtual void Synchronize(bool prepass);

  /**
   * Return the size of the last rendered image
   */
  virtual int *GetSize() {
    return this->Size; }

  /**
   * Get the most recent color buffer RGBA
   */
  virtual vtkUnsignedCharArray *GetColorBuffer()
    { return this->ColorBuffer; }

  /**
   * Get the most recent zbufer buffer
   */
  virtual vtkFloatArray *GetZBuffer()
    { return this->ZBuffer; }

protected:
  vtkWindowNode();
  ~vtkWindowNode();

  //TODO: use a map with string keys being renderable's member name
  //state
  int Size[2];

  // stores the results of a render
  vtkUnsignedCharArray*ColorBuffer;
  vtkFloatArray *ZBuffer;

private:
  vtkWindowNode(const vtkWindowNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWindowNode&) VTK_DELETE_FUNCTION;
};

#endif
