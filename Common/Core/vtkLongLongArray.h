/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLongLongArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLongLongArray - dynamic, self-adjusting array of long long
// .SECTION Description
// vtkLongLongArray is an array of values of type long long.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef __vtkLongLongArray_h
#define __vtkLongLongArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(__vtkLongLongArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE long long
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<long long>
#endif
class VTKCOMMONCORE_EXPORT vtkLongLongArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkLongLongArray* New();
  vtkTypeMacro(vtkLongLongArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#ifdef __WRAP__
  vtkCreateWrappedArrayInterface(long long);
#endif
  // Description:
  // Get the minimum data value in its native type.
  static long long GetDataTypeValueMin() { return VTK_LONG_LONG_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static long long GetDataTypeValueMax() { return VTK_LONG_LONG_MAX; }

protected:
  vtkLongLongArray();
  ~vtkLongLongArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<long long> RealSuperclass;
  //ETX
  vtkLongLongArray(const vtkLongLongArray&);  // Not implemented.
  void operator=(const vtkLongLongArray&);  // Not implemented.
};

#endif
