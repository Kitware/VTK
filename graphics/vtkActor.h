/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.h
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
// .NAME vtkActor - represents an object (geometry & properties) in a rendered scene 
// .SECTION Description
// 
// vtkActor is used to represent an entity in a rendering scene.  It inherits
// functions related to the actors position, and orientation from
// vtkProp. The actor also has scaling and maintains a reference to the
// defining geometry (i.e., the mapper), rendering properties, and possibly a
// texture map. vtkActor combines these instance variables into one 4x4
// transformation matrix as follows: [x y z 1] = [x y z 1] Translate(-origin)
// Scale(scale) Rot(y) Rot(x) Rot (z) Trans(origin) Trans(position)

// .SECTION See Also
// vtkProperty vtkTexture vtkMapper vtkAssembly vtkFollower vtkLODActor

#ifndef __vtkActor_h
#define __vtkActor_h

#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkMapper.h"

class vtkRenderer;
class vtkPropCollection;
class vtkActorCollection;

class VTK_EXPORT vtkActor : public vtkProp3D
{
public:
  vtkTypeMacro(vtkActor,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates an actor with the following defaults: origin(0,0,0)
  // position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
  // orientation=(0,0,0). No user defined matrix and no texture map.
  static vtkActor *New();

  // Description: 
  // For some exporters and other other operations we must be
  // able to collect all the actors or volumes. These methods
  // are used in that process.
  virtual void GetActors(vtkPropCollection *);

  // Description:
  // Support the standard render methods.
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentGeometry(vtkViewport *viewport);

  // Description:
  // This causes the actor to be rendered. It in turn will render the actor's
  // property, texture map and then mapper. If a property hasn't been
  // assigned, then the actor will create one automatically. Note that a side
  // effect of this method is that the pipeline will be updated.
  virtual void Render(vtkRenderer *, vtkMapper *) {};

  // Description:
  // Shallow copy of an actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description: 
  // Set/Get the property object that controls this actors surface
  // properties.  This should be an instance of a vtkProperty object.  Every
  // actor must have a property associated with it.  If one isn't specified,
  // then one will be generated automatically. Multiple actors can share one
  // property object.
  void SetProperty(vtkProperty *lut);
  vtkProperty *GetProperty();

  // Description: 
  // Set/Get the property object that controls this actors backface surface
  // properties.  This should be an instance of a vtkProperty object. If one
  // isn't specified, then the front face properties will be used.  Multiple
  // actors can share one property object.
  void SetBackfaceProperty(vtkProperty *lut);
  vtkProperty *GetBackfaceProperty();

  // Description: 
  // Set/Get the texture object to control rendering texture maps.  This will
  // be a vtkTexture object. An actor does not need to have an associated
  // texture map and multiple actors can share one texture.
  vtkSetObjectMacro(Texture,vtkTexture);
  vtkGetObjectMacro(Texture,vtkTexture);

  // Description:
  // This is the method that is used to connect an actor to the end of a
  // visualization pipeline, i.e. the mapper. This should be a subclass
  // of vtkMapper. Typically vtkPolyDataMapper and vtkDataSetMapper will
  // be used.
  vtkSetObjectMacro(Mapper,vtkMapper);

  // Description:
  // Returns the Mapper that this actor is getting its data from.
  vtkGetObjectMacro(Mapper,vtkMapper);

  // Description:
  // Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
  // method GetBounds(float bounds[6]) is available from the superclass.)
  void GetBounds(float bounds[6]) {this->vtkProp3D::GetBounds( bounds );};
  float *GetBounds();

  // Description:
  // Apply the current properties to all parts that compose this actor.
  // This method is overloaded in vtkAssembly to apply the assemblies'
  // properties to all its parts in a recursive manner. Typically the
  // use of this method is to set the desired properties in the assembly,
  // and then push the properties down to the assemblies parts with
  // ApplyProperties().
  virtual void ApplyProperties() {return;};

  // Description:
  // Get the actors mtime plus consider its properties, texture and
  // usermatrix if set.
  unsigned long int GetMTime();
  
  // Description:
  // Return the mtime of anything that would cause the rendered image to 
  // appear differently. Usually this involves checking the mtime of the 
  // prop plus anything else it depends on such as properties, textures
  // etc.
  virtual unsigned long GetRedrawMTime();

  // Description:
  // The following methods are for compatibility. The methods will be deprecated
  // in the near future. Use vtkProp::GetNextPath() (and related functionality)
  // to get the parts in an assembly (or more correctly, the paths in the
  // assembly).
  virtual void InitPartTraversal();
  virtual vtkActor *GetNextPart();
  virtual int GetNumberOfParts();
  
protected:
  vtkActor();
  ~vtkActor();
  vtkActor(const vtkActor&) {};
  void operator=(const vtkActor&) {};

  vtkProperty *Property; 
  vtkProperty *BackfaceProperty; 
  vtkTexture *Texture; 
  vtkMapper *Mapper;

  // is this actor opaque
  int GetIsOpaque();
  
  // Bounds are cached in an actor - the MapperBounds are also cache to
  // help know when the Bounds need to be recomputed.
  float        MapperBounds[6];
  vtkTimeStamp BoundsMTime;

};

#endif

