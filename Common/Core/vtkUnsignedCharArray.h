// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnsignedCharArray
 * @brief   dynamic, self-adjusting array of unsigned char
 *
 * vtkUnsignedCharArray is an array of values of type unsigned char.
 * It provides methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 */

#ifndef vtkUnsignedCharArray_h
#define vtkUnsignedCharArray_h

#include "vtkAOSDataArrayTemplate.h" // Real Superclass
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkDataArray.h"

// Fake the superclass for non-Python wrappers.
// Python can handle the templated superclass; Java and others cannot.
#if !defined(__VTK_WRAP__) || defined(__VTK_WRAP_PYTHON__)
#define vtkDataArray vtkAOSDataArrayTemplate<unsigned char>
#endif
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkUnsignedCharArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkUnsignedCharArray, vtkDataArray);
#if !defined(__VTK_WRAP__) || defined(__VTK_WRAP_PYTHON__)
#undef vtkDataArray
#endif
  static vtkUnsignedCharArray* New();
  static vtkUnsignedCharArray* ExtendedNew();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if (defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)) && !defined(__VTK_WRAP_PYTHON__)
  vtkCreateWrappedArrayInterface(unsigned char);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkUnsignedCharArray* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkUnsignedCharArray*>(Superclass::FastDownCast(source));
  }

protected:
  vtkUnsignedCharArray();
  ~vtkUnsignedCharArray() override;

private:
  typedef vtkAOSDataArrayTemplate<unsigned char> RealSuperclass;

  vtkUnsignedCharArray(const vtkUnsignedCharArray&) = delete;
  void operator=(const vtkUnsignedCharArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkUnsignedCharArray);

VTK_ABI_NAMESPACE_END
#endif
