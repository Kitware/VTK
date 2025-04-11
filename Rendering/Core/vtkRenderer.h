// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderer
 * @brief   abstract specification for renderers
 *
 * vtkRenderer provides an abstract specification for renderers. A renderer
 * is an object that controls the rendering process for objects. Rendering
 * is the process of converting geometry, a specification for lights, and
 * a camera view into an image. vtkRenderer also performs coordinate
 * transformation between world coordinates, view coordinates (the computer
 * graphics rendering coordinate system), and display coordinates (the
 * actual screen coordinates on the display device). Certain advanced
 * rendering features such as two-sided lighting can also be controlled.
 *
 * @sa
 * vtkRenderWindow vtkActor vtkCamera vtkLight vtkVolume
 */

#ifndef vtkRenderer_h
#define vtkRenderer_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkViewport.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include "vtkActorCollection.h"  // Needed for access in inline members
#include "vtkVolumeCollection.h" // Needed for access in inline members

#include <array> // To store matrices

VTK_ABI_NAMESPACE_BEGIN
class vtkFXAAOptions;
class vtkRenderWindow;
class vtkVolume;
class vtkCuller;
class vtkActor;
class vtkActor2D;
class vtkCamera;
class vtkFrameBufferObjectBase;
class vtkInformation;
class vtkLightCollection;
class vtkCullerCollection;
class vtkLight;
class vtkHardwareSelector;
class vtkRendererDelegate;
class vtkRenderPass;
class vtkTexture;

class vtkRecti;
class vtkVector3d;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkRenderer : public vtkViewport
{
public:
  vtkTypeMacro(vtkRenderer, vtkViewport);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a vtkRenderer with a black background, a white ambient light,
   * two-sided lighting turned on, a viewport of (0,0,1,1), and backface
   * culling turned off.
   */
  static vtkRenderer* New();

  ///@{
  /**
   * Add/Remove different types of props to the renderer.
   * These methods are all synonyms to AddViewProp and RemoveViewProp.
   * They are here for convenience and backwards compatibility.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void AddActor(vtkProp* p);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void AddVolume(vtkProp* p);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void RemoveActor(vtkProp* p);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void RemoveVolume(vtkProp* p);
  ///@}

  /**
   * Add a light to the list of lights.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void AddLight(vtkLight*);

  /**
   * Remove a light from the list of lights.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void RemoveLight(vtkLight*);

  /**
   * Remove all lights from the list of lights.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void RemoveAllLights();

  /**
   * Return the collection of lights.
   */
  vtkLightCollection* GetLights();

  /**
   * Set the collection of lights.
   * We cannot name it SetLights because of TestSetGet
   * \pre lights_exist: lights!=0
   * \post lights_set: lights==this->GetLights()
   */
  void SetLightCollection(vtkLightCollection* lights);

  /**
   * Create and add a light to renderer.
   */
  void CreateLight();

  /**
   * Create a new Light sutible for use with this type of Renderer.
   * For example, a vtkMesaRenderer should create a vtkMesaLight
   * in this function.   The default is to just call vtkLight::New.
   */
  virtual vtkLight* MakeLight();

  ///@{
  /**
   * Turn on/off two-sided lighting of surfaces. If two-sided lighting is
   * off, then only the side of the surface facing the light(s) will be lit,
   * and the other side dark. If two-sided lighting on, both sides of the
   * surface will be lit.
   */
  vtkGetMacro(TwoSidedLighting, vtkTypeBool);
  vtkSetMacro(TwoSidedLighting, vtkTypeBool);
  vtkBooleanMacro(TwoSidedLighting, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the automatic repositioning of lights as the camera moves.
   * If LightFollowCamera is on, lights that are designated as Headlights
   * or CameraLights will be adjusted to move with this renderer's camera.
   * If LightFollowCamera is off, the lights will not be adjusted.

   * (Note: In previous versions of vtk, this light-tracking
   * functionality was part of the interactors, not the renderer. For
   * backwards compatibility, the older, more limited interactor
   * behavior is enabled by default. To disable this mode, turn the
   * interactor's LightFollowCamera flag OFF, and leave the renderer's
   * LightFollowCamera flag ON.)
   */
  vtkSetMacro(LightFollowCamera, vtkTypeBool);
  vtkGetMacro(LightFollowCamera, vtkTypeBool);
  vtkBooleanMacro(LightFollowCamera, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off a flag which disables the automatic light creation capability.
   * Normally in VTK if no lights are associated with the renderer, then a light
   * is automatically created. However, in special circumstances this feature is
   * undesirable, so the following boolean is provided to disable automatic
   * light creation. (Turn AutomaticLightCreation off if you do not want lights
   * to be created.)
   */
  vtkGetMacro(AutomaticLightCreation, vtkTypeBool);
  vtkSetMacro(AutomaticLightCreation, vtkTypeBool);
  vtkBooleanMacro(AutomaticLightCreation, vtkTypeBool);
  ///@}

  /**
   * Ask the lights in the scene that are not in world space
   * (for instance, Headlights or CameraLights that are attached to the
   * camera) to update their geometry to match the active camera.
   */
  virtual vtkTypeBool UpdateLightsGeometryToFollowCamera();

  /**
   * Return the collection of volumes.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkVolumeCollection* GetVolumes();

  /**
   * Return any actors in this renderer.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkActorCollection* GetActors();

  /**
   * Specify the camera to use for this renderer.
   */
  void SetActiveCamera(vtkCamera*);

  /**
   * Get the current camera. If there is not camera assigned to the
   * renderer already, a new one is created automatically.
   * This does *not* reset the camera.
   */
  vtkCamera* GetActiveCamera();

  /**
   * Create a new Camera sutible for use with this type of Renderer.
   * For example, a vtkMesaRenderer should create a vtkMesaCamera
   * in this function.   The default is to just call vtkCamera::New.
   */
  virtual vtkCamera* MakeCamera();

  ///@{
  /**
   * When this flag is off, the renderer will not erase the background
   * or the Zbuffer.  It is used to have overlapping renderers.
   * Both the RenderWindow Erase and Render Erase must be on
   * for the camera to clear the renderer.  By default, Erase is on.
   */
  vtkSetMacro(Erase, vtkTypeBool);
  vtkGetMacro(Erase, vtkTypeBool);
  vtkBooleanMacro(Erase, vtkTypeBool);
  ///@}

  ///@{
  /**
   * When this flag is off, render commands are ignored.  It is used to either
   * multiplex a vtkRenderWindow or render only part of a vtkRenderWindow.
   * By default, Draw is on.
   */
  vtkSetMacro(Draw, vtkTypeBool);
  vtkGetMacro(Draw, vtkTypeBool);
  vtkBooleanMacro(Draw, vtkTypeBool);
  ///@}

  /**
   * This function is called to capture an instance of vtkProp that requires
   * special handling during vtkRenderWindow::CaptureGL2PSSpecialProps().
   */
  int CaptureGL2PSSpecialProp(vtkProp*);

  /**
   * Set the prop collection object used during
   * vtkRenderWindow::CaptureGL2PSSpecialProps(). Do not call manually, this
   * is handled automatically by the render window.
   */
  void SetGL2PSSpecialPropCollection(vtkPropCollection*);

  /**
   * Add an culler to the list of cullers.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void AddCuller(vtkCuller*);

  /**
   * Remove an actor from the list of cullers.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void RemoveCuller(vtkCuller*);

  /**
   * Return the collection of cullers.
   */
  vtkCullerCollection* GetCullers();

  ///@{
  /**
   * Set the intensity of ambient lighting.
   */
  vtkSetVector3Macro(Ambient, double);
  vtkGetVectorMacro(Ambient, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the amount of time this renderer is allowed to spend
   * rendering its scene. This is used by vtkLODActor's.
   */
  vtkSetMacro(AllocatedRenderTime, double);
  virtual double GetAllocatedRenderTime();
  ///@}

  /**
   * Get the ratio between allocated time and actual render time.
   * TimeFactor has been taken out of the render process.
   * It is still computed in case someone finds it useful.
   * It may be taken away in the future.
   */
  virtual double GetTimeFactor();

  /**
   * CALLED BY vtkRenderWindow ONLY. End-user pass your way and call
   * vtkRenderWindow::Render().
   * Create an image. This is a superclass method which will in turn
   * call the DeviceRender method of Subclasses of vtkRenderer.
   */
  virtual void Render();

  /**
   * Create an image. Subclasses of vtkRenderer must implement this method.
   */
  virtual void DeviceRender() {}

  /**
   * Render opaque polygonal geometry. Default implementation just calls
   * UpdateOpaquePolygonalGeometry().
   * Subclasses of vtkRenderer that can deal with, e.g. hidden line removal must
   * override this method.
   */
  virtual void DeviceRenderOpaqueGeometry(vtkFrameBufferObjectBase* fbo = nullptr);

  /**
   * Render translucent polygonal geometry. Default implementation just call
   * UpdateTranslucentPolygonalGeometry().
   * Subclasses of vtkRenderer that can deal with depth peeling must
   * override this method.
   * If UseDepthPeeling and UseDepthPeelingForVolumes are true, volumetric data
   * will be rendered here as well.
   * It updates boolean ivar LastRenderingUsedDepthPeeling.
   */
  virtual void DeviceRenderTranslucentPolygonalGeometry(vtkFrameBufferObjectBase* fbo = nullptr);

  /**
   * Internal method temporarily removes lights before reloading them
   * into graphics pipeline.
   */
  virtual void ClearLights() {}

  /**
   * Clear the image to the background color.
   */
  virtual void Clear() {}

  /**
   * Returns the number of visible actors.
   */
  int VisibleActorCount();

  /**
   * Returns the number of visible volumes.
   */
  int VisibleVolumeCount();

  /**
   * Compute the bounding box of all the visible props
   * Used in ResetCamera() and ResetCameraClippingRange()
   */
  void ComputeVisiblePropBounds(double bounds[6]);

  /**
   * Wrapper-friendly version of ComputeVisiblePropBounds
   */
  double* ComputeVisiblePropBounds() VTK_SIZEHINT(6);

  /**
   * Reset the camera clipping range based on the bounds of the
   * visible actors. This ensures that no props are cut off
   */
  virtual void ResetCameraClippingRange();

  ///@{
  /**
   * Reset the camera clipping range based on a bounding box.
   */
  virtual void ResetCameraClippingRange(const double bounds[6]);
  virtual void ResetCameraClippingRange(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  ///@}

  ///@{
  /**
   * Specify tolerance for near clipping plane distance to the camera as a
   * percentage of the far clipping plane distance. By default this will be
   * set to 0.01 for 16 bit zbuffers and 0.001 for higher depth z buffers
   */
  vtkSetClampMacro(NearClippingPlaneTolerance, double, 0, 0.99);
  vtkGetMacro(NearClippingPlaneTolerance, double);
  ///@}

  ///@{
  /**
   * Specify enlargement of bounds when resetting the
   * camera clipping range.  By default the range is not expanded by
   * any percent of the (far - near) on the near and far sides
   */
  vtkSetClampMacro(ClippingRangeExpansion, double, 0, 0.99);
  vtkGetMacro(ClippingRangeExpansion, double);
  ///@}

  /**
   * Automatically set up the camera based on the visible actors.
   * The camera will reposition itself to view the center point of the actors,
   * and move along its initial view plane normal (i.e., vector defined from
   * camera position to focal point) so that all of the actors can be seen.
   */
  virtual void ResetCamera();

  /**
   * Automatically set up the camera based on a specified bounding box
   * (xmin, xmax, ymin, ymax, zmin, zmax). Camera will reposition itself so
   * that its focal point is the center of the bounding box, and adjust its
   * distance and position to preserve its initial view plane normal
   * (i.e., vector defined from camera position to focal point). Note: if
   * the view plane is parallel to the view up axis, the view up axis will
   * be reset to one of the three coordinate axes.
   */
  virtual void ResetCamera(const double bounds[6]);

  /**
   * Alternative version of ResetCamera(bounds[6]);
   */
  virtual void ResetCamera(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);

  /**
   * Automatically set up the camera based on the visible actors.
   * Use a screen space bounding box to zoom closer to the data.
   *
   * OffsetRatio can be used to add a zoom offset.
   * Default value is 0.9, which means that the camera will be 10% further from the data
   */
  virtual void ResetCameraScreenSpace(double offsetRatio = 0.9);

  /**
   * Automatically set up the camera based on a specified bounding box
   * (xmin, xmax, ymin, ymax, zmin, zmax).
   * Use a screen space bounding box to zoom closer to the data.
   *
   * OffsetRatio can be used to add a zoom offset.
   * Default value is 0.9, which means that the camera will be 10% further from the data.
   */
  virtual void ResetCameraScreenSpace(const double bounds[6], double offsetRatio = 0.9);

  using vtkViewport::DisplayToWorld;

  /**
   * Convert a vtkVector3d from display space to world space.
   */
  vtkVector3d DisplayToWorld(const vtkVector3d& display);

  /**
   * Automatically set up the camera focal point and zoom factor to
   * observe the \p box in display coordinates.
   * \p OffsetRatio can be used to add a zoom offset.
   */
  void ZoomToBoxUsingViewAngle(const vtkRecti& box, double offsetRatio = 1.0);

  /**
   * Alternative version of ResetCameraScreenSpace(bounds[6]);
   *
   * OffsetRatio can be used to add a zoom offset.
   * Default value is 0.9, which means that the camera will be 10% further from the data.
   */
  virtual void ResetCameraScreenSpace(double xmin, double xmax, double ymin, double ymax,
    double zmin, double zmax, double offsetRatio = 0.9);

  ///@{
  /**
   * Specify the rendering window in which to draw. This is automatically set
   * when the renderer is created by MakeRenderer.  The user probably
   * shouldn't ever need to call this method.
   */
  void SetRenderWindow(vtkRenderWindow*);
  vtkRenderWindow* GetRenderWindow() { return this->RenderWindow; }
  vtkWindow* GetVTKWindow() override;
  ///@}

  ///@{
  /**
   * Turn on/off using backing store. This may cause the re-rendering
   * time to be slightly slower when the view changes. But it is
   * much faster when the image has not changed, such as during an
   * expose event.
   */
  vtkSetMacro(BackingStore, vtkTypeBool);
  vtkGetMacro(BackingStore, vtkTypeBool);
  vtkBooleanMacro(BackingStore, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off interactive status.  An interactive renderer is one that
   * can receive events from an interactor.  Should only be set if
   * there are multiple renderers in the same section of the viewport.
   */
  vtkSetMacro(Interactive, vtkTypeBool);
  vtkGetMacro(Interactive, vtkTypeBool);
  vtkBooleanMacro(Interactive, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the layer that this renderer belongs to.  This is only used if
   * there are layered renderers.

   * Note: Changing the layer will update the PreserveColorBuffer setting. If
   * the layer is 0, PreserveColorBuffer will be set to false, making the
   * bottom renderer opaque. If the layer is non-zero, PreserveColorBuffer will
   * be set to true, giving the renderer a transparent background. If other
   * PreserveColorBuffer configurations are desired, they must be adjusted after
   * the layer is set.
   */
  virtual void SetLayer(int layer);
  vtkGetMacro(Layer, int);
  ///@}

  ///@{
  /**
   * By default, the renderer at layer 0 is opaque, and all non-zero layer
   * renderers are transparent. This flag allows this behavior to be overridden.
   * If true, this setting will force the renderer to preserve the existing
   * color buffer regardless of layer. If false, it will always be cleared at
   * the start of rendering.

   * This flag influences the Transparent() method, and is updated by calls to
   * SetLayer(). For this reason it should only be set after changing the layer.
   */
  vtkGetMacro(PreserveColorBuffer, vtkTypeBool);
  vtkSetMacro(PreserveColorBuffer, vtkTypeBool);
  vtkBooleanMacro(PreserveColorBuffer, vtkTypeBool);
  ///@}

  ///@{
  /**
   * By default, the depth buffer is reset for each renderer. If this flag is
   * true, this renderer will use the existing depth buffer for its rendering.
   */
  vtkSetMacro(PreserveDepthBuffer, vtkTypeBool);
  vtkGetMacro(PreserveDepthBuffer, vtkTypeBool);
  vtkBooleanMacro(PreserveDepthBuffer, vtkTypeBool);
  ///@}

  /**
   * Returns a boolean indicating if this renderer is transparent.  It is
   * transparent if it is not in the deepest layer of its render window.
   */
  vtkTypeBool Transparent();

  /**
   * Convert world point coordinates to view coordinates.
   */
  void WorldToView() override;

  ///@{
  /**
   * Convert view point coordinates to world coordinates.
   */
  void ViewToWorld() override;
  void ViewToWorld(double& wx, double& wy, double& wz) override;
  ///@}

  /**
   * Convert world point coordinates to view coordinates.
   */
  void WorldToView(double& wx, double& wy, double& wz) override;

  ///@{
  /**
   * Convert to from pose coordinates
   */
  void WorldToPose(double& wx, double& wy, double& wz) override;
  void PoseToWorld(double& wx, double& wy, double& wz) override;
  void ViewToPose(double& wx, double& wy, double& wz) override;
  void PoseToView(double& wx, double& wy, double& wz) override;
  ///@}

  /**
   * Given a pixel location, return the Z value. The z value is
   * normalized (0,1) between the front and back clipping planes.
   * By default this functions accesses the `vtkRenderWindow`'s depth buffer
   * that is only valid right after this specific renderer has rendered.
   * If `SafeGetZ` is On, this function will use a `vtkHardwareSelector` to
   * get the depth information in flight. This approach always works,
   * but takes more time as it invokes a render on the whole scene.
   */
  double GetZ(int x, int y);

  ///@{
  /**
   * If this flag is On `GetZ(int, int)` will use a vtkHardwareSelector
   * internally to determine the Z value. Otherwise, it will use
   * `vtkRenderWindow::GetZbufferValue`.
   * See `GetZ(int, int)` documentation for more information.
   * Default is off.
   */
  vtkSetMacro(SafeGetZ, bool);
  vtkGetMacro(SafeGetZ, bool);
  vtkBooleanMacro(SafeGetZ, bool);
  ///@}

  /**
   * Return the MTime of the renderer also considering its ivars.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Get the time required, in seconds, for the last Render call.
   */
  vtkGetMacro(LastRenderTimeInSeconds, double);
  ///@}

  ///@{
  /**
   * Should be used internally only during a render
   * Get the number of props that were rendered using a
   * RenderOpaqueGeometry or RenderTranslucentPolygonalGeometry call.
   * This is used to know if something is in the frame buffer.
   */
  vtkGetMacro(NumberOfPropsRendered, int);
  ///@}

  ///@{
  /**
   * Return the prop (via a vtkAssemblyPath) that has the highest z value
   * at the given x, y position in the viewport.  Basically, the top most
   * prop that renders the pixel at selectionX, selectionY will be returned.
   * If nothing was picked then NULL is returned.  This method selects from
   * the renderer's Prop list.
   */
  vtkAssemblyPath* PickProp(double selectionX, double selectionY) override
  {
    return this->PickProp(selectionX, selectionY, selectionX, selectionY);
  }
  vtkAssemblyPath* PickProp(
    double selectionX1, double selectionY1, double selectionX2, double selectionY2) override;
  ///@}

  ///@{
  /**
   * Return the prop (via a vtkAssemblyPath) that has the highest z value
   * at the given x, y position in the viewport.  Basically, the top most
   * prop that renders the pixel at selectionX, selectionY will be returned.
   * If nothing was picked then NULL is returned.  This method selects from
   * the renderer's Prop list. Additionally, you can set the field
   * association of the hardware selector used internally, and get its selection
   * result by passing a non-null vtkSmartPointer<vtkSelection>. The picked prop
   * is guaranteed to be the first node in the selection result.
   */
  vtkAssemblyPath* PickProp(double selectionX, double selectionY, int fieldAssociation,
    vtkSmartPointer<vtkSelection> selection) override
  {
    return this->PickProp(
      selectionX, selectionY, selectionX, selectionY, fieldAssociation, selection);
  }
  vtkAssemblyPath* PickProp(double selectionX1, double selectionY1, double selectionX2,
    double selectionY2, int fieldAssociation, vtkSmartPointer<vtkSelection> selection) override;
  ///@}

  /**
   * Do anything necessary between rendering the left and right viewpoints
   * in a stereo render. Doesn't do anything except in the derived
   * vtkIceTRenderer in ParaView.
   */
  virtual void StereoMidpoint() {}

  /**
   * Compute the aspect ratio of this renderer for the current tile. When
   * tiled displays are used the aspect ratio of the renderer for a given
   * tile may be different that the aspect ratio of the renderer when rendered
   * in it entirety
   */
  double GetTiledAspectRatio();

  /**
   * This method returns 1 if the ActiveCamera has already been set or
   * automatically created by the renderer. It returns 0 if the
   * ActiveCamera does not yet exist.
   */
  vtkTypeBool IsActiveCameraCreated() { return (this->ActiveCamera != nullptr); }

  ///@{
  /**
   * Turn on/off rendering of translucent material with depth peeling
   * technique. The render window must have alpha bits (ie call
   * SetAlphaBitPlanes(1)) and no multisample buffer (ie call
   * SetMultiSamples(0) ) to support depth peeling.
   * If UseDepthPeeling is on and the GPU supports it, depth peeling is used
   * for rendering translucent materials.
   * If UseDepthPeeling is off, alpha blending is used.
   * Initial value is off.
   */
  vtkSetMacro(UseDepthPeeling, vtkTypeBool);
  vtkGetMacro(UseDepthPeeling, vtkTypeBool);
  vtkBooleanMacro(UseDepthPeeling, vtkTypeBool);
  ///@}

  /**
   * This flag is on and the GPU supports it, depth-peel volumes along with
   * the translucent geometry. Only supported on OpenGL2 with dual-depth
   * peeling. Default is false.
   */
  vtkSetMacro(UseDepthPeelingForVolumes, bool);
  vtkGetMacro(UseDepthPeelingForVolumes, bool);
  vtkBooleanMacro(UseDepthPeelingForVolumes, bool);

  ///@{
  /**
   * In case of use of depth peeling technique for rendering translucent
   * material, define the threshold under which the algorithm stops to
   * iterate over peel layers. This is the ratio of the number of pixels
   * that have been touched by the last layer over the total number of pixels
   * of the viewport area.
   * Initial value is 0.0, meaning rendering have to be exact. Greater values
   * may speed-up the rendering with small impact on the quality.
   */
  vtkSetClampMacro(OcclusionRatio, double, 0.0, 0.5);
  vtkGetMacro(OcclusionRatio, double);
  ///@}

  ///@{
  /**
   * In case of depth peeling, define the maximum number of peeling layers.
   * Initial value is 4. A special value of 0 means no maximum limit.
   * It has to be a positive value.
   */
  vtkSetMacro(MaximumNumberOfPeels, int);
  vtkGetMacro(MaximumNumberOfPeels, int);
  ///@}

  ///@{
  /**
   * Tells if the last call to DeviceRenderTranslucentPolygonalGeometry()
   * actually used depth peeling.
   * Initial value is false.
   */
  vtkGetMacro(LastRenderingUsedDepthPeeling, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable or disable Screen Space Ambient Occlusion.
   * SSAO darkens some pixels to improve depth perception.
   */
  vtkSetMacro(UseSSAO, bool);
  vtkGetMacro(UseSSAO, bool);
  vtkBooleanMacro(UseSSAO, bool);
  ///@}

  ///@{
  /**
   * When using SSAO, define the SSAO hemisphere radius.
   * Default is 0.5
   */
  vtkSetMacro(SSAORadius, double);
  vtkGetMacro(SSAORadius, double);
  ///@}

  ///@{
  /**
   * When using SSAO, define the bias when comparing samples.
   * Default is 0.01
   */
  vtkSetMacro(SSAOBias, double);
  vtkGetMacro(SSAOBias, double);
  ///@}

  ///@{
  /**
   * When using SSAO, define the number of samples.
   * Default is 32
   */
  vtkSetMacro(SSAOKernelSize, unsigned int);
  vtkGetMacro(SSAOKernelSize, unsigned int);
  ///@}

  ///@{
  /**
   * When using SSAO, define blurring of the ambient occlusion.
   * Blurring can help to improve the result if samples number is low.
   * Default is false
   */
  vtkSetMacro(SSAOBlur, bool);
  vtkGetMacro(SSAOBlur, bool);
  vtkBooleanMacro(SSAOBlur, bool);
  ///@}

  ///@{
  /**
   * Set/Get a custom Render call. Allows to hook a Render call from an
   * external project.It will be used in place of vtkRenderer::Render() if it
   * is not NULL and its Used ivar is set to true.
   * Initial value is NULL.
   */
  void SetDelegate(vtkRendererDelegate* d);
  vtkGetObjectMacro(Delegate, vtkRendererDelegate);
  ///@}

  ///@{
  /**
   * Get the current hardware selector. If the Selector is set, it implies the
   * current render pass is for selection. Mappers/Properties may choose to
   * behave differently when rendering for hardware selection.
   */
  vtkGetObjectMacro(Selector, vtkHardwareSelector);
  ///@}

  ///@{
  /**
   * Set/Get the texture to be used for the monocular or stereo left eye
   * background. If set and enabled this gets the priority over the gradient
   * background.
   */
  virtual void SetLeftBackgroundTexture(vtkTexture*);
  vtkTexture* GetLeftBackgroundTexture();
  virtual void SetBackgroundTexture(vtkTexture*);
  vtkGetObjectMacro(BackgroundTexture, vtkTexture);
  ///@}

  ///@{
  /**
   * Set/Get the texture to be used for the right eye background. If set
   * and enabled this gets the priority over the gradient background.
   */
  virtual void SetRightBackgroundTexture(vtkTexture*);
  vtkGetObjectMacro(RightBackgroundTexture, vtkTexture);
  ///@}

  ///@{
  /**
   * Set/Get whether this viewport should have a textured background.
   * Default is off.
   */
  vtkSetMacro(TexturedBackground, bool);
  vtkGetMacro(TexturedBackground, bool);
  vtkBooleanMacro(TexturedBackground, bool);
  ///@}

  // method to release graphics resources in any derived renderers.
  virtual void ReleaseGraphicsResources(vtkWindow*);

  ///@{
  /**
   * Turn on/off FXAA anti-aliasing, if supported. Initial value is off.
   */
  vtkSetMacro(UseFXAA, bool);
  vtkGetMacro(UseFXAA, bool);
  vtkBooleanMacro(UseFXAA, bool);
  ///@}

  ///@{
  /**
   * The configuration object for FXAA antialiasing.
   */
  vtkGetObjectMacro(FXAAOptions, vtkFXAAOptions);
  virtual void SetFXAAOptions(vtkFXAAOptions*);
  ///@}

  ///@{
  /**
   * Turn on/off rendering of shadows if supported
   * Initial value is off.
   */
  vtkSetMacro(UseShadows, vtkTypeBool);
  vtkGetMacro(UseShadows, vtkTypeBool);
  vtkBooleanMacro(UseShadows, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If this flag is true and the rendering engine supports it, wireframe
   * geometry will be drawn using hidden line removal.
   */
  vtkSetMacro(UseHiddenLineRemoval, vtkTypeBool);
  vtkGetMacro(UseHiddenLineRemoval, vtkTypeBool);
  vtkBooleanMacro(UseHiddenLineRemoval, vtkTypeBool);
  ///@}

  // Set/Get a custom render pass.
  // Initial value is NULL.
  void SetPass(vtkRenderPass* p);
  vtkGetObjectMacro(Pass, vtkRenderPass);

  ///@{
  /**
   * Set/Get the information object associated with this algorithm.
   */
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);
  ///@}

  ///@{
  /**
   * If this flag is true and the rendering engine supports it, image based
   * lighting is enabled and surface rendering displays environment reflections.
   * Image Based Lighting rely on the environment texture to compute lighting
   * if it has been provided.
   */
  vtkSetMacro(UseImageBasedLighting, bool);
  vtkGetMacro(UseImageBasedLighting, bool);
  vtkBooleanMacro(UseImageBasedLighting, bool);
  ///@}

  ///@{
  /**
   * Set/Get the environment texture used for image based lighting.
   * This texture is supposed to represent the scene background.
   * @sa vtkTexture::UseSRGBColorSpaceOn
   */
  VTK_MARSHALGETTER(EnvironmentTextureProperty)
  vtkGetObjectMacro(EnvironmentTexture, vtkTexture);
  VTK_MARSHALSETTER(EnvironmentTextureProperty)
  void SetEnvironmentTextureProperty(vtkTexture* texture) { this->SetEnvironmentTexture(texture); }
  virtual void SetEnvironmentTexture(vtkTexture* texture, bool isSRGB = false);
  ///@}

  ///@{
  /**
   * Set/Get the environment up vector.
   */
  vtkGetVector3Macro(EnvironmentUp, double);
  vtkSetVector3Macro(EnvironmentUp, double);
  ///@}

  ///@{
  /**
   * Set/Get the environment right vector.
   */
  vtkGetVector3Macro(EnvironmentRight, double);
  vtkSetVector3Macro(EnvironmentRight, double);
  ///@}

  ///@{
  /**
   * If UseOIT is on and there are translucent props in the scene, the renderer will use the
   * OrderIndependentTranslucentPass to render. If UseOIT is disabled, traditional depth sorting is
   * used for translucency.
   * By default, UseOIT is on.
   *
   * \note OIT is a newer(better) approach for translucent rendering but doesn't support hardware
   * multi-sampling. Use FXAA in that case.
   *
   * \sa SetUseFXAA()
   */
  vtkSetMacro(UseOIT, bool);
  vtkGetMacro(UseOIT, bool);
  vtkBooleanMacro(UseOIT, bool);
  ///@}

protected:
  vtkRenderer();
  ~vtkRenderer() override;

  // internal method to expand bounding box to consider model transform
  // matrix or model view transform matrix based on whether or not deering
  // frustum is used. 'bounds' buffer is mutated to the expanded box.
  virtual void ExpandBounds(double bounds[6], vtkMatrix4x4* matrix);

  vtkCamera* ActiveCamera;
  vtkLight* CreatedLight;

  vtkLightCollection* Lights;
  vtkCullerCollection* Cullers;

  vtkActorCollection* Actors;
  vtkVolumeCollection* Volumes;

  double Ambient[3];
  vtkRenderWindow* RenderWindow;
  double AllocatedRenderTime;
  double TimeFactor;
  vtkTypeBool TwoSidedLighting;
  vtkTypeBool AutomaticLightCreation;
  vtkTypeBool BackingStore;
  unsigned char* BackingImage;
  int BackingStoreSize[2];
  vtkTimeStamp RenderTime;

  double LastRenderTimeInSeconds;

  vtkTypeBool LightFollowCamera;

  // Allocate the time for each prop
  void AllocateTime();

  // Internal variables indicating the number of props
  // that have been or will be rendered in each category.
  int NumberOfPropsRendered;

  // A temporary list of props used for culling, and traversal
  // of all props when rendering
  vtkProp** PropArray;
  int PropArrayCount;

  // Indicates if the renderer should receive events from an interactor.
  // Typically only used in conjunction with transparent renderers.
  vtkTypeBool Interactive;

  // Shows what layer this renderer belongs to.  Only of interested when
  // there are layered renderers.
  int Layer;
  vtkTypeBool PreserveColorBuffer;
  vtkTypeBool PreserveDepthBuffer;

  // Holds the result of ComputeVisiblePropBounds so that it is visible from
  // wrapped languages
  double ComputedVisiblePropBounds[6];

  /**
   * Specifies the minimum distance of the near clipping
   * plane as a percentage of the far clipping plane distance.  Values below
   * this threshold are clipped to NearClippingPlaneTolerance*range[1].
   * Note that values which are too small may cause problems on systems
   * with low z-buffer resolution.
   */
  double NearClippingPlaneTolerance;

  /**
   * Specify enlargement of bounds when resetting the
   * camera clipping range.
   */
  double ClippingRangeExpansion;

  /**
   * When this flag is off, the renderer will not erase the background
   * or the Zbuffer.  It is used to have overlapping renderers.
   * Both the RenderWindow Erase and Render Erase must be on
   * for the camera to clear the renderer.  By default, Erase is on.
   */
  vtkTypeBool Erase;

  /**
   * When this flag is off, render commands are ignored.  It is used to either
   * multiplex a vtkRenderWindow or render only part of a vtkRenderWindow.
   * By default, Draw is on.
   */
  vtkTypeBool Draw;

  /**
   * Temporary collection used by vtkRenderWindow::CaptureGL2PSSpecialProps.
   */
  vtkPropCollection* GL2PSSpecialPropCollection;

  /**
   * Gets the ActiveCamera CompositeProjectionTransformationMatrix, only computing it if necessary.
   * This function expects that this->ActiveCamera is not nullptr.
   */
  const std::array<double, 16>& GetCompositeProjectionTransformationMatrix();

  /**
   * Gets the ActiveCamera ProjectionTransformationMatrix, only computing it if necessary.
   * This function expects that this->ActiveCamera is not nullptr.
   */
  const std::array<double, 16>& GetProjectionTransformationMatrix();

  /**
   * Gets the ActiveCamera ViewTransformMatrix, only computing it if necessary.
   * This function expects that this->ActiveCamera is not nullptr.
   */
  const std::array<double, 16>& GetViewTransformMatrix();

  /**
   * Ask all props to update and draw any opaque and translucent
   * geometry. This includes both vtkActors and vtkVolumes
   * Returns the number of props that rendered geometry.
   */
  virtual int UpdateGeometry(vtkFrameBufferObjectBase* fbo = nullptr);

  /**
   * Ask all props to update and draw any translucent polygonal
   * geometry. This includes both vtkActors and vtkVolumes
   * Return the number of rendered props.
   * It is called once with alpha blending technique. It is called multiple
   * times with depth peeling technique.
   */
  virtual int UpdateTranslucentPolygonalGeometry();

  /**
   * Ask all props to update and draw any opaque polygonal
   * geometry. This includes both vtkActors and vtkVolumes
   * Return the number of rendered props.
   */
  virtual int UpdateOpaquePolygonalGeometry();

  /**
   * Ask the active camera to do whatever it needs to do prior to rendering.
   * Creates a camera if none found active.
   */
  virtual int UpdateCamera();

  /**
   * Update the geometry of the lights in the scene that are not in world
   * space (for instance, Headlights or CameraLights that are attached to the
   * camera).
   */
  virtual vtkTypeBool UpdateLightGeometry();

  /**
   * Ask all lights to load themselves into rendering pipeline.
   * This method will return the actual number of lights that were on.
   */
  virtual int UpdateLights() { return 0; }

  /**
   * Get the current camera and reset it only if it gets created
   * automatically (see GetActiveCamera).
   * This is only used internally.
   */
  vtkCamera* GetActiveCameraAndResetIfCreated();

  /**
   * If this flag is on and the rendering engine supports it, FXAA will be used
   * to antialias the scene. Default is off.
   */
  bool UseFXAA;

  /**
   * Holds the FXAA configuration.
   */
  vtkFXAAOptions* FXAAOptions;

  /**
   * If this flag is on and the rendering engine supports it render shadows
   * Initial value is off.
   */
  vtkTypeBool UseShadows;

  /**
   * When this flag is on and the rendering engine supports it, wireframe
   * polydata will be rendered using hidden line removal.
   */
  vtkTypeBool UseHiddenLineRemoval;

  /**
   * If this flag is on and the GPU supports it, depth peeling is used
   * for rendering translucent materials.
   * If this flag is off, alpha blending is used.
   * Initial value is off.
   */
  vtkTypeBool UseDepthPeeling;

  /**
   * This flag is on and the GPU supports it, depth-peel volumes along with
   * the translucent geometry. Default is false;
   */
  bool UseDepthPeelingForVolumes;

  /**
   * In case of use of depth peeling technique for rendering translucent
   * material, define the threshold under which the algorithm stops to
   * iterate over peel layers. This is the ratio of the number of pixels
   * that have been touched by the last layer over the total number of pixels
   * of the viewport area.
   * Initial value is 0.0, meaning rendering have to be exact. Greater values
   * may speed-up the rendering with small impact on the quality.
   */
  double OcclusionRatio;

  /**
   * In case of depth peeling, define the maximum number of peeling layers.
   * Initial value is 4. A special value of 0 means no maximum limit.
   * It has to be a positive value.
   */
  int MaximumNumberOfPeels;

  bool UseSSAO = false;
  double SSAORadius = 0.5;
  double SSAOBias = 0.01;
  unsigned int SSAOKernelSize = 32;
  bool SSAOBlur = false;

  /**
   * If UseOIT is on and there are translucent props in the scene, the renderer will use the
   * OrderIndependentTranslucentPass to render. If UseOIT is disabled, traditional depth sorting is
   * used for translucency.
   * By default, UseOIT is on.
   *
   * \note OIT is a newer(better) approach for translucent rendering but doesn't support hardware
   * multi-sampling. Use FXAA in that case.
   *
   * \sa SetUseFXAA()
   */
  bool UseOIT = true;

  /**
   * Tells if the last call to DeviceRenderTranslucentPolygonalGeometry()
   * actually used depth peeling.
   * Initial value is false.
   */
  vtkTypeBool LastRenderingUsedDepthPeeling;

  // HARDWARE SELECTION ----------------------------------------
  friend class vtkHardwareSelector;

  /**
   * Called by vtkHardwareSelector when it begins rendering for selection.
   */
  void SetSelector(vtkHardwareSelector* selector)
  {
    this->Selector = selector;
    this->Modified();
  }

  // End Ivars for visible cell selecting.
  vtkHardwareSelector* Selector;

  //---------------------------------------------------------------
  friend class vtkRendererDelegate;
  vtkRendererDelegate* Delegate;

  bool TexturedBackground;
  vtkTexture* BackgroundTexture;
  vtkTexture* RightBackgroundTexture;

  friend class vtkRenderPass;
  vtkRenderPass* Pass;

  // Arbitrary extra information associated with this renderer
  vtkInformation* Information;

  bool UseImageBasedLighting;
  vtkTexture* EnvironmentTexture;

  double EnvironmentUp[3];
  double EnvironmentRight[3];

private:
  /**
   * Cache of CompositeProjectionTransformationMatrix.
   */
  std::array<double, 16> CompositeProjectionTransformationMatrix;

  /**
   * Tiled Aspect Ratio used to get the transform in this->CompositeProjectionTransformationMatrix.
   */
  double LastCompositeProjectionTransformationMatrixTiledAspectRatio;

  /**
   * Modified time from the camera when this->CompositeProjectionTransformationMatrix was set.
   */
  vtkMTimeType LastCompositeProjectionTransformationMatrixCameraModified;

  /**
   * Cache of ProjectionTransformationMatrix.
   */
  std::array<double, 16> ProjectionTransformationMatrix;

  /**
   * Tiled Aspect Ratio used to get the transform in this->ProjectionTransformationMatrix.
   */
  double LastProjectionTransformationMatrixTiledAspectRatio;

  /**
   * Modified time from the camera when this->ProjectionTransformationMatrix was set.
   */
  vtkMTimeType LastProjectionTransformationMatrixCameraModified;

  /**
   * Cache of ViewTransformMatrix.
   */
  std::array<double, 16> ViewTransformMatrix;

  /**
   * Modified time from the camera when this->ViewTransformMatrix was set.
   */
  vtkMTimeType LastViewTransformCameraModified;

  /**
   * If this flag affect GetZ. See Get/Set macro for more information.
   */
  bool SafeGetZ = false;

  vtkRenderer(const vtkRenderer&) = delete;
  void operator=(const vtkRenderer&) = delete;
};

inline vtkLightCollection* vtkRenderer::GetLights()
{
  return this->Lights;
}

/**
 * Get the list of cullers for this renderer.
 */
inline vtkCullerCollection* vtkRenderer::GetCullers()
{
  return this->Cullers;
}

VTK_ABI_NAMESPACE_END
#endif
