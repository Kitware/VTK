// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariPolyDataMapperNode
 * @brief   links vtkActor and vtkMapper to ANARI
 *
 * Parse VTK data and properties into the appropriate ANARI objects
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariPolyDataMapperNode_h
#define vtkAnariPolyDataMapperNode_h

#include "vtkPolyDataMapperNode.h"
#include "vtkRenderingAnariCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkActor;
class vtkAnariPolyDataMapperInheritInterface;
class vtkAnariPolyDataMapperNodeInternals;
class vtkAnariActorNode;
class vtkAnariSceneGraph;
class vtkPolyData;

class VTKRENDERINGANARICORE_EXPORT vtkAnariPolyDataMapperNode : public vtkPolyDataMapperNode
{
public:
  static vtkAnariPolyDataMapperNode* New();
  vtkTypeMacro(vtkAnariPolyDataMapperNode, vtkPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Ensure this node has been initialized.
   */
  void Build(bool prepass) override;
  /**
   * Sync ANARIGeometry + ANARIMaterial parameters with vtkPolyData.
   */
  void Synchronize(bool prepass) override;
  /**
   * Make ANARI calls to render me.
   */
  void Render(bool prepass) override;
  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

protected:
  vtkAnariPolyDataMapperNode();
  ~vtkAnariPolyDataMapperNode() override;

  vtkActor* GetVtkActor() const;
  vtkAnariActorNode* GetAnariActorNode() const;
  bool ActorWasModified() const;
  void RenderSurfaceModels();
  void ClearSurfaces();
  void SetActorNodeName();

  /**
   * Return true if the inherit interface is not a nullptr.
   */
  bool InheritInterfaceInitialized() const;

  /**
   * Convenience function to create an inherit interface. Arguments can be given to construct the
   * interface class.
   */
  template <typename T, typename... Args>
  void CreateInheritInterface(Args... args)
  {
    static_assert(std::is_base_of<vtkAnariPolyDataMapperInheritInterface, T>::value,
      "vtkAnariPolyDataMapperNode::CreateInheritInterface, T should be a base of "
      "vtkAnariPolyDataMapperInheritInterface");
    std::shared_ptr<T> interface = std::make_shared<T>(std::forward<Args>(args)...);
    this->SetInheritInterface(interface);
  }

  void AnariRenderPoly(vtkAnariActorNode* anariActorNode, vtkPolyData* poly, double* diffuse,
    double opacity, const std::string& materialName);

  vtkAnariPolyDataMapperNodeInternals* Internal{ nullptr };
  vtkAnariSceneGraph* RendererNode{ nullptr };

private:
  vtkAnariPolyDataMapperNode(const vtkAnariPolyDataMapperNode&) = delete;
  void operator=(const vtkAnariPolyDataMapperNode&) = delete;

  /**
   * Set the inherit interface to use internally.
   * Inherit interface is used to override rendering functionality for child of this class.
   */
  void SetInheritInterface(
    std::shared_ptr<vtkAnariPolyDataMapperInheritInterface> inheritInterface);

  vtkMTimeType PolyDataMTime = 0;
};

VTK_ABI_NAMESPACE_END
#endif
