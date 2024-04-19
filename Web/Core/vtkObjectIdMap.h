// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkObjectIdMap
 * @brief   class used to assign Id to any VTK object and be able
 * to retrieve it base on its id.
 */

#ifndef vtkObjectIdMap_h
#define vtkObjectIdMap_h

#include "vtkObject.h"
#include "vtkWebCoreModule.h" // needed for exports

VTK_ABI_NAMESPACE_BEGIN
class VTKWEBCORE_EXPORT vtkObjectIdMap : public vtkObject
{
public:
  static vtkObjectIdMap* New();
  vtkTypeMacro(vtkObjectIdMap, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Retrieve a unique identifier for the given object or generate a new one
   * if its global id was never requested.
   */
  vtkTypeUInt32 GetGlobalId(vtkObject* obj);

  /**
   * Retrieve a vtkObject based on its global id. If not found return nullptr
   */
  vtkObject* GetVTKObject(vtkTypeUInt32 globalId);

  /**
   * Assign an active key (string) to an existing object.
   * This is usually used to provide another type of access to specific
   * vtkObject that we want to retrieve easily using a string.
   * Return the global Id of the given registered object
   */
  vtkTypeUInt32 SetActiveObject(const char* objectType, vtkObject* obj);

  /**
   * Retrieve a previously stored object based on a name
   */
  vtkObject* GetActiveObject(const char* objectType);

  /**
   * Given an object, remove any internal reference count due to
   * internal Id/Object mapping.
   * Returns true if the item existed in the map and was deleted.
   */
  bool FreeObject(vtkObject* obj);

  /**
   * Given an id, remove any internal reference count due to
   * internal Id/Object mapping.
   * Returns true if the id existed in the map and was deleted.
   */
  bool FreeObjectById(vtkTypeUInt32 id);

protected:
  vtkObjectIdMap();
  ~vtkObjectIdMap() override;

private:
  vtkObjectIdMap(const vtkObjectIdMap&) = delete;
  void operator=(const vtkObjectIdMap&) = delete;

  struct vtkInternals;
  vtkInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
