/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPath.h
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
// .NAME vtkAssemblyPath - a list of nodes that form an assembly path
// .SECTION Description
// vtkAssemblyPath represents an ordered list of assembly nodes that
// represent a fully evaluated assembly path. This class is used primarily
// for picking. Note that the use of this class is to add one or more
// assembly nodes to form the path. (An assembly node consists of an instance
// of vtkProp and vtkMatrix4x4, the matrix may be NULL.) As each node is
// added, the matrices are concatenated to create a final, evaluated matrix.

// .SECTION See Also
// vtkAssemblyNode vtkAssembly vtkActor vtkMatrix4x4 vtkProp vtkAbstractPicker

#ifndef __vtkAssemblyPath_h
#define __vtkAssemblyPath_h

#include "vtkCollection.h"

class vtkAssemblyNode;
class vtkMatrix4x4;
class vtkTransform;
class vtkProp;

class VTK_COMMON_EXPORT vtkAssemblyPath : public vtkCollection
{
public:
  vtkTypeMacro(vtkAssemblyPath,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate empty path with identify matrix.
  static vtkAssemblyPath *New();

  // Description:
  // Convenience method adds a prop and matrix together,
  // creating an assembly node transparently. The matrix
  // pointer m may be NULL. Note: that matrix is the one,
  // if any, associated with the prop. 
  void AddNode(vtkProp *p, vtkMatrix4x4 *m);

  // Description:
  // Get the next assembly node in the list. The node returned
  // contains a pointer to a prop and a 4x4 matrix. The matrix
  // is evaluated based on the preceding assembly hierarchy
  // (i.e., the matrix is not necessarily as the same as the
  // one that was added with AddNode() because of the 
  // concatenation of matrices in the assembly hierarchy).
  vtkAssemblyNode *GetNextNode();

  // Description:
  // Get the first assembly node in the list. See the comments for
  // GetNextNode() regarding the contents of the returned node. (Note: This
  // node corresponds to the vtkProp associated with the vtkRenderer.
  vtkAssemblyNode *GetFirstNode();
  
  // Description:
  // Get the last assembly node in the list. See the comments
  // for GetNextNode() regarding the contents of the returned node.
  vtkAssemblyNode *GetLastNode();
  
  // Description:
  // Delete the last assembly node in the list. This is like
  // a stack pop.
  void DeleteLastNode();
  
  // Description:
  // Perform a shallow copy (reference counted) on the
  // incoming path.
  void ShallowCopy(vtkAssemblyPath *path);
  
  // Description:
  // Override the standard GetMTime() to check for the modified times
  // of the nodes in this path.
  virtual unsigned long GetMTime();

protected:
  vtkAssemblyPath();
  ~vtkAssemblyPath();
  vtkAssemblyPath(const vtkAssemblyPath &);
  void operator=(const vtkAssemblyPath &);
  
  void AddNode(vtkAssemblyNode *n); //Internal method adds assembly node
  vtkTransform *Transform; //Used to perform matrix concatentation
  vtkProp *TransformedProp; //A transformed prop used to do the rendering

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

};

#endif
