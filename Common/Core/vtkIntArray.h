/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIntArray - dynamic, self-adjusting array of int
// .SECTION Description
// vtkIntArray is an array of values of type int.  It provides
// methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.
//
// The C++ standard does not define the exact size of the int type, so use
// of this type directly is discouraged.  If an array of 32 bit integers is
// needed, prefer vtkTypeInt32Array to this class.

#ifndef vtkIntArray_h
#define vtkIntArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtkIntArray_cxx) && (defined(VTK_USE_64BIT_IDS) || !defined(vtkIdTypeArray_h))
# define VTK_DATA_ARRAY_TEMPLATE_TYPE int
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<int>
#endif
class VTKCOMMONCORE_EXPORT vtkIntArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkIntArray* New();
  vtkTypeMacro(vtkIntArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(int);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static int GetDataTypeValueMin() { return VTK_INT_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static int GetDataTypeValueMax() { return VTK_INT_MAX; }

protected:
  vtkIntArray();
  ~vtkIntArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<int> RealSuperclass;
  //ETX
  vtkIntArray(const vtkIntArray&);  // Not implemented.
  void operator=(const vtkIntArray&);  // Not implemented.
};

#endif
