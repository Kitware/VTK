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

#ifndef __vtkArrayData_h
#define __vtkArrayData_h

#include "vtkArray.h"
#include "vtkDataObject.h"

class vtkArray;
class vtkInformation;
class vtkInformationVector;

// .NAME vtkArrayData - Pipeline data object that acts as a container
// for a single vtkArray
class VTK_FILTERING_EXPORT vtkArrayData : public vtkDataObject
{
public:
  static vtkArrayData* New();
  vtkTypeRevisionMacro(vtkArrayData, vtkDataObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  static vtkArrayData* GetData(vtkInformation* info);
  static vtkArrayData* GetData(vtkInformationVector* v, int i = 0);

  // Description:
  // Sets the vtkArray instance contained by this object
  vtkSetObjectMacro(Array, vtkArray);
  
  // Description:
  // Returns the vtkArray instance (if any) contained by this object
  vtkGetObjectMacro(Array, vtkArray);

protected:
  vtkArrayData();
  ~vtkArrayData();

  vtkArray* Array;

private:
  vtkArrayData(const vtkArrayData&); // Not implemented
  void operator=(const vtkArrayData&); // Not implemented
};

#endif

