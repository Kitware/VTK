/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariWindowNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAnariWindowNode
 * @brief   links vtkRendererWindows to ANARI
 *
 * Translates vtkRenderWindow state into ANARI rendering calls
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariWindowNode_h
#define vtkAnariWindowNode_h

#include "vtkRenderingAnariModule.h" // For export macro
#include "vtkWindowNode.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGANARI_EXPORT vtkAnariWindowNode : public vtkWindowNode
{
public:
  static vtkAnariWindowNode* New();
  vtkTypeMacro(vtkAnariWindowNode, vtkWindowNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ANARI calls to make visible.
   */
  void Render(bool prepass) override;

protected:
  vtkAnariWindowNode();
  ~vtkAnariWindowNode() = default;

private:
  vtkAnariWindowNode(const vtkAnariWindowNode&) = delete;
  void operator=(const vtkAnariWindowNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
