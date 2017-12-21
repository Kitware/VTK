/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAssemblyPath
 * @brief   a list of nodes that form an assembly path
 *
 * vtkAssemblyPath represents an ordered list of assembly nodes that
 * represent a fully evaluated assembly path. This class is used primarily
 * for picking. Note that the use of this class is to add one or more
 * assembly nodes to form the path. (An assembly node consists of an instance
 * of vtkProp and vtkMatrix4x4, the matrix may be NULL.) As each node is
 * added, the matrices are concatenated to create a final, evaluated matrix.
 *
 * @sa
 * vtkAssemblyNode vtkAssembly vtkActor vtkMatrix4x4 vtkProp vtkAbstractPicker
*/

#ifndef vtkAssemblyPath_h
#define vtkAssemblyPath_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCollection.h"
#include "vtkAssemblyNode.h" // used for inlines

class vtkMatrix4x4;
class vtkTransform;
class vtkProp;

class VTKRENDERINGCORE_EXPORT vtkAssemblyPath : public vtkCollection
{
public:
  vtkTypeMacro(vtkAssemblyPath, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate empty path with identify matrix.
   */
  static vtkAssemblyPath *New();

  /**
   * Convenience method adds a prop and matrix together,
   * creating an assembly node transparently. The matrix
   * pointer m may be NULL. Note: that matrix is the one,
   * if any, associated with the prop.
   */
  void AddNode(vtkProp *p, vtkMatrix4x4 *m);

  /**
   * Get the next assembly node in the list. The node returned
   * contains a pointer to a prop and a 4x4 matrix. The matrix
   * is evaluated based on the preceding assembly hierarchy
   * (i.e., the matrix is not necessarily as the same as the
   * one that was added with AddNode() because of the
   * concatenation of matrices in the assembly hierarchy).
   */
  vtkAssemblyNode *GetNextNode();

  /**
   * Get the first assembly node in the list. See the comments for
   * GetNextNode() regarding the contents of the returned node. (Note: This
   * node corresponds to the vtkProp associated with the vtkRenderer.
   */
  vtkAssemblyNode *GetFirstNode();

  /**
   * Get the last assembly node in the list. See the comments
   * for GetNextNode() regarding the contents of the returned node.
   */
  vtkAssemblyNode *GetLastNode();

  /**
   * Delete the last assembly node in the list. This is like
   * a stack pop.
   */
  void DeleteLastNode();

  /**
   * Perform a shallow copy (reference counted) on the
   * incoming path.
   */
  void ShallowCopy(vtkAssemblyPath *path);

  /**
   * Override the standard GetMTime() to check for the modified times
   * of the nodes in this path.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkAssemblyNode *GetNextNode(vtkCollectionSimpleIterator &cookie)
    { return static_cast<vtkAssemblyNode *>(this->GetNextItemAsObject(cookie)); }

protected:
  vtkAssemblyPath();
  ~vtkAssemblyPath() override;

  void AddNode(vtkAssemblyNode *n); //Internal method adds assembly node
  vtkTransform *Transform; //Used to perform matrix concatenation
  vtkProp *TransformedProp; //A transformed prop used to do the rendering

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
    { this->vtkCollection::AddItem(o); }

private:
  vtkAssemblyPath(const vtkAssemblyPath&) = delete;
  void operator=(const vtkAssemblyPath&) = delete;
};

#endif
