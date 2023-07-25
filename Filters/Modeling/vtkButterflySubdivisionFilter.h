// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkButterflySubdivisionFilter
 * @brief   generate a subdivision surface using the Butterfly Scheme
 *
 * vtkButterflySubdivisionFilter is an interpolating subdivision scheme
 * that creates four new triangles for each triangle in the mesh. The
 * user can specify the NumberOfSubdivisions. This filter implements the
 * 8-point butterfly scheme described in: Zorin, D., Schroder, P., and
 * Sweldens, W., "Interpolating Subdivisions for Meshes with Arbitrary
 * Topology," Computer Graphics Proceedings, Annual Conference Series,
 * 1996, ACM SIGGRAPH, pp.189-192. This scheme improves previous
 * butterfly subdivisions with special treatment of vertices with valence
 * other than 6.
 *
 * Currently, the filter only operates on triangles. Users should use the
 * vtkTriangleFilter to triangulate meshes that contain polygons or
 * triangle strips.
 *
 * The filter interpolates point data using the same scheme. New
 * triangles created at a subdivision step will have the cell data of
 * their parent cell.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * vtkInterpolatingSubdivisionFilter vtkLinearSubdivisionFilter
 */

#ifndef vtkButterflySubdivisionFilter_h
#define vtkButterflySubdivisionFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkInterpolatingSubdivisionFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkIdList;
class vtkIntArray;

class VTKFILTERSMODELING_EXPORT vtkButterflySubdivisionFilter
  : public vtkInterpolatingSubdivisionFilter
{
public:
  ///@{
  /**
   * Construct object with NumberOfSubdivisions set to 1.
   */
  static vtkButterflySubdivisionFilter* New();
  vtkTypeMacro(vtkButterflySubdivisionFilter, vtkInterpolatingSubdivisionFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

protected:
  vtkButterflySubdivisionFilter() = default;
  ~vtkButterflySubdivisionFilter() override = default;

private:
  int GenerateSubdivisionPoints(vtkPolyData* inputDS, vtkIntArray* edgeData, vtkPoints* outputPts,
    vtkPointData* outputPD) override;
  void GenerateButterflyStencil(
    vtkIdType p1, vtkIdType p2, vtkPolyData* polys, vtkIdList* stencilIds, double* weights);
  void GenerateLoopStencil(
    vtkIdType p1, vtkIdType p2, vtkPolyData* polys, vtkIdList* stencilIds, double* weights);
  void GenerateBoundaryStencil(
    vtkIdType p1, vtkIdType p2, vtkPolyData* polys, vtkIdList* stencilIds, double* weights);

  vtkButterflySubdivisionFilter(const vtkButterflySubdivisionFilter&) = delete;
  void operator=(const vtkButterflySubdivisionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
