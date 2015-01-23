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

#ifndef vtkUnsignedLongLongArray_h
#define vtkUnsignedLongLongArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtkUnsignedLongLongArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE unsigned long long
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<unsigned long long>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedLongLongArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkUnsignedLongLongArray* New();
  vtkTypeMacro(vtkUnsignedLongLongArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned long long);
#endif

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
  typedef vtkDataArrayTemplate<unsigned long long> RealSuperclass;
  //ETX
  vtkUnsignedLongLongArray(const vtkUnsignedLongLongArray&);  // Not implemented.
  void operator=(const vtkUnsignedLongLongArray&);  // Not implemented.
};

#endif
