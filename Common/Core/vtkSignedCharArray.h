// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSignedCharArray
 * @brief   dynamic, self-adjusting array of signed char
 *
 * vtkSignedCharArray is an array of values of type signed char.
 * It provides methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 */

#ifndef vtkSignedCharArray_h
#define vtkSignedCharArray_h

#include "vtkAOSDataArrayTemplate.h" // Real Superclass
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkDataArray.h"

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<signed char>
#endif
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkSignedCharArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkSignedCharArray, vtkDataArray);
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkSignedCharArray* New();
  static vtkSignedCharArray* ExtendedNew();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(signed char);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkSignedCharArray* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkSignedCharArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static signed char GetDataTypeValueMin() { return VTK_SIGNED_CHAR_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static signed char GetDataTypeValueMax() { return VTK_SIGNED_CHAR_MAX; }

protected:
  vtkSignedCharArray();
  ~vtkSignedCharArray() override;

private:
  typedef vtkAOSDataArrayTemplate<signed char> RealSuperclass;

  vtkSignedCharArray(const vtkSignedCharArray&) = delete;
  void operator=(const vtkSignedCharArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkSignedCharArray);

VTK_ABI_NAMESPACE_END
#endif
