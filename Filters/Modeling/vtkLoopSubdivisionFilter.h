// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLoopSubdivisionFilter
 * @brief   generate a subdivision surface using the Loop Scheme
 *
 * vtkLoopSubdivisionFilter is an approximating subdivision scheme that
 * creates four new triangles for each triangle in the mesh. The user can
 * specify the NumberOfSubdivisions. Loop's subdivision scheme is
 * described in: Loop, C., "Smooth Subdivision surfaces based on
 * triangles,", Masters Thesis, University of Utah, August 1987.
 * For a nice summary of the technique see, Hoppe, H., et. al,
 * "Piecewise Smooth Surface Reconstruction,:, Proceedings of Siggraph 94
 * (Orlando, Florida, July 24-29, 1994). In Computer Graphics
 * Proceedings, Annual Conference Series, 1994, ACM SIGGRAPH,
 * pp. 295-302.
 * <P>
 * The filter only operates on triangles. Users should use the
 * vtkTriangleFilter to triangulate meshes that contain polygons or
 * triangle strips.
 * <P>
 * The filter approximates point data using the same scheme. New
 * triangles create at a subdivision step will have the cell data of
 * their parent cell.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * vtkApproximatingSubdivisionFilter
 */

#ifndef vtkLoopSubdivisionFilter_h
#define vtkLoopSubdivisionFilter_h

#include "vtkApproximatingSubdivisionFilter.h"
#include "vtkFiltersModelingModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkIntArray;
class vtkPoints;
class vtkIdList;

class VTKFILTERSMODELING_EXPORT vtkLoopSubdivisionFilter : public vtkApproximatingSubdivisionFilter
{
public:
  ///@{
  /**
   * Construct object with NumberOfSubdivisions set to 1.
   */
  static vtkLoopSubdivisionFilter* New();
  vtkTypeMacro(vtkLoopSubdivisionFilter, vtkApproximatingSubdivisionFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

protected:
  vtkLoopSubdivisionFilter() = default;
  ~vtkLoopSubdivisionFilter() override = default;

  int GenerateSubdivisionPoints(vtkPolyData* inputDS, vtkIntArray* edgeData, vtkPoints* outputPts,
    vtkPointData* outputPD) override;
  int GenerateEvenStencil(vtkIdType p1, vtkPolyData* polys, vtkIdList* stencilIds, double* weights);
  void GenerateOddStencil(
    vtkIdType p1, vtkIdType p2, vtkPolyData* polys, vtkIdList* stencilIds, double* weights);

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkLoopSubdivisionFilter(const vtkLoopSubdivisionFilter&) = delete;
  void operator=(const vtkLoopSubdivisionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
