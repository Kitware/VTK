/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssembly.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAssembly
 * @brief   create hierarchies of vtkProp3Ds (transformable props)
 *
 * vtkAssembly is an object that groups vtkProp3Ds, its subclasses, and
 * other assemblies into a tree-like hierarchy. The vtkProp3Ds and
 * assemblies can then be transformed together by transforming just the root
 * assembly of the hierarchy.
 *
 * A vtkAssembly object can be used in place of an vtkProp3D since it is a
 * subclass of vtkProp3D. The difference is that vtkAssembly maintains a list
 * of vtkProp3D instances (its "parts") that form the assembly. Then, any
 * operation that transforms (i.e., scales, rotates, translates) the parent
 * assembly will transform all its parts.  Note that this process is
 * recursive: you can create groups consisting of assemblies and/or
 * vtkProp3Ds to arbitrary depth.
 *
 * To add an assembly to the renderer's list of props, you only need to
 * add the root of the assembly. During rendering, the parts of the
 * assembly are rendered during a hierarchical traversal process.
 *
 * @warning
 * Collections of assemblies are slower to render than an equivalent list
 * of actors. This is because to support arbitrary nesting of assemblies,
 * the state of the assemblies (i.e., transformation matrices) must
 * be propagated through the assembly hierarchy.
 *
 * @warning
 * Assemblies can consist of hierarchies of assemblies, where one actor or
 * assembly used in one hierarchy is also used in other hierarchies. However,
 * make that there are no cycles (e.g., parent->child->parent), this will
 * cause program failure.
 *
 * @warning
 * If you wish to create assemblies without any transformation (using the
 * assembly strictly as a grouping mechanism), then you may wish to
 * consider using vtkPropAssembly.
 *
 * @sa
 * vtkActor vtkTransform vtkMapper vtkPolyDataMapper vtkPropAssembly
*/

#ifndef vtkAssembly_h
#define vtkAssembly_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkAssemblyPaths;
class vtkProp3DCollection;
class vtkMapper;
class vtkProperty;
class vtkActor;

class VTKRENDERINGCORE_EXPORT vtkAssembly : public vtkProp3D
{
public:
  static vtkAssembly *New();

  vtkTypeMacro(vtkAssembly, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add a part to the list of parts.
   */
  void AddPart(vtkProp3D *);

  /**
   * Remove a part from the list of parts,
   */
  void RemovePart(vtkProp3D *);

  /**
   * Return the parts (direct descendants) of this assembly.
   */
  vtkProp3DCollection *GetParts()
    { return this->Parts; }

  //@{
  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void GetVolumes(vtkPropCollection *) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Render this assembly and all its parts.
   * The rendering process is recursive.
   * Note that a mapper need not be defined. If not defined, then no geometry
   * will be drawn for this assembly. This allows you to create "logical"
   * assemblies; that is, assemblies that only serve to group and transform
   * its parts.
   */
  int RenderOpaqueGeometry(vtkViewport *ren) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *ren) VTK_OVERRIDE;
  int RenderVolumetricGeometry(vtkViewport *ren) VTK_OVERRIDE;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  //@{
  /**
   * Methods to traverse the parts of an assembly. Each part (starting from
   * the root) will appear properly transformed and with the correct
   * properties (depending upon the ApplyProperty and ApplyTransform ivars).
   * Note that the part appears as an instance of vtkProp. These methods
   * should be contrasted to those that traverse the list of parts using
   * GetParts().  GetParts() returns a list of children of this assembly, not
   * necessarily with the correct transformation or properties. To use the
   * methods below - first invoke InitPathTraversal() followed by repeated
   * calls to GetNextPath().  GetNextPath() returns a NULL pointer when the
   * list is exhausted.
   */
  void InitPathTraversal() VTK_OVERRIDE;
  vtkAssemblyPath *GetNextPath() VTK_OVERRIDE;
  int GetNumberOfPaths() VTK_OVERRIDE;
  //@}

  /**
   * Get the bounds for the assembly as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  void GetBounds(double bounds[6])
    { this->vtkProp3D::GetBounds( bounds ); }
  double *GetBounds() VTK_OVERRIDE;

  /**
   * Override default GetMTime method to also consider all of the
   * assembly's parts.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Shallow copy of an assembly. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp *prop) VTK_OVERRIDE;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE DO NOT USE THIS
   * METHOD OUTSIDE OF THE RENDERING PROCESS Overload the superclass' vtkProp
   * BuildPaths() method. Paths consist of an ordered sequence of actors,
   * with transformations properly concatenated.
   */
  void BuildPaths(vtkAssemblyPaths *paths, vtkAssemblyPath *path) VTK_OVERRIDE;

protected:
  vtkAssembly();
  ~vtkAssembly() VTK_OVERRIDE;

  // Keep a list of direct descendants of the assembly hierarchy
  vtkProp3DCollection *Parts;

  // Support the BuildPaths() method. Caches last paths built for
  // performance.
  vtkTimeStamp PathTime;
  virtual void UpdatePaths(); //apply transformations and properties recursively

private:
  vtkAssembly(const vtkAssembly&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAssembly&) VTK_DELETE_FUNCTION;
};

#endif
