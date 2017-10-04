/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDoubleArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDoubleArray
 * @brief   dynamic, self-adjusting array of double
 *
 * vtkDoubleArray is an array of values of type double.  It provides
 * methods for insertion and retrieval of values and will
 * automatically resize itself to hold new data.
*/

#ifndef vtkDoubleArray_h
#define vtkDoubleArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAOSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __VTK_WRAP__
#define vtkDataArray vtkAOSDataArrayTemplate<double>
#endif
class VTKCOMMONCORE_EXPORT vtkDoubleArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkDoubleArray, vtkDataArray)
#ifndef __VTK_WRAP__
#undef vtkDataArray
#endif
  static vtkDoubleArray* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAOSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(double);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkDoubleArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkDoubleArray*>(Superclass::FastDownCast(source));
  }

  /**
   * Get the minimum data value in its native type.
   */
  static double GetDataTypeValueMin() { return VTK_DOUBLE_MIN; }

  /**
   * Get the maximum data value in its native type.
   */
  static double GetDataTypeValueMax() { return VTK_DOUBLE_MAX; }

protected:
  vtkDoubleArray();
  ~vtkDoubleArray() override;

private:

  typedef vtkAOSDataArrayTemplate<double> RealSuperclass;

  vtkDoubleArray(const vtkDoubleArray&) = delete;
  void operator=(const vtkDoubleArray&) = delete;
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkDoubleArray)


#endif
