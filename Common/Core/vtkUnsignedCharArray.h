/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedCharArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<unsigned char>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedCharArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkUnsignedCharArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkUnsignedCharArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned char);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkUnsignedCharArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkUnsignedCharArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static unsigned char GetDataTypeValueMin() { return VTK_UNSIGNED_CHAR_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static unsigned char GetDataTypeValueMax() { return VTK_UNSIGNED_CHAR_MAX; }

protected:
  vtkUnsignedCharArray();
  ~vtkUnsignedCharArray() VTK_OVERRIDE;

private:

  typedef vtkAOSDataArrayTemplate<unsigned char> RealSuperclass;

  vtkUnsignedCharArray(const vtkUnsignedCharArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnsignedCharArray&) VTK_DELETE_FUNCTION;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkUnsignedCharArray)

#endif
