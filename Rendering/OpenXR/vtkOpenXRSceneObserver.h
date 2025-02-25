// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenXRSceneObserver_h
#define vtkOpenXRSceneObserver_h

#include "vtkObject.h"
#include "vtkRenderingOpenXRModule.h" // For export macro

#include <memory> // For std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkOpenXRRenderWindow;
class vtkOpenXRSceneComponent;

/**
 * `vtkOpenXRSceneObserver` is a wrapper around OpenXR scene understanding extensions
 *
 * `vtkOpenXRSceneObserver` uses an event based mecanism to retrieve components.
 * `vtkOpenXRSceneObserver` will invoke `vtkCommand::UpdateDataEvent`
 * when a new component is detected by the runtime.
 * This event will forward the new component as calldata.
 *
 * A `vtkOpenXRSceneObserver` is instanciated, initialized and updated by `vtkOpenXRRenderWindow`
 * if `vtkOpenXRRenderWindow::EnableSceneUnderstanding` is `true`.
 * `vtkOpenXRSceneObserver::Initialize` will be called by `vtkOpenXRRenderWindow` when initialized
 * `vtkOpenXRSceneObserver::UpdateSceneData` is
 * automatically called by `vtkOpenXRRenderWindow::Render()` for the window scene observer.
 *
 * You may instantiate it manually and handle it's lifetime and updates on your own.
 *
 * @see `vtkOpenXRRenderWindow::SetEnableSceneUnderstanding`
 * @see `vtkOpenXRRenderWindow::GetSceneObserver`
 * @see `vtkOpenXRSceneComponent`
 *
 * @example
 * ```cpp
 * void OnNewComponent(vtkObject* object, unsigned long, void* calldata) {
 *   auto* component = static_cast<vtkOpenXRSceneComponent*>(calldata);
 * }
 *
 * vtkNew<vtkOpenXRRemotingRenderWindow> renderWindow;
 * renderWindow->SetEnableSceneUnderstanding(true);
 * renderWindow->Initialize();
 *
 * vtkOpenXRSceneObserver* sceneObserver = renderWindow->GetSceneObserver();
 * assert(sceneObserver && "Something went wrong!");
 *
 * vtkNew<vtkCallbackCommand> callback;
 * callback->SetCallback(OnNewComponent);
 *
 * sceneObserver->AddObserver(vtkCommand::UpdateDataEvent, callback);
 * ```
 */
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRSceneObserver : public vtkObject
{
  struct vtkInternals;

public:
  static vtkOpenXRSceneObserver* New();
  vtkTypeMacro(vtkOpenXRSceneObserver, vtkObject);

  /**
   * Creates runtime scene observer.
   */
  bool Initialize();

  /**
   * Update scene data by polling the runtime.
   *
   * This function does nothing if less than MinimumInterval seconds elapsed since last call.
   */
  bool UpdateSceneData();

  ///@{
  /**
   * - SnapshotComplete: The runtime must return a scene that is a consistent and complete snapshot
   *   of the environment, inferring the size and shape of objects as needed where the objects were
   *   not directly observed, in order to generate a watertight representation of the scene.
   * - SnapshotIncompleteFast: The runtime must return a consistent snapshot of the scene with
   *   meshes that do not overlap adjacent meshes at their edges, but may skip returning objects
   *   with XrSceneObjectTypeMSFT of XR_SCENE_OBJECT_TYPE_INFERRED_MSFT in order to return
   *   the scene faster.
   * - OcclusionOptimized: The runtime may react to this value by computing scenes more quickly
   *   and reusing existing mesh buffer IDs more often to minimize app overhead,
   *   with potential tradeoffs such as returning meshes that are not watertight,
   *   meshes that overlap adjacent meshes at their edges to allow partial updates
   *   in the future, or other reductions in mesh quality that are less observable
   *   when mesh is used for occlusion only.
   *
   * Default: SnapshotComplete
   */
  enum SceneConsistency
  {
    // enum values from OpenXR
    SnapshotComplete = 1,
    SnapshotIncompleteFast = 2,
    OcclusionOptimized = 3,
  };
  vtkGetMacro(ComputeConsistency, SceneConsistency);
  vtkSetMacro(ComputeConsistency, SceneConsistency);
  ///@}

  ///@{
  /**
   * Finite positive radius of the clipping sphere, expressed in meters.
   *
   * Sphere is centered on the left eye position.
   *
   * This only filters what is retrieved from the runtime.
   * It has no effect on runtime environment analysis.
   *
   * Default: 2.0
   */
  vtkGetMacro(ClippingRadius, double);
  vtkSetClampMacro(ClippingRadius, double, 0.0, 1e100);
  ///@}

  /**
   * List of features. Not all features may be supported by the runtime,
   * and not all features have to be queried everytime.
   */
  enum SceneFeature
  {
    // enum values from XrSceneComputeFeatureMSFT
    Markers = 1000147000,
  };

  ///@{
  /**
   * Enable or disable or check availability of a scene feature.
   *
   * `EnableComputeFeature` returns `true` if the feature has been (or is already) enabled.
   *
   * By default, all supported features are enabled.
   */
  bool EnableComputeFeature(SceneFeature feature);
  void DisableComputeFeature(SceneFeature feature);
  bool IsComputeFeatureEnabled(SceneFeature feature) const;
  bool IsComputeFeatureSupported(SceneFeature feature) const;
  ///@}

  ///@{
  /**
   * Set the minimum interval between two runtime queries.
   *
   * Expressed in seconds.
   *
   * Default: 2.0
   */
  vtkSetMacro(MinimumInterval, double);
  vtkGetMacro(MinimumInterval, double);
  ///@}

protected:
  vtkOpenXRSceneObserver();
  ~vtkOpenXRSceneObserver() override;

private:
  vtkOpenXRSceneObserver(const vtkOpenXRSceneObserver&) = delete;
  void operator=(const vtkOpenXRSceneObserver&) = delete;

  bool CreateMSFTSceneObserver();

  std::unique_ptr<vtkInternals> Impl{};

  double MinimumInterval{ 2.0 };
  double ClippingRadius{ 2.0 };
  SceneConsistency ComputeConsistency{ SceneConsistency::SnapshotComplete };
};

VTK_ABI_NAMESPACE_END

#endif
