/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
#include "vtkViewport.h"

class vtkRenderWindow;
class vtkRayCaster;
class vtkVolume;
class vtkRayCaster;
class vtkCuller;

class VTK_EXPORT vtkRenderer : public vtkViewport
{
public:

// Description:
// Create a vtkRenderer with a black background, a white ambient light, 
// two-sided lighting turned on, a viewport of (0,0,1,1), and backface culling
// turned off.
  vtkRenderer();

  ~vtkRenderer();
  static vtkRenderer *New();
  const char *GetClassName() {return "vtkRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);


// Description:
// Add a light to the list of lights.
  void AddLight(vtkLight *);


// Description:
// Add an actor to the list of actors.
  void AddActor(vtkActor *);


// Description:
// Add a volume to the list of volumes.
  void AddVolume(vtkVolume *);



// Description:
// Remove a light from the list of lights.
  void RemoveLight(vtkLight *);


// Description:
// Remove an actor from the list of actors.
  void RemoveActor(vtkActor *);


// Description:
// Remove a volume from the list of volumes.
  void RemoveVolume(vtkVolume *);


  vtkLightCollection *GetLights();
  vtkActorCollection *GetActors();
  vtkVolumeCollection *GetVolumes();


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

  vtkCullerCollection *GetCullers();

  // Description:
  // Set the intensity of ambient lighting.
  vtkSetVector3Macro(Ambient,float);
  vtkGetVectorMacro(Ambient,float,3);

  // Description:
  // Set/Get the amount of time this renderer is allowed to spend
  // rendering its scene. This is used by vtkLODActor's.
  vtkSetMacro(AllocatedRenderTime,float);
  vtkGetMacro(AllocatedRenderTime,float);

  // Description:
  // Create an image. This is a superclass method which will in turn 
  // call the DeviceRender method of Subclasses of vtkRenderer
  virtual void Render();

  // Description:
  // Create an image. Subclasses of vtkRenderer must implement this method.
  virtual void DeviceRender() {};

  // Description:
  // Ask all actors to build and draw themselves.
  // Returns the number of actors processed.
  virtual int UpdateActors(void);

  // Description:
  // Ask the volumes to build and draw themselves.
  // Returns the number of volumes processed.
  virtual int UpdateVolumes(void);

  // Description:
  // Ask the active camera to do whatever it needs to do prior to rendering.
  // Creates a camera if none found active.
  virtual int UpdateCameras(void);

  // Description:
  // Ask all lights to load themselves into rendering pipeline.
  // This method will return the actual number of lights that were on.
  virtual int UpdateLights(void) {return 0;};

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

  vtkRenderWindow *GetRenderWindow() {return RenderWindow;};
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
// Convert world point coordinates to view coordinates.
  void WorldToView();


// Description:
// Convert view point coordinates to world coordinates.
  void ViewToWorld();

  virtual void ViewToWorld(float &wx, float &wy, float &wz);

// Description:
// Convert world point coordinates to view coordinates.
  virtual void WorldToView(float &wx, float &wy, float &wz);


  vtkGetObjectMacro(RayCaster,vtkRayCaster);

// Description:
// Given a pixel location, return the Z value
  float GetZ (int x, int y);


  void Render2D();

  unsigned long int GetMTime();

  // Description:
  // Get the time required, in seconds, for the last Render call.
  vtkGetMacro( LastRenderTimeInSeconds, float );

protected:

  vtkRayCaster *RayCaster;

  vtkCamera *ActiveCamera;
  vtkLight  *CreatedLight;
  vtkLightCollection Lights;
  vtkActorCollection Actors;
  vtkVolumeCollection Volumes;

  vtkCullerCollection Cullers;

  float              Ambient[3];  
  vtkRenderWindow    *RenderWindow;
  float              AllocatedRenderTime;
  int                TwoSidedLighting;
  int                BackingStore;
  unsigned char      *BackingImage;
  vtkTimeStamp       RenderTime;

  float              LastRenderTimeInSeconds;
};

// Description:
// Get the list of lights for this renderer.
inline vtkLightCollection *vtkRenderer::GetLights() {return &(this->Lights);}

// Description:
// Get the list of actors for this renderer.
inline vtkActorCollection *vtkRenderer::GetActors() {return &(this->Actors);}

// Description:
// Get the list of volumes for this renderer.
inline vtkVolumeCollection *vtkRenderer::GetVolumes(){return &(this->Volumes);}

// Description:
// Get the list of cullers for this renderer.
inline vtkCullerCollection *vtkRenderer::GetCullers(){return &(this->Cullers);}


#endif
