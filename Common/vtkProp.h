/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.h
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
// .NAME vtkProp - abstract superclass for all actors, volumes and annotations
// .SECTION Description
// vtkProp is an abstract superclass for any objects that can exist in a
// rendered scene (either 2D or 3D). Instances of vtkProp may respond to
// various render methods (e.g., RenderOpaqueGeometry()). vtkProp also
// defines the API for picking, LOD manipulation, and common instance 
// variables that control visibility, picking, and dragging.
// .SECTION See Also
// vtkActor2D vtkActor vtkVolume vtkProp3D

#ifndef __vtkProp_h
#define __vtkProp_h

#include "vtkObject.h"
#include "vtkRayCastStructures.h"
#include "vtkAssemblyPaths.h"

class vtkViewport;
class vtkPropCollection;
class vtkWindow;
class vtkMatrix4x4;

class VTK_EXPORT vtkProp : public vtkObject
{
public:
  // Description:
  // Creates an instance with visibility=1, pickable=1,
  // and dragable=1.
  static vtkProp* New();

  vtkTypeMacro(vtkProp,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description: 
  // For some exporters and other other operations we must be
  // able to collect all the actors or volumes. These methods
  // are used in that process.
  virtual void GetActors(vtkPropCollection *) {}
  virtual void GetActors2D(vtkPropCollection *) {}
  virtual void GetVolumes(vtkPropCollection *) {}

  // Description:
  // Set/Get visibility of this vtkProp.
  vtkSetMacro(Visibility, int);
  vtkGetMacro(Visibility, int);
  vtkBooleanMacro(Visibility, int);

  // Description:
  // Set/Get the pickable instance variable.  This determines if the vtkProp
  // can be picked (typically using the mouse). Also see dragable.
  vtkSetMacro(Pickable,int);
  vtkGetMacro(Pickable,int);
  vtkBooleanMacro(Pickable,int);

  // Description:
  // This method is invoked when an instance of vtkProp (or subclass, 
  // e.g., vtkActor) is picked by vtkPicker.
  void SetPickMethod(void (*f)(void *), void *arg);
  void SetPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Method invokes PickMethod() if one defined and the prop is picked.
  virtual void Pick();

  // Description:
  // Set/Get the value of the dragable instance variable. This determines if 
  // an Prop, once picked, can be dragged (translated) through space.
  // This is typically done through an interactive mouse interface.
  // This does not affect methods such as SetPosition, which will continue
  // to work.  It is just intended to prevent some vtkProp'ss from being
  // dragged from within a user interface.
  vtkSetMacro(Dragable,int);
  vtkGetMacro(Dragable,int);
  vtkBooleanMacro(Dragable,int);

  // Description:
  // Return the mtime of anything that would cause the rendered image to 
  // appear differently. Usually this involves checking the mtime of the 
  // prop plus anything else it depends on such as properties, textures
  // etc.
  virtual unsigned long GetRedrawMTime() {return this->GetMTime();}
  
  // Description:
  // Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  // in world coordinates. NULL means that the bounds are not defined.
  virtual float *GetBounds() {return NULL;}

  // Description:
  // Shallow copy of this vtkProp.
  virtual void ShallowCopy(vtkProp *prop);

  // Description:
  // vtkProp and its subclasses can be picked by subclasses of
  // vtkAbstractPicker (e.g., vtkPropPicker). The following methods interface
  // with the picking classes and return "pick paths". A pick path is a
  // hierarchical, ordered list of props that form an assembly.  Most often,
  // when a vtkProp is picked, its path consists of a single node (i.e., the
  // prop). However, classes like vtkAssembly and vtkPropAssembly can return
  // more than one path, each path being several layers deep. (See
  // vtkAssemblyPath for more information.)  To use these methods - first
  // invoke InitPathTraversal() followed by repeated calls to GetNextPath().
  // GetNextPath() returns a NULL pointer when the list is exhausted.
  virtual void InitPathTraversal();
  virtual vtkAssemblyPath *GetNextPath();
  virtual int GetNumberOfPaths() {return 1;}

  // Description:
  // These methods are used by subclasses to place a matrix (if any) in the
  // prop prior to rendering. Generally used only for picking. See vtkProp3D
  // for more information.
  virtual void PokeMatrix(vtkMatrix4x4 *vtkNotUsed(matrix)) {}
  virtual vtkMatrix4x4 *GetMatrix() {return NULL;}

//BTX  
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // Do we need to ray cast this prop?
  // For certain types of props (vtkLODProp3D for example) this
  // method is not guaranteed to work outside of the rendering
  // process, since a level of detail must be selected before this
  // question can be answered.
  virtual int RequiresRayCasting() { return 0; }

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // Does this prop render into an image?
  // For certain types of props (vtkLODProp3D for example) this
  // method is not guaranteed to work outside of the rendering
  // process, since a level of detail must be selected before this
  // question can be answered.
  virtual int RequiresRenderingIntoImage() { return 0; }

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // All concrete subclasses must be able to render themselves.
  // There are five key render methods in vtk and they correspond
  // to five different points in the rendering cycle. Any given
  // prop may implement one or more of these methods. The first two 
  // methods are designed to render 3D geometry such as polygons
  // lines, triangles. We render the opaque first then the transparent.
  // Ray casting is different from the other methods in that the
  // rendering process is driven not by the mapper but by the ray caster.
  // Two methods are required to support ray casting - one for 
  // initialization, and the other to cast a ray (in viewing coordinates.)
  // The next three methods are primarily intended for volume rendering
  // and supports any technique that returns an image to be composited.  
  // The RenderIntoImage() method causes the rendering to occur, and the
  // GetRGBAImage() and GetZImage() methods are used to gather results.
  // The last method is to render any 2D annotation or overlays.
  // Except for the ray casting methods, these methods return an integer value 
  // indicating whether or not this render method was applied to this 
  // data. For the ray cast initialization, the integer indicated whether or
  // not the initialization was successful. For ray casting, the integer 
  // return value indicates whether or not the ray intersected something.
  virtual int RenderOpaqueGeometry(      vtkViewport *) { return 0; }
  virtual int RenderTranslucentGeometry( vtkViewport *) { return 0; }
  virtual int InitializeRayCasting(      vtkViewport *) { return 0; }
  virtual int CastViewRay(         VTKRayCastRayInfo *) { return 0; }
  virtual int RenderIntoImage(           vtkViewport *) { return 0; }
  virtual float *GetRGBAImage()                         { return NULL; }
  virtual float *GetZImage()                            { return NULL; }
  virtual int RenderOverlay(             vtkViewport *) { return 0; }

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {}

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // The EstimatedRenderTime may be used to select between different props,
  // for example in LODProp it is used to select the level-of-detail.
  // The value is returned in seconds. For simple geometry the accuracy may
  // not be great due to buffering. For ray casting, which is already
  // multi-resolution, the current resolution of the image is factored into
  // the time. We need the viewport for viewing parameters that affect timing.
  // The no-arguments version simply returns the value of the variable with
  // no estimation.
  virtual float GetEstimatedRenderTime( vtkViewport * )
    { return this->EstimatedRenderTime; }
  virtual float GetEstimatedRenderTime(){ return this->EstimatedRenderTime; }
  
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // This method is used by, for example, the vtkLODProp3D in order to
  // initialize the estimated render time at start-up to some user defined
  // value.
  virtual void SetEstimatedRenderTime(float t) 
    {this->EstimatedRenderTime = t; this->SavedEstimatedRenderTime = t;}
    
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
  // When the EstimatedRenderTime is first set to 0.0 (in the
  // SetAllocatedRenderTime method) the old value is saved. This
  // method is used to restore that old value should the render be
  // aborted.
  virtual void RestoreEstimatedRenderTime()
    { this->EstimatedRenderTime = this->SavedEstimatedRenderTime; }
  
  
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // This method is intended to allow the renderer to add to the
  // EstimatedRenderTime in props that require information that
  // the renderer has in order to do this. For example, props
  // that are rendered with a ray casting method do not know
  // themselves how long it took for them to render. We don't want to
  // cause a this->Modified() when we set this value since it is not
  // really a modification to the object. (For example, we don't want
  // to rebuild matrices at every render because the estimated render time
  // is changing)
  virtual void AddEstimatedRenderTime(float t, vtkViewport *vtkNotUsed(vp))
    {this->EstimatedRenderTime+=t;}

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // The renderer may use the allocated rendering time to determine
  // how to render this actor. Therefore it might need the information
  // provided in the viewport.
  // A side effect of this method is to reset the EstimatedRenderTime to
  // 0.0. This way, each of the ways that this prop may be rendered can
  // be timed and added together into this value.
  virtual void SetAllocatedRenderTime(float t, vtkViewport *vtkNotUsed(v)) 
    {
    this->AllocatedRenderTime = t;
    this->SavedEstimatedRenderTime = this->EstimatedRenderTime;
    this->EstimatedRenderTime = 0.0;
    }

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  vtkGetMacro(AllocatedRenderTime, float);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Get/Set the multiplier for the render time. This is used
  // for culling and is a number between 0 and 1. It is used
  // to create the allocated render time value.
  void SetRenderTimeMultiplier( float t ) { this->RenderTimeMultiplier = t; }
  vtkGetMacro(RenderTimeMultiplier, float);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Used to construct assembly paths and perform part traversal.
  virtual void BuildPaths(vtkAssemblyPaths *paths, vtkAssemblyPath *path);

//ETX

protected:
  vtkProp();
  ~vtkProp();
  vtkProp(const vtkProp&);
  void operator=(const vtkProp&);

  int Visibility;
  int Pickable;
  unsigned long PickTag;
  int Dragable;

  float AllocatedRenderTime;
  float EstimatedRenderTime;
  float SavedEstimatedRenderTime;
  float RenderTimeMultiplier;

  // support multi-part props and access to paths of prop
  // stuff that follows is used to build the assembly hierarchy
  vtkAssemblyPaths *Paths;
  
};

#endif


