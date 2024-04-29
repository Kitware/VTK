// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointCloudFilter
 * @brief   abstract class for filtering a point cloud
 *
 *
 * vtkPointCloudFilter serves as a base for classes that filter point clouds.
 * It takes as input any vtkPointSet (which represents points explicitly
 * using vtkPoints) and produces as output an explicit representation of
 * filtered points via a vtkPolyData. This output vtkPolyData will populate
 * its instance of vtkPoints, and typically no cells will be defined (i.e.,
 * no vtkVertex or vtkPolyVertex are contained in the output unless
 * explicitly requested). Also, after filter execution, the user can request
 * a vtkIdType* point map which indicates how the input points were mapped to
 * the output. A value of PointMap[i] < 0 (where i is the ith input point)
 * means that the ith input point was removed. Otherwise PointMap[i]
 * indicates the position in the output vtkPoints array (point cloud).
 *
 * Optionally the filter may produce a second output. This second output is
 * another vtkPolyData with a vtkPoints that contains the points that were
 * removed during processing. To produce this second output, you must enable
 * GenerateOutliers. If this optional, second output is created, then the
 * contents of the PointMap are modified as well. In this case, a PointMap[i]
 * < 0 means that the ith input point has been mapped to the (-PointMap[i])-1
 * position in the second output's vtkPoints.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * The filter copies point attributes from input to output consistent
 * with the filtering operation.
 *
 * @warning
 * It is convenient to use vtkPointGaussianMapper to render the points (since
 * this mapper does not require cells to be defined, and it is quite fast).
 *
 * @sa
 * vtkRadiusOutlierRemoval vtkPointGaussianMapper vtkThresholdPoints
 */

#ifndef vtkPointCloudFilter_h
#define vtkPointCloudFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPointSet;
class vtkPolyData;

class VTKFILTERSPOINTS_EXPORT vtkPointCloudFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods to obtain type information, and print information.
   */
  vtkTypeMacro(vtkPointCloudFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Retrieve a map which indicates, on a point-by-point basis, where each
   * input point was placed into the output. In other words, map[i] indicates
   * where the ith input point is located in the output array of points. If
   * map[i] < 0, then the ith input point was removed during filter
   * execution.  This method returns valid information only after the filter
   * executes.
   */
  const vtkIdType* GetPointMap();

  /**
   * Return the number of points removed after filter execution. The
   * information returned is valid only after the filter executes.
   */
  vtkIdType GetNumberOfPointsRemoved();

  ///@{
  /**
   * If this method is enabled (true), then a second output will be created
   * that contains the outlier points. By default this is off (false).  Note
   * that if enabled, the PointMap is modified as well: the outlier points
   * are listed as well, with similar meaning, except their value is negated
   * and shifted by -1.
   */
  vtkSetMacro(GenerateOutliers, bool);
  vtkGetMacro(GenerateOutliers, bool);
  vtkBooleanMacro(GenerateOutliers, bool);
  ///@}

  ///@{
  /**
   * If this method is enabled (true), then the outputs will contain a vertex
   * cells (i.e., a vtkPolyVertex for each output). This takes a lot more
   * memory but some VTK filters need cells to function properly. By default
   * this is off (false).
   */
  vtkSetMacro(GenerateVertices, bool);
  vtkGetMacro(GenerateVertices, bool);
  vtkBooleanMacro(GenerateVertices, bool);
  ///@}

protected:
  vtkPointCloudFilter();
  ~vtkPointCloudFilter() override;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned on error.
  virtual int FilterPoints(vtkPointSet* input) = 0;

  // Keep track of which points are removed through the point map
  vtkIdType* PointMap;
  vtkIdType NumberOfPointsRemoved;

  // Does a second output need to be created?
  bool GenerateOutliers;

  // Should output vertex cells be created?
  bool GenerateVertices;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  void GenerateVerticesIfRequested(vtkPolyData* output);

private:
  vtkPointCloudFilter(const vtkPointCloudFilter&) = delete;
  void operator=(const vtkPointCloudFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
