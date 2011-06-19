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
// .NAME vtkRenderer - abstract specification for renderers
// .SECTION Description
// vtkRenderer provides an abstract specification for renderers. A renderer
// is an object that controls the rendering process for objects. Rendering
// is the process of converting geometry, a specification for lights, and
// a camera view into an image. vtkRenderer also performs coordinate
// transformation between world coordinates, view coordinates (the computer
// graphics rendering coordinate system), and display coordinates (the
// actual screen coordinates on the display device). Certain advanced
// rendering features such as two-sided lighting can also be controlled.

// .SECTION See Also
// vtkRenderWindow vtkActor vtkCamera vtkLight vtkVolume

#ifndef __vtkRenderer_h
#define __vtkRenderer_h

#include "vtkViewport.h"

#include "vtkVolumeCollection.h" // Needed for access in inline members
#include "vtkActorCollection.h" // Needed for access in inline members

class vtkRenderWindow;
class vtkVolume;
class vtkCuller;
class vtkActor;
class vtkActor2D;
class vtkCamera;
class vtkLightCollection;
class vtkCullerCollection;
class vtkLight;
class vtkPainter;
class vtkIdentColoredPainter;
class vtkHardwareSelector;
class vtkRendererDelegate;
class vtkRenderPass;
class vtkTexture;

#if !defined(VTK_LEGACY_REMOVE)
class vtkVisibleCellSelector;
#endif


class VTK_RENDERING_EXPORT vtkRenderer : public vtkViewport
{
public:
  vtkTypeMacro(vtkRenderer,vtkViewport);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a vtkRenderer with a black background, a white ambient light,
  // two-sided lighting turned on, a viewport of (0,0,1,1), and backface
  // culling turned off.
  static vtkRenderer *New();

  // Description:
  // Add/Remove different types of props to the renderer.
  // These methods are all synonyms to AddViewProp and RemoveViewProp.
  // They are here for convenience and backwards compatibility.
  void AddActor(vtkProp *p);
  void AddVolume(vtkProp *p);
  void RemoveActor(vtkProp *p);
  void RemoveVolume(vtkProp *p);

  // Description:
  // Add a light to the list of lights.
  void AddLight(vtkLight *);

  // Description:
  // Remove a light from the list of lights.
  void RemoveLight(vtkLight *);

  // Description:
  // Remove all lights from the list of lights.
  void RemoveAllLights();

  // Description:
  // Return the collection of lights.
  vtkLightCollection *GetLights();

  // Description:
  // Set the collection of lights.
  // We cannot name it SetLights because of TestSetGet
  // \pre lights_exist: lights!=0
  // \post lights_set: lights==this->GetLights()
  void SetLightCollection(vtkLightCollection *lights);

  // Description:
  // Create and add a light to renderer.
  void CreateLight(void);

  // Description:
  // Create a new Light sutible for use with this type of Renderer.
  // For example, a vtkMesaRenderer should create a vtkMesaLight
  // in this function.   The default is to just call vtkLight::New.
  virtual vtkLight *MakeLight();

  // Description:
  // Turn on/off two-sided lighting of surfaces. If two-sided lighting is
  // off, then only the side of the surface facing the light(s) will be lit,
  // and the other side dark. If two-sided lighting on, both sides of the
  // surface will be lit.
  vtkGetMacro(TwoSidedLighting,int);
  vtkSetMacro(TwoSidedLighting,int);
  vtkBooleanMacro(TwoSidedLighting,int);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  // If LightFollowCamera is on, lights that are designated as Headlights
  // or CameraLights will be adjusted to move with this renderer's camera.
  // If LightFollowCamera is off, the lights will not be adjusted.
  //
  // (Note: In previous versions of vtk, this light-tracking
  // functionality was part of the interactors, not the renderer. For
  // backwards compatibility, the older, more limited interactor
  // behavior is enabled by default. To disable this mode, turn the
  // interactor's LightFollowCamera flag OFF, and leave the renderer's
  // LightFollowCamera flag ON.)
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);

  // Description:
  // Turn on/off a flag which disables the automatic light creation capability.
  // Normally in VTK if no lights are associated with the renderer, then a light
  // is automatically created. However, in special circumstances this feature is
  // undesirable, so the following boolean is provided to disable automatic
  // light creation. (Turn AutomaticLightCreation off if you do not want lights
  // to be created.)
  vtkGetMacro(AutomaticLightCreation,int);
  vtkSetMacro(AutomaticLightCreation,int);
  vtkBooleanMacro(AutomaticLightCreation,int);

  // Description:
  // Ask the lights in the scene that are not in world space
  // (for instance, Headlights or CameraLights that are attached to the
  // camera) to update their geometry to match the active camera.
  virtual int UpdateLightsGeometryToFollowCamera(void);

  // Description:
  // Return the collection of volumes.
  vtkVolumeCollection *GetVolumes();

  // Description:
  // Return any actors in this renderer.
  vtkActorCollection *GetActors();

  // Description:
  // Specify the camera to use for this renderer.
  void SetActiveCamera(vtkCamera *);

  // Description:
  // Get the current camera. If there is not camera assigned to the
  // renderer already, a new one is created automatically.
  // This does *not* reset the camera.
  vtkCamera *GetActiveCamera();

  // Description:
  // Create a new Camera sutible for use with this type of Renderer.
  // For example, a vtkMesaRenderer should create a vtkMesaCamera
  // in this function.   The default is to just call vtkCamera::New.
  virtual vtkCamera *MakeCamera();

  // Description:
  // When this flag is off, the renderer will not erase the background
  // or the Zbuffer.  It is used to have overlapping renderers.
  // Both the RenderWindow Erase and Render Erase must be on
  // for the camera to clear the renderer.  By default, Erase is on.
  vtkSetMacro(Erase, int);
  vtkGetMacro(Erase, int);
  vtkBooleanMacro(Erase, int);

  // Description:
  // When this flag is off, render commands are ignored.  It is used to either
  // multiplex a vtkRenderWindow or render only part of a vtkRenderWindow.
  // By default, Draw is on.
  vtkSetMacro(Draw, int);
  vtkGetMacro(Draw, int);
  vtkBooleanMacro(Draw, int);

  // Description:
  // Add an culler to the list of cullers.
  void AddCuller(vtkCuller *);

  // Description:
  // Remove an actor from the list of cullers.
  void RemoveCuller(vtkCuller *);

  // Description:
  // Return the collection of cullers.
  vtkCullerCollection *GetCullers();

  // Description:
  // Set the intensity of ambient lighting.
  vtkSetVector3Macro(Ambient,double);
  vtkGetVectorMacro(Ambient,double,3);

  // Description:
  // Set/Get the amount of time this renderer is allowed to spend
  // rendering its scene. This is used by vtkLODActor's.
  vtkSetMacro(AllocatedRenderTime,double);
  virtual double GetAllocatedRenderTime();

  // Description:
  // Get the ratio between allocated time and actual render time.
  // TimeFactor has been taken out of the render process.
  // It is still computed in case someone finds it useful.
  // It may be taken away in the future.
  virtual double GetTimeFactor();

  // Description:
  // CALLED BY vtkRenderWindow ONLY. End-user pass your way and call
  // vtkRenderWindow::Render().
  // Create an image. This is a superclass method which will in turn
  // call the DeviceRender method of Subclasses of vtkRenderer.
  virtual void Render();

  // Description:
  // Create an image. Subclasses of vtkRenderer must implement this method.
  virtual void DeviceRender() =0;

  // Description:
  // Render translucent polygonal geometry. Default implementation just call
  // UpdateTranslucentPolygonalGeometry().
  // Subclasses of vtkRenderer that can deal with depth peeling must
  // override this method.
  // It updates boolean ivar LastRenderingUsedDepthPeeling.
  virtual void DeviceRenderTranslucentPolygonalGeometry();

  // Description:
  // Clear the image to the background color.
  virtual void Clear() {};

  // Description:
  // Returns the number of visible actors.
  int VisibleActorCount();

  // Description:
  // Returns the number of visible volumes.
  int VisibleVolumeCount();

  // Description:
  // Compute the bounding box of all the visible props
  // Used in ResetCamera() and ResetCameraClippingRange()
  void ComputeVisiblePropBounds( double bounds[6] );

  // Description:
  // Wrapper-friendly version of ComputeVisiblePropBounds
  double *ComputeVisiblePropBounds();

  // Description:
  // Reset the camera clipping range based on the bounds of the
  // visible actors. This ensures that no props are cut off
  void ResetCameraClippingRange();

  // Description:
  // Reset the camera clipping range based on a bounding box.
  // This method is called from ResetCameraClippingRange()
  // If Deering frustrum is used then the bounds get expanded
  // by the camera's modelview matrix.
  void ResetCameraClippingRange( double bounds[6] );
  void ResetCameraClippingRange( double xmin, double xmax,
                                 double ymin, double ymax,
                                 double zmin, double zmax);

  // Description:
  // Specify tolerance for near clipping plane distance to the camera as a
  // percentage of the far clipping plane distance. By default this will be
  // set to 0.01 for 16 bit zbuffers and 0.001 for higher depth z buffers
  vtkSetClampMacro(NearClippingPlaneTolerance,double,0,0.99);
  vtkGetMacro(NearClippingPlaneTolerance,double);

  // Description:
  // Automatically set up the camera based on the visible actors.
  // The camera will reposition itself to view the center point of the actors,
  // and move along its initial view plane normal (i.e., vector defined from
  // camera position to focal point) so that all of the actors can be seen.
  void ResetCamera();

  // Description:
  // Automatically set up the camera based on a specified bounding box
  // (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
  // that its focal point is the center of the bounding box, and adjust its
  // distance and position to preserve its initial view plane normal
  // (i.e., vector defined from camera position to focal point). Note: is
  // the view plane is parallel to the view up axis, the view up axis will
  // be reset to one of the three coordinate axes.
  void ResetCamera(double bounds[6]);

  // Description:
  // Alternative version of ResetCamera(bounds[6]);
  void ResetCamera(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax);

  // Description:
  // Specify the rendering window in which to draw. This is automatically set
  // when the renderer is created by MakeRenderer.  The user probably
  // shouldn't ever need to call this method.
  void SetRenderWindow(vtkRenderWindow *);
  vtkRenderWindow *GetRenderWindow() {return this->RenderWindow;};
  virtual vtkWindow *GetVTKWindow();

  // Description:
  // Turn on/off using backing store. This may cause the re-rendering
  // time to be slightly slower when the view changes. But it is
  // much faster when the image has not changed, such as during an
  // expose event.
  vtkSetMacro(BackingStore,int);
  vtkGetMacro(BackingStore,int);
  vtkBooleanMacro(BackingStore,int);

  // Description:
  // Turn on/off interactive status.  An interactive renderer is one that
  // can receive events from an interactor.  Should only be set if
  // there are multiple renderers in the same section of the viewport.
  vtkSetMacro(Interactive,int);
  vtkGetMacro(Interactive,int);
  vtkBooleanMacro(Interactive,int);

  // Description:
  // Set/Get the layer that this renderer belongs to.  This is only used if
  // there are layered renderers.
  vtkSetMacro(Layer, int);
  vtkGetMacro(Layer, int);

  // Description:
  // Normally a renderer is treated as transparent if Layer > 0. To treat a
  // renderer at Layer 0 as transparent, set this flag to true.
  vtkSetMacro(PreserveDepthBuffer, int);
  vtkGetMacro(PreserveDepthBuffer, int);
  vtkBooleanMacro(PreserveDepthBuffer, int);

  // Description:
  // Returns a boolean indicating if this renderer is transparent.  It is
  // transparent if it is not in the deepest layer of its render window.
  int  Transparent();

  // Description:
  // Convert world point coordinates to view coordinates.
  void WorldToView();

  // Description:
  // Convert view point coordinates to world coordinates.
  void ViewToWorld();
  virtual void ViewToWorld(double &wx, double &wy, double &wz);

  // Description:
  // Convert world point coordinates to view coordinates.
  virtual void WorldToView(double &wx, double &wy, double &wz);

  // Description:
  // Given a pixel location, return the Z value. The z value is
  // normalized (0,1) between the front and back clipping planes.
  double GetZ (int x, int y);

  // Description:
  // Return the MTime of the renderer also considering its ivars.
  unsigned long GetMTime();

  // Description:
  // Get the time required, in seconds, for the last Render call.
  vtkGetMacro( LastRenderTimeInSeconds, double );

  // Description:
  // Should be used internally only during a render
  // Get the number of props that were rendered using a
  // RenderOpaqueGeometry or RenderTranslucentPolygonalGeometry call.
  // This is used to know if something is in the frame buffer.
  vtkGetMacro( NumberOfPropsRendered, int );

  // Description:
  // Return the prop (via a vtkAssemblyPath) that has the highest z value
  // at the given x, y position in the viewport.  Basically, the top most
  // prop that renders the pixel at selectionX, selectionY will be returned.
  // If nothing was picked then NULL is returned.  This method selects from
  // the renderers Prop list.
  vtkAssemblyPath* PickProp(double selectionX, double selectionY)
    {
      return this->PickProp(selectionX, selectionY, selectionX, selectionY);
    }
  vtkAssemblyPath* PickProp(double selectionX1, double selectionY1,
                            double selectionX2, double selectionY2);

  // Description:
  // Do anything necessary between rendering the left and right viewpoints
  // in a stereo render. Doesn't do anything except in the derived
  // vtkIceTRenderer in ParaView.
  virtual void StereoMidpoint() { return; };

  // Description:
  // Compute the aspect ratio of this renderer for the current tile. When
  // tiled displays are used the aspect ratio of the renderer for a given
  // tile may be diferent that the aspect ratio of the renderer when rendered
  // in it entirity
  double GetTiledAspectRatio();

  // Description:
  // This method returns 1 if the ActiveCamera has already been set or
  // automatically created by the renderer. It returns 0 if the
  // ActiveCamera does not yet exist.
  int IsActiveCameraCreated()
    { return (this->ActiveCamera != NULL); }


  // Description:
  // Turn on/off rendering of translucent material with depth peeling
  // technique. The render window must have alpha bits (ie call
  // SetAlphaBitPlanes(1)) and no multisample buffer (ie call
  // SetMultiSamples(0) ) to support depth peeling.
  // If UseDepthPeeling is on and the GPU supports it, depth peeling is used
  // for rendering translucent materials.
  // If UseDepthPeeling is off, alpha blending is used.
  // Initial value is off.
  vtkSetMacro(UseDepthPeeling,int);
  vtkGetMacro(UseDepthPeeling,int);
  vtkBooleanMacro(UseDepthPeeling,int);

  // Description:
  // In case of use of depth peeling technique for rendering translucent
  // material, define the threshold under which the algorithm stops to
  // iterate over peel layers. This is the ratio of the number of pixels
  // that have been touched by the last layer over the total number of pixels
  // of the viewport area.
  // Initial value is 0.0, meaning rendering have to be exact. Greater values
  // may speed-up the rendering with small impact on the quality.
  vtkSetClampMacro(OcclusionRatio,double,0.0,0.5);
  vtkGetMacro(OcclusionRatio,double);

  // Description:
  // In case of depth peeling, define the maximum number of peeling layers.
  // Initial value is 4. A special value of 0 means no maximum limit.
  // It has to be a positive value.
  vtkSetMacro(MaximumNumberOfPeels,int);
  vtkGetMacro(MaximumNumberOfPeels,int);

  // Description:
  // Tells if the last call to DeviceRenderTranslucentPolygonalGeometry()
  // actually used depth peeling.
  // Initial value is false.
  vtkGetMacro(LastRenderingUsedDepthPeeling,int);

  // Description:
  // Set/Get a custom Render call. Allows to hook a Render call from an
  // external project.It will be used in place of vtkRenderer::Render() if it
  // is not NULL and its Used ivar is set to true.
  // Initial value is NULL.
  void SetDelegate(vtkRendererDelegate *d);
  vtkGetObjectMacro(Delegate,vtkRendererDelegate);

  // Description:
  // Set/Get a custom render pass.
  // Initial value is NULL.
  void SetPass(vtkRenderPass *p);
  vtkGetObjectMacro(Pass,vtkRenderPass);

  // Description:
  // Get the current hardware selector. If the Selector is set, it implies the
  // current render pass is for selection. Mappers/Properties may choose to
  // behave differently when rendering for hardware selection.
  vtkGetObjectMacro(Selector, vtkHardwareSelector);

  // Description:
  // Set/Get the texture to be used for the background. If set
  // and enabled this gets the priority over the gradient background.
  void SetBackgroundTexture(vtkTexture*);
  vtkGetObjectMacro(BackgroundTexture, vtkTexture);

  // Description:
  // Set/Get whether this viewport should have a textured background.
  // Default is off.
  vtkSetMacro(TexturedBackground,bool);
  vtkGetMacro(TexturedBackground,bool);
  vtkBooleanMacro(TexturedBackground,bool);

//BTX
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
  int                PreserveDepthBuffer;

  // Holds the result of ComputeVisiblePropBounds so that it is visible from
  // wrapped languages
  double              ComputedVisiblePropBounds[6];

  // Description:
  // Specifies the minimum distance of the near clipping
  // plane as a percentage of the far clipping plane distance.  Values below
  // this threshold are clipped to NearClippingPlaneTolerance*range[1].
  // Note that values which are too small may cause problems on systems
  // with low z-buffer resolution.
  double              NearClippingPlaneTolerance;

  // Description:
  // When this flag is off, the renderer will not erase the background
  // or the Zbuffer.  It is used to have overlapping renderers.
  // Both the RenderWindow Erase and Render Erase must be on
  // for the camera to clear the renderer.  By default, Erase is on.
  int Erase;

  // Description:
  // When this flag is off, render commands are ignored.  It is used to either
  // multiplex a vtkRenderWindow or render only part of a vtkRenderWindow.
  // By default, Draw is on.
  int Draw;

  // Description:
  // Ask all props to update and draw any opaque and translucent
  // geometry. This includes both vtkActors and vtkVolumes
  // Returns the number of props that rendered geometry.
  virtual int UpdateGeometry(void);

  // Description:
  // Ask all props to update and draw any translucent polygonal
  // geometry. This includes both vtkActors and vtkVolumes
  // Return the number of rendered props.
  // It is called once with alpha blending technique. It is called multiple
  // times with depth peeling technique.
  virtual int UpdateTranslucentPolygonalGeometry();

  // Description:
  // Ask the active camera to do whatever it needs to do prior to rendering.
  // Creates a camera if none found active.
  virtual int UpdateCamera(void);

  // Description:
  // Update the geometry of the lights in the scene that are not in world
  // space (for instance, Headlights or CameraLights that are attached to the
  // camera).
  virtual int UpdateLightGeometry(void);

  // Description:
  // Ask all lights to load themselves into rendering pipeline.
  // This method will return the actual number of lights that were on.
  virtual int UpdateLights(void) {return 0;}

  // Description:
  // Get the current camera and reset it only if it gets created
  // automatically (see GetActiveCamera).
  // This is only used internally.
  vtkCamera *GetActiveCameraAndResetIfCreated();

  // Description:
  // If this flag is on and the GPU supports it, depth peeling is used
  // for rendering translucent materials.
  // If this flag is off, alpha blending is used.
  // Initial value is off.
  int UseDepthPeeling;

  // Description:
  // In case of use of depth peeling technique for rendering translucent
  // material, define the threshold under which the algorithm stops to
  // iterate over peel layers. This is the ratio of the number of pixels
  // that have been touched by the last layer over the total number of pixels
  // of the viewport area.
  // Initial value is 0.0, meaning rendering have to be exact. Greater values
  // may speed-up the rendering with small impact on the quality.
  double OcclusionRatio;

  // Description:
  // In case of depth peeling, define the maximum number of peeling layers.
  // Initial value is 4. A special value of 0 means no maximum limit.
  // It has to be a positive value.
  int MaximumNumberOfPeels;

  // Description:
  // Tells if the last call to DeviceRenderTranslucentPolygonalGeometry()
  // actually used depth peeling.
  // Initial value is false.
  int LastRenderingUsedDepthPeeling;

#if !defined(VTK_LEGACY_REMOVE)
  // VISIBLE CELL SELECTION ----------------------------------------
  friend class vtkVisibleCellSelector;

  //Description:
  // Call to put the Renderer into a mode in which it will color visible
  // polygons with an enoded index. Later the pixel colors can be retrieved to
  // determine what objects lie behind each pixel.
  enum {NOT_SELECTING = 0, COLOR_BY_PROCESSOR, COLOR_BY_ACTOR,
        COLOR_BY_CELL_ID_HIGH, COLOR_BY_CELL_ID_MID, COLOR_BY_CELL_ID_LOW,
        COLOR_BY_VERTEX};

  vtkSetMacro(SelectMode, int);
  vtkSetMacro(SelectConst, unsigned int);

   // Description:
  // Allows the use of customized Painters for selection.
  // If none is supplied with this method, a default will be created
  // automatically.
  void SetIdentPainter(vtkIdentColoredPainter*);

  // Description:
  // Renders each polygon with a color that represents an selection index.
  virtual int UpdateGeometryForSelection(void);

  // Description:
  // Called by UpdateGeometryForSelection to temporarily swap in a mapper to
  // render a prop in selection mode.
  vtkPainter* SwapInSelectablePainter(vtkProp *,
                                              int &);

  // Description:
  // Called by UpdateGeometryForSelection to restore a prop's original mapper.
  void SwapOutSelectablePainter(vtkProp *,
                                vtkPainter*,
                                int );

  // Description:
  // Used in Selection to recover a selected prop from an index.
  vtkProp            **PropsSelectedFrom;
  int                PropsSelectedFromCount;

  // Ivars for visible cell selecting
  int SelectMode;
  unsigned int SelectConst;
  vtkIdentColoredPainter *IdentPainter;
  // End Ivars for visible cell selecting.
#endif

  // HARDWARE SELECTION ----------------------------------------
  friend class vtkHardwareSelector;

  // Description:
  // Called by vtkHardwareSelector when it begins rendering for selection.
  void SetSelector(vtkHardwareSelector* selector)
    { this->Selector = selector; this->Modified(); }

  // End Ivars for visible cell selecting.
  vtkHardwareSelector* Selector;

  //---------------------------------------------------------------
  friend class vtkRendererDelegate;
  vtkRendererDelegate *Delegate;

  friend class vtkRenderPass;
  vtkRenderPass *Pass;

  bool TexturedBackground;
  vtkTexture* BackgroundTexture;

private:
  vtkRenderer(const vtkRenderer&);  // Not implemented.
  void operator=(const vtkRenderer&);  // Not implemented.
//ETX
};

inline vtkLightCollection *vtkRenderer::GetLights() {
  return this->Lights;
}

// Description:
// Get the list of cullers for this renderer.
inline vtkCullerCollection *vtkRenderer::GetCullers(){return this->Cullers;}


#endif
