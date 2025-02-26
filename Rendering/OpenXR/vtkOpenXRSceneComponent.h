// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenXRSceneComponent_h
#define vtkOpenXRSceneComponent_h

#include "vtkObject.h"
#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer

#include <array>   // For std::array
#include <cstdint> // For int types
#include <memory>  // For std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkMatrix4x4;

/**
 * vtkOpenXRSceneComponent represent a component in the XR scene.
 * This class is instanciated by vtkOpenXRSceneObserver when a new component has been detected
 * by the OpenXR runtime.
 *
 * This class is designed as a variant, where the component data varies depending on its type.
 *
 * @example
 * ```cpp
 * void OnNewComponent(vtkObject* observer, unsigned long event, void* calldata)
 * {
 *   auto* component = static_cast<vtkOpenXRSceneComponent*>(calldata);
 *   if(component->GetType() == vtkOpenXRSceneComponent::Marker)
 *   {
 *     someActor->SetUserMatrix(component->GetMarkerPose());
 *   }
 * }
 * ```
 */
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRSceneComponent : public vtkObject
{
  struct vtkInternals;

public:
  static vtkOpenXRSceneComponent* New();
  vtkTypeMacro(vtkOpenXRSceneComponent, vtkObject);

  enum ComponentTypes
  {
    Unknown, // May be used in case the runtime supports more than we know of
    Marker   // Only QrCode at this time
  };

  /**
   * Return last modified time given by the runtime. This value is valid for any component, but may
   * be updated differently by the runtime depending of component type.
   */
  vtkGetMacro(LastModifiedTime, int64_t);

  /**
   * Return component type
   */
  vtkGetMacro(Type, ComponentTypes);

  /**
   * This is the main represention of the marker.
   * The returned object is garanteed to be kept alive and will be updated
   * if the marker pose changes.
   * This means that this matrix will be `Modified` accordingly,
   * and thus can be used as a pipeline input!
   * Translation of matrix is position in world coordinates.
   * Rotation is orientation.
   */
  vtkMatrix4x4* GetMarkerPose() const;

  /**
   * QrCode decoded text if any
   */
  const std::string& GetMarkerText() const;

  ///@{
  /**
   * Marker physical size in meters (rectangle)
   */
  double GetMarkerWidth() const;
  double GetMarkerHeight() const;
  ///@}

  /**
   * Initialize the component for a given type, this changes the active representation.
   */
  void Initialize(ComponentTypes type);

  /**
   * Update internal representation of Markers
   */
  void UpdateMarkerRepresentation(
    int64_t lastModifiedTime, vtkMatrix4x4* matrix, double width, double height, std::string text);

protected:
  vtkOpenXRSceneComponent();
  ~vtkOpenXRSceneComponent() override;

private:
  vtkOpenXRSceneComponent(const vtkOpenXRSceneComponent&) = delete;
  void operator=(const vtkOpenXRSceneComponent&) = delete;

  int64_t LastModifiedTime{ 0 };
  ComponentTypes Type{ Unknown };
  std::unique_ptr<vtkInternals> Impl{};
};

VTK_ABI_NAMESPACE_END

#endif
