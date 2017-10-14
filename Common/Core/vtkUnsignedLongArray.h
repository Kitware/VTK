/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedLongArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnsignedLongArray
 * @brief   dynamic, self-adjusting array of unsigned long
 *
 * vtkUnsignedLongArray is an array of values of type unsigned long.
 * It provides methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 *
 * The C++ standard does not define the exact size of the unsigned long type,
 * so use of this type directly is discouraged.  If an array of 32 bit
 * unsigned integers is needed, prefer vtkTypeUInt32Array to this class.
 * If an array of 64 bit unsigned integers is needed, prefer
 * vtkUTypeInt64Array to this class.
*/

#ifndef vtkUnsignedLongArray_h
#define vtkUnsignedLongArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<unsigned long>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedLongArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkUnsignedLongArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkUnsignedLongArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned long);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkUnsignedLongArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkUnsignedLongArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static unsigned long GetDataTypeValueMin() { return VTK_UNSIGNED_LONG_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static unsigned long GetDataTypeValueMax() { return VTK_UNSIGNED_LONG_MAX; }

protected:
  vtkUnsignedLongArray();
  ~vtkUnsignedLongArray() override;

private:

  typedef vtkAOSDataArrayTemplate<unsigned long> RealSuperclass;

  vtkUnsignedLongArray(const vtkUnsignedLongArray&) = delete;
  void operator=(const vtkUnsignedLongArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkUnsignedLongArray)

#endif
