/*=========================================================================

  Program:   Visualization Library
  Module:    Renderer.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlRenderer - abstract specification for renderers
// .SECTION Description
// vlRenderer provides an abstract specification for renderers. A renderer
// is an object that controls the rendering process for objects. Rendering
// is the process of converting geometry, a specification for lights, and 
// a camera view into an image. vlRenderer also performs coordinate 
// transformation between world coordinates, view coordinates (the computer
// graphics rendering coordinate system), and display coordinates (the 
// actual screen coordinates on the display device).

#ifndef __vlRenderer_hh
#define __vlRenderer_hh

#include "Object.hh"
#include "Mat4x4.hh"
#include "LightC.hh"
#include "Camera.hh"
#include "ActorC.hh"
#include "GeomPrim.hh"

class vlRenderWindow;

class vlRenderer : public vlObject
{
public:
  vlRenderer();
  char *GetClassName() {return "vlRenderer";};
  void PrintSelf(ostream& os, vlIndent indent);

  void AddLights(vlLight *);
  void AddActors(vlActor *);
  void RemoveLights(vlLight *);
  void RemoveActors(vlActor *);
  vlLightCollection *GetLights();
  vlActorCollection *GetActors();
  void SetActiveCamera(vlCamera *);
  vlCamera *GetActiveCamera();

  // Description:
  // Set the background color of the rendering screen using an rgb color
  // specification.
  vlSetVector3Macro(Background,float);
  vlGetVectorMacro(Background,float,3);

  // Description:
  // Set the aspect ratio of the rendered image.
  vlSetVector2Macro(Aspect,float);
  vlGetVectorMacro(Aspect,float,2);

  // Description:
  // Set the level of ambient lighting.
  vlSetVector3Macro(Ambient,float);
  vlGetVectorMacro(Ambient,float,3);

  // Description:
  // Turn on/off whether objects are lit from behind with another light.
  // If backlighting is on, for every light that is created, a second 
  // opposing light is created to backlight the object.
  vlSetMacro(BackLight,int);
  vlGetMacro(BackLight,int);
  vlBooleanMacro(BackLight,int);

  // Description:
  // Turn on/off erasing the screen between images. Allows multiple exposure
  // sequences if turned on.
  vlSetMacro(Erase,int);
  vlGetMacro(Erase,int);
  vlBooleanMacro(Erase,int);

  // Description:
  // Create an image.
  virtual void Render() = 0;

  // Description:
  // Get a device specific geometry representation.
  virtual vlGeometryPrimitive *GetPrimitive(char *) = 0;
  
  // Description:
  // Ask all actors to build and draw themselves.
  virtual int UpdateActors(void) = 0;

  // Description:
  // Ask the camera to load its view matrix.
  virtual int UpdateCameras(void) = 0;

  // Description:
  // Ask all lights to load themselves into rendering pipeline.
  virtual int UpdateLights(void) = 0;

  void DoCameras();
  void DoLights();
  void DoActors();
  void ResetCamera();
  void ResetCamera(float bounds[6]);

  void SetRenderWindow(vlRenderWindow *);
  vlRenderWindow *GetRenderWindow() {return RenderWindow;};
  
  // Description:
  // Specify a point location in display (or screen) coordinates.
  vlSetVector3Macro(DisplayPoint,float);
  vlGetVectorMacro(DisplayPoint,float,3);

  // Description:
  // Specify a point location in view coordinates.
  vlSetVector3Macro(ViewPoint,float);
  vlGetVectorMacro(ViewPoint,float,3);

  // Description:
  // Specify a point location in world coordinates.
  vlSetVector4Macro(WorldPoint,float);
  vlGetVectorMacro(WorldPoint,float,4);

  // Description:
  // Specify the area for the renderer to draw in the rendering window. 
  // Coordinates are expressed as (xmin,ymin,xmax,ymax) where each
  // coordinate is 0 <= coordinate <= 1.0.
  vlSetVector4Macro(Viewport,float);
  vlGetVectorMacro(Viewport,float,4);

  void DisplayToView();
  void ViewToDisplay();
  void WorldToView();
  void ViewToWorld();
  void DisplayToWorld();
  void WorldToDisplay();

protected:
  vlCamera *ActiveCamera;
  vlLightCollection Lights;
  vlActorCollection Actors;
  float Ambient[3];  
  float Background[3];  
  int BackLight;
  vlRenderWindow *RenderWindow;
  float DisplayPoint[3];
  float ViewPoint[3];
  float WorldPoint[4];
  float Viewport[4];
  int   Erase;
  float Aspect[2];
};

// Description:
// Get the list of lights for this renderer.
inline vlLightCollection *vlRenderer::GetLights() {return &(this->Lights);};

// Description:
// Get the list of actors for this renderer.
inline vlActorCollection *vlRenderer::GetActors() {return &(this->Actors);};

// Description:
// Convert display (or screen) coordinates to world coordinates.
inline void vlRenderer::DisplayToWorld() {DisplayToView(); ViewToWorld();};

// Description:
// Convert world point coordinates to display (or screen) coordinates.
inline void vlRenderer::WorldToDisplay() {WorldToView(); ViewToDisplay();};

#endif
