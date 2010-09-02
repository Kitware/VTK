/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayData.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkArrayData - Pipeline data object that contains multiple vtkArray objects.
//
// .SECTION Description
// Because vtkArray cannot be stored as attributes of data objects (yet), a "carrier"
// object is needed to pass vtkArray through the pipeline.  vtkArrayData acts as a
// container of zero-to-many vtkArray instances, which can be retrieved via a zero-based
// index.  Note that a collection of arrays stored in vtkArrayData may-or-may-not have related
// types, dimensions, or extents.
//
// .SECTION See Also
// vtkArrayDataAlgorithm, vtkArray
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkArrayData_h
#define __vtkArrayData_h

#include "vtkArray.h"
#include "vtkDataObject.h"

class vtkArray;

class VTK_FILTERING_EXPORT vtkArrayData : public vtkDataObject
{
public:
  static vtkArrayData* New();
  vtkTypeMacro(vtkArrayData, vtkDataObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  static vtkArrayData* GetData(vtkInformation* info);
  static vtkArrayData* GetData(vtkInformationVector* v, int i = 0);

  // Description:
  // Adds a vtkArray to the collection
  void AddArray(vtkArray*);
  
  // Description:
  // Clears the contents of the collection
  void ClearArrays();
  
  // Description:
  // Returns the number of vtkArray instances in the collection
  vtkIdType GetNumberOfArrays();
  
  // Description:
  // Returns the n-th vtkArray in the collection
  vtkArray* GetArray(vtkIdType index);
  
  // Description:
  // Returns the array having called name from the collection
  vtkArray* GetArrayByName(const char *name);

  // Description:
  // Return class name of data type (VTK_ARRAY_DATA).
  virtual int GetDataObjectType() {return VTK_ARRAY_DATA;}

  virtual void ShallowCopy(vtkDataObject* other);
  virtual void DeepCopy(vtkDataObject* other);

protected:
  vtkArrayData();
  ~vtkArrayData();

private:
  vtkArrayData(const vtkArrayData&); // Not implemented
  void operator=(const vtkArrayData&); // Not implemented

//BTX
  class implementation;
  implementation* const Implementation;
//ETX
};

#endif

