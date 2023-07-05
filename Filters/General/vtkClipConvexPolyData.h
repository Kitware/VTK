// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkClipConvexPolyData
 * @brief   clip any dataset with user-specified implicit function or input scalar data
 *
 * vtkClipConvexPolyData is a filter that clips a convex polydata with a set
 * of planes. Its main usage is for clipping a bounding volume with frustum
 * planes (used later one in volume rendering).
 */

#ifndef vtkClipConvexPolyData_h
#define vtkClipConvexPolyData_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPlaneCollection;
class vtkPlane;
class vtkClipConvexPolyDataInternals;

class VTKFILTERSGENERAL_EXPORT vtkClipConvexPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkClipConvexPolyData* New();
  vtkTypeMacro(vtkClipConvexPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set all the planes at once using a vtkPlanes implicit function.
   * This also sets the D value.
   */
  void SetPlanes(vtkPlaneCollection* planes);
  vtkGetObjectMacro(Planes, vtkPlaneCollection);
  ///@}

  /**
   * Redefines this method, as this filter depends on time of its components
   * (planes)
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkClipConvexPolyData();
  ~vtkClipConvexPolyData() override;

  // The method that does it all...
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Clip the input with a given plane `p'.
   * tolerance ?
   */
  void ClipWithPlane(vtkPlane* p, double tolerance);

  /**
   * Tells if clipping the input by plane `p' creates some degeneracies.
   */
  bool HasDegeneracies(vtkPlane* p);

  /**
   * Delete calculation data.
   */
  void ClearInternals();

  /**
   * ?
   */
  void ClearNewVertices();

  /**
   * ?
   */
  void RemoveEmptyPolygons();

  vtkPlaneCollection* Planes;
  vtkClipConvexPolyDataInternals* Internal;

private:
  vtkClipConvexPolyData(const vtkClipConvexPolyData&) = delete;
  void operator=(const vtkClipConvexPolyData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
