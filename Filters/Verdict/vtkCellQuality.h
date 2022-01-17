/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellQuality.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 */

#ifndef vtkCellQuality_h
#define vtkCellQuality_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkMeshQuality.h"          // For QualityMeasureType

class vtkCell;
class vtkDataArray;
class vtkIdList;
class vtkPoints;

class VTKFILTERSVERDICT_EXPORT vtkCellQuality : public vtkDataSetAlgorithm
{
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
  vtkSetMacro(QualityMeasure, int);
  vtkGetMacro(QualityMeasure, int);
  void SetQualityMeasureToArea() { this->SetQualityMeasure(vtkMeshQuality::AREA); }
  void SetQualityMeasureToAspectFrobenius()
  {
    this->SetQualityMeasure(vtkMeshQuality::ASPECT_FROBENIUS);
  }
  void SetQualityMeasureToAspectGamma() { this->SetQualityMeasure(vtkMeshQuality::ASPECT_GAMMA); }
  void SetQualityMeasureToAspectRatio() { this->SetQualityMeasure(vtkMeshQuality::ASPECT_RATIO); }
  void SetQualityMeasureToCollapseRatio()
  {
    this->SetQualityMeasure(vtkMeshQuality::COLLAPSE_RATIO);
  }
  void SetQualityMeasureToCondition() { this->SetQualityMeasure(vtkMeshQuality::CONDITION); }
  void SetQualityMeasureToDiagonal() { this->SetQualityMeasure(vtkMeshQuality::DIAGONAL); }
  void SetQualityMeasureToDimension() { this->SetQualityMeasure(vtkMeshQuality::DIMENSION); }
  void SetQualityMeasureToDistortion() { this->SetQualityMeasure(vtkMeshQuality::DISTORTION); }
  void SetQualityMeasureToJacobian() { this->SetQualityMeasure(vtkMeshQuality::JACOBIAN); }
  void SetQualityMeasureToMaxAngle() { this->SetQualityMeasure(vtkMeshQuality::MAX_ANGLE); }
  void SetQualityMeasureToMaxAspectFrobenius()
  {
    this->SetQualityMeasure(vtkMeshQuality::MAX_ASPECT_FROBENIUS);
  }
  void SetQualityMeasureToMaxEdgeRatio()
  {
    this->SetQualityMeasure(vtkMeshQuality::MAX_EDGE_RATIO);
  }
  void SetQualityMeasureToMedAspectFrobenius()
  {
    this->SetQualityMeasure(vtkMeshQuality::MED_ASPECT_FROBENIUS);
  }
  void SetQualityMeasureToMinAngle() { this->SetQualityMeasure(vtkMeshQuality::MIN_ANGLE); }
  void SetQualityMeasureToOddy() { this->SetQualityMeasure(vtkMeshQuality::ODDY); }
  void SetQualityMeasureToRadiusRatio() { this->SetQualityMeasure(vtkMeshQuality::RADIUS_RATIO); }
  void SetQualityMeasureToRelativeSizeSquared()
  {
    this->SetQualityMeasure(vtkMeshQuality::RELATIVE_SIZE_SQUARED);
  }
  void SetQualityMeasureToScaledJacobian()
  {
    this->SetQualityMeasure(vtkMeshQuality::SCALED_JACOBIAN);
  }
  void SetQualityMeasureToShapeAndSize()
  {
    this->SetQualityMeasure(vtkMeshQuality::SHAPE_AND_SIZE);
  }
  void SetQualityMeasureToShape() { this->SetQualityMeasure(vtkMeshQuality::SHAPE); }
  void SetQualityMeasureToShearAndSize()
  {
    this->SetQualityMeasure(vtkMeshQuality::SHEAR_AND_SIZE);
  }
  void SetQualityMeasureToShear() { this->SetQualityMeasure(vtkMeshQuality::SHEAR); }
  void SetQualityMeasureToSkew() { this->SetQualityMeasure(vtkMeshQuality::SKEW); }
  void SetQualityMeasureToStretch() { this->SetQualityMeasure(vtkMeshQuality::STRETCH); }
  void SetQualityMeasureToTaper() { this->SetQualityMeasure(vtkMeshQuality::TAPER); }
  void SetQualityMeasureToVolume() { this->SetQualityMeasure(vtkMeshQuality::VOLUME); }
  void SetQualityMeasureToWarpage() { this->SetQualityMeasure(vtkMeshQuality::WARPAGE); }
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
   * MAX_ANGLE
   * MIN_ANGLE
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
   * JACOBIAN
   * RADIUS_RATIO
   * RELATIVE_SIZE_SQUARED
   * SCALED_JACOBIAN
   * SHAPE
   * SHAPE_AND_SIZE
   * VOLUME
   */
  double ComputeTetQuality(vtkCell*);

  /**
   * Set/Get the particular estimator used to measure the quality of hexahedra.
   * The default is NONE and valid values also include
   * CONDITION
   * DIAGONAL
   * DIMENSION
   * DISTORTION
   * EDGE_RATIO
   * JACOBIAN
   * MAX_ASPECT_FROBENIUS
   * MAX_ASPECT_FROBENIUS
   * MAX_EDGE_RATIO
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

  int QualityMeasure;

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

#endif // vtkCellQuality_h
