/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedShortArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnsignedShortArray
 * @brief   dynamic, self-adjusting array of unsigned short
 *
 * vtkUnsignedShortArray is an array of values of type unsigned short.
 * It provides methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
 *
 * The C++ standard does not define the exact size of the unsigned short type,
 * so use of this type directly is discouraged.  If an array of 16 bit
 * unsigned integers is needed, prefer vtkTypeUInt16Array to this class.
*/

#ifndef vtkUnsignedShortArray_h
#define vtkUnsignedShortArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<unsigned short>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedShortArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkUnsignedShortArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkUnsignedShortArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned short);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkUnsignedShortArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkUnsignedShortArray*>(
          Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static unsigned short GetDataTypeValueMin() { return VTK_UNSIGNED_SHORT_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static unsigned short GetDataTypeValueMax() { return VTK_UNSIGNED_SHORT_MAX; }

protected:
  vtkUnsignedShortArray();
  ~vtkUnsignedShortArray() override;

private:

  typedef vtkAOSDataArrayTemplate<unsigned short> RealSuperclass;

  vtkUnsignedShortArray(const vtkUnsignedShortArray&) = delete;
  void operator=(const vtkUnsignedShortArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkUnsignedShortArray)

#endif
