/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.h
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
// .NAME vtkActor - represents an object (geometry & properties) in a rendered scene 
// .SECTION Description
// vtkActor is used to represent an entity in a rendering scene.  It
// inherits functions related to the actors position, and orientation
// from vtkProp. The actor also has scaling and maintains a reference to the
// defining geometry (i.e., the mapper), rendering properties, and
// possibly a texture map.
// vtkActor combines these instance variables into one 4x4
// transformation matrix as follows: [x y z 1] = [x y z 1]
// Translate(-origin) Scale(scale) Rot(y) Rot(x) Rot (z) Trans(origin)
// Trans(position)

// .SECTION See Also
// vtkProperty vtkTexture vtkMapper vtkActorDevice
// vtkAssembly vtkFollower vtkLODActor

#ifndef __vtkActor_h
#define __vtkActor_h

#include "vtkProp.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkMapper.h"
#include "vtkTransform.h"
#include "vtkAssemblyPaths.h"

class vtkRenderer;

class VTK_EXPORT vtkActor : public vtkProp
{
 public:
  // Description:
  // Creates an actor with the following defaults: origin(0,0,0)
  // position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
  // orientation=(0,0,0). No user defined matrix and no texture map.
  vtkActor();
  ~vtkActor();
  static vtkActor *New();
  const char *GetClassName() {return "vtkActor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This causes the actor to be rendered. It in turn will render the
  // actor's property, texture map and then mapper. If a property
  // hasn't been assigned, then the actor will create one
  // automatically. Note that a side effect of this method is that the
  virtual void Render(vtkRenderer *ren);

  virtual void Render(vtkRenderer *, vtkMapper *) {};

  // Description:
  // Shallow copy of an actor.
  vtkActor &operator=(const vtkActor& actor);

  // Description: 
  // Set/Get the property object that controls this
  // actors surface properties.  This should be an instance of a
  // vtkProperty object.  Every actor must have a property associated
  // with it.  If one isn't specified, then one will be generated
  // automatically. Multiple actors can share one property object.
  void SetProperty(vtkProperty *lut);
  vtkProperty *GetProperty();

  void SetBackfaceProperty(vtkProperty *lut);
  vtkProperty *GetBackfaceProperty();

  // Description: 
  // Set/Get the texture object to control rendering
  // texture maps.  This will be a vtkTexture object. An actor does
  // not need to have an associated texture map and multiple actors
  // can share one texture.
  vtkSetObjectMacro(Texture,vtkTexture);
  vtkGetObjectMacro(Texture,vtkTexture);

  // Description:
  // This is the method that is used to connect an actor to the end of a
  // visualization pipeline, i.e. the mapper. This should be a subclass
  // of vtkMapper. Typically vtkPolyDataMapper and vtkDataSetMapper will
  // be used.
  vtkSetReferenceCountedObjectMacro(Mapper,vtkMapper);

  // Description:
  // Returns the Mapper that this actor is getting its data from.
  vtkGetObjectMacro(Mapper,vtkMapper);

  // Description:
  // Set/Get the scale of the actor. Scaling in performed independently on the
  // X, Y and Z axis. A scale of zero is illegal and will be replaced with one.
  vtkSetVector3Macro(Scale,float);
  vtkGetVectorMacro(Scale,float,3);

  // Description:
  // Get the matrix from the position, origin, scale and orientation
  // This matrix is cached, so multiple GetMatrix() calls will be
  // efficient.
  void GetMatrix(vtkMatrix4x4 *m);

  // Description:
  // Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
  // method GetBounds(float bounds[6]) is available from the superclass.)
  float *GetBounds();

  // Description:
  // Subclasses of vtkActor can be composed of one or more parts. A part is an
  // actor or subclass of actor (e.g., vtkAssembly). The methods 
  // InitPartTraversal() and GetNextPart() allow you to get at the parts
  // that compose the actor. To use these methods - first invoke 
  // InitPartTraversal() followed by repeated calls to GetNextPart(). 
  // GetNextPart() returns a NULL pointer when the list is exhausted. (These
  // methods differ from the vtkAssembly::GetParts() method, which returns 
  // a list of the parts that are first level children of the assembly.)
  virtual void InitPartTraversal() {this->TraversalLocation = 0;};
  virtual vtkActor *GetNextPart();
  virtual int GetNumberOfParts() {return 1;};

  // Description:
  // Used to construct assembly paths and perform part traversal.
  virtual void BuildPaths(vtkAssemblyPaths *paths, vtkActorCollection *path);

  // Description:
  // Apply the current properties to all parts that compose this actor.
  // This method is overloaded in vtkAssembly to apply the assemblies'
  // properties to all its parts in a recursive manner. Typically the
  // use of this method is to set the desired properties in the assembly,
  // and then push the properties down to the assemblies parts with
  // ApplyProperties().
  virtual void ApplyProperties() {return;};

  // Description:
  // Update visualization pipeline and any other parts of actor that are
  // necessary.
  virtual void Update();

  unsigned long int GetMTime();//overload superclasses' implementation

  // Description:
  // The renderer may use the allocated rendering time to determine
  // how to render this actor. (LOD Experiment)
  // The set method is not a macro in order to avoid resetting the mtime of
  // the actor - otherwise the actor would have been modified during every 
  // render.
  void SetAllocatedRenderTime(float t) {this->AllocatedRenderTime = t;};
  vtkGetMacro(AllocatedRenderTime, float);
  
  // Description:
  // For legacy compatability. Do not use.
  void SetProperty(vtkProperty& lut) {this->SetProperty(&lut);};
  void SetBackfaceProperty(vtkProperty& lut) 
    {this->SetBackfaceProperty(&lut);};
  void GetMatrix(vtkMatrix4x4 &m) {this->GetMatrix(&m);}

  
protected:
  vtkProperty *Property; 
  vtkProperty *BackfaceProperty; 
  vtkTexture *Texture; 
  vtkMapper *Mapper;
  float Scale[3];

  // this stuff supports multiple-part actors (e.g. assemblies)
  int TraversalLocation;
  
  // This is for LOD experiment
  float AllocatedRenderTime;

  // Bounds are cached in an actor - the MapperBounds are also cache to
  // help know when the Bounds need to be recomputed.
  float        MapperBounds[6];
  vtkTimeStamp BoundsMTime;

};

#endif

