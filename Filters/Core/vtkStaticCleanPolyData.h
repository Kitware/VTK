// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticCleanPolyData
 * @brief   merge duplicate points, and/or remove unused points and/or remove degenerate cells
 *
 * vtkStaticCleanPolyData is a filter that takes polygonal data as input and
 * generates polygonal data as output. vtkStaticCleanPolyData will merge
 * duplicate points (within specified tolerance), and if enabled, transform
 * degenerate cells into appropriate forms (for example, a triangle is
 * converted into a line if two points of triangle are merged).
 *
 * Conversion of degenerate cells is controlled by the flags
 * ConvertLinesToPoints, ConvertPolysToLines, ConvertStripsToPolys which act
 * cumulatively such that a degenerate strip may become a poly.
 * The full set is
 * Line with 1 points -> Vert (if ConvertLinesToPoints)
 * Poly with 2 points -> Line (if ConvertPolysToLines)
 * Poly with 1 points -> Vert (if ConvertPolysToLines && ConvertLinesToPoints)
 * Strp with 3 points -> Poly (if ConvertStripsToPolys)
 * Strp with 2 points -> Line (if ConvertStripsToPolys && ConvertPolysToLines)
 * Strp with 1 points -> Vert (if ConvertStripsToPolys && ConvertPolysToLines
 *   && ConvertLinesToPoints)
 *
 * Internally this class uses vtkStaticPointLocator, which is a threaded, and
 * much faster locator (especially for large data) than the incremental
 * locators that vtkCleanPolyData uses. Note because of these and other
 * differences, the output of this filter may be different than
 * vtkCleanPolyData.
 *
 * @warning
 * Merging points can alter topology, including introducing non-manifold
 * forms. The tolerance should be chosen carefully to avoid these problems.
 * Large tolerances (of size > locator bin width) may generate poor results.
 *
 * @warning
 * Unlike vtkCleanPolyData, point merging is always performed (i.e., there
 * is no PointMergingOff()).
 *
 * @warning
 * Unlike vtkCleanPolyData, conversion from one cell type to another is
 * disabled/off. This produces more predictable behavior in many applications.
 *
 * @warning
 * The vtkStaticCleanPolyData filter is similar in operation to
 * vtkCleanPolyData. However, vtkStaticCleanPolyData is non-incremental and
 * uses a much faster (especially for larger datasets) threading approach and
 * when merging points with a non-zero tolerance. However because of the
 * difference in the traversal order in the point merging process, the output
 * of the filters may be different.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkCleanPolyData vtkStaticCleanUnstructuredGrid
 */

#ifndef vtkStaticCleanPolyData_h
#define vtkStaticCleanPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkStaticPointLocator.h" // For enums

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkStaticCleanPolyData : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods to instantiate, print, and provide type information.
   */
  static vtkStaticCleanPolyData* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkStaticCleanPolyData, vtkPolyDataAlgorithm);
  ///@}

  ///@{
  /**
   * By default ToleranceIsAbsolute is false and Tolerance is
   * a fraction of Bounding box diagonal, if true, AbsoluteTolerance is
   * used when adding points to locator (merging)
   */
  vtkSetMacro(ToleranceIsAbsolute, bool);
  vtkBooleanMacro(ToleranceIsAbsolute, bool);
  vtkGetMacro(ToleranceIsAbsolute, bool);
  ///@}

  ///@{
  /**
   * Specify tolerance in terms of fraction of bounding box length.  Default
   * is 0.0. This takes effect only if ToleranceIsAbsolute is false.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Specify tolerance in absolute terms. Default is 1.0.
   */
  vtkSetClampMacro(AbsoluteTolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(AbsoluteTolerance, double);
  ///@}

  ///@{
  /**
   * Specify the name of a point data array associated with the point merging
   * process. If a data array is specified, and exists in the input point
   * data, then point merging will switch into a mode where merged points
   * must be both geometrically coincident and have matching point data
   * (i.e., an exact match of position and data - tolerances have no
   * effect). Note that the number of tuples in the merging data array must
   * be equal to the number of points in the input. By default, no data array
   * is associated with the input points (i.e., the name of the data array is
   * empty).
   */
  vtkSetStringMacro(MergingArray);
  vtkGetStringMacro(MergingArray);
  ///@}

  ///@{
  /**
   * Turn on/off conversion of degenerate lines to points. Default is Off.
   */
  vtkSetMacro(ConvertLinesToPoints, bool);
  vtkBooleanMacro(ConvertLinesToPoints, bool);
  vtkGetMacro(ConvertLinesToPoints, bool);
  ///@}

  ///@{
  /**
   * Turn on/off conversion of degenerate polys to lines. Default is Off.
   */
  vtkSetMacro(ConvertPolysToLines, bool);
  vtkBooleanMacro(ConvertPolysToLines, bool);
  vtkGetMacro(ConvertPolysToLines, bool);
  ///@}

  ///@{
  /**
   * Turn on/off conversion of degenerate strips to polys. Default is Off.
   */
  vtkSetMacro(ConvertStripsToPolys, bool);
  vtkBooleanMacro(ConvertStripsToPolys, bool);
  vtkGetMacro(ConvertStripsToPolys, bool);
  ///@}

  ///@{
  /**
   * Indicate whether points unused by any cell are removed from the output.
   * By default this point removal is on. Note that when this is off, the
   * filter can successfully process datasets with no cells (and just
   * points). If on, and there are no cells, than the output will be empty.
   */
  vtkSetMacro(RemoveUnusedPoints, bool);
  vtkBooleanMacro(RemoveUnusedPoints, bool);
  vtkGetMacro(RemoveUnusedPoints, bool);
  ///@}

  ///@{
  /**
   * Indicate whether a merge map should be produced on output. The merge
   * map, if requested, maps each input point to its output point id, or
   * provides a value of -1 if the input point is not used in the output.
   * The merge map is associated with the filter's output field data and
   * is named "PointMergeMap". By default, ProduceMergeMap is disabled.
   */
  vtkSetMacro(ProduceMergeMap, bool);
  vtkBooleanMacro(ProduceMergeMap, bool);
  vtkGetMacro(ProduceMergeMap, bool);
  ///@}

  ///@{
  /**
   * Indicate whether point coordinates and point data of merged points are
   * averaged. By default, the point coordinates and attribute data are not
   * averaged, and the point coordinate and data of the single, remaining
   * merged point is retained. Otherwise, the data coordinates and attribute
   * values of all merged points are averaged. By default this feature is
   * disabled.
   */
  vtkSetMacro(AveragePointData, bool);
  vtkBooleanMacro(AveragePointData, bool);
  vtkGetMacro(AveragePointData, bool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  /**
   * Retrieve the internal locator to manually configure it, for example
   * specifying the number of points per bucket, or controlling the traversal
   * order. This method is generally used for debugging or testing purposes.
   */
  vtkGetObjectMacro(Locator, vtkStaticPointLocator);

  ///@{
  // This filter is difficult to stream.  To produce invariant results, the
  // whole input must be processed at once.  This flag allows the user to
  // select whether strict piece invariance is required.  By default it is
  // on.  When off, the filter can stream, but results may change.
  vtkSetMacro(PieceInvariant, bool);
  vtkGetMacro(PieceInvariant, bool);
  vtkBooleanMacro(PieceInvariant, bool);
  ///@}

  /**
   * Get the MTime of this object also considering the locator.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkStaticCleanPolyData();
  ~vtkStaticCleanPolyData() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Tolerance;
  double AbsoluteTolerance;
  char* MergingArray;
  bool ConvertLinesToPoints;
  bool ConvertPolysToLines;
  bool ConvertStripsToPolys;
  bool ToleranceIsAbsolute;
  bool RemoveUnusedPoints;
  bool ProduceMergeMap;
  bool AveragePointData;
  int OutputPointsPrecision;
  bool PieceInvariant;

  // Internal locator for performing point merging
  vtkSmartPointer<vtkStaticPointLocator> Locator;

private:
  vtkStaticCleanPolyData(const vtkStaticCleanPolyData&) = delete;
  void operator=(const vtkStaticCleanPolyData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
