/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCharArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCharArray
 * @brief   dynamic, self-adjusting array of char
 *
 * vtkCharArray is an array of values of type char.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
*/

#ifndef vtkCharArray_h
#define vtkCharArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<char>
#endif
class VTKCOMMONCORE_EXPORT vtkCharArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkCharArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkCharArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(char);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkCharArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkCharArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static char GetDataTypeValueMin() { return VTK_CHAR_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static char GetDataTypeValueMax() { return VTK_CHAR_MAX; }

protected:
  vtkCharArray();
  ~vtkCharArray() VTK_OVERRIDE;

private:

  typedef vtkAOSDataArrayTemplate<char> RealSuperclass;

  vtkCharArray(const vtkCharArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCharArray&) VTK_DELETE_FUNCTION;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkCharArray)

#endif
