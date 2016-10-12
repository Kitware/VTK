/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkVolumeCollection.h" // Needed for access in inline members
#include "vtkActorCollection.h" // Needed for access in inline members

class vtkFXAAOptions;
class vtkRenderWindow;
class vtkVolume;
class vtkCuller;
class vtkActor;
class vtkActor2D;
class vtkCamera;
class vtkInformation;
class vtkLightCollection;
class vtkCullerCollection;
class vtkLight;
class vtkHardwareSelector;
class vtkRendererDelegate;
class vtkRenderPass;
class vtkTexture;

class VTKRENDERINGCORE_EXPORT vtkRenderer : public vtkViewport
{
public:
  vtkTypeMacro(vtkRenderer,vtkViewport);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Create a vtkRenderer with a black background, a white ambient light,
   * two-sided lighting turned on, a viewport of (0,0,1,1), and backface
   * culling turned off.
   */
  static vtkRenderer *New();

  //@{
  /**
   * Add/Remove different types of props to the renderer.
   * These methods are all synonyms to AddViewProp and RemoveViewProp.
   * They are here for convenience and backwards compatibility.
   */
  void AddActor(vtkProp *p);
  void AddVolume(vtkProp *p);
  void RemoveActor(vtkProp *p);
  void RemoveVolume(vtkProp *p);
  //@}

  /**
   * Add a light to the list of lights.
   */
  void AddLight(vtkLight *);

  /**
   * Remove a light from the list of lights.
   */
  void RemoveLight(vtkLight *);

  /**
   * Remove all lights from the list of lights.
   */
  void RemoveAllLights();

  /**
   * Return the collection of lights.
   */
  vtkLightCollection *GetLights();

  /**
   * Set the collection of lights.
   * We cannot name it SetLights because of TestSetGet
   * \pre lights_exist: lights!=0
   * \post lights_set: lights==this->GetLights()
   */
  void SetLightCollection(vtkLightCollection *lights);

  /**
   * Create and add a light to renderer.
   */
  void CreateLight(void);

  /**
   * Create a new Light sutible for use with this type of Renderer.
   * For example, a vtkMesaRenderer should create a vtkMesaLight
   * in this function.   The default is to just call vtkLight::New.
   */
  virtual vtkLight *MakeLight();

  //@{
  /**
   * Turn on/off two-sided lighting of surfaces. If two-sided lighting is
   * off, then only the side of the surface facing the light(s) will be lit,
   * and the other side dark. If two-sided lighting on, both sides of the
   * surface will be lit.
   */
  vtkGetMacro(TwoSidedLighting,int);
  vtkSetMacro(TwoSidedLighting,int);
  vtkBooleanMacro(TwoSidedLighting,int);
  //@}

  //@{
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
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);
  //@}

  //@{
  /**
   * Turn on/off a flag which disables the automatic light creation capability.
   * Normally in VTK if no lights are associated with the renderer, then a light
   * is automatically created. However, in special circumstances this feature is
   * undesirable, so the following boolean is provided to disable automatic
   * light creation. (Turn AutomaticLightCreation off if you do not want lights
   * to be created.)
   */
  vtkGetMacro(AutomaticLightCreation,int);
  vtkSetMacro(AutomaticLightCreation,int);
  vtkBooleanMacro(AutomaticLightCreation,int);
  //@}

  /**
   * Ask the lights in the scene that are not in world space
   * (for instance, Headlights or CameraLights that are attached to the
   * camera) to update their geometry to match the active camera.
   */
  virtual int UpdateLightsGeometryToFollowCamera(void);

  /**
   * Return the collection of volumes.
   */
  vtkVolumeCollection *GetVolumes();

  /**
   * Return any actors in this renderer.
   */
  vtkActorCollection *GetActors();

  /**
   * Specify the camera to use for this renderer.
   */
  void SetActiveCamera(vtkCamera *);

  /**
   * Get the current camera. If there is not camera assigned to the
   * renderer already, a new one is created automatically.
   * This does *not* reset the camera.
   */
  vtkCamera *GetActiveCamera();

  /**
   * Create a new Camera sutible for use with this type of Renderer.
   * For example, a vtkMesaRenderer should create a vtkMesaCamera
   * in this function.   The default is to just call vtkCamera::New.
   */
  virtual vtkCamera *MakeCamera();

  //@{
  /**
   * When this flag is off, the renderer will not erase the background
   * or the Zbuffer.  It is used to have overlapping renderers.
   * Both the RenderWindow Erase and Render Erase must be on
   * for the camera to clear the renderer.  By default, Erase is on.
   */
  vtkSetMacro(Erase, int);
  vtkGetMacro(Erase, int);
  vtkBooleanMacro(Erase, int);
  //@}

  //@{
  /**
   * When this flag is off, render commands are ignored.  It is used to either
   * multiplex a vtkRenderWindow or render only part of a vtkRenderWindow.
   * By default, Draw is on.
   */
  vtkSetMacro(Draw, int);
  vtkGetMacro(Draw, int);
  vtkBooleanMacro(Draw, int);
  //@}

  /**
   * This function is called to capture an instance of vtkProp that requires
   * special handling during vtkRenderWindow::CaptureGL2PSSpecialProps().
   */
  int CaptureGL2PSSpecialProp(vtkProp *);

  /**
   * Set the prop collection object used during
   * vtkRenderWindow::CaptureGL2PSSpecialProps(). Do not call manually, this
   * is handled automatically by the render window.
   */
  void SetGL2PSSpecialPropCollection(vtkPropCollection *);

  /**
   * Add an culler to the list of cullers.
   */
  void AddCuller(vtkCuller *);

  /**
   * Remove an actor from the list of cullers.
   */
  void RemoveCuller(vtkCuller *);

  /**
   * Return the collection of cullers.
   */
  vtkCullerCollection *GetCullers();

  //@{
  /**
   * Set the intensity of ambient lighting.
   */
  vtkSetVector3Macro(Ambient,double);
  vtkGetVectorMacro(Ambient,double,3);
  //@}

  //@{
  /**
   * Set/Get the amount of time this renderer is allowed to spend
   * rendering its scene. This is used by vtkLODActor's.
   */
  vtkSetMacro(AllocatedRenderTime,double);
  virtual double GetAllocatedRenderTime();
  //@}

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
  virtual void DeviceRender() =0;

  /**
   * Render opaque polygonal geometry. Default implementation just calls
   * UpdateOpaquePolygonalGeometry().
   * Subclasses of vtkRenderer that can deal with, e.g. hidden line removal must
   * override this method.
   */
  virtual void DeviceRenderOpaqueGeometry();

  /**
   * Render translucent polygonal geometry. Default implementation just call
   * UpdateTranslucentPolygonalGeometry().
   * Subclasses of vtkRenderer that can deal with depth peeling must
   * override this method.
   * It updates boolean ivar LastRenderingUsedDepthPeeling.
   */
  virtual void DeviceRenderTranslucentPolygonalGeometry();

  /**
   * Internal method temporarily removes lights before reloading them
   * into graphics pipeline.
   */
  virtual void ClearLights(void) {};

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
  void ComputeVisiblePropBounds( double bounds[6] );

  /**
   * Wrapper-friendly version of ComputeVisiblePropBounds
   */
  double *ComputeVisiblePropBounds();

  /**
   * Reset the camera clipping range based on the bounds of the
   * visible actors. This ensures that no props are cut off
   */
  void ResetCameraClippingRange();

  //@{
  /**
   * Reset the camera clipping range based on a bounding box.
   * This method is called from ResetCameraClippingRange()
   * If Deering frustrum is used then the bounds get expanded
   * by the camera's modelview matrix.
   */
  void ResetCameraClippingRange( double bounds[6] );
  void ResetCameraClippingRange( double xmin, double xmax,
                                 double ymin, double ymax,
                                 double zmin, double zmax);
  //@}

  //@{
  /**
   * Specify tolerance for near clipping plane distance to the camera as a
   * percentage of the far clipping plane distance. By default this will be
   * set to 0.01 for 16 bit zbuffers and 0.001 for higher depth z buffers
   */
  vtkSetClampMacro(NearClippingPlaneTolerance,double,0,0.99);
  vtkGetMacro(NearClippingPlaneTolerance,double);
  //@}

  //@{
  /**
   * Specify enlargement of bounds when resetting the
   * camera clipping range.  By default the range is not expanded by
   * any percent of the (far - near) on the near and far sides
   */
  vtkSetClampMacro(ClippingRangeExpansion,double,0,0.99);
  vtkGetMacro(ClippingRangeExpansion,double);
  //@}

  /**
   * Automatically set up the camera based on the visible actors.
   * The camera will reposition itself to view the center point of the actors,
   * and move along its initial view plane normal (i.e., vector defined from
   * camera position to focal point) so that all of the actors can be seen.
   */
  virtual void ResetCamera();

  /**
   * Automatically set up the camera based on a specified bounding box
   * (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
   * that its focal point is the center of the bounding box, and adjust its
   * distance and position to preserve its initial view plane normal
   * (i.e., vector defined from camera position to focal point). Note: is
   * the view plane is parallel to the view up axis, the view up axis will
   * be reset to one of the three coordinate axes.
   */
  virtual void ResetCamera(double bounds[6]);

  /**
   * Alternative version of ResetCamera(bounds[6]);
   */
  virtual void ResetCamera(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax);

  //@{
  /**
   * Specify the rendering window in which to draw. This is automatically set
   * when the renderer is created by MakeRenderer.  The user probably
   * shouldn't ever need to call this method.
   */
  void SetRenderWindow(vtkRenderWindow *);
  vtkRenderWindow *GetRenderWindow() {return this->RenderWindow;};
  virtual vtkWindow *GetVTKWindow();
  //@}

  //@{
  /**
   * Turn on/off using backing store. This may cause the re-rendering
   * time to be slightly slower when the view changes. But it is
   * much faster when the image has not changed, such as during an
   * expose event.
   */
  vtkSetMacro(BackingStore,int);
  vtkGetMacro(BackingStore,int);
  vtkBooleanMacro(BackingStore,int);
  //@}

  //@{
  /**
   * Turn on/off interactive status.  An interactive renderer is one that
   * can receive events from an interactor.  Should only be set if
   * there are multiple renderers in the same section of the viewport.
   */
  vtkSetMacro(Interactive,int);
  vtkGetMacro(Interactive,int);
  vtkBooleanMacro(Interactive,int);
  //@}

  //@{
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
  //@}

  //@{
  /**
   * By default, the renderer at layer 0 is opaque, and all non-zero layer
   * renderers are transparent. This flag allows this behavior to be overridden.
   * If true, this setting will force the renderer to preserve the existing
   * color buffer regardless of layer. If false, it will always be cleared at
   * the start of rendering.

   * This flag influences the Transparent() method, and is updated by calls to
   * SetLayer(). For this reason it should only be set after changing the layer.
   */
  vtkGetMacro(PreserveColorBuffer, int);
  vtkSetMacro(PreserveColorBuffer, int);
  vtkBooleanMacro(PreserveColorBuffer, int);
  //@}

  //@{
  /**
   * By default, the depth buffer is reset for each renderer. If this flag is
   * true, this renderer will use the existing depth buffer for its rendering.
   */
  vtkSetMacro(PreserveDepthBuffer, int);
  vtkGetMacro(PreserveDepthBuffer, int);
  vtkBooleanMacro(PreserveDepthBuffer, int);
  //@}

  /**
   * Returns a boolean indicating if this renderer is transparent.  It is
   * transparent if it is not in the deepest layer of its render window.
   */
  int  Transparent();

  /**
   * Convert world point coordinates to view coordinates.
   */
  void WorldToView();

  //@{
  /**
   * Convert view point coordinates to world coordinates.
   */
  void ViewToWorld();
  virtual void ViewToWorld(double &wx, double &wy, double &wz);
  //@}

  /**
   * Convert world point coordinates to view coordinates.
   */
  virtual void WorldToView(double &wx, double &wy, double &wz);

  /**
   * Given a pixel location, return the Z value. The z value is
   * normalized (0,1) between the front and back clipping planes.
   */
  double GetZ (int x, int y);

  /**
   * Return the MTime of the renderer also considering its ivars.
   */
  vtkMTimeType GetMTime();

  //@{
  /**
   * Get the time required, in seconds, for the last Render call.
   */
  vtkGetMacro( LastRenderTimeInSeconds, double );
  //@}

  //@{
  /**
   * Should be used internally only during a render
   * Get the number of props that were rendered using a
   * RenderOpaqueGeometry or RenderTranslucentPolygonalGeometry call.
   * This is used to know if something is in the frame buffer.
   */
  vtkGetMacro( NumberOfPropsRendered, int );
  //@}

  //@{
  /**
   * Return the prop (via a vtkAssemblyPath) that has the highest z value
   * at the given x, y position in the viewport.  Basically, the top most
   * prop that renders the pixel at selectionX, selectionY will be returned.
   * If nothing was picked then NULL is returned.  This method selects from
   * the renderers Prop list.
   */
  vtkAssemblyPath* PickProp(double selectionX, double selectionY)
  {
    return this->PickProp(selectionX, selectionY, selectionX, selectionY);
  }
  vtkAssemblyPath* PickProp(double selectionX1, double selectionY1,
                            double selectionX2, double selectionY2);
  //@}

  /**
   * Do anything necessary between rendering the left and right viewpoints
   * in a stereo render. Doesn't do anything except in the derived
   * vtkIceTRenderer in ParaView.
   */
  virtual void StereoMidpoint() { return; };

  /**
   * Compute the aspect ratio of this renderer for the current tile. When
   * tiled displays are used the aspect ratio of the renderer for a given
   * tile may be different that the aspect ratio of the renderer when rendered
   * in it entirity
   */
  double GetTiledAspectRatio();

  /**
   * This method returns 1 if the ActiveCamera has already been set or
   * automatically created by the renderer. It returns 0 if the
   * ActiveCamera does not yet exist.
   */
  int IsActiveCameraCreated()
    { return (this->ActiveCamera != NULL); }


  //@{
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
  vtkSetMacro(UseDepthPeeling,int);
  vtkGetMacro(UseDepthPeeling,int);
  vtkBooleanMacro(UseDepthPeeling,int);
  //@}

  //@{
  /**
   * In case of use of depth peeling technique for rendering translucent
   * material, define the threshold under which the algorithm stops to
   * iterate over peel layers. This is the ratio of the number of pixels
   * that have been touched by the last layer over the total number of pixels
   * of the viewport area.
   * Initial value is 0.0, meaning rendering have to be exact. Greater values
   * may speed-up the rendering with small impact on the quality.
   */
  vtkSetClampMacro(OcclusionRatio,double,0.0,0.5);
  vtkGetMacro(OcclusionRatio,double);
  //@}

  //@{
  /**
   * In case of depth peeling, define the maximum number of peeling layers.
   * Initial value is 4. A special value of 0 means no maximum limit.
   * It has to be a positive value.
   */
  vtkSetMacro(MaximumNumberOfPeels,int);
  vtkGetMacro(MaximumNumberOfPeels,int);
  //@}

  //@{
  /**
   * Tells if the last call to DeviceRenderTranslucentPolygonalGeometry()
   * actually used depth peeling.
   * Initial value is false.
   */
  vtkGetMacro(LastRenderingUsedDepthPeeling,int);
  //@}

  //@{
  /**
   * Set/Get a custom Render call. Allows to hook a Render call from an
   * external project.It will be used in place of vtkRenderer::Render() if it
   * is not NULL and its Used ivar is set to true.
   * Initial value is NULL.
   */
  void SetDelegate(vtkRendererDelegate *d);
  vtkGetObjectMacro(Delegate,vtkRendererDelegate);
  //@}

  //@{
  /**
   * Get the current hardware selector. If the Selector is set, it implies the
   * current render pass is for selection. Mappers/Properties may choose to
   * behave differently when rendering for hardware selection.
   */
  vtkGetObjectMacro(Selector, vtkHardwareSelector);
  //@}

  //@{
  /**
   * Set/Get the texture to be used for the background. If set
   * and enabled this gets the priority over the gradient background.
   */
  virtual void SetBackgroundTexture(vtkTexture*);
  vtkGetObjectMacro(BackgroundTexture, vtkTexture);
  //@}

  //@{
  /**
   * Set/Get whether this viewport should have a textured background.
   * Default is off.
   */
  vtkSetMacro(TexturedBackground,bool);
  vtkGetMacro(TexturedBackground,bool);
  vtkBooleanMacro(TexturedBackground,bool);
  //@}

  // method to release graphics resources in any derived renderers.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  //@{
  /**
   * Turn on/off FXAA anti-aliasing, if supported. Initial value is off.
   */
  vtkSetMacro(UseFXAA, bool)
  vtkGetMacro(UseFXAA, bool)
  vtkBooleanMacro(UseFXAA, bool)
  //@}

  //@{
  /**
   * The configuration object for FXAA antialiasing.
   */
  vtkGetObjectMacro(FXAAOptions, vtkFXAAOptions)
  virtual void SetFXAAOptions(vtkFXAAOptions*);
  //@}

  //@{
  /**
   * Turn on/off rendering of shadows if supported
   * Initial value is off.
   */
  vtkSetMacro(UseShadows,int);
  vtkGetMacro(UseShadows,int);
  vtkBooleanMacro(UseShadows,int);
  //@}

  //@{
  /**
   * If this flag is true and the rendering engine supports it, wireframe
   * geometry will be drawn using hidden line removal.
   */
  vtkSetMacro(UseHiddenLineRemoval, int)
  vtkGetMacro(UseHiddenLineRemoval, int)
  vtkBooleanMacro(UseHiddenLineRemoval, int)
  //@}

  // Set/Get a custom render pass.
  // Initial value is NULL.
  void SetPass(vtkRenderPass *p);
  vtkGetObjectMacro(Pass, vtkRenderPass);

  //@{
  /**
   * Set/Get the information object associated with this algorithm.
   */
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);
  //@}

protected:
  vtkRenderer();
  ~vtkRenderer();

  // internal method for doing a render for picking purposes
  virtual void PickRender(vtkPropCollection *props);
  virtual void PickGeometry();

  // internal method to expand bounding box to consider model transform
  // matrix or model view transform matrix based on whether or not deering
  // frustum is used.
  virtual void ExpandBounds(double bounds[6], vtkMatrix4x4 *matrix);

  vtkCamera *ActiveCamera;
  vtkLight  *CreatedLight;

  vtkLightCollection *Lights;
  vtkCullerCollection *Cullers;

  vtkActorCollection *Actors;
  vtkVolumeCollection *Volumes;

  double              Ambient[3];
  vtkRenderWindow    *RenderWindow;
  double              AllocatedRenderTime;
  double              TimeFactor;
  int                TwoSidedLighting;
  int                AutomaticLightCreation;
  int                BackingStore;
  unsigned char      *BackingImage;
  int                BackingStoreSize[2];
  vtkTimeStamp       RenderTime;

  double              LastRenderTimeInSeconds;

  int                LightFollowCamera;

  // Allocate the time for each prop
  void               AllocateTime();

  // Internal variables indicating the number of props
  // that have been or will be rendered in each category.
  int                NumberOfPropsRendered;

  // A temporary list of props used for culling, and traversal
  // of all props when rendering
  vtkProp            **PropArray;
  int                PropArrayCount;

  // A temporary list used for picking
  vtkAssemblyPath    **PathArray;
  int                PathArrayCount;

  // Indicates if the renderer should receive events from an interactor.
  // Typically only used in conjunction with transparent renderers.
  int                Interactive;

  // Shows what layer this renderer belongs to.  Only of interested when
  // there are layered renderers.
  int                Layer;
  int                PreserveColorBuffer;
  int                PreserveDepthBuffer;

  // Holds the result of ComputeVisiblePropBounds so that it is visible from
  // wrapped languages
  double              ComputedVisiblePropBounds[6];

  /**
   * Specifies the minimum distance of the near clipping
   * plane as a percentage of the far clipping plane distance.  Values below
   * this threshold are clipped to NearClippingPlaneTolerance*range[1].
   * Note that values which are too small may cause problems on systems
   * with low z-buffer resolution.
   */
  double              NearClippingPlaneTolerance;

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
  int Erase;

  /**
   * When this flag is off, render commands are ignored.  It is used to either
   * multiplex a vtkRenderWindow or render only part of a vtkRenderWindow.
   * By default, Draw is on.
   */
  int Draw;

  /**
   * Temporary collection used by vtkRenderWindow::CaptureGL2PSSpecialProps.
   */
  vtkPropCollection *GL2PSSpecialPropCollection;

  /**
   * Ask all props to update and draw any opaque and translucent
   * geometry. This includes both vtkActors and vtkVolumes
   * Returns the number of props that rendered geometry.
   */
  virtual int UpdateGeometry();

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
  virtual int UpdateCamera(void);

  /**
   * Update the geometry of the lights in the scene that are not in world
   * space (for instance, Headlights or CameraLights that are attached to the
   * camera).
   */
  virtual int UpdateLightGeometry(void);

  /**
   * Ask all lights to load themselves into rendering pipeline.
   * This method will return the actual number of lights that were on.
   */
  virtual int UpdateLights(void) {return 0;}

  /**
   * Get the current camera and reset it only if it gets created
   * automatically (see GetActiveCamera).
   * This is only used internally.
   */
  vtkCamera *GetActiveCameraAndResetIfCreated();

  /**
   * If this flag is on and the rendering engine supports it, FXAA will be used
   * to antialias the scene. Default is off.
   */
  bool UseFXAA;

  /**
   * Holds the FXAA configuration.
   */
  vtkFXAAOptions *FXAAOptions;

  /**
   * If this flag is on and the rendering engine supports it render shadows
   * Initial value is off.
   */
  int UseShadows;

  /**
   * When this flag is on and the rendering engine supports it, wireframe
   * polydata will be rendered using hidden line removal.
   */
  int UseHiddenLineRemoval;

  /**
   * If this flag is on and the GPU supports it, depth peeling is used
   * for rendering translucent materials.
   * If this flag is off, alpha blending is used.
   * Initial value is off.
   */
  int UseDepthPeeling;

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

  /**
   * Tells if the last call to DeviceRenderTranslucentPolygonalGeometry()
   * actually used depth peeling.
   * Initial value is false.
   */
  int LastRenderingUsedDepthPeeling;

  // HARDWARE SELECTION ----------------------------------------
  friend class vtkHardwareSelector;

  /**
   * Called by vtkHardwareSelector when it begins rendering for selection.
   */
  void SetSelector(vtkHardwareSelector* selector)
    { this->Selector = selector; this->Modified(); }

  // End Ivars for visible cell selecting.
  vtkHardwareSelector* Selector;

  //---------------------------------------------------------------
  friend class vtkRendererDelegate;
  vtkRendererDelegate *Delegate;

  bool TexturedBackground;
  vtkTexture* BackgroundTexture;

  friend class vtkRenderPass;
  vtkRenderPass *Pass;

  // Arbitrary extra information associated with this renderer
  vtkInformation* Information;

private:
  vtkRenderer(const vtkRenderer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRenderer&) VTK_DELETE_FUNCTION;

};

inline vtkLightCollection *vtkRenderer::GetLights() {
  return this->Lights;
}

/**
 * Get the list of cullers for this renderer.
 */
inline vtkCullerCollection *vtkRenderer::GetCullers(){return this->Cullers;}

#endif
