/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdTypeArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIdTypeArray
 * @brief   dynamic, self-adjusting array of vtkIdType
 *
 * vtkIdTypeArray is an array of values of type vtkIdType.
 * It provides methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
*/

#ifndef vtkIdTypeArray_h
#define vtkIdTypeArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<vtkIdType>
#endif
class VTKCOMMONCORE_EXPORT vtkIdTypeArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkIdTypeArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkIdTypeArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(vtkIdType);
#else

  /**
   * Get the data type.
   */
  int GetDataType() VTK_OVERRIDE
  {
      // This needs to overwritten from superclass because
      // the templated superclass is not able to differentiate
      // vtkIdType from a long long or an int since vtkIdType
      // is simply a typedef. This means that
      // vtkAOSDataArrayTemplate<vtkIdType> != vtkIdTypeArray.
      return VTK_ID_TYPE;
  }
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkIdTypeArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkIdTypeArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static vtkIdType GetDataTypeValueMin() { return VTK_ID_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static vtkIdType GetDataTypeValueMax() { return VTK_ID_MAX; }

protected:
  vtkIdTypeArray();
  ~vtkIdTypeArray() VTK_OVERRIDE;

private:

  typedef vtkAOSDataArrayTemplate<vtkIdType> RealSuperclass;

  vtkIdTypeArray(const vtkIdTypeArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIdTypeArray&) VTK_DELETE_FUNCTION;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkIdTypeArray)

#endif
