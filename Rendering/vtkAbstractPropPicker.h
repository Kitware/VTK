/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPropPicker.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkAbstractPropPicker - abstract API for pickers that can pick an instance of vtkProp
// .SECTION Description
// vtkAbstractPropPicker is an abstract superclass for pickers that can pick
// an instance of vtkProp. Some pickers, like vtkWorldPointPicker (not a
// subclass of this class), cannot identify the prop that is
// picked. Subclasses of vtkAbstractPropPicker return a prop in the form of a
// vtkAssemblyPath when a pick is invoked. Note that an vtkAssemblyPath
// contain a list of vtkAssemblyNodes, each of which in turn contains a
// reference to a vtkProp and a 4x4 transformation matrix. The path fully
// describes the entire pick path, so you can pick assemblies or portions of
// assemblies, or just grab the tail end of the vtkAssemblyPath (which is the
// picked prop).

// .SECTION Caveats
// Because a vtkProp can be placed into different assemblies, or even in
// different leaf positions of the same assembly, the vtkAssemblyPath is
// used to fully qualify exactly which use of the vtkProp was picked, 
// including its position (since vtkAssemblyPath includes a transformation
// matrix per node).
//
// The class returns information about picked actors, props, etc. Note that
// what is returned by these methods is the top level of the assembly
// path. This can cause a lot of confusion! For example, if you pick a
// vtkAssembly, and the returned vtkAssemblyPath has as a leaf a vtkActor,
// then if you invoke GetActor(), you will get NULL, even though an actor was
// indeed picked. (GetAssembly() will return something.) Note that the safest
// thing to do is to do a GetProp(), which will always return something if
// something was picked. A better way to manage picking is to work with
// vtkAssemblyPath, since this completely defines the pick path from top to
// bottom in a assembly hierarchy, and avoids confusion when the same prop is
// used in different assemblies.
//
// The returned assembly paths refer to assembly nodes that in turn refer
// to vtkProp and vtkMatrix. This association to vtkProp is not a reference
// counted association, meaning that dangling references are possible if
// you do a pick, get an assembly path, and then delete a vtkProp. (Reason:
// assembly paths create many self-referencing loops that destroy reference
// counting.)

// .SECTION See Also 
// vtkPropPicker vtkPicker vtkWorldPointPicker vtkCellPicker vtkPointPicker 
// vtkAssemblyPath vtkAssemblyNode vtkAssemblyPaths vtkAbstractPicker
// vtkRenderer

#ifndef __vtkAbstractPropPicker_h
#define __vtkAbstractPropPicker_h

#include "vtkAbstractPicker.h"

class vtkProp;
class vtkPropAssembly;
class vtkAssembly;
class vtkActor;
class vtkVolume;
class vtkProp3D;

class VTK_EXPORT vtkAbstractPropPicker : public vtkAbstractPicker
{
public:
  vtkTypeMacro(vtkAbstractPropPicker,vtkAbstractPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the vtkAssemblyPath that has been picked. The assembly path lists
  // all the vtkProps that form an assembly. If no assembly is present, then
  // the assembly path will have one node (which is the picked prop). The
  // set method is used internally to set the path. (Note: the structure of
  // an assembly path is a collection of vtkAssemblyNode, each node pointing
  // to a vtkProp and (possibly) a transformation matrix.)
  vtkSetObjectMacro(Path,vtkAssemblyPath);
  vtkGetObjectMacro(Path,vtkAssemblyPath);

  // The following are convenience methods to maintain API with older
  // versions of VTK, and to allow query for the return type of a pick. Note:
  // the functionality of these methods can also be obtained by using the
  // returned vtkAssemblyPath and using the IsA() to determine type.

  // Description:
  // Return the vtkProp that has been picked. If NULL, nothing was picked.
  // If anything at all was picked, this method will return something.
  virtual vtkProp *GetProp();
  
  // Description:
  // Return the vtkProp that has been picked. If NULL, no vtkProp3D was picked.
  virtual vtkProp3D *GetProp3D();
  
  // Description:
  // Return the vtkActor that has been picked. If NULL, no actor was picked.
  virtual vtkActor *GetActor();
  
  // Description:
  // Return the vtkActor2D that has been picked. If NULL, no actor2D was 
  // picked.
  virtual vtkActor2D *GetActor2D();
  
  // Description:
  // Return the vtkVolume that has been picked. If NULL, no volume was picked.
  virtual vtkVolume *GetVolume();
  
  // Description:
  // Return the vtkAssembly that has been picked. If NULL, no assembly 
  // was picked. (Note: the returned assembly is the first node in the
  // assembly path. If the path is one node long, then the assembly and
  // the prop are the same, assuming that the first node is a vtkAssembly.)
  virtual vtkAssembly *GetAssembly();
  
  // Description:
  // Return the vtkPropAssembly that has been picked. If NULL, no prop
  // assembly was picked. (Note: the returned prop assembly is the first node
  // in the assembly path. If the path is one node long, then the prop
  // assembly and the prop are the same, assuming that the first node is a
  // vtkPropAssembly.)
  virtual vtkPropAssembly *GetPropAssembly();
  
protected:
  vtkAbstractPropPicker();
  ~vtkAbstractPropPicker();
  vtkAbstractPropPicker(const vtkAbstractPropPicker&);
  void operator=(const vtkAbstractPropPicker&);

  void Initialize();
  
  vtkAssemblyPath *Path; //this is what is picked, and includes the prop
};

#endif
