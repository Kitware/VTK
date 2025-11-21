// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003-2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkMeshQuality
 * @brief   Calculate functions of quality of the elements of a mesh
 *
 * vtkMeshQuality computes one or more functions of (geometric)
 * quality for each 2-D and 3-D cell (triangle, quadrilateral, tetrahedron, pyramid,
 * wedge, or hexahedron) of a mesh. These functions of quality are then averaged
 * over the entire mesh. The minimum, average, maximum, and unbiased variance
 * of quality for each type of cell is stored in the output mesh's FieldData.
 * The FieldData arrays are named "Mesh Triangle Quality,"
 * "Mesh Quadrilateral Quality," "Mesh Tetrahedron Quality," "Mesh Pyramid Quality,"
 * "Mesh Wedge Quality," and "Mesh Hexahedron Quality." Each array has
 * a single tuple with 5 components. The first 4 components are the quality
 * statistics mentioned above; the final value is the number of cells of the
 * given type. This final component makes aggregation of statistics for
 * distributed mesh data possible.
 *
 * By default, the per-cell quality is added to the mesh's cell data, in
 * an array named "Quality." Cell types not supported by
 * this filter will have an entry of 0. Use SaveCellQualityOff() to
 * store only the final statistics.
 *
 * This version of the filter written by Philippe Pebay and David Thompson
 * overtakes an older version written by Leila Baghdadi, Hanif Ladak, and
 * David Steinman at the Imaging Research Labs, Robarts Research Institute.
 * That version only supported tetrahedral radius ratio. See the
 * CompatibilityModeOn() member for information on how to make this filter
 * behave like the previous implementation.
 * For more information on the triangle quality functions of this class, cf.
 * Pebay & Baker 2003, Analysis of triangle quality measures, Math Comp 72:244.
 * For more information on the quadrangle quality functions of this class, cf.
 * Pebay 2004, Planar Quadrangle Quality Measures, Eng Comp 20:2.
 *
 * @warning
 * While more general than before, this class does not address many
 * cell types, including triangle strips and fans in 2D (among others).
 * Most quadrilateral quality functions are intended for planar quadrilaterals
 * only.
 * The minimal angle is not, strictly speaking, a quality function, but it is
 * provided because of its usage by many authors.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef vtkMeshQuality_h
#define vtkMeshQuality_h

#include "vtkDataSetAlgorithm.h"
#include "vtkDeprecation.h"          // For deprecation macro
#include "vtkFiltersVerdictModule.h" // For export macro

#include <functional> // For std::function

VTK_ABI_NAMESPACE_BEGIN
class vtkCell;
class vtkDataArray;
class vtkDoubleArray;

class VTKFILTERSVERDICT_EXPORT vtkMeshQuality : public vtkDataSetAlgorithm
{
private:
  class vtkMeshQualityFunctor;

public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkMeshQuality, vtkDataSetAlgorithm);
  static vtkMeshQuality* New();

  ///@{
  /**
   * This variable controls whether or not cell quality is stored as
   * cell data in the resulting mesh or discarded (leaving only the
   * aggregate quality average of the entire mesh, recorded in the
   * FieldData).
   */
  vtkSetMacro(SaveCellQuality, vtkTypeBool);
  vtkGetMacro(SaveCellQuality, vtkTypeBool);
  vtkBooleanMacro(SaveCellQuality, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If set to true, then this filter will output 2 quality arrays instead of one.
   * The second array is names "Quality (Linear Approx)" and features measure for all non-linear
   * cells in addition to the linear ones, but treated like if they were linear.
   *
   * @note In the array "Quality", any non-linear cell quality is set to NaN.
   */
  vtkSetMacro(LinearApproximation, bool);
  vtkGetMacro(LinearApproximation, bool);
  vtkBooleanMacro(LinearApproximation, bool);
  ///@}

  /**
   * Enum which lists the Quality Measures Types.
   *
   * - AREA: Element area computed via Jacobian.
   * - ASPECT_FROBENIUS: Frobenius condition number of transformation matrix from reference
   *   element (equilateral triangle or regular tetrahedron).
   * - ASPECT_GAMMA: Root-mean-square edge to volume ratio.
   * - ASPECT_RATIO: Ratio of greatest edge length to characteristic radius.
   * - COLLAPSE_RATIO: Ratio of minimum vertex height to opposing triangle's longest edge;
   *   useful to identify sliver tetrahedra.
   * - CONDITION: Condition number of the Jacobian matrix or weighted Jacobian matrix.
   * - DIAGONAL: Minimum diagonal length divided by maximum diagonal length.
   * - DIMENSION: Characteristic length for stable time step calculation.
   * - DISTORTION: Ratio of minimum Jacobian to actual volume, scaled by parent element volume.
   * - EDGE_RATIO: Ratio between the longest and the shortest edge.
   * - EQUIANGLE_SKEW: Angular deviation from ideal equiangle configuration.
   * - EQUIVOLUME_SKEW: Volume-based skewness measure.
   * - INRADIUS: The radius of the largest sphere that fits snugly inside it.
   * - JACOBIAN: Minimum point-wise volume at corners or element center.
   * - MAX_ANGLE: Maximal (nonoriented) angle, expressed in degrees.
   * - MAX_ASPECT_FROBENIUS: Maximum Frobenius aspect of corner triangles or tetrahedra.
   * - MAX_EDGE_RATIO: Maximum edge length ratio at element center.
   * - MAX_STRETCH: Maximum stretch of quadrilateral faces.
   * - MEAN_ASPECT_FROBENIUS: Average aspect Frobenius measure across element components.
   * - MEAN_RATIO: Ratio of element volume to volume of equilateral element with same RMS
   *   edge length.
   * - MED_ASPECT_FROBENIUS: Average/median Frobenius aspect of corner triangles or tetrahedra.
   * - MIN_ANGLE: Minimal (nonoriented) angle, expressed in degrees.
   * - NODAL_JACOBIAN_RATIO: Minimum Jacobian divided by maximum Jacobian over all nodes.
   * - NORMALIZED_INRADIUS: Ratio of minimum sub-element inner radius to outer element radius.
   * - ODDY: General distortion measure based on left Cauchy-Green Tensor.
   * - RADIUS_RATIO: Ratio of circumradius to inradius.
   * - RELATIVE_SIZE_SQUARED: Min(J, 1/J) where J is determinant of weighted Jacobian matrix
   *   (requires average cell size computation).
   * - SCALED_JACOBIAN: Minimum Jacobian divided by the lengths of edge vectors, normalized
   *   appropriately.
   * - SHAPE: Quality based on mean ratio of weighted Jacobian matrix.
   * - SHAPE_AND_SIZE: Product of shape measure and relative average size (requires average
   *   cell size computation).
   * - SHEAR: Measure based on condition number of Jacobian Skew matrix.
   * - SHEAR_AND_SIZE: Product of shear measure and relative average size (requires average
   *   cell size computation).
   * - SKEW: Maximum |cos A| where A is angle between edges at element center, or angular
   *   deviation measure.
   * - SQUISH_INDEX: Measure of element squishing/compression.
   * - STRETCH: Ratio of minimum edge length to maximum diagonal length, scaled by geometric
   *   constants.
   * - TAPER: Ratio representing variation in areas/volumes from opposite edges or faces.
   * - VOLUME: Element volume computed via Jacobian.
   * - WARPAGE: Cosine of minimum dihedral angle formed by planes intersecting in diagonals;
   *   indicates non-planarity.
   */
  enum class QualityMeasureTypes
  {
    AREA = 28,
    ASPECT_FROBENIUS = 3,
    ASPECT_GAMMA = 27,
    ASPECT_RATIO = 1,
    COLLAPSE_RATIO = 7,
    CONDITION = 9,
    DIAGONAL = 21,
    DIMENSION = 22,
    DISTORTION = 15,
    EDGE_RATIO = 0,
    EQUIANGLE_SKEW = 29,
    EQUIVOLUME_SKEW = 30,
    JACOBIAN = 25,
    INRADIUS = 37,
    MAX_ANGLE = 8,
    MAX_ASPECT_FROBENIUS = 5,
    MAX_EDGE_RATIO = 16,
    MAX_STRETCH = 31,
    MEAN_ASPECT_FROBENIUS = 32,
    MEAN_RATIO = 33,
    MED_ASPECT_FROBENIUS = 4,
    MIN_ANGLE = 6,
    NODAL_JACOBIAN_RATIO = 34,
    NORMALIZED_INRADIUS = 35,
    ODDY = 23,
    RADIUS_RATIO = 2,
    RELATIVE_SIZE_SQUARED = 12,
    SCALED_JACOBIAN = 10,
    SHAPE = 13,
    SHAPE_AND_SIZE = 14,
    SHEAR = 11,
    SHEAR_AND_SIZE = 24,
    SKEW = 17,
    SQUISH_INDEX = 36,
    STRETCH = 20,
    TAPER = 18,
    VOLUME = 19,
    WARPAGE = 26,
    TOTAL_QUALITY_MEASURE_TYPES = 38,
    NONE = TOTAL_QUALITY_MEASURE_TYPES
  };

  /**
   * Array which lists the Quality Measures Names.
   */
  static const char* QualityMeasureNames[];

  ///@{
  /**
   * Set/Get the particular estimator used to function the quality of triangles.
   * The default is RADIUS_RATIO and valid values also include
   * ASPECT_RATIO, ASPECT_FROBENIUS, and EDGE_RATIO, MIN_ANGLE, MAX_ANGLE, CONDITION,
   * SCALED_JACOBIAN, RELATIVE_SIZE_SQUARED, SHAPE, SHAPE_AND_SIZE, DISTORTION,
   * EQUIANGLE_SKEW, and NORMALIZED_INRADIUS.
   */
  vtkSetEnumMacro(TriangleQualityMeasure, QualityMeasureTypes);
  virtual void SetTriangleQualityMeasure(int measure)
  {
    this->SetTriangleQualityMeasure(static_cast<QualityMeasureTypes>(measure));
  }
  vtkGetEnumMacro(TriangleQualityMeasure, QualityMeasureTypes);
  void SetTriangleQualityMeasureToArea()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::AREA);
  }
  void SetTriangleQualityMeasureToEdgeRatio()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::EDGE_RATIO);
  }
  void SetTriangleQualityMeasureToAspectRatio()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::ASPECT_RATIO);
  }
  void SetTriangleQualityMeasureToRadiusRatio()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::RADIUS_RATIO);
  }
  void SetTriangleQualityMeasureToAspectFrobenius()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::ASPECT_FROBENIUS);
  }
  void SetTriangleQualityMeasureToMinAngle()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::MIN_ANGLE);
  }
  void SetTriangleQualityMeasureToMaxAngle()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::MAX_ANGLE);
  }
  void SetTriangleQualityMeasureToCondition()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::CONDITION);
  }
  void SetTriangleQualityMeasureToScaledJacobian()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::SCALED_JACOBIAN);
  }
  void SetTriangleQualityMeasureToRelativeSizeSquared()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::RELATIVE_SIZE_SQUARED);
  }
  void SetTriangleQualityMeasureToShape()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::SHAPE);
  }
  void SetTriangleQualityMeasureToShapeAndSize()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::SHAPE_AND_SIZE);
  }
  void SetTriangleQualityMeasureToDistortion()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::DISTORTION);
  }
  void SetTriangleQualityMeasureToEquiangleSkew()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::EQUIANGLE_SKEW);
  }
  void SetTriangleQualityMeasureToNormalizedInradius()
  {
    this->SetTriangleQualityMeasure(QualityMeasureTypes::NORMALIZED_INRADIUS);
  }
  ///@}

  ///@{
  /**
   * Set/Get the particular estimator used to measure the quality of quadrilaterals.
   * The default is EDGE_RATIO and valid values also include
   * RADIUS_RATIO, ASPECT_RATIO, MAX_EDGE_RATIO SKEW, TAPER, WARPAGE, AREA,
   * STRETCH, MIN_ANGLE, MAX_ANGLE, ODDY, CONDITION, JACOBIAN, SCALED_JACOBIAN,
   * SHEAR, SHAPE, RELATIVE_SIZE_SQUARED, SHAPE_AND_SIZE, SHEAR_AND_SIZE, DISTORTION,
   * and EQUIANGLE_SKEW.
   *
   * Scope: Except for EDGE_RATIO, these estimators are intended for planar
   * quadrilaterals only; use at your own risk if you really want to assess non-planar
   * quadrilateral quality with those.
   */
  vtkSetEnumMacro(QuadQualityMeasure, QualityMeasureTypes);
  virtual void SetQuadQualityMeasure(int measure)
  {
    this->SetQuadQualityMeasure(static_cast<QualityMeasureTypes>(measure));
  }
  vtkGetEnumMacro(QuadQualityMeasure, QualityMeasureTypes);
  void SetQuadQualityMeasureToEdgeRatio()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::EDGE_RATIO);
  }
  void SetQuadQualityMeasureToAspectRatio()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::ASPECT_RATIO);
  }
  void SetQuadQualityMeasureToRadiusRatio()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::RADIUS_RATIO);
  }
  void SetQuadQualityMeasureToMedAspectFrobenius()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::MED_ASPECT_FROBENIUS);
  }
  void SetQuadQualityMeasureToMaxAspectFrobenius()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::MAX_ASPECT_FROBENIUS);
  }
  void SetQuadQualityMeasureToMaxEdgeRatio()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::MAX_EDGE_RATIO);
  }
  void SetQuadQualityMeasureToSkew() { this->SetQuadQualityMeasure(QualityMeasureTypes::SKEW); }
  void SetQuadQualityMeasureToTaper() { this->SetQuadQualityMeasure(QualityMeasureTypes::TAPER); }
  void SetQuadQualityMeasureToWarpage()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::WARPAGE);
  }
  void SetQuadQualityMeasureToArea() { this->SetQuadQualityMeasure(QualityMeasureTypes::AREA); }
  void SetQuadQualityMeasureToStretch()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::STRETCH);
  }
  void SetQuadQualityMeasureToMinAngle()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::MIN_ANGLE);
  }
  void SetQuadQualityMeasureToMaxAngle()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::MAX_ANGLE);
  }
  void SetQuadQualityMeasureToOddy() { this->SetQuadQualityMeasure(QualityMeasureTypes::ODDY); }
  void SetQuadQualityMeasureToCondition()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::CONDITION);
  }
  void SetQuadQualityMeasureToJacobian()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::JACOBIAN);
  }
  void SetQuadQualityMeasureToScaledJacobian()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::SCALED_JACOBIAN);
  }
  void SetQuadQualityMeasureToShear() { this->SetQuadQualityMeasure(QualityMeasureTypes::SHEAR); }
  void SetQuadQualityMeasureToShape() { this->SetQuadQualityMeasure(QualityMeasureTypes::SHAPE); }
  void SetQuadQualityMeasureToRelativeSizeSquared()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::RELATIVE_SIZE_SQUARED);
  }
  void SetQuadQualityMeasureToShapeAndSize()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::SHAPE_AND_SIZE);
  }
  void SetQuadQualityMeasureToShearAndSize()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::SHEAR_AND_SIZE);
  }
  void SetQuadQualityMeasureToDistortion()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::DISTORTION);
  }
  void SetQuadQualityMeasureToEquiangleSkew()
  {
    this->SetQuadQualityMeasure(QualityMeasureTypes::EQUIANGLE_SKEW);
  }
  ///@}

  ///@{
  /**
   * Set/Get the particular estimator used to measure the quality of tetrahedra.
   * The default is RADIUS_RATIO and valid values also include
   * ASPECT_RATIO, ASPECT_FROBENIUS, EDGE_RATIO, COLLAPSE_RATIO, ASPECT_GAMMA, VOLUME,
   * CONDITION, JACOBIAN, SCALED_JACOBIAN, SHAPE, RELATIVE_SIZE_SQUARED, SHAPE_AND_SIZE,
   * DISTORTION, EQUIANGLE_SKEW, EQUIVOLUME_SKEW, MEAN_RATIO, NORMALIZED_INRADIUS, and SQUISH_INDEX.
   */
  vtkSetEnumMacro(TetQualityMeasure, QualityMeasureTypes);
  virtual void SetTetQualityMeasure(int measure)
  {
    this->SetTetQualityMeasure(static_cast<QualityMeasureTypes>(measure));
  }
  vtkGetEnumMacro(TetQualityMeasure, QualityMeasureTypes);
  void SetTetQualityMeasureToEdgeRatio()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::EDGE_RATIO);
  }
  void SetTetQualityMeasureToAspectRatio()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::ASPECT_RATIO);
  }
  void SetTetQualityMeasureToRadiusRatio()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::RADIUS_RATIO);
  }
  void SetTetQualityMeasureToAspectFrobenius()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::ASPECT_FROBENIUS);
  }
  void SetTetQualityMeasureToInradius()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::INRADIUS);
  }
  void SetTetQualityMeasureToMinAngle()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::MIN_ANGLE);
  }
  void SetTetQualityMeasureToCollapseRatio()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::COLLAPSE_RATIO);
  }
  void SetTetQualityMeasureToAspectGamma()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::ASPECT_GAMMA);
  }
  void SetTetQualityMeasureToVolume() { this->SetTetQualityMeasure(QualityMeasureTypes::VOLUME); }
  void SetTetQualityMeasureToCondition()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::CONDITION);
  }
  void SetTetQualityMeasureToJacobian()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::JACOBIAN);
  }
  void SetTetQualityMeasureToScaledJacobian()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::SCALED_JACOBIAN);
  }
  void SetTetQualityMeasureToShape() { this->SetTetQualityMeasure(QualityMeasureTypes::SHAPE); }
  void SetTetQualityMeasureToRelativeSizeSquared()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::RELATIVE_SIZE_SQUARED);
  }
  void SetTetQualityMeasureToShapeAndSize()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::SHAPE_AND_SIZE);
  }
  void SetTetQualityMeasureToDistortion()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::DISTORTION);
  }
  void SetTetQualityMeasureToEquiangleSkew()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::EQUIANGLE_SKEW);
  }
  void SetTetQualityMeasureToEquivolumeSkew()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::EQUIVOLUME_SKEW);
  }
  void SetTetQualityMeasureToMeanRatio()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::MEAN_RATIO);
  }
  void SetTetQualityMeasureToNormalizedInradius()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::NORMALIZED_INRADIUS);
  }
  void SetTetQualityMeasureToSquishIndex()
  {
    this->SetTetQualityMeasure(QualityMeasureTypes::SQUISH_INDEX);
  }
  ///@}

  ///@{
  /**
   * Set/Get the particular estimator used to measure the quality of pyramids.
   * The default is SHAPE and valid values also include
   * EQUIANGLE_SKEW, JACOBIAN, SCALED_JACOBIAN, and VOLUME.
   */
  vtkSetEnumMacro(PyramidQualityMeasure, QualityMeasureTypes);
  virtual void SetPyramidQualityMeasure(int measure)
  {
    this->SetPyramidQualityMeasure(static_cast<QualityMeasureTypes>(measure));
  }
  vtkGetEnumMacro(PyramidQualityMeasure, QualityMeasureTypes);
  void SetPyramidQualityMeasureToEquiangleSkew()
  {
    this->SetPyramidQualityMeasure(QualityMeasureTypes::EQUIANGLE_SKEW);
  }
  void SetPyramidQualityMeasureToJacobian()
  {
    this->SetPyramidQualityMeasure(QualityMeasureTypes::JACOBIAN);
  }
  void SetPyramidQualityMeasureToScaledJacobian()
  {
    this->SetPyramidQualityMeasure(QualityMeasureTypes::SCALED_JACOBIAN);
  }
  void SetPyramidQualityMeasureToShape()
  {
    this->SetPyramidQualityMeasure(QualityMeasureTypes::SHAPE);
  }
  void SetPyramidQualityMeasureToVolume()
  {
    this->SetPyramidQualityMeasure(QualityMeasureTypes::VOLUME);
  }
  ///@}

  ///@{
  /**
   * Set/Get the particular estimator used to measure the quality of wedges.
   * The default is EDGE_RATIO and valid values also include
   * CONDITION, DISTORTION, EQUIANGLE_SKEW, JACOBIAN, MAX_ASPECT_FROBENIUS, MAX_STRETCH,
   * MEAN_ASPECT_FROBENIUS, SCALED_JACOBIAN, SHAPE, and VOLUME.
   */
  vtkSetEnumMacro(WedgeQualityMeasure, QualityMeasureTypes);
  virtual void SetWedgeQualityMeasure(int measure)
  {
    this->SetWedgeQualityMeasure(static_cast<QualityMeasureTypes>(measure));
  }
  vtkGetEnumMacro(WedgeQualityMeasure, QualityMeasureTypes);
  void SetWedgeQualityMeasureToCondition()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::CONDITION);
  }
  void SetWedgeQualityMeasureToDistortion()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::DISTORTION);
  }
  void SetWedgeQualityMeasureToEdgeRatio()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::EDGE_RATIO);
  }
  void SetWedgeQualityMeasureToEquiangleSkew()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::EQUIANGLE_SKEW);
  }
  void SetWedgeQualityMeasureToJacobian()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::JACOBIAN);
  }
  void SetWedgeQualityMeasureToMaxAspectFrobenius()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::MAX_ASPECT_FROBENIUS);
  }
  void SetWedgeQualityMeasureToMaxStretch()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::MAX_STRETCH);
  }
  void SetWedgeQualityMeasureToMeanAspectFrobenius()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::MEAN_ASPECT_FROBENIUS);
  }
  void SetWedgeQualityMeasureToScaledJacobian()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::SCALED_JACOBIAN);
  }
  void SetWedgeQualityMeasureToShape() { this->SetWedgeQualityMeasure(QualityMeasureTypes::SHAPE); }
  void SetWedgeQualityMeasureToVolume()
  {
    this->SetWedgeQualityMeasure(QualityMeasureTypes::VOLUME);
  }
  ///@}

  ///@{
  /**
   * Set/Get the particular estimator used to measure the quality of hexahedra.
   * The default is MAX_ASPECT_FROBENIUS and valid values also include
   * EDGE_RATIO, MAX_ASPECT_FROBENIUS, MAX_EDGE_RATIO, SKEW, TAPER, VOLUME,
   * STRETCH, DIAGONAL, DIMENSION, ODDY, CONDITION, JACOBIAN,
   * SCALED_JACOBIAN, SHEAR, SHAPE, RELATIVE_SIZE_SQUARED, SHAPE_AND_SIZE,
   * SHEAR_AND_SIZE, DISTORTION, EQUIANGLE_SKEW, and NODAL_JACOBIAN_RATIO.
   */
  vtkSetEnumMacro(HexQualityMeasure, QualityMeasureTypes);
  virtual void SetHexQualityMeasure(int measure)
  {
    this->SetHexQualityMeasure(static_cast<QualityMeasureTypes>(measure));
  }
  vtkGetEnumMacro(HexQualityMeasure, QualityMeasureTypes);
  void SetHexQualityMeasureToEdgeRatio()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::EDGE_RATIO);
  }
  void SetHexQualityMeasureToMedAspectFrobenius()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::MED_ASPECT_FROBENIUS);
  }
  void SetHexQualityMeasureToMaxAspectFrobenius()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::MAX_ASPECT_FROBENIUS);
  }
  void SetHexQualityMeasureToMaxEdgeRatio()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::MAX_EDGE_RATIO);
  }
  void SetHexQualityMeasureToSkew() { this->SetHexQualityMeasure(QualityMeasureTypes::SKEW); }
  void SetHexQualityMeasureToTaper() { this->SetHexQualityMeasure(QualityMeasureTypes::TAPER); }
  void SetHexQualityMeasureToVolume() { this->SetHexQualityMeasure(QualityMeasureTypes::VOLUME); }
  void SetHexQualityMeasureToStretch() { this->SetHexQualityMeasure(QualityMeasureTypes::STRETCH); }
  void SetHexQualityMeasureToDiagonal()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::DIAGONAL);
  }
  void SetHexQualityMeasureToDimension()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::DIMENSION);
  }
  void SetHexQualityMeasureToOddy() { this->SetHexQualityMeasure(QualityMeasureTypes::ODDY); }
  void SetHexQualityMeasureToCondition()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::CONDITION);
  }
  void SetHexQualityMeasureToJacobian()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::JACOBIAN);
  }
  void SetHexQualityMeasureToScaledJacobian()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::SCALED_JACOBIAN);
  }
  void SetHexQualityMeasureToShear() { this->SetHexQualityMeasure(QualityMeasureTypes::SHEAR); }
  void SetHexQualityMeasureToShape() { this->SetHexQualityMeasure(QualityMeasureTypes::SHAPE); }
  void SetHexQualityMeasureToRelativeSizeSquared()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::RELATIVE_SIZE_SQUARED);
  }
  void SetHexQualityMeasureToShapeAndSize()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::SHAPE_AND_SIZE);
  }
  void SetHexQualityMeasureToShearAndSize()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::SHEAR_AND_SIZE);
  }
  void SetHexQualityMeasureToDistortion()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::DISTORTION);
  }
  void SetHexQualityMeasureToEquiangleSkew()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::EQUIANGLE_SKEW);
  }
  void SetHexQualityMeasureToNodalJacobianRatio()
  {
    this->SetHexQualityMeasure(QualityMeasureTypes::NODAL_JACOBIAN_RATIO);
  }
  ///@}

  /**
   * Calculate the area of a triangle.
   *
   * @note Supports only vtkTriangle, vtkQuadraticTriangle, and vtkBiQuadraticTriangle.
   */
  static double TriangleArea(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Frobenius condition number of the transformation matrix from an equilateral
   * triangle to a triangle.
   * The Frobenius aspect of a triangle \f$t\f$, when the reference element is
   * equilateral, is:
   * \f$\frac{|t|^2_2}{2\sqrt{3}{\cal A}}\f$,
   * where \f$|t|^2_2\f$ and \f$\cal A\f$ respectively denote the sum of the
   * squared edge lengths and the area of \f$t\f$.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the aspect ratio of a triangle.
   * The aspect ratio of a triangle \f$t\f$ is:
   * \f$\frac{|t|_\infty}{2\sqrt{3}r}\f$,
   * where \f$|t|_\infty\f$ and \f$r\f$ respectively denote the greatest edge
   * length and the inradius of \f$t\f$.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleAspectRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the condition number of a triangle.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleCondition(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the distortion of a triangle.
   *
   * @note Supports only vtkTriangle, vtkQuadraticTriangle, and vtkBiQuadraticTriangle.
   */
  static double TriangleDistortion(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the edge ratio of a triangle.
   * The edge ratio of a triangle \f$t\f$ is:
   * \f$\frac{|t|_\infty}{|t|_0}\f$,
   * where \f$|t|_\infty\f$ and \f$|t|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$t\f$.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleEdgeRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the equiangle skew of a triangle.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleEquiangleSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the maximal (nonoriented) angle of a triangle, expressed in degrees.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleMaxAngle(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the minimal (nonoriented) angle of a triangle, expressed in degrees.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleMinAngle(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the normalized in-radius of a triangle.
   * The minimum normalized in-radius of a triangle is the ratio of the minimum
   * sub-triangle inner radius to the outer triangle radius.
   *
   * @note Supports only vtkTriangle and vtkQuadraticTriangle.
   */
  static double TriangleNormalizedInradius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the radius ratio of a triangle.
   * The radius ratio of a triangle \f$t\f$ is:
   * \f$\frac{R}{2r}\f$,
   * where \f$R\f$ and \f$r\f$ respectively denote the circumradius and
   * the inradius of \f$t\f$.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleRadiusRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the square of the relative size of a triangle.
   *
   * Note: TriangleRelativeSizeSquared will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average triangle size.
   *
   * @note Supports only vtkTriangle.
   * @pre Requires computing triangle average size first.
   */
  static double TriangleRelativeSizeSquared(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the scaled Jacobian of a triangle.
   *
   * @note Supports only vtkTriangle, vtkQuadraticTriangle.
   */
  static double TriangleScaledJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape of a triangle.
   *
   * @note Supports only vtkTriangle.
   */
  static double TriangleShape(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the product of shape and relative size of a triangle.
   *
   * Note: TriangleShapeAndSize will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average triangle size.
   *
   * @note Supports only vtkTriangle.
   * @pre Requires computing triangle average size first.
   */
  static double TriangleShapeAndSize(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the area of a quadrilateral.
   * The area of a quadrilateral is the Jacobian at quad center.
   *
   * @note Supports only vtkQuad, vtkQuadraticQuad, and vtkBiQuadraticQuad.
   */
  static double QuadArea(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the aspect ratio of a planar quadrilateral.
   * The aspect ratio of a planar quadrilateral \f$q\f$ is:
   * \f$\frac{|q|_1|q|_\infty}{4{\cal A}}\f$,
   * where \f$|q|_1\f$, \f$|q|_\infty\f$ and \f${\cal A}\f$ respectively denote the
   * perimeter, the greatest edge length and the area of \f$q\f$.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadAspectRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the condition number of a quadrilateral.
   * The condition number of a quadrilateral is the (maximum) condition number of the
   * Jacobian matrix at the 4 corners.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadCondition(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the distortion of a quadrilateral.
   * The distortion of a quadrilateral is {min(|J|)/actual area} * parent area,
   * parent area = 4 for quad.
   *
   * @note Supports only vtkQuad and vtkQuadraticQuad.
   */
  static double QuadDistortion(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the edge ratio of a quadrilateral.
   * The edge ratio of a quadrilateral \f$q\f$ is:
   * \f$\frac{|q|_\infty}{|q|_0}\f$,
   * where \f$|q|_\infty\f$ and \f$|q|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$q\f$.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadEdgeRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the equiangle skew of a quadrilateral.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadEquiangleSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Jacobian of a quadrilateral.
   * The Jacobian of a quadrilateral is the minimum point-wise volume of local map
   * at 4 corners & center of quad.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the maximum (nonoriented) angle of a quadrilateral, expressed in degrees.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadMaxAngle(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the maximal Frobenius aspect of the 4 corner triangles of a planar quadrilateral,
   * when the reference triangle elements are right isosceles at the quadrangle vertices.
   * The Frobenius aspect of a triangle \f$t\f$, when the reference element is
   * right isosceles at vertex \f$V\f$, is:
   * \f$\frac{f^2+g^2}{4{\cal A}}\f$,
   * where \f$f^2+g^2\f$ and \f$\cal A\f$ respectively denote the sum of the
   * squared lengths of the edges attached to \f$V\f$ and the area of \f$t\f$.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadMaxAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the maximum edge length ratio of a quadrilateral at quad center.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadMaxEdgeRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the average Frobenius aspect of the 4 corner triangles of a planar quadrilateral,
   * when the reference triangle elements are right isosceles at the quadrangle vertices.
   * The Frobenius aspect of a triangle \f$t\f$, when the reference element is
   * right isosceles at vertex \f$V\f$, is:
   * \f$\frac{f^2+g^2}{4{\cal A}}\f$,
   * where \f$f^2+g^2\f$ and \f$\cal A\f$ respectively denote the sum of the
   * squared lengths of the edges attached to \f$V\f$ and the area of \f$t\f$.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadMedAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the minimal (nonoriented) angle of a quadrilateral, expressed in degrees.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadMinAngle(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the oddy of a quadrilateral.
   * The oddy of a quadrilateral is the general distortion measure based on left
   * Cauchy-Green Tensor.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadOddy(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the radius ratio of a planar quadrilateral. The name is only used by analogy
   * with the triangle radius ratio, because in general a quadrilateral does not have a
   * circumcircle nor an incircle.
   * The radius ratio of a planar quadrilateral \f$q\f$ is:
   * \f$\frac{|q|_2h_{\max}}{\min_i{\cal A}_i}\f$,
   * where \f$|q|_2\f$, \f$h_{\max}\f$ and \f$\min{\cal A}_i\f$ respectively denote
   * the sum of the squared edge lengths, the greatest amongst diagonal and edge
   * lengths and the smallest area of the 4 triangles extractable from \f$q\f$.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadRadiusRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the relative size squared of a quadrilateral.
   * The relative size squared of a quadrilateral is the Min(J, 1 / J), where J is the
   * determinant of weighted Jacobian matrix.
   *
   * Note: QuadRelativeSizeSquared will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average quad size.
   *
   * @note Supports only vtkQuad.
   * @pre Requires computing quad average size first.
   */
  static double QuadRelativeSizeSquared(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the scaled Jacobian of a quadrilateral.
   * The scaled Jacobian of a quadrilateral is the minimum Jacobian divided by the lengths
   * of the 2 edge vectors.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadScaledJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shear of a quadrilateral.
   * The shear of a quadrilateral is 2 / Condition number of weighted Jacobian matrix.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadShape(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape and size of a quadrilateral.
   * The shape and size of a quadrilateral is product of shape and average size.
   *
   * Note: QuadShapeAndSize will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average triangle size.
   *
   * @note Supports only vtkQuad.
   * @pre Requires computing quad average size first.
   */
  static double QuadShapeAndSize(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shear of a quadrilateral.
   * The shear of a quadrilateral is 2 / Condition number of Jacobian Skew matrix.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadShear(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shear and size of a quadrilateral.
   * The shear and size of a quadrilateral is product of shear and average size.
   *
   * Note: QuadShearAndSize will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average triangle size.
   *
   * @note Supports only vtkQuad.
   * @pre Requires computing quad average size first.
   */
  static double QuadShearAndSize(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the skew of a quadrilateral.
   * The skew of a quadrilateral is the maximum |cos A|, where A is the angle
   * between edges at the quad center.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the stretch of a quadrilateral.
   * The stretch of a quadrilateral is Sqrt(2) * minimum edge length / maximum diagonal length.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadStretch(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the taper of a quadrilateral.
   * The taper of a quadrilateral is the ratio of lengths derived from opposite edges.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadTaper(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the warpage of a quadrilateral.
   * The warpage of a quadrilateral is the cosine of Minimum Dihedral Angle formed by
   * Planes Intersecting in Diagonals.
   *
   * @note Supports only vtkQuad.
   */
  static double QuadWarpage(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Frobenius condition number of the transformation matrix from a regular
   * tetrahedron to a tetrahedron.
   * The Frobenius aspect of a tetrahedron \f$K\f$, when the reference element is
   * regular, is:
   * \f$\frac{\frac{3}{2}(l_{11}+l_{22}+l_{33}) - (l_{12}+l_{13}+l_{23})}
   * {3(\sqrt{2}\det{T})^\frac{2}{3}}\f$,
   * where \f$T\f$ and \f$l_{ij}\f$ respectively denote the edge matrix of \f$K\f$
   * and the entries of \f$L=T^t\,T\f$.
   *
   * @note Supports only vtkTetra.
   */
  static double TetAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the aspect gamma of a tetrahedron.
   * The aspect gamma of a tetrahedron is:
   * Srms**3 / (8.479670*V) where Srms = sqrt(Sum(Si**2)/6), Si = edge length.
   *
   * @note Supports only vtkTetra.
   */
  static double TetAspectGamma(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the aspect ratio of a tetrahedron.
   * The aspect ratio of a tetrahedron \f$K\f$ is:
   * \f$\frac{|K|_\infty}{2\sqrt{6}r}\f$,
   * where \f$|K|_\infty\f$ and \f$r\f$ respectively denote the greatest edge
   * length and the inradius of \f$K\f$.
   *
   * @note Supports only vtkTetra.
   */
  static double TetAspectRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the collapse ratio of a tetrahedron.
   * The collapse ratio is a dimensionless number defined as the smallest ratio of the
   * height of a vertex above its opposing triangle to the longest edge of that opposing
   * triangle across all vertices of the tetrahedron.
   *
   * @note Supports only vtkTetra.
   */
  static double TetCollapseRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the condition number of a tetrahedron.
   * The condition number of a tetrahedron is Condition number of the Jacobian matrix at any corner.
   *
   * @note Supports only vtkTetra.
   */
  static double TetCondition(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the distortion of a tetrahedron.
   * The distortion of a quadrilateral is {min(|J|)/actual volume} * parent volume,
   * parent volume = 1 / 6 for a tetrahedron.
   *
   * @note Supports only vtkTetra and vtkQuadraticTetra.
   */
  static double TetDistortion(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the edge ratio of a tetrahedron.
   * The edge ratio of a tetrahedron \f$K\f$ is:
   * \f$\frac{|K|_\infty}{|K|_0}\f$,
   * where \f$|K|_\infty\f$ and \f$|K|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$K\f$.
   *
   * @note Supports only vtkTetra.
   */
  static double TetEdgeRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the equiangle skew of a tetrahedron.
   *
   * @note Supports only vtkTetra.
   */
  static double TetEquiangleSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the equivolume skew of a tetrahedron.
   *
   * @note Supports only vtkTetra and vtkQuadraticTetra.
   */
  static double TetEquivolumeSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the inradius of a tetrahedron.
   * The inradius of a tetrahedron is the radius of the largest sphere that fits snugly inside it.
   *
   * @note Supports only vtkTetra, vtkQuadraticTetra.
   */
  static double TetInradius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Jacobian of a tetrahedron.
   * The jacobian of a tetrahedron is the minimum point-wise volume at any corner.
   *
   * @note Supports only vtkTetra, vtkQuadraticTetra.
   */
  static double TetJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the mean ratio of a tetrahedron.
   * The mean ratio of a tetrahedron is the ratio of tetrahedron volume over the volume of an
   * equilateral tetrahedron with the same RMS edge length.
   *
   * @note Supports only vtkTetra and vtkQuadraticTetra.
   */
  static double TetMeanRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the minimal (nonoriented) dihedral angle of a tetrahedron, expressed in degrees.
   *
   * @note Supports only vtkTetra.
   */
  static double TetMinAngle(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the normalized in-radius of a tetrahedron.
   * The minimum normalized in-radius of a tetrahedron is the ratio of the minimum
   * sub-tetrahedron inner radius to the outer tetrahedron radius.
   *
   * @note Supports only vtkTetra and vtkQuadraticTetra.
   */
  static double TetNormalizedInradius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the radius ratio of a tetrahedron.
   * The radius ratio of a tetrahedron \f$K\f$ is:
   * \f$\frac{R}{3r}\f$,
   * where \f$R\f$ and \f$r\f$ respectively denote the circumradius and
   * the inradius of \f$K\f$.
   *
   * @note Supports only vtkTetra.
   */
  static double TetRadiusRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the relative size squared of a tetrahedron.
   * The relative size squared of a tetrahedron is Min(J, 1 / J), where J is determinant
   * of weighted Jacobian matrix.
   *
   * Note: TetRelativeSizeSquared will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average tet size.
   *
   * @note Supports only vtkTetra.
   * @pre Requires computing tet average size first.
   */
  static double TetRelativeSizeSquared(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the scaled Jacobian of a tetrahedron.
   * The scaled jacobian of a tetrahedron is the minimum Jacobian divided
   * by the lengths of 3 edge vectors.
   *
   * @note Supports only vtkTetra and vtkQuadraticTetra.
   */
  static double TetScaledJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape of a tetrahedron.
   * The shape of a tetrahedron is 3 / Mean Ratio of weighted Jacobian matrix.
   *
   * @note Supports only vtkTetra.
   */
  static double TetShape(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape and size of a tetrahedron.
   * The shape and size of a tetrahedron is product of shape and average size.
   *
   * Note: TetShapeAndSize will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average tet size.
   *
   * @note Supports only vtkTetra.
   * @pre Requires computing tet average size first.
   */
  static double TetShapeAndSize(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the squish index of a tetrahedron.
   *
   * @note Supports only vtkTetra.
   */
  static double TetSquishIndex(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the volume of a tetrahedron.
   * The volume of a tetrahedron is (1/6) * Jacobian at corner node.
   *
   * @note Supports only vtkTetra and vtkQuadraticTetra.
   */
  static double TetVolume(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the equiangle skew of a pyramid.
   *
   * @note Supports only vtkPyramid.
   */
  static double PyramidEquiangleSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Jacobian of a pyramid.
   * The jacobian of a tetrahedron is the minimum point-wise volume at any corner.
   *
   * @note Supports only vtkPyramid.
   */
  static double PyramidJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Jacobian of a pyramid.
   * The jacobian of a tetrahedron is the minimum point-wise volume at any corner.
   *
   * @note Supports only vtkPyramid.
   */
  static double PyramidScaledJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape of a pyramid.
   * The shape of a pyramid is 4 divided by the minimum mean ratio of the
   * Jacobian matrix at each element corner.
   *
   * @note Supports only vtkPyramid.
   */
  static double PyramidShape(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the volume of a pyramid.
   *
   * @note Supports only vtkPyramid.
   */
  static double PyramidVolume(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the condition number of a wedge.
   * The condition number of a wedge is equivalent to the max aspect Frobenius.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeCondition(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the distortion of a wedge.
   * The distortion of a wedge is {min(|J|) / actual volume } * parent volume.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeDistortion(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the edge ratio of a wedge.
   * The edge ratio of a wedge is Hmax / Hmin, where Hmax and Hmin are respectively
   * the maximum and the minimum edge lengths.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeEdgeRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the equiangle skew of a wedge.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeEquiangleSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Jacobian of a wedge.
   * The jacobian of a wedge is the min{((L_2 X L_0) * L_3)_k}.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the max aspect Frobenius of a wedge.
   * The max aspect Frobenius of a wedge is max(F_0123, F_1204, F_2015, F_3540, F_4351, F_5432).
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeMaxAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the max stretch of a wedge.
   * The maximum stretch of a wedge is maximum stretch (S) of the three quadrilateral faces:
   * q = max[S_1043, S_1254, S_2035]
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeMaxStretch(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the mean aspect Frobenius of a wedge.
   * The mean aspect Frobenius of a wedge is:
   * 1/6 * (F_0123 + F_1204 + F+2015 + F_3540 + F_4351 + F_5432).
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeMeanAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the scaled Jacobian a wedge.
   * The jacobian of a wedge is the minimum point-wise volume at any corner divided by the
   * corresponding edge lengths and normalized to the unit wedge:
   * q = min(  2 / sqrt(3) * ((L_2 X L_0) * L_3)_k / sqrt(mag(L_2) * mag(L_0) * mag(L_3))),
   * where ((L_2 X L_0) * L_3)_k is the determinant of the Jacobian of the tetrahedron defined
   * at the kth corner node, and L_2, L_0 and L_3 are the edges defined according to the
   * standard for tetrahedral elements.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeScaledJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape of a wedge.
   * The shape of a wedge is 3 divided by the minimum mean ratio of the Jacobian matrix at each
   * element corner.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeShape(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the volume of a wedge.
   *
   * @note Supports only vtkWedge.
   */
  static double WedgeVolume(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the condition of a hexahedron.
   * The condition of a hexahedron is equivalent to HexMaxAspectFrobenius.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexCondition(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the diagonal of a hexahedron.
   * The diagonal of a hexahedron Minimum diagonal length / maximum diagonal length.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexDiagonal(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the dimension of a hexahedron.
   * The dimension of a hexahedron is the Pronto-specific characteristic length
   * for stable time step calculation, where characteristic length = Volume / 2 grad Volume.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexDimension(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the distortion of a hexahedron.
   * The distortion of a hexahedron is {min(|J|)/actual volume} * parent volume,
   * parent volume = 8 for a hexahedron.
   *
   * @note Supports only vtkHexahedron vtkQuadraticHexahedron and vtkTriQuadraticHexahedron.
   */
  static double HexDistortion(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the edge ratio of a hexahedron.
   * The edge ratio of a hexahedron \f$H\f$ is:
   * \f$\frac{|H|_\infty}{|H|_0}\f$,
   * where \f$|H|_\infty\f$ and \f$|H|_0\f$ respectively denote the greatest and
   * the smallest edge lengths of \f$H\f$.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexEdgeRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the equiangle skew of a hexahedron.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexEquiangleSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the Jacobian of a hexahedron.
   * The jacobian of a hexahedron is the minimum point-wise of local map at
   * 8 corners & center of the hexahedron.
   *
   * @note Supports only vtkHexahedron and vtkTriQuadraticHexahedron
   */
  static double HexJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the maximal Frobenius aspect of the 8 corner tetrahedra of a hexahedron,
   * when the reference tetrahedral elements are right isosceles at the hexahedron vertices.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexMaxAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the maximum edge ratio of a hexahedron at its center.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexMaxEdgeRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the average Frobenius aspect of the 8 corner tetrahedra of a hexahedron,
   * when the reference tetrahedral elements are right isosceles at the hexahedron vertices.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexMedAspectFrobenius(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the nodal Jacobian ratio of a hexahedron.
   * The nodal Jacobian ratio of a hexahedron is min(Jacobian) / max(Jacobian) over all nodes.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexNodalJacobianRatio(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the oddy of a hexahedron.
   * The oddy of a hexahedron is the general distortion measure based on left
   * Cauchy-Green Tensor.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexOddy(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the relative size squared of a hexahedron.
   * The relative size squared of a hexahedron is Min(J, 1 / J), where J is determinant
   * of weighted Jacobian matrix.
   *
   * Note: HexRelativeSizeSquared will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average hex size.
   *
   * @note Supports only vtkHexahedron.
   * @pre Requires computing hex average size first.
   */
  static double HexRelativeSizeSquared(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the scaled Jacobian of a hexahedron.
   * The scaled jacobian of a hexahedron is the minimum Jacobian divided
   * by the lengths of 3 edge vectors.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexScaledJacobian(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape of a hexahedron.
   * The shape of a hexahedron is 3 / Mean Ratio of weighted Jacobian matrix.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexShape(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shape and size of a hexahedron.
   * The shape and size of a hexahedron is product of shape and average size.
   *
   * Note: HexShapeAndSize will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average hex size.
   *
   * @note Supports only vtkHexahedron.
   * @pre Requires computing hex average size first.
   */
  static double HexShapeAndSize(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shear of a hexahedron.
   * The shear of a hexahedron is 3 / Mean Ratio of Jacobian Skew matrix.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexShear(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the shear and size of a hexahedron.
   * The shear and size of a hexahedron is product of shear and average size.
   *
   * Note: HexShearAndSize will return 0.0 if the MeshQuality filter has NOT
   * been executed, because it relies on the average hex size.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexShearAndSize(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the skew of a hexahedron.
   * The skew of a hexahedron is the maximum |cos A|, where A is the angle
   * between edges at the hexahedron center.
   *
   * @note Supports only vtkHexahedron.
   * @pre Requires computing hex average size first.
   */
  static double HexSkew(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the stretch of a hexahedron.
   * The stretch of a hexahedron is Sqrt(3) * minimum edge length / maximum diagonal length.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexStretch(vtkCell* cell, bool linearApproximation = false);

  /**
   * Calculate the taper of a hexahedron.
   * The taper of a hexahedron is the ratio of lengths derived from opposite edges.
   *
   * @note Supports only vtkHexahedron.
   */
  static double HexTaper(vtkCell* cell, bool linearApproximation = false);
  /**
   * Calculate the volume of a hexahedron.
   * The volume of a hexahedron is the Jacobian at the hexahedron center.
   *
   * @note Supports only vtkHexahedron, vtkQuadraticHexahedron and vtkTriQuadraticHexahedron.
   */
  static double HexVolume(vtkCell* cell, bool linearApproximation = false);

  /**
   * These methods are deprecated. Use Get/SetSaveCellQuality() instead.
   *
   * Formerly, SetRatio could be used to disable computation
   * of the tetrahedral radius ratio so that volume alone could be computed.
   * Now, cell quality is always computed, but you may decide not
   * to store the result for each cell.
   * This allows average cell quality of a mesh to be
   * calculated without requiring per-cell storage.
   */
  VTK_DEPRECATED_IN_9_6_0("Please use SetSaveCellQuality instead.")
  virtual void SetRatio(vtkTypeBool r) { this->SetSaveCellQuality(r); }
  VTK_DEPRECATED_IN_9_6_0("Please use GetSaveCellQuality instead.")
  vtkTypeBool GetRatio() { return this->GetSaveCellQuality(); }
  VTK_DEPRECATED_IN_9_6_0("Please use SaveCellQualityOn instead.")
  void RatioOn() { this->SaveCellQualityOn(); }
  VTK_DEPRECATED_IN_9_6_0("Please use SaveCellQualityOff instead.")
  void RatioOff() { this->SaveCellQualityOff(); }

protected:
  vtkMeshQuality();
  ~vtkMeshQuality() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool SaveCellQuality;
  QualityMeasureTypes TriangleQualityMeasure;
  QualityMeasureTypes QuadQualityMeasure;
  QualityMeasureTypes TetQualityMeasure;
  QualityMeasureTypes PyramidQualityMeasure;
  QualityMeasureTypes WedgeQualityMeasure;
  QualityMeasureTypes HexQualityMeasure;
  bool LinearApproximation;

  using CellQualityType = std::function<double(vtkCell*, bool)>;
  CellQualityType GetTriangleQualityMeasureFunction() const;
  CellQualityType GetQuadraticTriangleQualityMeasureFunction() const;
  CellQualityType GetBiQuadraticTriangleQualityMeasureFunction() const;
  CellQualityType GetQuadQualityMeasureFunction() const;
  CellQualityType GetQuadraticQuadQualityMeasureFunction() const;
  CellQualityType GetBiQuadraticQuadQualityMeasureFunction() const;
  CellQualityType GetTetQualityMeasureFunction() const;
  CellQualityType GetQuadraticTetQualityMeasureFunction() const;
  CellQualityType GetPyramidQualityMeasureFunction() const;
  CellQualityType GetWedgeQualityMeasureFunction() const;
  CellQualityType GetHexQualityMeasureFunction() const;
  CellQualityType GetQuadraticHexQualityMeasureFunction() const;
  CellQualityType GetTriQuadraticHexQualityMeasureFunction() const;

  // Variables used to store the average size (2D: area / 3D: volume)
  static double TriangleAverageSize;
  static double QuadAverageSize;
  static double TetAverageSize;
  static double PyramidAverageSize;
  static double WedgeAverageSize;
  static double HexAverageSize;

private:
  vtkMeshQuality(const vtkMeshQuality&) = delete;
  void operator=(const vtkMeshQuality&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkMeshQuality_h
