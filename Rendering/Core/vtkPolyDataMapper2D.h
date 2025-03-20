// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataMapper2D
 * @brief   draw vtkPolyData onto the image plane
 *
 * vtkPolyDataMapper2D is a mapper that renders 3D polygonal data
 * (vtkPolyData) onto the 2D image plane (i.e., the renderer's viewport).
 * By default, the 3D data is transformed into 2D data by ignoring the
 * z-coordinate of the 3D points in vtkPolyData, and taking the x-y values
 * as local display values (i.e., pixel coordinates). Alternatively, you
 * can provide a vtkCoordinate object that will transform the data into
 * local display coordinates (use the vtkCoordinate::SetCoordinateSystem()
 * methods to indicate which coordinate system you are transforming the
 * data from).
 *
 * @sa
 * vtkMapper2D vtkActor2D
 */

#ifndef vtkPolyDataMapper2D_h
#define vtkPolyDataMapper2D_h

#include "vtkMapper2D.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCoordinate;
class vtkPolyData;
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkPolyDataMapper2D : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkPolyDataMapper2D, vtkMapper2D);
  static vtkPolyDataMapper2D* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the input to the mapper.
   */
  void SetInputData(vtkPolyData* in);
  vtkPolyData* GetInput();
  ///@}

  ///@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(vtkScalarsToColors* lut);
  vtkScalarsToColors* GetLookupTable();
  ///@}

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  ///@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  vtkSetMacro(ScalarVisibility, vtkTypeBool);
  vtkGetMacro(ScalarVisibility, vtkTypeBool);
  vtkBooleanMacro(ScalarVisibility, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Control how the scalar data is mapped to colors.  By default
   * (ColorModeToDefault), unsigned char scalars are treated as
   * colors, and NOT mapped through the lookup table, while everything
   * else is. ColorModeToDirectScalar extends ColorModeToDefault such
   * that all integer types are treated as colors with values in the
   * range 0-255 and floating types are treated as colors with values
   * in the range 0.0-1.0. Setting
   * ColorModeToMapScalars means that all scalar data will be mapped through
   * the lookup table.  (Note that for multi-component scalars, the
   * particular component to use for mapping can be specified using the
   * ColorByArrayComponent() method.)
   */
  vtkSetMacro(ColorMode, int);
  vtkGetMacro(ColorMode, int);
  void SetColorModeToDefault();
  void SetColorModeToMapScalars();
  void SetColorModeToDirectScalars();
  ///@}

  /**
   * Return the method of coloring scalar data.
   */
  const char* GetColorModeAsString();

  ///@{
  /**
   * Control whether the mapper sets the lookuptable range based on its
   * own ScalarRange, or whether it will use the LookupTable ScalarRange
   * regardless of it's own setting. By default the Mapper is allowed to set
   * the LookupTable range, but users who are sharing LookupTables between
   * mappers/actors will probably wish to force the mapper to use the
   * LookupTable unchanged.
   */
  vtkSetMacro(UseLookupTableScalarRange, vtkTypeBool);
  vtkGetMacro(UseLookupTableScalarRange, vtkTypeBool);
  vtkBooleanMacro(UseLookupTableScalarRange, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify range in terms of scalar minimum and maximum (smin,smax). These
   * values are used to map scalars into lookup table. Has no effect when
   * UseLookupTableScalarRange is true.
   */
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetVectorMacro(ScalarRange, double, 2);
  ///@}

  ///@{
  /**
   * Control how the filter works with scalar point data and cell attribute
   * data.  By default (ScalarModeToDefault), the filter will use point data,
   * and if no point data is available, then cell data is used. Alternatively
   * you can explicitly set the filter to use point data
   * (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
   * You can also choose to get the scalars from an array in point field
   * data (ScalarModeToUsePointFieldData) or cell field data
   * (ScalarModeToUseCellFieldData).  If scalars are coming from a field
   * data array, you must call ColorByArrayComponent before you call
   * GetColors.
   */
  vtkSetMacro(ScalarMode, int);
  vtkGetMacro(ScalarMode, int);
  void SetScalarModeToDefault() { this->SetScalarMode(VTK_SCALAR_MODE_DEFAULT); }
  void SetScalarModeToUsePointData() { this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA); }
  void SetScalarModeToUseCellData() { this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_DATA); }
  void SetScalarModeToUsePointFieldData()
  {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
  }
  void SetScalarModeToUseCellFieldData()
  {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  }
  ///@}

  ///@{
  /**
   * Choose which component of which field data array to color by.
   */
  void ColorByArrayComponent(int arrayNum, int component);
  void ColorByArrayComponent(const char* arrayName, int component);
  ///@}

  /**
   * Get the array name or number and component to color by.
   */
  vtkGetStringMacro(ArrayName);
  vtkSetStringMacro(ArrayName);
  vtkGetMacro(ArrayId, int);
  vtkSetMacro(ArrayId, int);
  vtkGetMacro(ArrayAccessMode, int);
  vtkSetMacro(ArrayAccessMode, int);
  vtkGetMacro(ArrayComponent, int);
  vtkSetMacro(ArrayComponent, int);

  /**
   * Overload standard modified time function. If lookup table is modified,
   * then this object is modified as well.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify a vtkCoordinate object to be used to transform the vtkPolyData
   * point coordinates. By default (no vtkCoordinate specified), the point
   * coordinates are taken as viewport coordinates (pixels in the viewport
   * into which the mapper is rendering).
   */
  virtual void SetTransformCoordinate(vtkCoordinate*);
  vtkGetObjectMacro(TransformCoordinate, vtkCoordinate);
  ///@}

  ///@{
  /**
   * Specify whether or not rounding to integers the transformed points when
   * TransformCoordinate is set. By default, it does not use double precision.
   */
  vtkGetMacro(TransformCoordinateUseDouble, bool);
  vtkSetMacro(TransformCoordinateUseDouble, bool);
  vtkBooleanMacro(TransformCoordinateUseDouble, bool);
  ///@}

  /**
   * Map the scalars (if there are any scalars and ScalarVisibility is on)
   * through the lookup table, returning an unsigned char RGBA array. This is
   * typically done as part of the rendering process. The alpha parameter
   * allows the blending of the scalars with an additional alpha (typically
   * which comes from a vtkActor, etc.)
   */
  vtkUnsignedCharArray* MapScalars(double alpha);

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

protected:
  vtkPolyDataMapper2D();
  ~vtkPolyDataMapper2D() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  vtkUnsignedCharArray* Colors;

  vtkScalarsToColors* LookupTable;
  vtkTypeBool ScalarVisibility;
  vtkTimeStamp BuildTime;
  double ScalarRange[2];
  vtkTypeBool UseLookupTableScalarRange;
  int ColorMode;
  int ScalarMode;

  vtkCoordinate* TransformCoordinate;
  bool TransformCoordinateUseDouble;

  // for coloring by a component of a field data array
  int ArrayId;
  char* ArrayName = nullptr;
  int ArrayComponent;
  int ArrayAccessMode;

private:
  vtkPolyDataMapper2D(const vtkPolyDataMapper2D&) = delete;
  void operator=(const vtkPolyDataMapper2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
