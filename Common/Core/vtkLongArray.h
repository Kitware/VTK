/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLongArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLongArray - dynamic, self-adjusting array of long
// .SECTION Description
// vtkLongArray is an array of values of type long.  It provides
// methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef vtkLongArray_h
#define vtkLongArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtkLongArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE long
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<long>
#endif
class VTKCOMMONCORE_EXPORT vtkLongArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkLongArray* New();
  vtkTypeMacro(vtkLongArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(long);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static long GetDataTypeValueMin() { return VTK_LONG_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static long GetDataTypeValueMax() { return VTK_LONG_MAX; }

protected:
  vtkLongArray();
  ~vtkLongArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<long> RealSuperclass;
  //ETX
  vtkLongArray(const vtkLongArray&);  // Not implemented.
  void operator=(const vtkLongArray&);  // Not implemented.
};

#endif
