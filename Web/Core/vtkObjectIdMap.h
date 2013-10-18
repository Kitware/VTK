/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectIdMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkObjectIdMap - class used to assign Id to any VTK object and be able
// to retreive it base on its id.

#ifndef __vtkObjectIdMap_h
#define __vtkObjectIdMap_h

#include "vtkObject.h"
#include "vtkWebCoreModule.h" // needed for exports

class VTKWEBCORE_EXPORT vtkObjectIdMap : public vtkObject
{
public:
  static vtkObjectIdMap* New();
  vtkTypeMacro(vtkObjectIdMap, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Retreive a unique identifier for the given object or generate a new one
  // if its global id was never requested.
  vtkTypeUInt32 GetGlobalId(vtkObject* obj);

  // Description:
  // Retreive a vtkObject based on its global id. If not found return NULL
  vtkObject* GetVTKObject(vtkTypeUInt32 globalId);

  // Description:
  // Assign an active key (string) to an existing object.
  // This is usually used to provide another type of access to specific
  // vtkObject that we want to retreive easily using a string.
  // Return the global Id of the given registered object
  vtkTypeUInt32 SetActiveObject(const char* objectType, vtkObject* obj);

  // Description:
  // Retreive a previously stored object based on a name
  vtkObject* GetActiveObject(const char* objectType);

  // Description:
  // Remove any internal reference count due to internal Id/Object mapping
  void FreeObject(vtkObject* obj);

//BTX
protected:
  vtkObjectIdMap();
  ~vtkObjectIdMap();

private:
  vtkObjectIdMap(const vtkObjectIdMap&); // Not implemented
  void operator=(const vtkObjectIdMap&); // Not implemented

  struct vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
