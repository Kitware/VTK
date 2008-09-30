/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFactoredArrayData.h
  
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

#ifndef __vtkFactoredArrayData_h
#define __vtkFactoredArrayData_h

#include "vtkArray.h"
#include "vtkDataObject.h"

class vtkArray;

// .NAME vtkFactoredArrayData - Pipeline data object that contains a
// collection of vtkArray instances.  Used to store the "factored"
// representation of a larger array.

class VTK_FILTERING_EXPORT vtkFactoredArrayData : public vtkDataObject
{
public:
  static vtkFactoredArrayData* New();
  vtkTypeRevisionMacro(vtkFactoredArrayData, vtkDataObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  static vtkFactoredArrayData* GetData(vtkInformation* info);
  static vtkFactoredArrayData* GetData(vtkInformationVector* v, int i = 0);

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

protected:
  vtkFactoredArrayData();
  ~vtkFactoredArrayData();

private:
  vtkFactoredArrayData(const vtkFactoredArrayData&); // Not implemented
  void operator=(const vtkFactoredArrayData&); // Not implemented

//BTX
  class implementation;
  implementation* const Implementation;
//ETX
};

#endif

