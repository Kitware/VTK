// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIntArray
 * @brief   dynamic, self-adjusting array of int
 *
 * vtkIntArray is an array of values of type int.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 *
 * The C++ standard does not define the exact size of the int type, so use
 * of this type directly is discouraged.  If an array of 32 bit integers is
 * needed, prefer vtkTypeInt32Array to this class.
 */

#ifndef vtkIntArray_h
#define vtkIntArray_h

#include "vtkAOSDataArrayTemplate.h" // Real Superclass
#include "vtkCommonCoreModule.h"     // For export macro
#include "vtkDataArray.h"

// Fake the superclass for non-Python wrappers.
// Python can handle the templated superclass; Java and others cannot.
#if !defined(__VTK_WRAP__) || defined(__VTK_WRAP_PYTHON__)
#define vtkDataArray vtkAOSDataArrayTemplate<int>
#endif
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkIntArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkIntArray, vtkDataArray);
#if !defined(__VTK_WRAP__) || defined(__VTK_WRAP_PYTHON__)
#undef vtkDataArray
#endif
  static vtkIntArray* New();
  static vtkIntArray* ExtendedNew();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if (defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)) && !defined(__VTK_WRAP_PYTHON__)
  vtkCreateWrappedArrayInterface(int);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkIntArray* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkIntArray*>(Superclass::FastDownCast(source));
  }

protected:
  vtkIntArray();
  ~vtkIntArray() override;

private:
  typedef vtkAOSDataArrayTemplate<int> RealSuperclass;

  vtkIntArray(const vtkIntArray&) = delete;
  void operator=(const vtkIntArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkIntArray);

VTK_ABI_NAMESPACE_END
#endif
