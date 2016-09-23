/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPropPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractPropPicker
 * @brief   abstract API for pickers that can pick an instance of vtkProp
 *
 * vtkAbstractPropPicker is an abstract superclass for pickers that can pick
 * an instance of vtkProp. Some pickers, like vtkWorldPointPicker (not a
 * subclass of this class), cannot identify the prop that is
 * picked. Subclasses of vtkAbstractPropPicker return a prop in the form of a
 * vtkAssemblyPath when a pick is invoked. Note that an vtkAssemblyPath
 * contain a list of vtkAssemblyNodes, each of which in turn contains a
 * reference to a vtkProp and a 4x4 transformation matrix. The path fully
 * describes the entire pick path, so you can pick assemblies or portions of
 * assemblies, or just grab the tail end of the vtkAssemblyPath (which is the
 * picked prop).
 *
 * @warning
 * Because a vtkProp can be placed into different assemblies, or even in
 * different leaf positions of the same assembly, the vtkAssemblyPath is
 * used to fully qualify exactly which use of the vtkProp was picked,
 * including its position (since vtkAssemblyPath includes a transformation
 * matrix per node).
 *
 * @warning
 * The class returns information about picked actors, props, etc. Note that
 * what is returned by these methods is the top level of the assembly
 * path. This can cause a lot of confusion! For example, if you pick a
 * vtkAssembly, and the returned vtkAssemblyPath has as a leaf a vtkActor,
 * then if you invoke GetActor(), you will get NULL, even though an actor was
 * indeed picked. (GetAssembly() will return something.) Note that the safest
 * thing to do is to do a GetViewProp(), which will always return something if
 * something was picked. A better way to manage picking is to work with
 * vtkAssemblyPath, since this completely defines the pick path from top to
 * bottom in a assembly hierarchy, and avoids confusion when the same prop is
 * used in different assemblies.
 *
 * @warning
 * The returned assembly paths refer to assembly nodes that in turn refer
 * to vtkProp and vtkMatrix. This association to vtkProp is not a reference
 * counted association, meaning that dangling references are possible if
 * you do a pick, get an assembly path, and then delete a vtkProp. (Reason:
 * assembly paths create many self-referencing loops that destroy reference
 * counting.)
 *
 * @sa
 * vtkPropPicker vtkPicker vtkWorldPointPicker vtkCellPicker vtkPointPicker
 * vtkAssemblyPath vtkAssemblyNode vtkAssemblyPaths vtkAbstractPicker
 * vtkRenderer
*/

#ifndef vtkAbstractPropPicker_h
#define vtkAbstractPropPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractPicker.h"

class vtkProp;
class vtkPropAssembly;
class vtkAssembly;
class vtkActor;
class vtkVolume;
class vtkProp3D;
class vtkAssemblyPath;
class vtkActor2D;

class VTKRENDERINGCORE_EXPORT vtkAbstractPropPicker : public vtkAbstractPicker
{
public:
  vtkTypeMacro(vtkAbstractPropPicker,vtkAbstractPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Return the vtkAssemblyPath that has been picked. The assembly path lists
   * all the vtkProps that form an assembly. If no assembly is present, then
   * the assembly path will have one node (which is the picked prop). The
   * set method is used internally to set the path. (Note: the structure of
   * an assembly path is a collection of vtkAssemblyNode, each node pointing
   * to a vtkProp and (possibly) a transformation matrix.)
   */
  virtual void SetPath(vtkAssemblyPath*);
  vtkGetObjectMacro(Path,vtkAssemblyPath);
  //@}

  // The following are convenience methods to maintain API with older
  // versions of VTK, and to allow query for the return type of a pick. Note:
  // the functionality of these methods can also be obtained by using the
  // returned vtkAssemblyPath and using the IsA() to determine type.

  /**
   * Return the vtkProp that has been picked. If NULL, nothing was picked.
   * If anything at all was picked, this method will return something.
   */
  virtual vtkProp* GetViewProp();

  /**
   * Return the vtkProp that has been picked. If NULL, no vtkProp3D was picked.
   */
  virtual vtkProp3D *GetProp3D();

  /**
   * Return the vtkActor that has been picked. If NULL, no actor was picked.
   */
  virtual vtkActor *GetActor();

  /**
   * Return the vtkActor2D that has been picked. If NULL, no actor2D was
   * picked.
   */
  virtual vtkActor2D *GetActor2D();

  /**
   * Return the vtkVolume that has been picked. If NULL, no volume was picked.
   */
  virtual vtkVolume *GetVolume();

  /**
   * Return the vtkAssembly that has been picked. If NULL, no assembly
   * was picked. (Note: the returned assembly is the first node in the
   * assembly path. If the path is one node long, then the assembly and
   * the prop are the same, assuming that the first node is a vtkAssembly.)
   */
  virtual vtkAssembly *GetAssembly();

  /**
   * Return the vtkPropAssembly that has been picked. If NULL, no prop
   * assembly was picked. (Note: the returned prop assembly is the first node
   * in the assembly path. If the path is one node long, then the prop
   * assembly and the prop are the same, assuming that the first node is a
   * vtkPropAssembly.)
   */
  virtual vtkPropAssembly *GetPropAssembly();

protected:
  vtkAbstractPropPicker();
  ~vtkAbstractPropPicker();

  void Initialize();

  vtkAssemblyPath *Path; //this is what is picked, and includes the prop
private:
  vtkAbstractPropPicker(const vtkAbstractPropPicker&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAbstractPropPicker&) VTK_DELETE_FUNCTION;
};

#endif
