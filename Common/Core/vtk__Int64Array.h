/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk__Int64Array.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtk__Int64Array - dynamic, self-adjusting array of __int64
// .SECTION Description
// vtk__Int64Array is an array of values of type __int64.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef vtk__Int64Array_h
#define vtk__Int64Array_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtk__Int64Array_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE __int64
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<__int64>
#endif
class VTKCOMMONCORE_EXPORT vtk__Int64Array : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtk__Int64Array* New();
  vtkTypeMacro(vtk__Int64Array,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(__int64);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static __int64 GetDataTypeValueMin() { return VTK___INT64_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static __int64 GetDataTypeValueMax() { return VTK___INT64_MAX; }

protected:
  vtk__Int64Array(vtkIdType numComp=1);
  ~vtk__Int64Array();

private:
  //BTX
  typedef vtkDataArrayTemplate<__int64> RealSuperclass;
  //ETX
  vtk__Int64Array(const vtk__Int64Array&);  // Not implemented.
  void operator=(const vtk__Int64Array&);  // Not implemented.
};

#endif
