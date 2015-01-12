/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedCharArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnsignedCharArray - dynamic, self-adjusting array of unsigned char
// .SECTION Description
// vtkUnsignedCharArray is an array of values of type unsigned char.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef vtkUnsignedCharArray_h
#define vtkUnsignedCharArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtkUnsignedCharArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE unsigned char
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<unsigned char>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedCharArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkUnsignedCharArray* New();
  vtkTypeMacro(vtkUnsignedCharArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned char);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static unsigned char GetDataTypeValueMin() { return VTK_UNSIGNED_CHAR_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static unsigned char GetDataTypeValueMax() { return VTK_UNSIGNED_CHAR_MAX; }

protected:
  vtkUnsignedCharArray();
  ~vtkUnsignedCharArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<unsigned char> RealSuperclass;
  //ETX
  vtkUnsignedCharArray(const vtkUnsignedCharArray&);  // Not implemented.
  void operator=(const vtkUnsignedCharArray&);  // Not implemented.
};

#endif
