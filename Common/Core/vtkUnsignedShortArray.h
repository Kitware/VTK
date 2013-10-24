/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedShortArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnsignedShortArray - dynamic, self-adjusting array of unsigned short
// .SECTION Description
// vtkUnsignedShortArray is an array of values of type unsigned short.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef __vtkUnsignedShortArray_h
#define __vtkUnsignedShortArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(__vtkUnsignedShortArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE unsigned short
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<unsigned short>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedShortArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkUnsignedShortArray* New();
  vtkTypeMacro(vtkUnsignedShortArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#ifdef __WRAP__
  vtkCreateWrappedArrayInterface(unsigned short);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static unsigned short GetDataTypeValueMin() { return VTK_UNSIGNED_SHORT_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static unsigned short GetDataTypeValueMax() { return VTK_UNSIGNED_SHORT_MAX; }

protected:
  vtkUnsignedShortArray();
  ~vtkUnsignedShortArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<unsigned short> RealSuperclass;
  //ETX
  vtkUnsignedShortArray(const vtkUnsignedShortArray&);  // Not implemented.
  void operator=(const vtkUnsignedShortArray&);  // Not implemented.
};

#endif
