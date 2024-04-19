// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnsignedIntArray
 * @brief   dynamic, self-adjusting array of unsigned int
 *
 * vtkUnsignedIntArray is an array of values of type unsigned int.  It
 * provides methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 *
 * The C++ standard does not define the exact size of the unsigned int type,
 * so use of this type directly is discouraged.  If an array of 32 bit unsigned
 * integers is needed, prefer vtkTypeUInt32Array to this class.
 */

#ifndef vtkUnsignedIntArray_h
#define vtkUnsignedIntArray_h

#include "vtkAOSDataArrayTemplate.h" // Real Superclass
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkDataArray.h"

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<unsigned int>
#endif
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkUnsignedIntArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkUnsignedIntArray, vtkDataArray);
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkUnsignedIntArray* New();
  static vtkUnsignedIntArray* ExtendedNew();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned int);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkUnsignedIntArray* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkUnsignedIntArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static unsigned int GetDataTypeValueMin() { return VTK_UNSIGNED_INT_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static unsigned int GetDataTypeValueMax() { return VTK_UNSIGNED_INT_MAX; }

protected:
  vtkUnsignedIntArray();
  ~vtkUnsignedIntArray() override;

private:
  typedef vtkAOSDataArrayTemplate<unsigned int> RealSuperclass;

  vtkUnsignedIntArray(const vtkUnsignedIntArray&) = delete;
  void operator=(const vtkUnsignedIntArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkUnsignedIntArray);

VTK_ABI_NAMESPACE_END
#endif
