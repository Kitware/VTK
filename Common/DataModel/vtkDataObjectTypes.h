// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataObject
 * @brief   helper class to get VTK data object types as string and instantiate them
 *
 * vtkDataObjectTypes is a helper class that supports conversion between
 * integer types defined in vtkType.h and string names as well as creation
 * of data objects from either integer or string types. This class has
 * to be updated every time a new data type is added to VTK.
 * @sa
 * vtkDataObject
 */

#ifndef vtkDataObjectTypes_h
#define vtkDataObjectTypes_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;

class VTKCOMMONDATAMODEL_EXPORT vtkDataObjectTypes : public vtkObject
{
public:
  static vtkDataObjectTypes* New();

  vtkTypeMacro(vtkDataObjectTypes, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Given an int (as defined in vtkType.h) identifier for a class
   * return it's classname.
   */
  static const char* GetClassNameFromTypeId(int typeId);

  /**
   * Given a data object classname, return it's int identified (as
   * defined in vtkType.h)
   */
  static int GetTypeIdFromClassName(const char* classname);

  /**
   * Create (New) and return a data object of the given classname.
   */
  static VTK_NEWINSTANCE vtkDataObject* NewDataObject(const char* classname);

  /**
   * Create (New) and return a data object of the given type id.
   */
  static VTK_NEWINSTANCE vtkDataObject* NewDataObject(int typeId);

  /*
   * Returns true if the `typeId` is same or a subclass of
   * `targetTypeId`.
   */
  static bool TypeIdIsA(int typeId, int targetTypeId);

  /**
   * Given two data types, returns the closest common data type.
   * If both data types ids are valid, at worse, this will return
   * `VTK_DATA_OBJECT`. If one of the types is invalid (or unknown),
   * simply returns the valid (or known) type. If both are invalid, returns -1.
   */
  static int GetCommonBaseTypeId(int typeA, int typeB);

protected:
  vtkDataObjectTypes() = default;
  ~vtkDataObjectTypes() override = default;

  /**
   * Method used to validate data object types, for testing purposes
   */
  static int Validate();

private:
  vtkDataObjectTypes(const vtkDataObjectTypes&) = delete;
  void operator=(const vtkDataObjectTypes&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
