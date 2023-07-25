// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCharArray
 * @brief   dynamic, self-adjusting array of char
 *
 * vtkCharArray is an array of values of type char.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 *
 * @warning
 * This class should be avoided in favor of either
 * vtkSignedCharArray or vtkUnsignedCharArray. On some systems
 * the underlying data will be stored as unsigned chars and others
 * it will be stored as signed chars. Additionally, saving this
 * array out and then reading it back in it could be transformed to
 * a vtkSignedCharArray or vtkUnsignedCharArray and if that happens
 * the result of a vtkCharArray::SafeDownCast() of that pointer will be
 * a null pointer.
 *
 * @sa
 * vtkSignedCharArray vtkUnsignedCharArray
 */

#ifndef vtkCharArray_h
#define vtkCharArray_h

#include "vtkAOSDataArrayTemplate.h" // Real Superclass
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkDataArray.h"

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<char>
#endif
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkCharArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkCharArray, vtkDataArray);
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkCharArray* New();
  static vtkCharArray* ExtendedNew();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(char);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkCharArray* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkCharArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static char GetDataTypeValueMin() { return VTK_CHAR_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static char GetDataTypeValueMax() { return VTK_CHAR_MAX; }

protected:
  vtkCharArray();
  ~vtkCharArray() override;

private:
  typedef vtkAOSDataArrayTemplate<char> RealSuperclass;

  vtkCharArray(const vtkCharArray&) = delete;
  void operator=(const vtkCharArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkCharArray);

VTK_ABI_NAMESPACE_END
#endif
