/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSignedCharArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSignedCharArray - dynamic, self-adjusting array of signed char
// .SECTION Description
// vtkSignedCharArray is an array of values of type signed char.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef __vtkSignedCharArray_h
#define __vtkSignedCharArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(__vtkSignedCharArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE signed char
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<signed char>
#endif
class VTKCOMMONCORE_EXPORT vtkSignedCharArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkSignedCharArray* New();
  vtkTypeMacro(vtkSignedCharArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#ifdef __WRAP__
  vtkCreateWrappedArrayInterface(signed char);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static signed char GetDataTypeValueMin() { return VTK_SIGNED_CHAR_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static signed char GetDataTypeValueMax() { return VTK_SIGNED_CHAR_MAX; }

protected:
  vtkSignedCharArray();
  ~vtkSignedCharArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<signed char> RealSuperclass;
  //ETX
  vtkSignedCharArray(const vtkSignedCharArray&);  // Not implemented.
  void operator=(const vtkSignedCharArray&);  // Not implemented.
};

#endif
