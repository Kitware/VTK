/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLongArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLongArray
 * @brief   dynamic, self-adjusting array of long
 *
 * vtkLongArray is an array of values of type long.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 *
 * The C++ standard does not define the exact size of the long type, so use
 * of this type directly is discouraged.  If an array of 32 bit integers is
 * needed, prefer vtkTypeInt32Array to this class.  If an array of 64 bit
 * integers is needed, prefer vtkTypeInt64Array to this class.
*/

#ifndef vtkLongArray_h
#define vtkLongArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<long>
#endif
class VTKCOMMONCORE_EXPORT vtkLongArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkLongArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkLongArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(long);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkLongArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkLongArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static long GetDataTypeValueMin() { return VTK_LONG_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static long GetDataTypeValueMax() { return VTK_LONG_MAX; }

protected:
  vtkLongArray();
  ~vtkLongArray() VTK_OVERRIDE;

private:

  typedef vtkAOSDataArrayTemplate<long> RealSuperclass;

  vtkLongArray(const vtkLongArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLongArray&) VTK_DELETE_FUNCTION;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkLongArray)

#endif
