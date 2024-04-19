// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellQuality
 * @brief   Calculate functions of quality of the elements of a mesh
 *
 * vtkCellQuality computes one or more functions of (geometric) quality for each
 * cell of a mesh.  The per-cell quality is added to the mesh's cell data, in an
 * array named "CellQuality." Cell types not supported by this filter or undefined
 * quality of supported cell types will have an entry of -1.
 *
 * @warning
 * Most quadrilateral quality functions are intended for planar quadrilaterals
 * only.  The minimal angle is not, strictly speaking, a quality function, but
 * it is provided because of its usage by many authors.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef vtkCellQuality_h
#define vtkCellQuality_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkMeshQuality.h"          // For QualityMeasureType

VTK_ABI_NAMESPACE_BEGIN
class vtkCell;
class vtkCellQualityFunctor;
class vtkDataArray;
class vtkIdList;
class vtkPoints;

class VTKFILTERSVERDICT_EXPORT vtkCellQuality : public vtkDataSetAlgorithm
{
private:
  friend class vtkCellQualityFunctor;

public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkCellQuality, vtkDataSetAlgorithm);
  static vtkCellQuality* New();

  ///@{
  /**
   * Set/Get the particular estimator used to function the quality of all
   * supported geometries. For qualities that are not defined for certain
   * geometries, later program logic ensures that CellQualityNone static
   * function will be used so that a predefined value is returned for the
   * request.
   * There is no default value for this call and valid values include all
   * possible qualities supported by this class.
   *
   * For Quality Measure values see vtkMeshQuality's enum QualityMeasureType.
   */
  using QualityMeasureTypes = vtkMeshQuality::QualityMeasureTypes;
  vtkSetEnumMacro(QualityMeasure, QualityMeasureTypes);
  virtual void SetQualityMeasure(int measure)
  {
    this->SetQualityMeasure(static_cast<QualityMeasureTypes>(measure));
  }
  vtkGetEnumMacro(QualityMeasure, QualityMeasureTypes);
  void SetQualityMeasureToArea() { this->SetQualityMeasure(QualityMeasureTypes::AREA); }
  void SetQualityMeasureToAspectFrobenius()
  {
    this->SetQualityMeasure(QualityMeasureTypes::ASPECT_FROBENIUS);
  }
  void SetQualityMeasureToAspectGamma()
  {
    this->SetQualityMeasure(QualityMeasureTypes::ASPECT_GAMMA);
  }
  void SetQualityMeasureToAspectRatio()
  {
    this->SetQualityMeasure(QualityMeasureTypes::ASPECT_RATIO);
  }
  void SetQualityMeasureToCollapseRatio()
  {
    this->SetQualityMeasure(QualityMeasureTypes::COLLAPSE_RATIO);
  }
  void SetQualityMeasureToCondition() { this->SetQualityMeasure(QualityMeasureTypes::CONDITION); }
  void SetQualityMeasureToDiagonal() { this->SetQualityMeasure(QualityMeasureTypes::DIAGONAL); }
  void SetQualityMeasureToDimension() { this->SetQualityMeasure(QualityMeasureTypes::DIMENSION); }
  void SetQualityMeasureToDistortion() { this->SetQualityMeasure(QualityMeasureTypes::DISTORTION); }
  void SetQualityMeasureToJacobian() { this->SetQualityMeasure(QualityMeasureTypes::JACOBIAN); }
  void SetQualityMeasureToMaxAngle() { this->SetQualityMeasure(QualityMeasureTypes::MAX_ANGLE); }
  void SetQualityMeasureToMaxAspectFrobenius()
  {
    this->SetQualityMeasure(QualityMeasureTypes::MAX_ASPECT_FROBENIUS);
  }
  void SetQualityMeasureToMaxEdgeRatio()
  {
    this->SetQualityMeasure(QualityMeasureTypes::MAX_EDGE_RATIO);
  }
  void SetQualityMeasureToMedAspectFrobenius()
  {
    this->SetQualityMeasure(QualityMeasureTypes::MED_ASPECT_FROBENIUS);
  }
  void SetQualityMeasureToMinAngle() { this->SetQualityMeasure(QualityMeasureTypes::MIN_ANGLE); }
  void SetQualityMeasureToOddy() { this->SetQualityMeasure(QualityMeasureTypes::ODDY); }
  void SetQualityMeasureToRadiusRatio()
  {
    this->SetQualityMeasure(QualityMeasureTypes::RADIUS_RATIO);
  }
  void SetQualityMeasureToRelativeSizeSquared()
  {
    this->SetQualityMeasure(QualityMeasureTypes::RELATIVE_SIZE_SQUARED);
  }
  void SetQualityMeasureToScaledJacobian()
  {
    this->SetQualityMeasure(QualityMeasureTypes::SCALED_JACOBIAN);
  }
  void SetQualityMeasureToShapeAndSize()
  {
    this->SetQualityMeasure(QualityMeasureTypes::SHAPE_AND_SIZE);
  }
  void SetQualityMeasureToShape() { this->SetQualityMeasure(QualityMeasureTypes::SHAPE); }
  void SetQualityMeasureToShearAndSize()
  {
    this->SetQualityMeasure(QualityMeasureTypes::SHEAR_AND_SIZE);
  }
  void SetQualityMeasureToShear() { this->SetQualityMeasure(QualityMeasureTypes::SHEAR); }
  void SetQualityMeasureToSkew() { this->SetQualityMeasure(QualityMeasureTypes::SKEW); }
  void SetQualityMeasureToStretch() { this->SetQualityMeasure(QualityMeasureTypes::STRETCH); }
  void SetQualityMeasureToTaper() { this->SetQualityMeasure(QualityMeasureTypes::TAPER); }
  void SetQualityMeasureToVolume() { this->SetQualityMeasure(QualityMeasureTypes::VOLUME); }
  void SetQualityMeasureToWarpage() { this->SetQualityMeasure(QualityMeasureTypes::WARPAGE); }
  ///@}

  ///@{
  /**
   * Set/Get the return value for unsupported geometry. Unsupported geometry
   * are geometries that are not supported by this filter currently, future
   * implementation might include support for them. The default value for
   * UnsupportedGeometry is -1.
   */
  vtkSetMacro(UnsupportedGeometry, double);
  vtkGetMacro(UnsupportedGeometry, double);
  ///@}

  ///@{
  /**
   * Set/Get the return value for undefined quality. Undefined quality
   * are qualities that could be addressed by this filter but is not well
   * defined for the particular geometry of cell in question, e.g. a
   * volume query for a triangle. Undefined quality will always be undefined.
   * The default value for UndefinedQuality is -1.
   */
  vtkSetMacro(UndefinedQuality, double);
  vtkGetMacro(UndefinedQuality, double);
  ///@}

  double TriangleStripArea(vtkCell*);
  double PixelArea(vtkCell*);
  double PolygonArea(vtkCell*);

protected:
  ~vtkCellQuality() override;
  vtkCellQuality();

  /**
   * Set/Get the particular estimator used to function the quality of triangles.
   * The default is NONE and valid values also include
   * ASPECT_FROBENIUS
   * ASPECT_RATIO
   * CONDITION
   * DISTORTION
   * EDGE_RATIO
   * EQUIANGLE_SKEW
   * MAX_ANGLE
   * MIN_ANGLE
   * NORMALIZED_INRADIUS
   * RADIUS_RATIO
   * RELATIVE_SIZE_SQUARED
   * SCALED_JACOBIAN
   * SHAPE
   * SHAPE_AND_SIZE
   */
  double ComputeTriangleQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of quadrilaterals.
   * The default is NONE and valid values also include
   * AREA
   * ASPECT_RATIO
   * CONDITION
   * DISTORTION
   * EDGE_RATIO
   * EQUIANGLE_SKEW
   * JACOBIAN
   * MAX_ANGLE
   * MAX_EDGE_RATIO
   * MIN_ANGLE
   * ODDY
   * RADIUS_RATIO
   * RELATIVE_SIZE_SQUARED
   * SCALED_JACOBIAN
   * SHAPE
   * SHAPE_AND_SIZE
   * SHEAR
   * SHEAR_AND_SIZE
   * SKEW
   * STRETCH
   * TAPER
   * WARPAGE
   * Scope: Except for EDGE_RATIO, these estimators are intended for planar
   * quadrilaterals only; use at your own risk if you really want to assess non-planar
   * quadrilateral quality with those.
   */
  double ComputeQuadQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of tetrahedra.
   * The default is NONE and valid values also include
   * ASPECT_FROBENIUS
   * ASPECT_GAMMA
   * ASPECT_RATIO
   * COLLAPSE_RATIO
   * CONDITION
   * DISTORTION
   * EDGE_RATIO
   * EQUIANGLE_SKEW
   * EQUIVOLUME_SKEW
   * JACOBIAN
   * MEAN_RATIO
   * NORMALIZED_INRADIUS
   * RADIUS_RATIO
   * RELATIVE_SIZE_SQUARED
   * SCALED_JACOBIAN
   * SHAPE
   * SHAPE_AND_SIZE
   * SQUISH_INDEX
   * VOLUME
   */
  double ComputeTetQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of pyramids.
   * The default is NONE and valid values also include
   * EQUIANGLE_SKEW
   * JACOBIAN
   * SCALED_JACOBIAN
   * SHAPE
   * VOLUME
   */
  double ComputePyramidQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of wedges.
   * The default is NONE and valid values also include
   * CONDITION
   * DISTORTION
   * EDGE_RATIO
   * EQUIANGLE_SKEW
   * JACOBIAN
   * MAX_ASPECT_FROBENIUS
   * MAX_STRETCH
   * MEAN_ASPECT_FROBENIUS
   * SCALED_JACOBIAN
   * SHAPE
   * VOLUME
   */
  double ComputeWedgeQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of hexahedra.
   * The default is NONE and valid values also include
   * CONDITION
   * DIAGONAL
   * DIMENSION
   * DISTORTION
   * EDGE_RATIO
   * EQUIANGLE_SKEW
   * JACOBIAN
   * MAX_ASPECT_FROBENIUS
   * MAX_ASPECT_FROBENIUS
   * MAX_EDGE_RATIO
   * NODAL_JACOBIAN_RATIO
   * ODDY
   * RELATIVE_SIZE_SQUARED
   * SCALED_JACOBIAN
   * SHAPE
   * SHAPE_AND_SIZE
   * SHEAR
   * SHEAR_AND_SIZE
   * SKEW
   * STRETCH
   * TAPER
   * VOLUME
   */
  double ComputeHexQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of triangle
   * strip.
   * The default is NONE and valid values also include
   * AREA
   */
  double ComputeTriangleStripQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of pixel.
   * The default is NONE and valid values also include
   * AREA
   */
  double ComputePixelQuality(vtkCell*);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMeshQuality::QualityMeasureTypes QualityMeasure;

  // Default return value for unsupported geometry
  double UnsupportedGeometry;

  // Default return value for qualities that are not well-defined for certain
  // types of supported geometries. e.g. volume of a triangle
  double UndefinedQuality;

private:
  vtkIdList* PointIds;
  vtkPoints* Points;

  vtkCellQuality(const vtkCellQuality&) = delete;
  void operator=(const vtkCellQuality&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellQuality_h
