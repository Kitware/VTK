/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCellQuality - Calculate functions of quality of the elements
//  of a mesh
//
// .SECTION Description
// vtkCellQuality computes one or more functions of (geometric) quality for each
// cell of a mesh.  The per-cell quality is added to the mesh's cell data, in an
// array named "Quality." Cell types not supported by this filter or undefined
// quality of supported cell types will have an entry of 0.
//
// .SECTION Caveats
// Most quadrilateral quality functions are intended for planar quadrilaterals
// only.  The minimal angle is not, strictly speaking, a quality function, but
// it is provided because of its usage by many authors.

#ifndef __vtkCellQuality_h
#define __vtkCellQuality_h

#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkCell;
class vtkDataArray;
class vtkIdList;
class vtkPoints;

class VTKFILTERSVERDICT_EXPORT vtkCellQuality : public vtkDataSetAlgorithm
{
  //BTX
  enum
    {
    NONE = 0,
    AREA,
    ASPECT_BETA,
    ASPECT_FROBENIUS,
    ASPECT_GAMMA,
    ASPECT_RATIO,
    COLLAPSE_RATIO,
    CONDITION,
    DIAGONAL,
    DIMENSION,
    DISTORTION,
    EDGE_RATIO,
    JACOBIAN,
    MAX_ANGLE,
    MAX_ASPECT_FROBENIUS,
    MAX_EDGE_RATIO,
    MED_ASPECT_FROBENIUS,
    MIN_ANGLE,
    NORMAL,
    ODDY,
    RADIUS_RATIO,
    RELATIVE_SIZE_SQUARED,
    SCALED_JACOBIAN,
    SHAPE,
    SHAPE_AND_SIZE,
    SHEAR,
    SHEAR_AND_SIZE,
    SKEW,
    STRETCH,
    TAPER,
    VOLUME,
    WARPAGE,
    };
  //ETX

public:
  void PrintSelf (ostream&, vtkIndent);
  vtkTypeMacro(vtkCellQuality, vtkDataSetAlgorithm);
  static vtkCellQuality* New ();

  // Description:
  // Set/Get the particular estimator used to function the quality of all
  // supported geometries. For qualities that are not defined for certain
  // geometries, later program logic ensures that CellQualityNone static
  // function will be used so that a predefined value is returned for the
  // request.
  // There is no default value for this call and valid values include all
  // possible qualities supported by this class.
  vtkSetMacro(QualityMeasure, int);
  vtkGetMacro(QualityMeasure, int);

  void SetQualityMeasureToArea ()
    {
    this->SetQualityMeasure(AREA);
    }
  void SetQualityMeasureToAspectBeta ()
    {
    this->SetQualityMeasure(ASPECT_BETA);
    }
  void SetQualityMeasureToAspectFrobenius ()
    {
    this->SetQualityMeasure(ASPECT_FROBENIUS);
    }
  void SetQualityMeasureToAspectGamma ()
    {
    this->SetQualityMeasure(ASPECT_GAMMA);
    }
  void SetQualityMeasureToAspectRatio ()
    {
    this->SetQualityMeasure(ASPECT_RATIO);
    }
  void SetQualityMeasureToCollapseRatio ()
    {
    this->SetQualityMeasure(COLLAPSE_RATIO);
    }
  void SetQualityMeasureToCondition ()
    {
    this->SetQualityMeasure(CONDITION);
    }
  void SetQualityMeasureToDiagonal ()
    {
    this->SetQualityMeasure(DIAGONAL);
    }
  void SetQualityMeasureToDimension ()
    {
    this->SetQualityMeasure(DIMENSION);
    }
  void SetQualityMeasureToDistortion ()
    {
    this->SetQualityMeasure(DISTORTION);
    }
  void SetQualityMeasureToJacobian ()
    {
    this->SetQualityMeasure(JACOBIAN);
    }
  void SetQualityMeasureToMaxAngle ()
    {
    this->SetQualityMeasure(MAX_ANGLE);
    }
  void SetQualityMeasureToMaxAspectFrobenius ()
    {
    this->SetQualityMeasure(MAX_ASPECT_FROBENIUS);
    }
  void SetQualityMeasureToMaxEdgeRatio ()
    {
    this->SetQualityMeasure(MAX_EDGE_RATIO);
    }
  void SetQualityMeasureToMedAspectFrobenius ()
    {
    this->SetQualityMeasure(MED_ASPECT_FROBENIUS);
    }
  void SetQualityMeasureToMinAngle ()
    {
    this->SetQualityMeasure(MIN_ANGLE);
    }
  void SetQualityMeasureToOddy ()
    {
    this->SetQualityMeasure(ODDY);
    }
  void SetQualityMeasureToRadiusRatio ()
    {
    this->SetQualityMeasure(RADIUS_RATIO);
    }
  void SetQualityMeasureToRelativeSizeSquared ()
    {
    this->SetQualityMeasure(RELATIVE_SIZE_SQUARED);
    }
  void SetQualityMeasureToScaledJacobian ()
    {
    this->SetQualityMeasure(SCALED_JACOBIAN);
    }
  void SetQualityMeasureToShapeAndSize ()
    {
    this->SetQualityMeasure(SHAPE_AND_SIZE);
    }
  void SetQualityMeasureToShape ()
    {
    this->SetQualityMeasure(SHAPE);
    }
  void SetQualityMeasureToShearAndSize ()
    {
    this->SetQualityMeasure(SHEAR_AND_SIZE);
    }
  void SetQualityMeasureToShear ()
    {
    this->SetQualityMeasure(SHEAR);
    }
  void SetQualityMeasureToSkew ()
    {
    this->SetQualityMeasure(SKEW);
    }
  void SetQualityMeasureToStretch ()
    {
    this->SetQualityMeasure(STRETCH);
    }
  void SetQualityMeasureToTaper ()
    {
    this->SetQualityMeasure(TAPER);
    }
  void SetQualityMeasureToVolume ()
    {
    this->SetQualityMeasure(VOLUME);
    }
  void SetQualityMeasureToWarpage ()
    {
    this->SetQualityMeasure(WARPAGE);
    }

  // Description:
  // Set/Get the return value for unsupported geometry. Unsupported geometry
  // are geometries that are not supported by this filter currently, future
  // implementation might include support for them. The defalut value for
  // UnsupportedGeometry is -1.
  vtkSetMacro(UnsupportedGeometry, double);
  vtkGetMacro(UnsupportedGeometry, double);

  // Description:
  // Set/Get the return value for undefined quality. Undefined quality
  // are qualities that could be addressed by this filter but is not well
  // defined for the particular geometry of cell in question, e.g. a
  // volume query for a triangle. Undefined quality will always be undefined.
  // The default value for UndefinedQuality is -1.
  vtkSetMacro(UndefinedQuality, double);
  vtkGetMacro(UndefinedQuality, double);

  double TriangleStripArea (vtkCell*);
  double PixelArea (vtkCell*);
  double PolygonArea (vtkCell*);

protected:
 ~vtkCellQuality ();
  vtkCellQuality ();

  // Description:
  // Set/Get the particular estimator used to function the quality of triangles.
  // The default is NONE and valid values also include
  // ASPECT_FROBENIUS
  // ASPECT_RATIO
  // CONDITION
  // DISTORTION
  // EDGE_RATIO
  // MAX_ANGLE
  // MIN_ANGLE
  // RADIUS_RATIO
  // RELATIVE_SIZE_SQUARED
  // SCALED_JACOBIAN
  // SHAPE
  // SHAPE_AND_SIZE
  double ComputeTriangleQuality (vtkCell*);

  // Description:
  // Set/Get the particular estimator used to measure the quality of quadrilaterals.
  // The default is NONE and valid values also include
  // AREA
  // ASPECT_RATIO
  // CONDITION
  // DISTORTION
  // EDGE_RATIO
  // JACOBIAN
  // MAX_ANGLE
  // MAX_EDGE_RATIO
  // MIN_ANGLE
  // ODDY
  // RADIUS_RATIO
  // RELATIVE_SIZE_SQUARED
  // SCALED_JACOBIAN
  // SHAPE
  // SHAPE_AND_SIZE
  // SHEAR
  // SHEAR_AND_SIZE
  // SKEW
  // STRETCH
  // TAPER
  // WARPAGE
  // Scope: Except for EDGE_RATIO, these estimators are intended for planar
  // quadrilaterals only; use at your own risk if you really want to assess non-planar
  // quadrilateral quality with those.
  double ComputeQuadQuality (vtkCell*);

  // Description:
  // Set/Get the particular estimator used to measure the quality of tetrahedra.
  // The default is NONE and valid values also include
  // ASPECT_BETA
  // ASPECT_FROBENIUS
  // ASPECT_GAMMA
  // ASPECT_RATIO
  // COLLAPSE_RATIO
  // CONDITION
  // DISTORTION
  // EDGE_RATIO
  // JACOBIAN
  // RADIUS_RATIO (identical to Verdict's aspect ratio beta)
  // RELATIVE_SIZE_SQUARED
  // SCALED_JACOBIAN
  // SHAPE
  // SHAPE_AND_SIZE
  // VOLUME
  double ComputeTetQuality (vtkCell*);

  // Description:
  // Set/Get the particular estimator used to measure the quality of hexahedra.
  // The default is NONE and valid values also include
  // CONDITION
  // DIAGONAL
  // DIMENSION
  // DISTORTION
  // EDGE_RATIO
  // JACOBIAN
  // MAX_ASPECT_FROBENIUS
  // MAX_ASPECT_FROBENIUS
  // MAX_EDGE_RATIO
  // ODDY
  // RELATIVE_SIZE_SQUARED
  // SCALED_JACOBIAN
  // SHAPE
  // SHAPE_AND_SIZE
  // SHEAR
  // SHEAR_AND_SIZE
  // SKEW
  // STRETCH
  // TAPER
  // VOLUME
  double ComputeHexQuality (vtkCell*);

  // Description:
  // Set/Get the particular estimator used to measure the quality of triangle
  // strip.
  // The default is NONE and valid values also include
  // AREA
  double ComputeTriangleStripQuality (vtkCell*);

  // Description:
  // Set/Get the particular estimator used to measure the quality of pixel.
  // The default is NONE and valid values also include
  // AREA
  double ComputePixelQuality (vtkCell*);

  virtual int RequestData
    (vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // Description:
  // A function called by some VERDICT triangle quality functions to test for
  // inverted triangles.
  // VERDICT only accepts plain function pointers which means the follow
  // function and member must be static. Unfortunately, this makes the use of
  // this part not thread safe.
  static int GetCurrentTriangleNormal (double point [3], double normal [3]);
  static double CurrentTriNormal [3];

  int QualityMeasure;

  // Default return value for unsupported geometry
  double UnsupportedGeometry;

  // Default return value for qualities that are not well-defined for certain
  // types of supported geometries. e.g. volume of a triangle
  double UndefinedQuality;

private:
  vtkIdList* PointIds;
  vtkPoints* Points;

  vtkCellQuality(const vtkCellQuality&); // Not implemented
  void operator=(const vtkCellQuality&); // Not implemented
};

#endif // vtkCellQuality_h
