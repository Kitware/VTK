// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkShortArray
 * @brief   dynamic, self-adjusting array of short
 *
 * vtkShortArray is an array of values of type short.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 *
 * The C++ standard does not define the exact size of the short type,
 * so use of this type directly is discouraged.  If an array of 16 bit
 * integers is needed, prefer vtkTypeInt16Array to this class.
 */

#ifndef vtkShortArray_h
#define vtkShortArray_h

#include "vtkAOSDataArrayTemplate.h" // Real Superclass
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkDataArray.h"

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<short>
#endif
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkShortArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkShortArray, vtkDataArray);
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkShortArray* New();
  static vtkShortArray* ExtendedNew();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(short);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkShortArray* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkShortArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static short GetDataTypeValueMin() { return VTK_SHORT_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static short GetDataTypeValueMax() { return VTK_SHORT_MAX; }

protected:
  vtkShortArray();
  ~vtkShortArray() override;

private:
  typedef vtkAOSDataArrayTemplate<short> RealSuperclass;

  vtkShortArray(const vtkShortArray&) = delete;
  void operator=(const vtkShortArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkShortArray);

VTK_ABI_NAMESPACE_END
#endif
