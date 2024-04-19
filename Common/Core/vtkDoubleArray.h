// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDoubleArray
 * @brief   dynamic, self-adjusting array of double
 *
 * vtkDoubleArray is an array of values of type double.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 */

#ifndef vtkDoubleArray_h
#define vtkDoubleArray_h

#include "vtkAOSDataArrayTemplate.h" // Real Superclass
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkDataArray.h"

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<double>
#endif
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkDoubleArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkDoubleArray, vtkDataArray);
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkDoubleArray* New();
  static vtkDoubleArray* ExtendedNew();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(double);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkDoubleArray* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkDoubleArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static double GetDataTypeValueMin() { return VTK_DOUBLE_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static double GetDataTypeValueMax() { return VTK_DOUBLE_MAX; }

protected:
  vtkDoubleArray();
  ~vtkDoubleArray() override;

private:
  typedef vtkAOSDataArrayTemplate<double> RealSuperclass;

  vtkDoubleArray(const vtkDoubleArray&) = delete;
  void operator=(const vtkDoubleArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkDoubleArray);

VTK_ABI_NAMESPACE_END
#endif
