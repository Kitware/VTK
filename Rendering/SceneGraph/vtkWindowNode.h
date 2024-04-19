// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkUnsignedCharArray;
class vtkFloatArray;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkWindowNode : public vtkViewNode
{
public:
  static vtkWindowNode* New();
  vtkTypeMacro(vtkWindowNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  void Build(bool prepass) override;

  /**
   * Get state of my renderable.
   */
  void Synchronize(bool prepass) override;

  /**
   * Return the size of the last rendered image
   */
  virtual int* GetSize() { return this->Size; }

  /**
   * Get the most recent color buffer RGBA
   */
  virtual vtkUnsignedCharArray* GetColorBuffer() { return this->ColorBuffer; }

  /**
   * Get the most recent zbuffer buffer
   */
  virtual vtkFloatArray* GetZBuffer() { return this->ZBuffer; }

protected:
  vtkWindowNode();
  ~vtkWindowNode() override;

  // TODO: use a map with string keys being renderable's member name
  // state
  int Size[2];

  // stores the results of a render
  vtkUnsignedCharArray* ColorBuffer;
  vtkFloatArray* ZBuffer;

private:
  vtkWindowNode(const vtkWindowNode&) = delete;
  void operator=(const vtkWindowNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
