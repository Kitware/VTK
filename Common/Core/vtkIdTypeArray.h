/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdTypeArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIdTypeArray - dynamic, self-adjusting array of vtkIdType
// .SECTION Description
// vtkIdTypeArray is an array of values of type vtkIdType.
// It provides methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.

#ifndef __vtkIdTypeArray_h
#define __vtkIdTypeArray_h

// Tell the template header how to give our superclass a DLL interface.
#if !(defined(__vtkIdTypeArray_cxx) && defined(VTK_USE_64BIT_IDS)) && (defined(VTK_USE_64BIT_IDS) || !defined(__vtkIntArray_h))
# define VTK_DATA_ARRAY_TEMPLATE_TYPE vtkIdType
#endif

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkDataArrayTemplate<vtkIdType>
#endif
class VTKCOMMONCORE_EXPORT vtkIdTypeArray : public vtkDataArray
#ifndef __WRAP__
#undef vtkDataArray
#endif
{
public:
  static vtkIdTypeArray* New();
  vtkTypeMacro(vtkIdTypeArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkDataArrayTemplate, which is ignored
  // by the wrappers.
#ifdef __WRAP__
  vtkCreateWrappedArrayInterface(vtkIdType);
#else

  // Description:
  // Get the data type.
  int GetDataType()
    {
      // This needs to overwritten from superclass because
      // the templated superclass is not able to differentiate
      // vtkIdType from a long long or an int since vtkIdType
      // is simply a typedef. This means that
      // vtkDataArrayTemplate<vtkIdType> != vtkIdTypeArray.
      return VTK_ID_TYPE;
    }
#endif

  // Description:
  // Get the minimum data value in its native type.
  static vtkIdType GetDataTypeValueMin() { return VTK_ID_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static vtkIdType GetDataTypeValueMax() { return VTK_ID_MAX; }

protected:
  vtkIdTypeArray();
  ~vtkIdTypeArray();

private:
  //BTX
  typedef vtkDataArrayTemplate<vtkIdType> RealSuperclass;
  //ETX
  vtkIdTypeArray(const vtkIdTypeArray&);  // Not implemented.
  void operator=(const vtkIdTypeArray&);  // Not implemented.
};

#endif
