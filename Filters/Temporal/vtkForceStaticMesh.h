// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware SAS
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkForceStaticMesh_h
#define vtkForceStaticMesh_h

#include "vtkFiltersTemporalModule.h" // Export macro
#include "vtkPassThrough.h"
#include "vtkSmartPointer.h" // For internal field

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkCompositeDataSet;

/**
 * @class vtkForceStaticMesh
 * @brief Takes in input as a cache the first time it is executed then use it as a static mesh
 *
 * The Force Static Mesh filter create a cache the first time it is used using its input. It will
 * then only update PointData, CellData and FieldData from the input if their dimensions are valid.
 * This filter will keep the initial given geometry as long as its input keeps the same number of
 * points and cells (and ForceCacheComputation is false). This may lead to inconsistent attributes
 * if the geometry has changed its connectivity.
 */
class VTKFILTERSTEMPORAL_EXPORT vtkForceStaticMesh : public vtkPassThrough
{
public:
  static vtkForceStaticMesh* New();
  vtkTypeMacro(vtkForceStaticMesh, vtkPassThrough);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * When set to true, this will force this filter to recompute its cache.
   * Default is false.
   */
  vtkSetMacro(ForceCacheComputation, bool);
  vtkGetMacro(ForceCacheComputation, bool);
  vtkBooleanMacro(ForceCacheComputation, bool);
  //@}

protected:
  vtkForceStaticMesh() = default;
  ~vtkForceStaticMesh() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkForceStaticMesh(const vtkForceStaticMesh&) = delete;
  void operator=(const vtkForceStaticMesh&) = delete;

  // Utility

  //@{
  /**
   * Check if cache is still valid by comparing:
   *  * the number of points
   *  * the number of cells
   * and those of each block in the context of composites
   */
  bool IsValidCache(vtkDataSet* input);
  bool IsValidCache(vtkCompositeDataSet* input);
  //@}

  //@{
  /**
   *  Shallow copy attributes into the cache
   */
  void InputToCache(vtkDataSet* input);
  void InputToCache(vtkCompositeDataSet* input);
  //@}

  // Fields
  bool ForceCacheComputation = false;
  bool CacheInitialized = false;
  vtkSmartPointer<vtkDataObject> Cache;
};

VTK_ABI_NAMESPACE_END
#endif
