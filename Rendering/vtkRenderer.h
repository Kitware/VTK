/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
// vtkRenderWindow vtkActor vtkCamera vtkLight vtkVolume vtkRayCaster

#ifndef __vtkRenderer_h
#define __vtkRenderer_h

#include "vtkMatrix4x4.h"
#include "vtkLightCollection.h"
#include "vtkVolumeCollection.h"
#include "vtkCullerCollection.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkViewport.h"
#include "vtkActorCollection.h"

class vtkRenderWindow;
class vtkRayCaster;
class vtkVolume;
class vtkRayCaster;
class vtkCuller;

class VTK_EXPORT vtkRenderer : public vtkViewport
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
  // Add a light to the list of lights.
  void AddLight(vtkLight *);

  // Description:
  // Add/Remove different types of props to the renderer.
  // These methods are all synonyms to AddProp and RemoveProp.
  // They are here for convenience and backwards compatibility.
  void AddActor(vtkProp *p) {this->AddProp(p);};
  void AddVolume(vtkProp *p) {this->AddProp(p);};
  void RemoveActor(vtkProp *p) {this->Actors->RemoveItem(p);this->RemoveProp(p);};
  void RemoveVolume(vtkProp *p) {this->Volumes->RemoveItem(p);this->RemoveProp(p);};

  // Description:
  // Remove a light from the list of lights.
  void RemoveLight(vtkLight *);

  // Description:
  // Return the collection of lights.
  vtkLightCollection *GetLights();
  
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
  // Get the current camera.
  vtkCamera *GetActiveCamera();

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
  vtkSetVector3Macro(Ambient,float);
  vtkGetVectorMacro(Ambient,float,3);

  // Description:
  // Set/Get the amount of time this renderer is allowed to spend
  // rendering its scene. This is used by vtkLODActor's.
  vtkSetMacro(AllocatedRenderTime,float);
  virtual float GetAllocatedRenderTime();

  // Description:
  // Get the ratio between allocated time and actual render time.
  // TimeFactor has been taken out of the render process.  
  // It is still computed in case someone finds it useful.
  // It may be taken away in the future.
  virtual float GetTimeFactor();

  // Description:
  // Create an image. This is a superclass method which will in turn 
  // call the DeviceRender method of Subclasses of vtkRenderer
  virtual void Render();

  // Description:
  // Create an image. Subclasses of vtkRenderer must implement this method.
  virtual void DeviceRender() =0;
  

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
  // Create and add a light to renderer.
  void CreateLight(void);

  // Description:
  // Compute the bounding box of all the visible props
  // Used in ResetCamera() and ResetCameraClippingRange()
  void ComputeVisiblePropBounds( float bounds[6] );

  // Description:
  // Reset the camera clipping range based on the bounds of the
  // visible actors. This ensures that no props are cut off
  void ResetCameraClippingRange();

  // Description:
  // Reset the camera clipping range based on a bounding box.
  // This method is called from ResetCameraClippingRange()
  void ResetCameraClippingRange( float bounds[6] );
  void ResetCameraClippingRange( float xmin, float xmax, 
				 float ymin, float ymax, 
				 float zmin, float zmax);

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
  void ResetCamera(float bounds[6]);

  // Description:
  // Alternative version of ResetCamera(bounds[6]);
  void ResetCamera(float xmin, float xmax, float ymin, float ymax, 
                   float zmin, float zmax);

  // Description:
  // Specify the rendering window in which to draw. This is automatically set
  // when the renderer is created by MakeRenderer.  The user probably
  // shouldn't ever need to call this method.
  void SetRenderWindow(vtkRenderWindow *);
  vtkRenderWindow *GetRenderWindow() {return this->RenderWindow;};
  virtual vtkWindow *GetVTKWindow();
  
  // Description:
  // Turn on/off two-sided lighting of surfaces. If two-sided lighting is
  // off, then only the side of the surface facing the light(s) will be lit,
  // and the other side dark. If two-sided lighting on, both sides of the 
  // surface will be lit.
  vtkGetMacro(TwoSidedLighting,int);
  vtkSetMacro(TwoSidedLighting,int);
  vtkBooleanMacro(TwoSidedLighting,int);

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
  // Returns a boolean indicating if this renderer is transparent.  It is
  // transparent if it is not in the deepest layer of its render window.
  int  Transparent();

  // Description:
  // Convert world point coordinates to view coordinates.
  void WorldToView();

  // Description:
  // Convert view point coordinates to world coordinates.
  void ViewToWorld();
  virtual void ViewToWorld(float &wx, float &wy, float &wz);

  // Description:
  // Convert world point coordinates to view coordinates.
  virtual void WorldToView(float &wx, float &wy, float &wz);

  // Description:
  // Return the RayCaster being used by this renderer.
  vtkGetObjectMacro(RayCaster,vtkRayCaster);

  // Description:
  // Given a pixel location, return the Z value
  float GetZ (int x, int y);

  // Description:
  // Render the overlay actors. This gets called from the RenderWindow
  // because it may need to be synchronized to happen after the
  // buffers have been swapped.
  void RenderOverlay();

  // Description:
  // Return the MTime of the renderer also considering its ivars.
  unsigned long GetMTime();

  // Description:
  // Get the time required, in seconds, for the last Render call.
  vtkGetMacro( LastRenderTimeInSeconds, float );

  // Description:
  // Detects reference loop renderer<->rayCaster
  void UnRegister(vtkObject *o);

  // Description:
  // Should be used internally only during a render
  // Get the number of props that were rendered using a
  // RenderOpaqueGeometry or RenderTranslucentGeometry call.
  // This is used to know if something is in the frame buffer.
  vtkGetMacro( NumberOfPropsRenderedAsGeometry, int );

  // Description:
  // Return the prop (via a vtkAssemblyPath) that has the highest z value 
  // at the given x, y position in the viewport.  Basically, the top most 
  // prop that renders the pixel at selectionX, selectionY will be returned.
  // If nothing was picked then NULL is returned.  This method selects from 
  // the renderers Prop list.
  vtkAssemblyPath* PickProp(float selectionX, float selectionY);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  // If LightFollowCamera is on, lights that are designated as Headlights
  // or CameraLights will be adjusted to move with this renderer's camera.
  // If LightFollowCamera is off, the lights will not be adjusted.  
  //
  // (Note: In previous versions of vtk, this light-tracking
  // functionality was part of the interactors, not the renderer.  For
  // backwards compatibility, the older, more limited interactor
  // behavior is enabled by default.  This mode has the possibly
  // unexpected property of turning the first light into a headlight,
  // no matter what its light type is.  To disable this mode, turn the
  // interactor's LightFollowCamera flag OFF, and leave the renderer's
  // LightFollowCamera flag ON.)
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);

protected:
  vtkRenderer();
  ~vtkRenderer();
  vtkRenderer(const vtkRenderer&);
  void operator=(const vtkRenderer&);

  // internal method for doing a render for picking purposes
  virtual void PickRender(vtkPropCollection *props);
  virtual void PickGeometry();
  
  vtkRayCaster *RayCaster;

  vtkCamera *ActiveCamera;
  vtkLight  *CreatedLight;

  vtkLightCollection *Lights;
  vtkCullerCollection *Cullers;

  vtkActorCollection *Actors;
  vtkVolumeCollection *Volumes;
  
  float              Ambient[3];  
  vtkRenderWindow    *RenderWindow;
  float              AllocatedRenderTime;
  float              TimeFactor;
  int                TwoSidedLighting;
  int                BackingStore;
  unsigned char      *BackingImage;
  vtkTimeStamp       RenderTime;

  float              LastRenderTimeInSeconds;

  int                LightFollowCamera;

  // Allocate the time for each prop
  void               AllocateTime();

  // Internal variables indicating the number of props
  // that have been or will be rendered in each category.
  int                NumberOfPropsRenderedAsGeometry;
  int                NumberOfPropsToRayCast;
  int                NumberOfPropsToRenderIntoImage;

  // A temporary list of props used for culling, and traversal
  // of all props when rendering
  vtkProp            **PropArray;
  int                PropArrayCount;

  // A sublist of props that need ray casting
  vtkProp            **RayCastPropArray;

  // A sublist of props that want to be rendered into an image
  vtkProp            **RenderIntoImagePropArray;

  // A temporary list used for picking
  vtkAssemblyPath    **PathArray;
  int                PathArrayCount;

  // Indicates if the renderer should receive events from an interactor.
  // Typically only used in conjunction with transparent renderers.
  int                Interactive;

  // Shows what layer this renderer belongs to.  Only of interested when
  // there are layered renderers.
  int                Layer;

  // Description:
  // Ask all props to update and draw any opaque and translucent
  // geometry. This includes both vtkActors and vtkVolumes
  // Returns the number of props that rendered geometry.
  virtual int UpdateGeometry(void);

  // Description:
  // Ask the active camera to do whatever it needs to do prior to rendering.
  // Creates a camera if none found active.
  virtual int UpdateCamera(void);

  // Description:
  // Update the geometry of the lights in the scene that are not in world space
  // (for instance, Headlights or CameraLights that are attached to the camera).
  virtual int UpdateLightGeometry(void);

  // Description:
  // Ask all lights to load themselves into rendering pipeline.
  // This method will return the actual number of lights that were on.
  virtual int UpdateLights(void) {return 0;};
};

// Description:
// Get the list of lights for this renderer.
inline vtkLightCollection *vtkRenderer::GetLights() {return this->Lights;}

// Description:
// Get the list of cullers for this renderer.
inline vtkCullerCollection *vtkRenderer::GetCullers(){return this->Cullers;}


#endif
