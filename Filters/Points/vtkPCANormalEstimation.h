// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPCANormalEstimation
 * @brief   generate point normals using local tangent planes
 *
 *
 * vtkPCANormalEstimation generates point normals using PCA (principal
 * component analysis).  Basically this estimates a local tangent plane
 * around each sample point p by considering a small neighborhood of points
 * around p, and fitting a plane to the neighborhood (via PCA). A good
 * introductory reference is Hoppe's "Surface reconstruction from
 * unorganized points."
 *
 * To use this filter, specify a neighborhood size (SampleSize) or/and a
 * neighborhood radius (Radius). This may have to be set via
 * experimentation. Both options can be set with SetSearchMode.
 * If SearchMode is set to KNN, K points (set by SampleSize) are selected
 * regardless of their location. If Radius is also set to a value different
 * from 0, the code checks if the farthest point found (K-th) is inside this
 * radius. In that case, the selection is performed again to return all
 * points inside this radius, indicating that the initial SampleSize was
 * probably too small compared to the cloud density. Otherwise, if the
 * farthest point is outside the radius, the selection is kept unchanged.
 * If SearchMode is set to Radius, the surrounding points are
 * selected inside the radius. If SampleSize is also set to a value greater
 * than 0, the code checks if at least SampleSize (K) points have been
 * selected. Otherwise, the selection is performed again to include
 * SampleSize (K) points, indicating that the initial Radius was
 * probably too small to estimate the normals relative to the low density
 * of the cloud.
 * Default behavior is KNN with no radius checked (radius is null).
 * Both approaches give the same results. The first approach is faster for
 * uniform point clouds, in other cases, the second approach is faster.
 * In addition, the user may optionally specify a point locator (instead of
 * the default locator), which is used to accelerate searches around the
 * sample point. Finally, the user should specify how to generate
 * consistently-oriented normals. As computed by PCA, normals may point in
 * arbitrary +/- orientation, which may not be consistent with neighboring
 * normals. There are three methods to address normal consistency: 1)
 * leave the normals as computed, 2) adjust the +/- sign of the normals so
 * that the normals all point towards a specified point, and 3) perform a
 * traversal of the point cloud and flip neighboring normals so that they
 * are mutually consistent.
 *
 * The output of this filter is the same as the input except that a normal
 * per point is produced. (Note that these are unit normals.) While any
 * vtkPointSet type can be provided as input, the output is represented by an
 * explicit representation of points via a vtkPolyData. This output polydata
 * will populate its instance of vtkPoints, but no cells will be defined
 * (i.e., no vtkVertex or vtkPolyVertex are contained in the output).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkPCACurvatureEstimation
 */

#ifndef vtkPCANormalEstimation_h
#define vtkPCANormalEstimation_h

#include "vtkConvertToPointCloud.h" // For adding cells to output polydata
#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"       // For vtkSmartPointer
#include "vtkStaticPointLocator.h" // For default locator

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractPointLocator;
class vtkIdList;

class VTKFILTERSPOINTS_EXPORT vtkPCANormalEstimation : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkPCANormalEstimation* New();
  vtkTypeMacro(vtkPCANormalEstimation, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * This enum is used to control how the closest neighbor is calculated
   */
  typedef enum
  {
    KNN = 0,
    RADIUS = 1
  } NeighborSearchMode;

  ///@{
  /**
   * Configure how the filter selects the neighbor points used to calculate
   * the PCA. By default (KNN mode and radius set to 0), K (SampleSize) points
   * are selected regardless of their location relative to the sampled point.
   * The radius can also be set to ensure that a sufficiently large
   * neighborhood is taken into account, if not (i.e. all points fall inside
   * the radius), the second approach is performed. A second approach is to
   * select neighboring points inside a radius (RADIUS), only the neighborhood
   * of the sampled point is considered. If K (SampleSize) is also set, the
   * number of points found inside the radius must be larger than K, if not
   * the first approach is performed.
   * Both approaches give the same results. The first approach is faster for
   * uniform point clouds, in other cases, the second approach is faster.
   */
  vtkSetMacro(SearchMode, int);
  vtkGetMacro(SearchMode, int);
  void SetSearchModeToKNN() { this->SetSearchMode(KNN); }
  void SetSearchModeToRadius() { this->SetSearchMode(RADIUS); }
  ///@}

  ///@{
  /**
   * For each sampled point, specify the number of the closest, surrounding
   * points used to estimate the normal (the so called k-neighborhood). By
   * default 25 points are used. Smaller numbers may speed performance at the
   * cost of accuracy.
   */
  vtkSetClampMacro(SampleSize, int, 1, VTK_INT_MAX);
  vtkGetMacro(SampleSize, int);
  ///@}

  ///@{
  /**
   * For each sampled point, specify the radius within which the surrounding
   * points used to estimate the normal are selected. By default a 1 meter
   * radius is used. Smaller radius may speed performance at the cost of
   * accuracy.
   */
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  ///@}

  /**
   * This enum is used to control how normals oriented is controlled.
   */
  enum Style
  {
    AS_COMPUTED = 0,
    POINT = 1,
    GRAPH_TRAVERSAL = 3
  };

  ///@{
  /**
   * Configure how the filter addresses consistency in normal
   * oreientation. When initially computed using PCA, a point normal may
   * point in the + or - direction, which may not be consistent with
   * neighboring points. To address this, various strategies have been used
   * to create consistent normals. The simplest approach is to do nothing
   * (AsComputed). Another simple approach is to flip the normal based on its
   * direction with respect to a specified point (i.e., point normals will
   * point towrads the specified point). Finally, a full traversal of points
   * across the graph of neighboring, connected points produces the best
   * results but is computationally expensive.
   */
  vtkSetMacro(NormalOrientation, int);
  vtkGetMacro(NormalOrientation, int);
  void SetNormalOrientationToAsComputed() { this->SetNormalOrientation(AS_COMPUTED); }
  void SetNormalOrientationToPoint() { this->SetNormalOrientation(POINT); }
  void SetNormalOrientationToGraphTraversal() { this->SetNormalOrientation(GRAPH_TRAVERSAL); }
  ///@}

  ///@{
  /**
   * If the normal orientation is to be consistent with a specified
   * direction, then an orientation point should be set. The sign of the
   * normals will be modified so that they point towards this point. By
   * default, the specified orientation point is (0,0,0).
   */
  vtkSetVector3Macro(OrientationPoint, double);
  vtkGetVectorMacro(OrientationPoint, double, 3);
  ///@}

  ///@{
  /**
   * The normal orientation can be flipped by enabling this flag.
   */
  vtkSetMacro(FlipNormals, bool);
  vtkGetMacro(FlipNormals, bool);
  vtkBooleanMacro(FlipNormals, bool);
  ///@}

  ///@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(vtkAbstractPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractPointLocator);
  ///@}

  ///@{
  /**
   * Set/Get the cell generation mode.
   * Available modes are:
   * - vtkConvertToPointCloud::NO_CELLS:
   * No cells are generated, set by default
   * - vtkConvertToPointCloud::POLYVERTEX_CELL:
   * A single polyvertex cell is generated
   * - vtkConvertToPointCloud::VERTEX_CELLS:
   * One vertex cell by point, not efficient to generate
   */
  vtkSetMacro(CellGenerationMode, int);
  vtkGetMacro(CellGenerationMode, int);
  ///@}

protected:
  vtkPCANormalEstimation() = default;
  ~vtkPCANormalEstimation() override;

  // IVars
  int SampleSize = 25;
  double Radius = 0.; // Radius is not checked by default (in meter)
  vtkSmartPointer<vtkAbstractPointLocator> Locator = vtkStaticPointLocator::New();
  int SearchMode = vtkPCANormalEstimation::KNN;
  int NormalOrientation = vtkPCANormalEstimation::POINT;
  double OrientationPoint[3] = { 0. };
  bool FlipNormals = false;
  int CellGenerationMode = vtkConvertToPointCloud::NO_CELLS;

  // Methods used to produce consistent normal orientations
  void TraverseAndFlip(
    vtkPoints* inPts, float* normals, char* pointMap, vtkIdList* wave, vtkIdList* wave2);

  // Pipeline management
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPCANormalEstimation(const vtkPCANormalEstimation&) = delete;
  void operator=(const vtkPCANormalEstimation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
