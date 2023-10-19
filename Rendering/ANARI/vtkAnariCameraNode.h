/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariCameraNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAnariCameraNode
 * @brief   links vtkCamera to ANARI
 *
 * Translates vtkCamera state into ANARICamera state
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariCameraNode_h
#define vtkAnariCameraNode_h

#include "vtkCameraNode.h"
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGANARI_EXPORT vtkAnariCameraNode : public vtkCameraNode
{
public:
  static vtkAnariCameraNode* New();
  vtkTypeMacro(vtkAnariCameraNode, vtkCameraNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ANARI calls to render me.
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

protected:
  vtkAnariCameraNode() = default;
  ~vtkAnariCameraNode() = default;

private:
  vtkAnariCameraNode(const vtkAnariCameraNode&) = delete;
  void operator=(const vtkAnariCameraNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
