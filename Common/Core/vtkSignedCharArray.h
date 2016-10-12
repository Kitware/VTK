/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSignedCharArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<signed char>
#endif
class VTKCOMMONCORE_EXPORT vtkSignedCharArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkSignedCharArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkSignedCharArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(signed char);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkSignedCharArray* FastDownCast(vtkAbstractArray *source)
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
  ~vtkSignedCharArray() VTK_OVERRIDE;

private:

  typedef vtkAOSDataArrayTemplate<signed char> RealSuperclass;

  vtkSignedCharArray(const vtkSignedCharArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSignedCharArray&) VTK_DELETE_FUNCTION;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkSignedCharArray)

#endif
