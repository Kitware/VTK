// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVtkJSViewNodeFactory
 * @brief   Constructs view nodes for traversing a scene for vtk-js
 *
 * vtkVtkJSViewNodeFactory constructs view nodes that are subsequently executed
 * as a scene graph is traversed. The generated view nodes inherit from
 * vtkViewNode and augment the synchronize and render traversal steps to
 * construct Json representations of the scene elements and to update the
 * pipelines associated with the datasets to render, respectively.
 *
 *
 * @sa
 * vtkVtkJSSceneGraphSerializer
 */

#ifndef vtkVtkJSViewNodeFactory_h
#define vtkVtkJSViewNodeFactory_h

#include "vtkRenderingVtkJSModule.h" // For export macro

#include "vtkViewNodeFactory.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkVtkJSSceneGraphSerializer;

class VTKRENDERINGVTKJS_EXPORT vtkVtkJSViewNodeFactory : public vtkViewNodeFactory
{
public:
  static vtkVtkJSViewNodeFactory* New();
  vtkTypeMacro(vtkVtkJSViewNodeFactory, vtkViewNodeFactory);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the Serializer object
   */
  void SetSerializer(vtkVtkJSSceneGraphSerializer*);
  vtkGetObjectMacro(Serializer, vtkVtkJSSceneGraphSerializer);
  ///@}

protected:
  vtkVtkJSViewNodeFactory();
  ~vtkVtkJSViewNodeFactory() override;

  vtkVtkJSSceneGraphSerializer* Serializer;

private:
  vtkVtkJSViewNodeFactory(const vtkVtkJSViewNodeFactory&) = delete;
  void operator=(const vtkVtkJSViewNodeFactory&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
