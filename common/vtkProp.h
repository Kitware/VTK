/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.h
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
// .NAME vtkProp - abstract superclass for all actors, volumes and annotations
// .SECTION Description
// vtkProp is an abstract superclass for actor type objects. Instances of
// vtkProp may respond to RenderGeometry, RenderVolume, and RenderPostSwap
// calls and may repond to more than one.
//
// .SECTION See Also
// vtkActor2D  vtkActor vtkVolume

#ifndef __vtkProp_h
#define __vtkProp_h

#include "vtkObject.h"
class vtkViewport;
class vtkActorCollection;
class vtkActor2DCollection;
class vtkVolumeCollection;

class VTK_EXPORT vtkProp : public vtkObject
{
public:
  static vtkProp* New() {return new vtkProp;};
  void PrintSelf(ostream& os, vtkIndent indent);
  const char *GetClassName() {return "vtkProp";};

  // Description:
  // Creates an Prop with visibility on.
  vtkProp();

  // Description:
  // All concrete subclasses must be able to render themselves.
  // There are five key render methods in vtk and they correspond
  // to five different points in the rendering cycle. Any given
  // prop may implement one or more of these methods. The first two 
  // methods are designed to render 3D geometry such as polygons
  // lines, triangles. We render the opaque first then the transparent.
  // The next two methods are primarily intended for volume rendering
  // and support ray casting and any other technique that returns an
  // image to be composited.  The fifth method is to render any 2D 
  // annotation or overlays.
  virtual void RenderOpaqueGeometry(vtkViewport *viewport) {};
  virtual void RenderTranslucentGeometry(vtkViewport *viewport) {};
  virtual void RenderRayCastImage(vtkViewport *viewport) {};
  virtual void RenderAndReturnImage(vtkViewport *viewport) {};
  virtual void RenderOverlay(vtkViewport *viewport) {};

  // Description: 
  // For some exporters and other other operations we must be
  // able to collect all the actors or volumes. These methods
  // are used in that process.
  virtual void GetActors(vtkActorCollection *ac) {};
  virtual void GetActors2D(vtkActor2DCollection *ac) {};
  virtual void GetVolumes(vtkVolumeCollection *vc) {};
  
  // Description:
  // Set/Get visibility of this vtkProp.
  vtkSetMacro(Visibility, int);
  vtkGetMacro(Visibility, int);
  vtkBooleanMacro(Visibility, int);

  // Description:
  // The renderer may use the allocated rendering time to determine
  // how to render this actor. (LOD Experiment)
  // The set method is not a macro in order to avoid resetting the mtime of
  // the actor - otherwise the actor would have been modified during every 
  // render.
  void SetAllocatedRenderTime(float t) {this->AllocatedRenderTime = t;};
  vtkGetMacro(AllocatedRenderTime, float);

  // Description:
  // Return the mtime of anything that would cause the rendered image to 
  // appear differently. Usually this involves checking the mtime of the 
  // prop plus anything else it depends on such as properties, textures
  // etc.
  virtual unsigned long GetRedrawMTime() {return this->GetMTime();};
  
  // Description:
  // Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  // in world coordinates. NULL means that the bounds are not defined.
  virtual float *GetBounds() {return NULL;};
  
protected:
  int Visibility;
  float AllocatedRenderTime;
};

#endif


