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
// The application of properties is order dependent!!! By order we mean the
// order in which the assemblies are rendered. Each time an assembly is 
// rendered, it propagates its properties through its part hierarchy. As a
// result, if an assembly (or actor) is part of two different assemblies
// (say a1 and a2), and the list of actors is reversed (say from a1,a2 to
// a2,a1), then the final image may change.
//
// Actor's that form parts of assemblies cannot use the UserDefined matrix
// defined in vtkActor. This is because this ivar is used during the assembly
// concatenation process.
 
// .SECTION See Also
// vtkActor vtkTransform vtkMapper vtkPolyMapper

#ifndef __vtkAssembly_h
#define __vtkAssembly_h

#include "vtkActor.hh"

class vtkAssembly : public vtkActor
{
public:
  vtkAssembly();
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
  virtual void ApplyTransformation();
  virtual void ApplyProperties();

  vtkActorCollection *GetComposingParts();
  float *GetBounds();

protected:
  vtkActorCollection Parts;
  int ApplyTransform;
  int ApplyProperty;
  vtkTimeStamp RenderTime;

  void AddComposingParts(vtkActorCollection &);
};

// Description:
// Get the list of parts for this assembly.
inline vtkActorCollection *vtkAssembly::GetParts() {return &(this->Parts);};

#endif


