/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAssembly.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#ifndef vtkPropAssembly_h
#define vtkPropAssembly_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp.h"

class VTKRENDERINGCORE_EXPORT vtkPropAssembly : public vtkProp
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
  virtual int RenderTranslucentPolygonalGeometry( vtkViewport *ren);
  virtual int RenderVolumetricGeometry( vtkViewport *ren);
  int RenderOverlay(vtkViewport *ren);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the bounds for this prop assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  // May return NULL in some cases (meaning the bounds is undefined).
  double *GetBounds();

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

  vtkPropCollection *Parts;
  double Bounds[6];

  // Support the BuildPaths() method,
  vtkTimeStamp PathTime;
  void UpdatePaths(); //apply transformations and properties recursively
private:
  vtkPropAssembly(const vtkPropAssembly&);  // Not implemented.
  void operator=(const vtkPropAssembly&);  // Not implemented.
};

#endif




