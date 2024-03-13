// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProp
 * @brief   abstract superclass for all actors, volumes and annotations
 *
 * vtkProp is an abstract superclass for any objects that can exist in a
 * rendered scene (either 2D or 3D). Instances of vtkProp may respond to
 * various render methods (e.g., RenderOpaqueGeometry()). vtkProp also
 * defines the API for picking, LOD manipulation, and common instance
 * variables that control visibility, picking, and dragging.
 * @sa
 * vtkActor2D vtkActor vtkVolume vtkProp3D
 */

#ifndef vtkProp_h
#define vtkProp_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO
#include <vector>                   // for method args

VTK_ABI_NAMESPACE_BEGIN
class vtkAssemblyPath;
class vtkAssemblyPaths;
class vtkHardwareSelector;
class vtkMatrix4x4;
class vtkPropCollection;
class vtkViewport;
class vtkWindow;
class vtkInformation;
class vtkInformationIntegerKey;
class vtkInformationDoubleVectorKey;
class vtkShaderProperty;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkProp : public vtkObject
{
public:
  vtkTypeMacro(vtkProp, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  virtual void GetActors(vtkPropCollection*) {}
  virtual void GetActors2D(vtkPropCollection*) {}
  virtual void GetVolumes(vtkPropCollection*) {}

  ///@{
  /**
   * Set/Get visibility of this vtkProp. Initial value is true.
   */
  vtkSetMacro(Visibility, vtkTypeBool);
  vtkGetMacro(Visibility, vtkTypeBool);
  vtkBooleanMacro(Visibility, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the pickable instance variable.  This determines if the vtkProp
   * can be picked (typically using the mouse). Also see dragable.
   * Initial value is true.
   */
  vtkSetMacro(Pickable, vtkTypeBool);
  vtkGetMacro(Pickable, vtkTypeBool);
  vtkBooleanMacro(Pickable, vtkTypeBool);
  ///@}

  /**
   * Method fires PickEvent if the prop is picked.
   */
  virtual void Pick();

  ///@{
  /**
   * Set/Get the value of the dragable instance variable. This determines if
   * an Prop, once picked, can be dragged (translated) through space.
   * This is typically done through an interactive mouse interface.
   * This does not affect methods such as SetPosition, which will continue
   * to work.  It is just intended to prevent some vtkProp'ss from being
   * dragged from within a user interface.
   * Initial value is true.
   */
  vtkSetMacro(Dragable, vtkTypeBool);
  vtkGetMacro(Dragable, vtkTypeBool);
  vtkBooleanMacro(Dragable, vtkTypeBool);
  ///@}

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, textures
   * etc.
   */
  virtual vtkMTimeType GetRedrawMTime() { return this->GetMTime(); }

  ///@{
  /**
   * In case the Visibility flag is true, tell if the bounds of this prop
   * should be taken into account or ignored during the computation of other
   * bounding boxes, like in vtkRenderer::ResetCamera().
   * Initial value is true.
   */
  vtkSetMacro(UseBounds, bool);
  vtkGetMacro(UseBounds, bool);
  vtkBooleanMacro(UseBounds, bool);
  ///@}

  /**
   * Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   * in world coordinates. NULL means that the bounds are not defined.
   */
  virtual double* GetBounds() VTK_SIZEHINT(6) { return nullptr; }

  /**
   * Shallow copy of this vtkProp.
   */
  virtual void ShallowCopy(vtkProp* prop);

  ///@{
  /**
   * vtkProp and its subclasses can be picked by subclasses of
   * vtkAbstractPicker (e.g., vtkPropPicker). The following methods interface
   * with the picking classes and return "pick paths". A pick path is a
   * hierarchical, ordered list of props that form an assembly.  Most often,
   * when a vtkProp is picked, its path consists of a single node (i.e., the
   * prop). However, classes like vtkAssembly and vtkPropAssembly can return
   * more than one path, each path being several layers deep. (See
   * vtkAssemblyPath for more information.)  To use these methods - first
   * invoke InitPathTraversal() followed by repeated calls to GetNextPath().
   * GetNextPath() returns a NULL pointer when the list is exhausted.
   */
  virtual void InitPathTraversal();
  virtual vtkAssemblyPath* GetNextPath();
  virtual int GetNumberOfPaths() { return 1; }
  ///@}

  /**
   * These methods are used by subclasses to place a matrix (if any) in the
   * prop prior to rendering. Generally used only for picking. See vtkProp3D
   * for more information.
   */
  virtual void PokeMatrix(vtkMatrix4x4* vtkNotUsed(matrix)) {}
  virtual vtkMatrix4x4* GetMatrix() { return nullptr; }

  ///@{
  /**
   * Set/Get property keys. Property keys can be digest by some rendering
   * passes.
   * For instance, the user may mark a prop as a shadow caster for a
   * shadow mapping render pass. Keys are documented in render pass classes.
   * Initial value is NULL.
   */
  vtkGetObjectMacro(PropertyKeys, vtkInformation);
  virtual void SetPropertyKeys(vtkInformation* keys);
  ///@}

  /**
   * Tells if the prop has all the required keys.
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool HasKeys(vtkInformation* requiredKeys);

  /**
   * Optional Key Indicating the texture unit for general texture mapping
   * Old OpenGL was a state machine where you would push or pop
   * items. The new OpenGL design is more mapper centric. Some
   * classes push a texture and then assume a mapper will use it.
   * The new design wants explicit communication of when a texture
   * is being used.  This key can be used to pass that information
   * down to a mapper.
   */
  static vtkInformationIntegerKey* GeneralTextureUnit();

  /**
   * Optional Key Indicating the texture transform for general texture mapping
   * Old OpenGL was a state machine where you would push or pop
   * items. The new OpenGL design is more mapper centric. Some
   * classes push a texture and then assume a mapper will use it.
   * The new design wants explicit communication of when a texture
   * is being used.  This key can be used to pass that information
   * down to a mapper.
   */
  static vtkInformationDoubleVectorKey* GeneralTextureTransform();

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * All concrete subclasses must be able to render themselves.
   * There are four key render methods in vtk and they correspond
   * to four different points in the rendering cycle. Any given
   * prop may implement one or more of these methods.
   * The first method is intended for rendering all opaque geometry. The
   * second method is intended for rendering all translucent polygonal
   * geometry. The third one is intended for rendering all translucent
   * volumetric geometry. Most of the volume rendering mappers draw their
   * results during this third method.
   * The last method is to render any 2D annotation or overlays.
   * Each of these methods return an integer value indicating
   * whether or not this render method was applied to this data.
   */
  virtual int RenderOpaqueGeometry(vtkViewport*) { return 0; }
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*) { return 0; }
  virtual int RenderVolumetricGeometry(vtkViewport*) { return 0; }
  virtual int RenderOverlay(vtkViewport*) { return 0; }

  /**
   * Render the opaque geometry only if the prop has all the requiredKeys.
   * This is recursive for composite props like vtkAssembly.
   * An implementation is provided in vtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredOpaqueGeometry(vtkViewport* v, vtkInformation* requiredKeys);

  /**
   * Render the translucent polygonal geometry only if the prop has all the
   * requiredKeys.
   * This is recursive for composite props like vtkAssembly.
   * An implementation is provided in vtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredTranslucentPolygonalGeometry(
    vtkViewport* v, vtkInformation* requiredKeys);

  /**
   * Render the volumetric geometry only if the prop has all the
   * requiredKeys.
   * This is recursive for composite props like vtkAssembly.
   * An implementation is provided in vtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredVolumetricGeometry(vtkViewport* v, vtkInformation* requiredKeys);

  /**
   * Render in the overlay of the viewport only if the prop has all the
   * requiredKeys.
   * This is recursive for composite props like vtkAssembly.
   * An implementation is provided in vtkProp but each composite prop
   * must override it.
   * It returns if the rendering was performed.
   * \pre v_exists: v!=0
   * \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
   */
  virtual bool RenderFilteredOverlay(vtkViewport* v, vtkInformation* requiredKeys);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * Does this prop have some translucent polygonal geometry?
   * This method is called during the rendering process to know if there is
   * some translucent polygonal geometry. A simple prop that has some
   * translucent polygonal geometry will return true. A composite prop (like
   * vtkAssembly) that has at least one sub-prop that has some translucent
   * polygonal geometry will return true.
   * Default implementation return false.
   */
  virtual vtkTypeBool HasTranslucentPolygonalGeometry() { return 0; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * Does this prop have some opaque geometry?
   * This method is called during the rendering process to know if there is
   * some opaque geometry. A simple prop that has some
   * opaque geometry will return true. A composite prop (like
   * vtkAssembly) that has at least one sub-prop that has some opaque
   * polygonal geometry will return true.
   * Default implementation return true.
   */
  virtual vtkTypeBool HasOpaqueGeometry() { return 1; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) {}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * The EstimatedRenderTime may be used to select between different props,
   * for example in LODProp it is used to select the level-of-detail.
   * The value is returned in seconds. For simple geometry the accuracy may
   * not be great due to buffering. For ray casting, which is already
   * multi-resolution, the current resolution of the image is factored into
   * the time. We need the viewport for viewing parameters that affect timing.
   * The no-arguments version simply returns the value of the variable with
   * no estimation.
   */
  virtual double GetEstimatedRenderTime(vtkViewport*) { return this->EstimatedRenderTime; }
  virtual double GetEstimatedRenderTime() { return this->EstimatedRenderTime; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * This method is used by, for example, the vtkLODProp3D in order to
   * initialize the estimated render time at start-up to some user defined
   * value.
   */
  virtual void SetEstimatedRenderTime(double t)
  {
    this->EstimatedRenderTime = t;
    this->SavedEstimatedRenderTime = t;
  }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * When the EstimatedRenderTime is first set to 0.0 (in the
   * SetAllocatedRenderTime method) the old value is saved. This
   * method is used to restore that old value should the render be
   * aborted.
   */
  virtual void RestoreEstimatedRenderTime()
  {
    this->EstimatedRenderTime = this->SavedEstimatedRenderTime;
  }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * This method is intended to allow the renderer to add to the
   * EstimatedRenderTime in props that require information that
   * the renderer has in order to do this. For example, props
   * that are rendered with a ray casting method do not know
   * themselves how long it took for them to render. We don't want to
   * cause a this->Modified() when we set this value since it is not
   * really a modification to the object. (For example, we don't want
   * to rebuild matrices at every render because the estimated render time
   * is changing)
   */
  virtual void AddEstimatedRenderTime(double t, vtkViewport* vtkNotUsed(vp))
  {
    this->EstimatedRenderTime += t;
  }

  ///@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * The renderer may use the allocated rendering time to determine
   * how to render this actor. Therefore it might need the information
   * provided in the viewport.
   * A side effect of this method is to reset the EstimatedRenderTime to
   * 0.0. This way, each of the ways that this prop may be rendered can
   * be timed and added together into this value.
   */
  virtual void SetAllocatedRenderTime(double t, vtkViewport* vtkNotUsed(v))
  {
    this->AllocatedRenderTime = t;
    this->SavedEstimatedRenderTime = this->EstimatedRenderTime;
    this->EstimatedRenderTime = 0.0;
  }
  ///@}

  ///@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  vtkGetMacro(AllocatedRenderTime, double);
  ///@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Get/Set the multiplier for the render time. This is used
   * for culling and is a number between 0 and 1. It is used
   * to create the allocated render time value.
   */
  void SetRenderTimeMultiplier(double t) { this->RenderTimeMultiplier = t; }
  vtkGetMacro(RenderTimeMultiplier, double);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used to construct assembly paths and perform part traversal.
   */
  virtual void BuildPaths(vtkAssemblyPaths* paths, vtkAssemblyPath* path);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  virtual bool GetSupportsSelection() { return false; }

  /**
   * allows a prop to update a selections color buffers
   *
   */
  virtual void ProcessSelectorPixelBuffers(
    vtkHardwareSelector* /* sel */, std::vector<unsigned int>& /* pixeloffsets */)
  {
  }

  ///@{
  /**
   * Get the number of consumers
   */
  vtkGetMacro(NumberOfConsumers, int);
  ///@}

  ///@{
  /**
   * Add or remove or get or check a consumer,
   */
  void AddConsumer(vtkObject* c);
  void RemoveConsumer(vtkObject* c);
  vtkObject* GetConsumer(int i);
  int IsConsumer(vtkObject* c);
  ///@}

  ///@{
  /**
   * Set/Get the shader property.
   */
  virtual void SetShaderProperty(vtkShaderProperty* property);
  virtual vtkShaderProperty* GetShaderProperty();
  ///@}

  ///@{
  // Get if we are in the translucent polygonal geometry pass
  virtual bool IsRenderingTranslucentPolygonalGeometry() { return false; }
  ///@}

protected:
  vtkProp();
  ~vtkProp() override;

  vtkTypeBool Visibility;
  vtkTypeBool Pickable;
  vtkTypeBool Dragable;
  bool UseBounds;

  double AllocatedRenderTime;
  double EstimatedRenderTime;
  double SavedEstimatedRenderTime;
  double RenderTimeMultiplier;

  // how many consumers does this object have
  int NumberOfConsumers;
  vtkObject** Consumers;

  // support multi-part props and access to paths of prop
  // stuff that follows is used to build the assembly hierarchy
  vtkAssemblyPaths* Paths;

  vtkInformation* PropertyKeys;

  // User-defined shader replacement and uniform variables
  vtkShaderProperty* ShaderProperty;

private:
  vtkProp(const vtkProp&) = delete;
  void operator=(const vtkProp&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
