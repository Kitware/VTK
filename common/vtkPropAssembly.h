/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAssembly.h
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
  int InitializeRayCasting(vtkViewport *);
  int CastViewRay(VTKRayCastRayInfo *);
  int RenderIntoImage(vtkViewport *);
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
  void ShallowCopy(vtkPropAssembly *PropAssembly);

  // Description:
  // Override default GetMTime method to also consider all of the
  // prop assembly's parts.
  unsigned long int GetMTime();

protected:
  vtkPropAssembly();
  ~vtkPropAssembly();
  vtkPropAssembly(const vtkPropAssembly&) {};
  void operator=(const vtkPropAssembly&) {};

  vtkPropCollection *Parts;
  float Bounds[6];
  
private:
  // hide the superclass' ShallowCopy() from the user and the compiler.
  void ShallowCopy(vtkProp *prop) { this->vtkProp::ShallowCopy( prop ); };
};

#endif




