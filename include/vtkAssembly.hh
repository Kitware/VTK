/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssembly.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkAssembly - create hierarchies of actors
// .SECTION Description
// vtkAssembly is a convienence object that groups actors together. The 
// group can be used to transform multiple actors simultaneously; or to 
// assign properties or other actor attributes such as visibility or a 
// texture map. Groups can be nested to form hierarchies.
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

// .SECTION Caveats
// Collections of assemblies are slower to render than an equivalent list
// of actors. This is because to support arbitrary nesting of assemblies, 
// the state of the assemblies (i.e., transformations and properties) must
// be propagated through the assembly hierarchy. 
//
// Assemblies can consist of hierarchies of assemblies, where one actor or
// assembly used in one hierarchy is also used in other hierarchies. However, 
// make that there are no cycles (e.g., parent->child->parent), this will
// cause program failure.
 
// .SECTION See Also
// vtkActor vtkTransform vtkMapper vtkPolyMapper

#ifndef __vtkAssembly_h
#define __vtkAssembly_h

#include "vtkActor.hh"

class vtkAssemblyPaths;

class vtkAssembly : public vtkActor
{
public:
  vtkAssembly();
  ~vtkAssembly();
  char *GetClassName() {return "vtkAssembly";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddPart(vtkActor *);
  void RemovePart(vtkActor *);
  vtkActorCollection *GetParts();

  // Description:
  // Enable/disable the recursive application of the assembly's
  // transformation matrix to its component parts. Enabling this
  // instance variable allows you to manipulate an assembly as if
  // it were a single component. Note: the application of the
  // transformation occurs during the rendering process.
  vtkSetMacro(ApplyTransform,int);
  vtkGetMacro(ApplyTransform,int);
  vtkBooleanMacro(ApplyTransform,int);

  // Description:
  // Enable/disable the recursive application of the assembly's
  // properties to its component parts. Enabling this instance 
  // variable allows you to set the same properties to all its 
  // component parts with a single command. Note: the application
  // of the properties occurs during the rendering process.
  vtkSetMacro(ApplyProperty,int);
  vtkGetMacro(ApplyProperty,int);
  vtkBooleanMacro(ApplyProperty,int);

  void Render(vtkRenderer *ren);

  void InitPartTraversal();
  vtkActor *GetNextPart();

  float *GetBounds();
  unsigned long int GetMTime();

protected:
  vtkActorCollection Parts;
  int ApplyTransform;
  int ApplyProperty;

  vtkActor CurrentActor; //used to propagate state
  vtkProperty CurrentProperty;
  vtkMatrix4x4 CurrentMatrix;

  vtkAssemblyPaths *Paths;
  void ApplyTransformation();
  void ApplyProperties();
  void BuildPath(vtkActorCollection *path);
  vtkActor *ApplyTransformation(vtkActorCollection *path);
  vtkActor *ApplyProperties(vtkActorCollection *path);

};

// Description:
// Get the list of parts for this assembly.
inline vtkActorCollection *vtkAssembly::GetParts() {return &(this->Parts);};


//BTX - begin tcl exclude
// class is a list of lists of actors; each list of actors represents a
// a sequence of actors in an assembly. The list of actors is represented 
// as an actor collection.
class vtkAssemblyPaths : public vtkCollection
{
 public:
  char *GetClassName() {return "vtkAssemblyPaths";};

  void AddItem(vtkActorCollection *a);
  void RemoveItem(vtkActorCollection *a);
  int IsItemPresent(vtkActorCollection *a);
  vtkActorCollection *GetNextItem();
};

// Description:
// Add an actor to the list.
inline void vtkAssemblyPaths::AddItem(vtkActorCollection *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove an actor from the list.
inline void vtkAssemblyPaths::RemoveItem(vtkActorCollection *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular actor is present. Returns its position
// in the list.
inline int vtkAssemblyPaths::IsItemPresent(vtkActorCollection *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next actor in the list.
inline vtkActorCollection *vtkAssemblyPaths::GetNextItem() 
{ 
  return (vtkActorCollection *)(this->GetNextItemAsObject());
}
//ETX end tcl exclude

#endif


