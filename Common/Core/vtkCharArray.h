/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCharArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCharArray - dynamic, self-adjusting array of char
// .SECTION Description
// vtkCharArray is an array of values of type char.  It provides
// methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef __vtkCharArray_h
#define __vtkCharArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(__vtkCharArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE char
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<char>
#endif
class VTKCOMMONCORE_EXPORT vtkCharArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkCharArray* New();
  vtkTypeMacro(vtkCharArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#ifdef __WRAP__
  vtkCreateWrappedArrayInterface(char);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static char GetDataTypeValueMin() { return VTK_CHAR_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static char GetDataTypeValueMax() { return VTK_CHAR_MAX; }

protected:
  vtkCharArray();
  ~vtkCharArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<char> RealSuperclass;
  //ETX
  vtkCharArray(const vtkCharArray&);  // Not implemented.
  void operator=(const vtkCharArray&);  // Not implemented.
};

#endif
