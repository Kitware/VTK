/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedIntArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnsignedIntArray - dynamic, self-adjusting array of unsigned int
// .SECTION Description
// vtkUnsignedIntArray is an array of values of type unsigned int.  It
// provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef vtkUnsignedIntArray_h
#define vtkUnsignedIntArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtkUnsignedIntArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE unsigned int
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<unsigned int>
#endif
class VTKCOMMONCORE_EXPORT vtkUnsignedIntArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkUnsignedIntArray* New();
  vtkTypeMacro(vtkUnsignedIntArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(unsigned int);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static unsigned int GetDataTypeValueMin() { return VTK_UNSIGNED_INT_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static unsigned int GetDataTypeValueMax() { return VTK_UNSIGNED_INT_MAX; }

protected:
  vtkUnsignedIntArray();
  ~vtkUnsignedIntArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<unsigned int> RealSuperclass;
  //ETX
  vtkUnsignedIntArray(const vtkUnsignedIntArray&);  // Not implemented.
  void operator=(const vtkUnsignedIntArray&);  // Not implemented.
};

#endif
