/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewport.h
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
// .NAME vtkViewport - abstract specification for Viewports
// .SECTION Description
// vtkViewport provides an abstract specification for Viewports. A Viewport
// is an object that controls the rendering process for objects. Rendering
// is the process of converting geometry, a specification for lights, and 
// a camera view into an image. vtkViewport also performs coordinate 
// transformation between world coordinates, view coordinates (the computer
// graphics rendering coordinate system), and display coordinates (the 
// actual screen coordinates on the display device). Certain advanced 
// rendering features such as two-sided lighting can also be controlled.

// .SECTION See Also
// vtkWindow vtkImager vtkRenderer

#ifndef __vtkViewport_h
#define __vtkViewport_h

#include "vtkObject.h"
#include "vtkPropCollection.h"
#include "vtkActor2DCollection.h"

class vtkWindow;
class vtkAssemblyPath;

class VTK_COMMON_EXPORT vtkViewport : public vtkObject
{
public:
  vtkTypeMacro(vtkViewport,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a prop to the list of props. Prop is the superclass of all 
  // actors, volumes, 2D actors, composite props etc.
  void AddProp(vtkProp *);

  // Description:
  // Return any props in this viewport.
  vtkPropCollection *GetProps() {return this->Props;};

  // Description:
  // Remove an actor from the list of actors.
  void RemoveProp(vtkProp *);

  // Description:
  // Add/Remove different types of props to the renderer.
  // These methods are all synonyms to AddProp and RemoveProp.
  // They are here for convenience and backwards compatibility.
  void AddActor2D(vtkProp* p) {this->AddProp(p);};
  void RemoveActor2D(vtkProp* p);
  vtkActor2DCollection *GetActors2D();

  // Description:
  // Set/Get the background color of the rendering screen using an rgb color
  // specification.
  vtkSetVector3Macro(Background,float);
  vtkGetVectorMacro(Background,float,3);

  // Description:
  // Set the aspect ratio of the rendered image. This is computed 
  // automatically and should not be set by the user.
  vtkSetVector2Macro(Aspect,float);
  vtkGetVectorMacro(Aspect,float,2);
  void ComputeAspect();
  
  // Description:
  // Set the aspect ratio of a pixel in the rendered image. 
  // This factor permits the image to rendered anisotropically
  // (i.e., stretched in one direction or the other).
  vtkSetVector2Macro(PixelAspect,float);
  vtkGetVectorMacro(PixelAspect,float,2);

  // Description:
  // Specify the viewport for the Viewport to draw in the rendering window. 
  // Coordinates are expressed as (xmin,ymin,xmax,ymax), where each
  // coordinate is 0 <= coordinate <= 1.0.
  vtkSetVector4Macro(Viewport,float);
  vtkGetVectorMacro(Viewport,float,4);

  // Description:
  // Set/get a point location in display (or screen) coordinates.
  // The lower left corner of the window is the origin and y increases
  // as you go up the screen.
  vtkSetVector3Macro(DisplayPoint,float);
  vtkGetVectorMacro(DisplayPoint,float,3);
  void GetDisplayPoint(double *a) 
    {
      a[0] = this->DisplayPoint[0];
      a[1] = this->DisplayPoint[1];
      a[2] = this->DisplayPoint[2];
      a[3] = 1.0;
    };

  // Description:
  // Specify a point location in view coordinates. The origin is in the 
  // middle of the viewport and it extends from -1 to 1 in all three
  // dimensions.
  vtkSetVector3Macro(ViewPoint,float);
  vtkGetVectorMacro(ViewPoint,float,3);

  // Description:
  // Specify a point location in world coordinates. This method takes 
  // homogeneous coordinates. 
  vtkSetVector4Macro(WorldPoint,float);
  vtkGetVectorMacro(WorldPoint,float,4);
  void GetWorldPoint(double *a) 
    {
      a[0] = this->WorldPoint[0];
      a[1] = this->WorldPoint[1];
      a[2] = this->WorldPoint[2];
      a[3] = this->WorldPoint[3];
    };
   
  // Description:
  // Return the center of this viewport in display coordinates.
  virtual float *GetCenter();

  // Description:
  // Is a given display point in this Viewport's viewport.
  virtual int IsInViewport(int x,int y); 

  // Description:
  // Return the vtkWindow that owns this vtkViewport.
  virtual vtkWindow *GetVTKWindow() = 0;

  // Description:
  // Specify a function to be called before rendering process begins.
  // Function will be called with argument provided.
  void SetStartRenderMethod(void (*f)(void *), void *arg);

  // Description:
  // Specify a function to be called when rendering process completes.
  // Function will be called with argument provided.
  void SetEndRenderMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetStartRenderMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetEndRenderMethodArgDelete(void (*f)(void *));

  // Description:
  // Convert display coordinates to view coordinates.
  virtual void DisplayToView(); // these get modified in subclasses

  // Description:
  // Convert view coordinates to display coordinates.
  virtual void ViewToDisplay(); // to handle stereo rendering

  // Description:
  // Convert world point coordinates to view coordinates.
  virtual void WorldToView();

  // Description:
  // Convert view point coordinates to world coordinates.
  virtual void ViewToWorld();

  // Description:
  // Convert display (or screen) coordinates to world coordinates.
  void DisplayToWorld() {this->DisplayToView(); this->ViewToWorld();};

  // Description:
  // Convert world point coordinates to display (or screen) coordinates.
  void WorldToDisplay() {this->WorldToView(); this->ViewToDisplay();};

  // Description:
  // These methods map from one coordinate system to another.
  // They are primarily used by the vtkCoordinate object and
  // are often strung together. These methods return valid information
  // only if the window has been realized (e.g., GetSize() returns
  // something other than (0,0)).
  virtual void LocalDisplayToDisplay(float &x, float &y);
  virtual void DisplayToNormalizedDisplay(float &u, float &v);
  virtual void NormalizedDisplayToViewport(float &x, float &y);
  virtual void ViewportToNormalizedViewport(float &u, float &v);
  virtual void NormalizedViewportToView(float &x, float &y, float &z);
  virtual void ViewToWorld(float &, float &, float &) {};
  virtual void DisplayToLocalDisplay(float &x, float &y);
  virtual void NormalizedDisplayToDisplay(float &u, float &v);
  virtual void ViewportToNormalizedDisplay(float &x, float &y);
  virtual void NormalizedViewportToViewport(float &u, float &v);
  virtual void ViewToNormalizedViewport(float &x, float &y, float &z);
  virtual void WorldToView(float &, float &, float &) {};

  // Description:
  // Get the size and origin of the viewport in display coordinates. Note:
  // if the window has not yet been realized, GetSize() and GetOrigin() 
  // return (0,0).
  int *GetSize();
  int *GetOrigin();

  // The following methods describe the public pick interface for picking
  // Props in a viewport.

  // Description:
  // Return the Prop that has the highest z value at the given x, y position
  // in the viewport.  Basically, the top most prop that renders the pixel at
  // selectionX, selectionY will be returned.  If no Props are there NULL is
  // returned.  This method selects from the Viewports Prop list.
  virtual vtkAssemblyPath* PickProp(float selectionX, float selectionY) = 0;

  // Description:
  // Same as PickProp with two arguments, but selects from the given
  // collection of Props instead of the Renderers props.  Make sure
  // the Props in the collection are in this renderer.
  vtkAssemblyPath* PickPropFrom(float selectionX, float selectionY, 
                                vtkPropCollection*);
  
  // Description:
  // Methods used to return the pick (x,y) in local display coordinates (i.e.,
  // it's that same as selectionX and selectionY).
  vtkGetMacro(PickX, float);
  vtkGetMacro(PickY, float);
  vtkGetMacro(IsPicking, int);

  // Description: 
  // Return the Z value for the last picked Prop.
  virtual float GetPickedZ() = 0;
  
protected:
  // Create a vtkViewport with a black background, a white ambient light, 
  // two-sided lighting turned on, a viewport of (0,0,1,1), and back face 
  // culling turned off.
  vtkViewport();
  ~vtkViewport();
  vtkViewport(const vtkViewport&);
  void operator=(const vtkViewport&);

  //BTX
  // Picking functions to be implemented by sub-classes
  // Perform the main picking loop
  virtual void DevicePickRender() = 0;
  // Enter a pick mode
  virtual void StartPick(unsigned int pickFromSize) = 0;
  // Set the pick id to the next id before drawing an object
  virtual void UpdatePickId() = 0;
  // Exit Pick mode
  virtual void DonePick() = 0; 
  // Return the id of the picked object, only valid after a call to DonePick
  virtual unsigned int GetPickedId() = 0;
  //ETX

  // Ivars for picking
  // Store a picked Prop (contained in an assembly path)
  vtkAssemblyPath* PickedProp;
  vtkPropCollection* PickFromProps;
  // Boolean flag to determine if picking is enabled for this render
  int IsPicking;
  unsigned int CurrentPickId;
  float PickX;
  float PickY;
  // End Ivars for picking
  
  vtkPropCollection *Props;
  vtkActor2DCollection *Actors2D;
  vtkWindow *VTKWindow;
  float Background[3];  
  float Viewport[4];
  float Aspect[2];
  float PixelAspect[2];
  float Center[2];

  unsigned long StartTag;
  unsigned long EndTag;

  int Size[2];
  int Origin[2];
  float DisplayPoint[3];
  float ViewPoint[3];
  float WorldPoint[4];

};



#endif
