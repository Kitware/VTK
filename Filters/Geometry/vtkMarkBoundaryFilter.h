// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMarkBoundaryFilter
 * @brief   mark points and cells that lie on the boundary of a dataset
 *
 * vtkMarkBoundaryFilter is a general-purpose filter to mark those cells and
 * points on the boundary of a dataset. The output of the filter is identical
 * to the input, with the exception that two, new data arrays are added to
 * the output that indicate which input points, and input cells, are on the
 * boundary. (Cells and points on the boundary are indicated by a value ==1,
 * otherwise ==0, and are accessed in the appropriate data array by cell id
 * or point id. The output data arrays are of type vtkUnsignedChar.)
 *
 * In general, n-dimensional faces are on the boundary of a (n+1)-dataset if
 * used by a single (n+1)-dimensional cell. So for example the boundary cells
 * of a volumetric dataset are those with quad faces used by only one
 * voxel. Boundary points are those points used by the boundary faces. A cell
 * may have multiple boundary faces.
 *
 * An optional array can be produced that encodes the boundary faces of each
 * cell. For each cell cellId that is on the boundary, the nth bit of the
 * value of the faces tuple at location cellId is set.
 *
 * @warning
 * This filter is similar to vtkGeometryFilter in that it identifies boundary
 * cells and points. However, vtkGeometryFilter produces output boundary faces
 * and points as output. vtkMarkBoundaryFilter simply identifies which cells
 * and points are on the boundary. (This filter is often used in parallel
 * computing applications to help distribute data across multiple processes.)
 *
 * @warning
 * The points and cells boundary arrays are of type unsigned char.  The
 * optional boundary faces array is of type of vtkIdType (either a 32-bit or
 * 64-but integral value depending on compilation flags). Hence if the number
 * of cell faces exceeds the number of bits in a face array value, then the
 * faces array cannot properly encode the boundary faces for that cell. This
 * may be a problem for cell types with an arbitrary number of faces such as
 * vtkPolyhedron and vtkPolygon.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkGeometryFilter vtkDataSetSurfaceFilter
 */

#ifndef vtkMarkBoundaryFilter_h
#define vtkMarkBoundaryFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeometryModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRY_EXPORT vtkMarkBoundaryFilter : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkMarkBoundaryFilter* New();
  vtkTypeMacro(vtkMarkBoundaryFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Enable the creation of the boundary faces array. This array encodes
   * which faces are on the boundary of the ith cell. By default, this array
   * is not created. To use this array, bit masking is required to determine
   * if the jth face is on the boundary.
   */
  vtkSetMacro(GenerateBoundaryFaces, bool);
  vtkGetMacro(GenerateBoundaryFaces, bool);
  vtkBooleanMacro(GenerateBoundaryFaces, bool);
  ///@}

  ///@{
  /**
   * Specify the names of the two data arrays which indicate which cells and points
   * are on the boundary. By default, "BoundaryPoints", "BoundaryCells", and
   * "BoundaryFaces" are used.
   */
  vtkSetStringMacro(BoundaryPointsName);
  vtkGetStringMacro(BoundaryPointsName);
  vtkSetStringMacro(BoundaryCellsName);
  vtkGetStringMacro(BoundaryCellsName);
  vtkSetStringMacro(BoundaryFacesName);
  vtkGetStringMacro(BoundaryFacesName);
  ///@}

protected:
  vtkMarkBoundaryFilter();
  ~vtkMarkBoundaryFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool GenerateBoundaryFaces;

  char* BoundaryPointsName;
  char* BoundaryCellsName;
  char* BoundaryFacesName;

private:
  vtkMarkBoundaryFilter(const vtkMarkBoundaryFilter&) = delete;
  void operator=(const vtkMarkBoundaryFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
