/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#ifndef vtkActor_h
#define vtkActor_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkRenderer;
class vtkPropCollection;
class vtkActorCollection;
class vtkTexture;
class vtkMapper;
class vtkProperty;

class VTKRENDERINGCORE_EXPORT vtkActor : public vtkProp3D
{
public:
  vtkTypeMacro(vtkActor, vtkProp3D);
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
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // This causes the actor to be rendered. It in turn will render the actor's
  // property, texture map and then mapper. If a property hasn't been
  // assigned, then the actor will create one automatically. Note that a side
  // effect of this method is that the pipeline will be updated.
  virtual void Render(vtkRenderer *, vtkMapper *) {}

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
  // Create a new property suitable for use with this type of Actor.
  // For example, a vtkMesaActor should create a vtkMesaProperty
  // in this function.   The default is to just call vtkProperty::New.
  virtual vtkProperty* MakeProperty();

  // Description:
  // Set/Get the property object that controls this actors backface surface
  // properties.  This should be an instance of a vtkProperty object. If one
  // isn't specified, then the front face properties will be used.  Multiple
  // actors can share one property object.
  void SetBackfaceProperty(vtkProperty *lut);
  vtkGetObjectMacro(BackfaceProperty,vtkProperty);

  // Description:
  // Set/Get the texture object to control rendering texture maps.  This will
  // be a vtkTexture object. An actor does not need to have an associated
  // texture map and multiple actors can share one texture.
  virtual void SetTexture(vtkTexture*);
  vtkGetObjectMacro(Texture, vtkTexture);

  // Description:
  // This is the method that is used to connect an actor to the end of a
  // visualization pipeline, i.e. the mapper. This should be a subclass
  // of vtkMapper. Typically vtkPolyDataMapper and vtkDataSetMapper will
  // be used.
  virtual void SetMapper(vtkMapper *);

  // Description:
  // Returns the Mapper that this actor is getting its data from.
  vtkGetObjectMacro(Mapper, vtkMapper);

  // Description:
  // Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
  // method GetBounds(double bounds[6]) is available from the superclass.)
  void GetBounds(double bounds[6]) {this->vtkProp3D::GetBounds( bounds );}
  double *GetBounds();

  // Description:
  // Apply the current properties to all parts that compose this actor.
  // This method is overloaded in vtkAssembly to apply the assemblies'
  // properties to all its parts in a recursive manner. Typically the
  // use of this method is to set the desired properties in the assembly,
  // and then push the properties down to the assemblies parts with
  // ApplyProperties().
  virtual void ApplyProperties() {}

  // Description:
  // Get the actors mtime plus consider its properties and texture if set.
  unsigned long int GetMTime();

  // Description:
  // Return the mtime of anything that would cause the rendered image to
  // appear differently. Usually this involves checking the mtime of the
  // prop plus anything else it depends on such as properties, textures,
  // etc.
  virtual unsigned long GetRedrawMTime();

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Used by vtkHardwareSelector to determine if the prop supports hardware
  // selection.
  virtual bool GetSupportsSelection();

protected:
  vtkActor();
  ~vtkActor();

  // is this actor opaque
  int GetIsOpaque();

  vtkProperty *Property;
  vtkProperty *BackfaceProperty;
  vtkTexture *Texture;
  vtkMapper *Mapper;

  // Bounds are cached in an actor - the MapperBounds are also cache to
  // help know when the Bounds need to be recomputed.
  double MapperBounds[6];
  vtkTimeStamp BoundsMTime;

private:
  vtkActor(const vtkActor&);  // Not implemented.
  void operator=(const vtkActor&);  // Not implemented.
};

#endif
