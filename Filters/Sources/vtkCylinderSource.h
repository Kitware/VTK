// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCylinderSource
 * @brief   generate a polygonal cylinder centered at the origin
 *
 * vtkCylinderSource creates a polygonal cylinder centered at Center; The axis of the cylinder is
 * aligned along the global y-axis.  The height and radius of the cylinder can be specified, as well
 * as the number of sides. It is also possible to control whether the cylinder is open-ended or
 * capped. The cylinder can be given a capsular shape by enabling hemispherical end caps. If you
 * have the end points of the cylinder, you should use a vtkLineSource followed by a vtkTubeFilter
 * instead of the vtkCylinderSource.
 *
 * @sa
 * vtkCylinder
 */

#ifndef vtkCylinderSource_h
#define vtkCylinderSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

VTK_ABI_NAMESPACE_BEGIN

// Forward declarations
class vtkFloatArray;

class VTKFILTERSSOURCES_EXPORT vtkCylinderSource : public vtkPolyDataAlgorithm
{
public:
  static vtkCylinderSource* New();
  vtkTypeMacro(vtkCylinderSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the height of the cylinder. Initial value is 1.
   */
  vtkSetClampMacro(Height, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Height, double);
  ///@}

  ///@{
  /**
   * Set the radius of the cylinder. Initial value is 0.5
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Set/Get cylinder center. Initial value is (0.0,0.0,0.0)
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  ///@}

  ///@{
  /**
   * Set the number of facets used to define cylinder. Initial value is 6.
   */
  vtkSetClampMacro(Resolution, int, 3, VTK_CELL_SIZE);
  vtkGetMacro(Resolution, int);
  ///@}

  ///@{
  /**
   * Turn on/off whether to cap cylinder with polygons. Initial value is true.
   */
  vtkSetMacro(Capping, vtkTypeBool);
  vtkGetMacro(Capping, vtkTypeBool);
  vtkBooleanMacro(Capping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get whether the capping should make the cylinder a capsule. This adds hemispherical caps at
   * the ends of the cylinder.
   *
   * \sa SetCapping()
   */
  vtkSetMacro(CapsuleCap, vtkTypeBool);
  vtkGetMacro(CapsuleCap, vtkTypeBool);
  vtkBooleanMacro(CapsuleCap, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Cause the spheres to be tessellated with edges along the latitude and longitude lines. If off,
   * triangles are generated at non-polar regions, which results in edges that are not parallel to
   * latitude and longitude lines. If on, quadrilaterals are generated everywhere except at the
   * poles. This can be useful for generating wireframe spheres with natural latitude and longitude
   * lines.
   */
  vtkSetMacro(LatLongTessellation, vtkTypeBool);
  vtkGetMacro(LatLongTessellation, vtkTypeBool);
  vtkBooleanMacro(LatLongTessellation, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkCylinderSource(int res = 6);
  ~vtkCylinderSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int CreateHemisphere(vtkPoints* points, vtkFloatArray* normals, vtkFloatArray* tcooords,
    vtkCellArray* newPolys, int startIdx = 0);

  double Height;
  double Radius;
  double Center[3];
  int Resolution;
  vtkTypeBool Capping;
  vtkTypeBool CapsuleCap;
  vtkTypeBool LatLongTessellation;
  int OutputPointsPrecision;

private:
  vtkCylinderSource(const vtkCylinderSource&) = delete;
  void operator=(const vtkCylinderSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
