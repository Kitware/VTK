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
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkViewport.h"

class vtkRenderWindow;
class vtkRayCaster;
class vtkVolume;
class vtkRayCaster;

class VTK_EXPORT vtkRenderer : public vtkViewport
{
public:
  vtkRenderer();
  ~vtkRenderer();
  static vtkRenderer *New();
  const char *GetClassName() {return "vtkRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddLight(vtkLight *);
  void AddActor(vtkActor *);
  void AddVolume(vtkVolume *);

  void RemoveLight(vtkLight *);
  void RemoveActor(vtkActor *);
  void RemoveVolume(vtkVolume *);

  vtkLightCollection *GetLights();
  vtkActorCollection *GetActors();
  vtkVolumeCollection *GetVolumes();

  void SetActiveCamera(vtkCamera *);
  vtkCamera *GetActiveCamera();

  // Description:
  // Set the intensity of ambient lighting.
  vtkSetVector3Macro(Ambient,float);
  vtkGetVectorMacro(Ambient,float,3);

  // Description:
  // Set/Get the amount of time this renderer is allowed to spend
  // rendering its scene. Zero indicates an infinite amount of time.
  // This is used by vtkLODActor's.
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
  virtual int UpdateActors(void) {return 0;};

  // Description:
  // Ask the volumes to build and draw themselves.
  // Returns the number of volumes processed.
  virtual int UpdateVolumes(void) {return 0;};

  // Description:
  // Ask the active camera to do whatever it needs to do prior to rendering.
  // Creates a camera if none found active.
  virtual int UpdateCameras(void) {return 0;};

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

  void ResetCamera();
  void ResetCamera(float bounds[6]);

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

  void WorldToView();
  void ViewToWorld();
  virtual void ViewToWorld(float &wx, float &wy, float &wz);
  virtual void WorldToView(float &wx, float &wy, float &wz);

  vtkGetObjectMacro(RayCaster,vtkRayCaster);
  float GetZ (int x, int y);

  void Render2D();

  unsigned long int GetMTime();

protected:

  vtkRayCaster *RayCaster;

  vtkCamera *ActiveCamera;
  vtkLight  *CreatedLight;
  vtkLightCollection Lights;
  vtkActorCollection Actors;
  vtkVolumeCollection Volumes;

  float Ambient[3];  
  vtkRenderWindow *RenderWindow;
  float AllocatedRenderTime;
  int   TwoSidedLighting;
  int   BackingStore;
  unsigned char *BackingImage;
  vtkTimeStamp RenderTime;
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


#endif
