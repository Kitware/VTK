/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFloatArray
 * @brief   dynamic, self-adjusting array of float
 *
 * vtkFloatArray is an array of values of type float.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
*/

#ifndef vtkFloatArray_h
#define vtkFloatArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<float>
#endif
class VTKCOMMONCORE_EXPORT vtkFloatArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkFloatArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif

  static vtkFloatArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(float);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkFloatArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkFloatArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static float GetDataTypeValueMin() { return VTK_FLOAT_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static float GetDataTypeValueMax() { return VTK_FLOAT_MAX; }


protected:
  vtkFloatArray();
  ~vtkFloatArray() VTK_OVERRIDE;

private:

  typedef vtkAOSDataArrayTemplate<float> RealSuperclass;

  vtkFloatArray(const vtkFloatArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFloatArray&) VTK_DELETE_FUNCTION;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkFloatArray)

#endif
