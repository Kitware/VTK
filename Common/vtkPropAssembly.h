/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAssembly.h
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
// .NAME vtkPropAssembly - create hierarchies of props
// .SECTION Description
// vtkPropAssembly is an object that groups props and other prop assemblies
// into a tree-like hierarchy. The props can then be treated as a group
// (e.g., turning visibility on and off).
//
// A vtkPropAssembly object can be used in place of an vtkProp since it is a
// subclass of vtkProp. The difference is that vtkPropAssembly maintains a
// list of other prop and prop assembly instances (its "parts") that form the
// assembly. Note that this process is recursive: you can create groups 
// consisting of prop assemblies to arbitrary depth.
//
// vtkPropAssembly's and vtkProp's that compose a prop assembly need not be
// added to a renderer's list of props, as long as the parent assembly is in
// the prop list. This is because they are automatically renderered during
// the hierarchical traversal process.

// .SECTION Caveats
// vtkPropAssemblies can consist of hierarchies of assemblies, where one
// actor or assembly used in one hierarchy is also used in other
// hierarchies. However, make that there are no cycles (e.g.,
// parent->child->parent), this will cause program failure.
 
// .SECTION See Also
// vtkProp3D vtkActor vtkAssembly vtkActor2D vtkVolume

#ifndef __vtkPropAssembly_h
#define __vtkPropAssembly_h

#include "vtkProp.h"

class VTK_EXPORT vtkPropAssembly : public vtkProp
{
public:
  vtkTypeMacro(vtkPropAssembly,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create with an empty parts list.
  static vtkPropAssembly *New();

  // Description:
  // Add a part to the list of parts.
  void AddPart(vtkProp *);

  // Description:
  // Remove a part from the list of parts,
  void RemovePart(vtkProp *);

  // Description:
  // Return the list of parts.
  vtkPropCollection *GetParts();
  
  // Description:
  // Render this assembly and all its parts.  The rendering process is
  // recursive. The parts of each assembly are rendered only if the
  // visibility for the prop is turned on.
  int RenderOpaqueGeometry(vtkViewport *ren);
  int RenderTranslucentGeometry(vtkViewport *ren);
  int RenderOverlay(vtkViewport *);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the bounds for this prop assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  // May return NULL in some cases (meaning the bounds is undefined).
  float *GetBounds();

  // Description:
  // Shallow copy of this vtkPropAssembly.
  void ShallowCopy(vtkProp *Prop);

  // Description:
  // Override default GetMTime method to also consider all of the
  // prop assembly's parts.
  unsigned long int GetMTime();

  // Description:
  // Methods to traverse the paths (i.e., leaf nodes) of a prop
  // assembly. These methods should be contrasted to those that traverse the
  // list of parts using GetParts().  GetParts() returns a list of children
  // of this assembly, not necessarily the leaf nodes of the assembly. To use
  // the methods below - first invoke InitPathTraversal() followed by
  // repeated calls to GetNextPath().  GetNextPath() returns a NULL pointer
  // when the list is exhausted. (See the superclass vtkProp for more
  // information about paths.)
  void InitPathTraversal();
  vtkAssemblyPath *GetNextPath();
  int GetNumberOfPaths();

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Overload the superclass' vtkProp BuildPaths() method.
  void BuildPaths(vtkAssemblyPaths *paths, vtkAssemblyPath *path);
//ETX  

protected:
  vtkPropAssembly();
  ~vtkPropAssembly();
  vtkPropAssembly(const vtkPropAssembly&);
  void operator=(const vtkPropAssembly&);

  vtkPropCollection *Parts;
  float Bounds[6];
  
  // Support the BuildPaths() method,
  vtkTimeStamp PathTime;
  void UpdatePaths(); //apply transformations and properties recursively
};

#endif




