// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticCleanUnstructuredGrid
 * @brief   merge duplicate points, removed unused points, in an vtkUnstructuredGrid
 *
 * vtkStaticCleanUnstructuredGrid is a filter that takes a
 * vtkUnstructuredGrid as input and produces a vtkUnstructuredGrid on output,
 * merging coincident points (as defined by a merging tolerance) and
 * optionally removing unused points. The filter does not modify the topology
 * of the input dataset, nor change the types of cells. It may however,
 * renumber the cell connectivity ids.
 *
 * For better performance, this filter employs threading using
 * vtkStaticPointLocator and its associated threaded methods. When the
 * merging tolerances==0.0, execution speeds are much faster as compared to
 * non-zero tolerances. When the merging tolerance > 0.0, there are issues
 * of processing order which can be controlled through the locator
 * (returned via GetLocator()). This behavior can be controlled by the
 * SetTraversalOrder() method - by default threading occurs via BIN_ORDER
 * (see vtkStaticPointLocator for more information).
 *
 * @warning
 * Merging points can alter cell geometry and produce degenerate cells. The
 * tolerance should be chosen carefully to avoid these problems. For example,
 * in an extreme case with a relatively large tolerance, all points of a
 * vtkHexahedron could be merged to a single point, in which case the
 * resulting hexahedron would be defined by eight repeats of the same point.
 *
 * @warning
 * If RemoveUnusedPoints is enabled, then any point not used by any of the
 * unstructured grid cells is eliminated (and not passed to the
 * output). Enabling this feature does impact performance.
 *
 * @warning
 * If ProduceMergeMap is enabled, then an output data array is produced,
 * associated with the output field data, that maps each input point to an
 * output point (or to -1 if an input point is unused in the output).
 *
 * @warning
 * Merging points affects point coordinates and data attributes. By default,
 * if points are merged, the point position and attribute data of only one
 * point (i.e., the point that all other points are merged to) is
 * retained. If AveragePointData is enabled, then the resulting point position
 * and attribute data values are determined by averaging the position and
 * attribute values of all the points that are merged together. This option
 * may have a significant performance impact if enabled.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkCleanPolyData vtkStaticCleanPolyData vtkStaticPointLocator
 */

#ifndef vtkStaticCleanUnstructuredGrid_h
#define vtkStaticCleanUnstructuredGrid_h

#include "vtkFiltersCoreModule.h"  // For export macro
#include "vtkSmartPointer.h"       // Pointer to locator
#include "vtkStaticPointLocator.h" // Locator
#include "vtkUnstructuredGridAlgorithm.h"
#include <vector> // API to vtkStaticCleanUnstructuredGrid

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkCellData;
class vtkPointData;
class vtkPoints;

class VTKFILTERSCORE_EXPORT vtkStaticCleanUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing the state of the object.
   */
  static vtkStaticCleanUnstructuredGrid* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkStaticCleanUnstructuredGrid, vtkUnstructuredGridAlgorithm);
  ///@}

  ///@{
  /**
   * Specify how the point merging tolerance is defined. By default
   * ToleranceIsAbsolute is false and the tolerance is a fraction of the
   * input's bounding box diagonal. If true, AbsoluteTolerance is the
   * tolerance used when performing point merging.
   */
  vtkSetMacro(ToleranceIsAbsolute, bool);
  vtkBooleanMacro(ToleranceIsAbsolute, bool);
  vtkGetMacro(ToleranceIsAbsolute, bool);
  ///@}

  ///@{
  /**
   * Specify the absolute point merging tolerance. The default value is
   * 0. This tolerance is used then ToleranceIsAbsolute is true.
   */
  vtkSetClampMacro(AbsoluteTolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(AbsoluteTolerance, double);
  ///@}

  ///@{
  /**
   * Specify the point merging tolerance in terms of the fraction of the
   * input dataset's bounding box length.  The default is 0. This tolerance
   * is used then ToleranceIsAbsolute is false.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);
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
   * Indicate whether points unused by any cell are removed from the output.
   * By default this point removal is on. Note that when this is off, the
   * filter can successfully process datasets with no cells (and just
   * points). If on in this case, and there are no cells, than the output
   * will be empty.
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
   * Set/get the desired precision for the output point type. See the
   * documentation for the vtkAlgorithm::DesiredOutputPrecision enum for an
   * explanation of the available precision settings.
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

  // These methods are made static so that other filters like vtkStaticCleanPolyData can use them
  static void MarkPointUses(vtkCellArray* ca, vtkIdType* mergeMap, unsigned char* ptUses);
  static vtkIdType BuildPointMap(
    vtkIdType numPts, vtkIdType* pmap, unsigned char* ptUses, std::vector<vtkIdType>& mergeMap);
  static void CopyPoints(
    vtkPoints* inPts, vtkPointData* inPD, vtkPoints* outPts, vtkPointData* outPD, vtkIdType* ptMap);
  static void AveragePoints(vtkPoints* inPts, vtkPointData* inPD, vtkPoints* outPts,
    vtkPointData* outPD, vtkIdType* ptMap, double tol);

protected:
  vtkStaticCleanUnstructuredGrid();
  ~vtkStaticCleanUnstructuredGrid() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool ToleranceIsAbsolute;
  double Tolerance;
  double AbsoluteTolerance;
  char* MergingArray;
  bool RemoveUnusedPoints;
  bool ProduceMergeMap;
  bool AveragePointData;
  int OutputPointsPrecision;
  bool PieceInvariant;

  // Internal locator for performing point merging
  vtkSmartPointer<vtkStaticPointLocator> Locator;

private:
  vtkStaticCleanUnstructuredGrid(const vtkStaticCleanUnstructuredGrid&) = delete;
  void operator=(const vtkStaticCleanUnstructuredGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
