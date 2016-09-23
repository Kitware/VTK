/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShortArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<short>
#endif
class VTKCOMMONCORE_EXPORT vtkShortArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkShortArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkShortArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(short);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkShortArray* FastDownCast(vtkAbstractArray *source)
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
  ~vtkShortArray() VTK_OVERRIDE;

private:

  typedef vtkAOSDataArrayTemplate<short> RealSuperclass;

  vtkShortArray(const vtkShortArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkShortArray&) VTK_DELETE_FUNCTION;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkShortArray)

#endif
