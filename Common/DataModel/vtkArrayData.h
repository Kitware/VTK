// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkArrayData
 * @brief   Pipeline data object that contains multiple vtkArray objects.
 *
 *
 * Because vtkArray cannot be stored as attributes of data objects (yet), a "carrier"
 * object is needed to pass vtkArray through the pipeline.  vtkArrayData acts as a
 * container of zero-to-many vtkArray instances, which can be retrieved via a zero-based
 * index.  Note that a collection of arrays stored in vtkArrayData may-or-may-not have related
 * types, dimensions, or extents.
 *
 * @sa
 * vtkArrayDataAlgorithm, vtkArray
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkArrayData_h
#define vtkArrayData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkArray;

class VTKCOMMONDATAMODEL_EXPORT vtkArrayData : public vtkDataObject
{
public:
  static vtkArrayData* New();
  vtkTypeMacro(vtkArrayData, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkArrayData* GetData(vtkInformation* info);
  static vtkArrayData* GetData(vtkInformationVector* v, int i = 0);

  /**
   * Adds a vtkArray to the collection
   */
  void AddArray(vtkArray*);

  /**
   * Clears the contents of the collection
   */
  void ClearArrays();

  /**
   * Returns the number of vtkArray instances in the collection
   */
  vtkIdType GetNumberOfArrays() VTK_FUTURE_CONST;

  /**
   * Returns the n-th vtkArray in the collection
   */
  vtkArray* GetArray(vtkIdType index);

  /**
   * Returns the array having called name from the collection
   */
  vtkArray* GetArrayByName(const char* name);

  /**
   * Return class name of data type (VTK_ARRAY_DATA).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_ARRAY_DATA; }

  void ShallowCopy(vtkDataObject* other) override;
  void DeepCopy(vtkDataObject* other) override;

protected:
  vtkArrayData();
  ~vtkArrayData() override;

private:
  vtkArrayData(const vtkArrayData&) = delete;
  void operator=(const vtkArrayData&) = delete;

  class implementation;
  implementation* const Implementation;
};

VTK_ABI_NAMESPACE_END
#endif
