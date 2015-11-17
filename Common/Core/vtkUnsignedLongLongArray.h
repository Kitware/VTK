/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedLongLongArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnsignedLongLongArray - dynamic, self-adjusting array of unsigned long long
// .SECTION Description
// vtkUnsignedLongLongArray is an array of values of type unsigned long long.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.
//
// This class should not be used directly, as it only exists on systems
// where the unsigned long long type is defined.  If you need an unsigned
// 64 bit integer data array, use vtkTypeUInt64Array instead.

#ifndef vtkUnsignedLongLongArray_h
#define vtkUnsignedLongLongArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAoSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkAoSDataArrayTemplate<unsigned long long>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedLongLongArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkUnsignedLongLongArray, vtkDataArray)
#ifndef __WRAP__
#undef vtkDataArray
#endif
  static vtkUnsignedLongLongArray* New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned long long);
#endif

  // Description:
  // A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
  static vtkUnsignedLongLongArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkUnsignedLongLongArray*>(
          Superclass::FastDownCast(source));
  }

  // Description:
  // Get the minimum data value in its native type.
  static unsigned long long GetDataTypeValueMin() {return VTK_UNSIGNED_LONG_LONG_MIN;}

  // Description:
  // Get the maximum data value in its native type.
  static unsigned long long GetDataTypeValueMax() {return VTK_UNSIGNED_LONG_LONG_MAX;}

protected:
  vtkUnsignedLongLongArray();
  ~vtkUnsignedLongLongArray();

private:
  //BTX
  typedef vtkAoSDataArrayTemplate<unsigned long long> RealSuperclass;
  //ETX
  vtkUnsignedLongLongArray(const vtkUnsignedLongLongArray&);  // Not implemented.
  void operator=(const vtkUnsignedLongLongArray&);  // Not implemented.
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkUnsignedLongLongArray)

#endif
