/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyNode.h
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
// .NAME vtkAssemblyNode - represent a node in an assembly
// .SECTION Description
// vtkAssemblyNode represents a node in an assembly. It is used by
// vtkAssemblyPath to create hierarchical assemblies of props. The
// props can be either 2D or 3D.
//
// An assembly node refers to a vtkProp, and possibly a vtkMatrix4x4.
// Nodes are used by vtkAssemblyPath to build fully evaluated path
// (matrices are concatenated through the path) that is used by picking
// and other operations involving assemblies.

// .SECTION Caveats
// The assembly node is guaranteed to contain a reference to an instance
// of vtkMatrix4x4 if the prop referred to by the node is of type
// vtkProp3D (or subclass). The matrix is evaluated through the assembly
// path, so the assembly node's matrix is a function of its location in 
// the vtkAssemblyPath.
//
// vtkAssemblyNode does not reference count its association with vtkProp.
// Therefore, do not create an assembly node, associate a prop with it,
// delete the prop, and then try to dereference the prop. The program
// will break! (Reason: vtkAssemblyPath (which uses vtkAssemblyNode)
// create self-referencing loops that destroy reference counting.)

// .SECTION see also
// vtkAssemblyPath vtkProp vtkPicker vtkMatrix4x4

#ifndef __vtkAssemblyNode_h
#define __vtkAssemblyNode_h

#include "vtkObject.h"
#include "vtkProp.h"
#include "vtkMatrix4x4.h"

class VTK_EXPORT vtkAssemblyNode : public vtkObject
{
public:
  // Description:
  // Create an assembly node.
  static vtkAssemblyNode *New();

  vtkTypeMacro(vtkAssemblyNode,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the prop that this assembly node refers to.
  void SetProp(vtkProp *prop);
  vtkGetObjectMacro(Prop, vtkProp);
  
  // Description:
  // Specify a transformation matrix associated with the prop.
  // Note: if the prop is not a type of vtkProp3D, then the
  // transformation matrix is ignored (and expected to be NULL).
  // Also, internal to this object the matrix is copied because
  // the matrix is used for computation by vtkAssemblyPath.
  void SetMatrix(vtkMatrix4x4 *matrix);
  vtkGetObjectMacro(Matrix, vtkMatrix4x4);
  
  // Description:
  // Override the standard GetMTime() to check for the modified times
  // of the prop and matrix.
  virtual unsigned long GetMTime();

protected:
  vtkAssemblyNode();
  ~vtkAssemblyNode();
  vtkAssemblyNode(const vtkAssemblyNode &);
  void operator=(const vtkAssemblyNode &);

private:
  vtkProp *Prop; //reference to vtkProp
  vtkMatrix4x4 *Matrix; //associated matrix
  
};

#endif
