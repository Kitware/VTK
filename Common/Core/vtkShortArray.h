/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShortArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShortArray - dynamic, self-adjusting array of short
// .SECTION Description
// vtkShortArray is an array of values of type short.  It provides
// methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef __vtkShortArray_h
#define __vtkShortArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(__vtkShortArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE short
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<short>
#endif
class VTKCOMMONCORE_EXPORT vtkShortArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkShortArray* New();
  vtkTypeMacro(vtkShortArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#ifdef __WRAP__
  vtkCreateWrappedArrayInterface(short);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static short GetDataTypeValueMin() { return VTK_SHORT_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static short GetDataTypeValueMax() { return VTK_SHORT_MAX; }

protected:
  vtkShortArray();
  ~vtkShortArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<short> RealSuperclass;
  //ETX
  vtkShortArray(const vtkShortArray&);  // Not implemented.
  void operator=(const vtkShortArray&);  // Not implemented.
};

#endif
