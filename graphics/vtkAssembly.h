/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssembly.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkAssembly - create hierarchies of actors
// .SECTION Description
// vtkAssembly is an object that groups actors and other assemblies into
// a tree-like hierarchy. The actors and assemblies can then be transformed
// together by transforming just the root assembly of the hierarchy.
//
// A vtkAssembly object can be used in place of an vtkActor since it is a 
// subclass of vtkActor. The difference is that vtkAssembly maintains a list
// of actor instances (its "parts") that form the assembly. Then, any 
// operation that modifies the parent assembly will modify all its parts.
// Note that this process is recursive: you can create groups consisting
// of assemblies and/or actors to arbitrary depth.
//
// Actor's (or assemblies) that compose an assembly need not be added to 
// a renderer's list of actors, as long as the parent assembly is in the
// list of actors. This is because they are automatically renderered 
// during the hierarchical traversal process.
//
// Since a vtkAssembly object is a derived class of vtkActor, it has
// properties and possibly a mapper. During the rendering process, if a
// mapper is associated with the assembly, it is rendered with these
// properties. Otherwise, the properties have no effect (i.e., on the
// children of the assembly).

// .SECTION Caveats
// Collections of assemblies are slower to render than an equivalent list
// of actors. This is because to support arbitrary nesting of assemblies, 
// the state of the assemblies (i.e., transformation matrices) must
// be propagated through the assembly hierarchy. 
//
// Assemblies can consist of hierarchies of assemblies, where one actor or
// assembly used in one hierarchy is also used in other hierarchies. However, 
// make that there are no cycles (e.g., parent->child->parent), this will
// cause program failure.
 
// .SECTION See Also
// vtkActor vtkTransform vtkMapper vtkPolyDataMapper

#ifndef __vtkAssembly_h
#define __vtkAssembly_h

#include "vtkActor.h"

class vtkAssemblyPaths;

class VTK_EXPORT vtkAssembly : public vtkActor
{
public:
  static vtkAssembly *New();

  vtkTypeMacro(vtkAssembly,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a part to the list of parts.
  void AddPart(vtkActor *);

  // Description:
  // Remove a part from the list of parts,
  void RemovePart(vtkActor *);

  // Description:
  // Return the parts of this asembly.
  vtkActorCollection *GetParts();

  // Description:
  // Render this assembly and all its parts. 
  // The rendering process is recursive.
  // Note that a mapper need not be defined. If not defined, then no geometry 
  // will be drawn for this assembly. This allows you to create "logical"
  // assemblies; that is, assemblies that only serve to group and transform
  // its parts.
  int RenderOpaqueGeometry(vtkViewport *ren);
  int RenderTranslucentGeometry(vtkViewport *ren);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Methods to traverse the parts of an assembly. Each part (starting from
  // the root) will appear properly transformed and with the correct
  // properties (depending upon the ApplyProperty and ApplyTransform ivars).
  // Note that the part appears as an actor. These methods should be contrasted
  // to those that traverse the list of parts using GetParts(). 
  // GetParts() returns
  // a list of children of this assembly, not necessarily with the correct
  // transformation or properties. To use these methods - first invoke 
  // InitPartTraversal() followed by repeated calls to GetNextPart(). 
  // GetNextPart() returns a NULL pointer when the list is exhausted.
  void InitPartTraversal();
  vtkActor *GetNextPart();
  int GetNumberOfParts();

  // Description:
  // Build assembly paths from this current assembly. Paths consist of
  // an ordered sequence of actors, with transformations properly concatenated.
  void BuildPaths(vtkAssemblyPaths *paths, vtkActorCollection *path);

  // Description:
  // Recursively apply properties to parts.
  void ApplyProperties(); 

  // Description:
  // Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  void GetBounds(float bounds[6]) {this->vtkProp3D::GetBounds( bounds );};
  float *GetBounds();

  // Description:
  // Override default GetMTime method to also consider all of the
  // assembly's parts.
  unsigned long int GetMTime();

  // Description:
  // Shallow copy of an assembly.
  void ShallowCopy(vtkAssembly *assembly);

protected:
  vtkAssembly();
  ~vtkAssembly();
  vtkAssembly(const vtkAssembly&) {};
  void operator=(const vtkAssembly&) {};

  vtkActorCollection *Parts;

  // stuff that follows is used to build the assembly hierarchy
  vtkAssemblyPaths *Paths;
  vtkTimeStamp PathTime;

  void UpdatePaths(); //apply transformations and properties recursively
  void DeletePaths(); //delete the paths

private:
  // hide the superclass' ShallowCopy() from the user and the compiler.
  void ShallowCopy(vtkProp *prop) { this->vtkProp::ShallowCopy( prop ); };
  void ShallowCopy(vtkProp3D *prop) { this->vtkProp3D::ShallowCopy( prop ); };
  void ShallowCopy(vtkActor *prop) { this->vtkActor::ShallowCopy( prop ); };
};

// Description:
// Get the list of parts for this assembly.
inline vtkActorCollection *vtkAssembly::GetParts() {return this->Parts;}

#endif




