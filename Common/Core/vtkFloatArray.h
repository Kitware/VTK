/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFloatArray - dynamic, self-adjusting array of float
// .SECTION Description
// vtkFloatArray is an array of values of type float.  It provides
// methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef vtkFloatArray_h
#define vtkFloatArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !defined(vtkFloatArray_cxx)
# define VTK_DATA_ARRAY_TEMPLATE_TYPE float
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<float>
#endif
class VTKCOMMONCORE_EXPORT vtkFloatArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkFloatArray* New();
  vtkTypeMacro(vtkFloatArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(float);
#endif

  // Description:
  // Get the minimum data value in its native type.
  static float GetDataTypeValueMin() { return VTK_FLOAT_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static float GetDataTypeValueMax() { return VTK_FLOAT_MAX; }


protected:
  vtkFloatArray();
  ~vtkFloatArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<float> RealSuperclass;
  //ETX
  vtkFloatArray(const vtkFloatArray&);  // Not implemented.
  void operator=(const vtkFloatArray&);  // Not implemented.
};

#endif
