// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAssemblyNode
 * @brief   represent a node in an assembly
 *
 * vtkAssemblyNode represents a node in an assembly. It is used by
 * vtkAssemblyPath to create hierarchical assemblies of props. The
 * props can be either 2D or 3D.
 *
 * An assembly node refers to a vtkProp, and possibly a vtkMatrix4x4.
 * Nodes are used by vtkAssemblyPath to build fully evaluated path
 * (matrices are concatenated through the path) that is used by picking
 * and other operations involving assemblies.
 *
 * @warning
 * The assembly node is guaranteed to contain a reference to an instance
 * of vtkMatrix4x4 if the prop referred to by the node is of type
 * vtkProp3D (or subclass). The matrix is evaluated through the assembly
 * path, so the assembly node's matrix is a function of its location in
 * the vtkAssemblyPath.
 *
 * @warning
 * vtkAssemblyNode does not reference count its association with vtkProp.
 * Therefore, do not create an assembly node, associate a prop with it,
 * delete the prop, and then try to dereference the prop. The program
 * will break! (Reason: vtkAssemblyPath (which uses vtkAssemblyNode)
 * create self-referencing loops that destroy reference counting.)
 *
 * @sa
 * vtkAssemblyPath vtkProp vtkPicker vtkMatrix4x4
 */

#ifndef vtkAssemblyNode_h
#define vtkAssemblyNode_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkProp;
class vtkMatrix4x4;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkAssemblyNode : public vtkObject
{
public:
  /**
   * Create an assembly node.
   */
  static vtkAssemblyNode* New();

  vtkTypeMacro(vtkAssemblyNode, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the prop that this assembly node refers to.
   */
  virtual void SetViewProp(vtkProp* prop);
  vtkGetObjectMacro(ViewProp, vtkProp);
  ///@}

  ///@{
  /**
   * Specify a transformation matrix associated with the prop.
   * Note: if the prop is not a type of vtkProp3D, then the
   * transformation matrix is ignored (and expected to be NULL).
   * Also, internal to this object the matrix is copied because
   * the matrix is used for computation by vtkAssemblyPath.
   */
  void SetMatrix(vtkMatrix4x4* matrix);
  vtkGetObjectMacro(Matrix, vtkMatrix4x4);
  ///@}

  /**
   * Override the standard GetMTime() to check for the modified times
   * of the prop and matrix.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkAssemblyNode();
  ~vtkAssemblyNode() override;

private:
  vtkProp* ViewProp;    // reference to vtkProp
  vtkMatrix4x4* Matrix; // associated matrix

  void operator=(const vtkAssemblyNode&) = delete;
  vtkAssemblyNode(const vtkAssemblyNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
