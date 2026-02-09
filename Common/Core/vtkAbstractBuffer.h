// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractBuffer
 * @brief   Abstract base class for vtkBuffer providing buffer protocol support.
 *
 * vtkAbstractBuffer provides an abstract interface for accessing buffer data
 * in a type-agnostic way. This enables Python buffer protocol support for
 * vtkBuffer template instantiations.
 *
 * @sa vtkBuffer
 */

#ifndef vtkAbstractBuffer_h
#define vtkAbstractBuffer_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONCORE_EXPORT vtkAbstractBuffer : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractBuffer, vtkObject);

  /**
   * Return the buffer pointer as a void pointer.
   */
  virtual void* GetVoidBuffer() = 0;

  /**
   * Return the number of elements in the buffer.
   */
  virtual vtkIdType GetNumberOfElements() const = 0;

  /**
   * Return the VTK data type identifier for the buffer's scalar type.
   * Returns one of VTK_FLOAT, VTK_DOUBLE, VTK_INT, etc.
   */
  virtual int GetDataType() const = 0;

  /**
   * Return the size in bytes of a single element.
   */
  virtual int GetDataTypeSize() const = 0;

protected:
  vtkAbstractBuffer() = default;
  ~vtkAbstractBuffer() override = default;

private:
  vtkAbstractBuffer(const vtkAbstractBuffer&) = delete;
  void operator=(const vtkAbstractBuffer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkAbstractBuffer.h
