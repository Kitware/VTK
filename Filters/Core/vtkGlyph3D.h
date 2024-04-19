// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGlyph3D
 * @brief   copy oriented and scaled glyph geometry to every input point
 *
 * vtkGlyph3D is a filter that copies a geometric representation (called
 * a glyph) to every point in the input dataset. The glyph is defined with
 * polygonal data from a source filter input. The glyph may be oriented
 * along the input vectors or normals, and it may be scaled according to
 * scalar data or vector magnitude. More than one glyph may be used by
 * creating a table of source objects, each defining a different glyph. If a
 * table of glyphs is defined, then the table can be indexed into by using
 * either scalar value or vector magnitude.
 *
 * To use this object you'll have to provide an input dataset and a source
 * to define the glyph. Then decide whether you want to scale the glyph and
 * how to scale the glyph (using scalar value or vector magnitude). Next
 * decide whether you want to orient the glyph, and whether to use the
 * vector data or normal data to orient it. Finally, decide whether to use a
 * table of glyphs, or just a single glyph. If you use a table of glyphs,
 * you'll have to decide whether to index into it with scalar value or with
 * vector magnitude.
 *
 * @warning
 * The scaling of the glyphs is controlled by the ScaleFactor ivar multiplied
 * by the scalar value at each point (if VTK_SCALE_BY_SCALAR is set), or
 * multiplied by the vector magnitude (if VTK_SCALE_BY_VECTOR is set),
 * Alternatively (if VTK_SCALE_BY_VECTORCOMPONENTS is set), the scaling
 * may be specified for x,y,z using the vector components. The
 * scale factor can be further controlled by enabling clamping using the
 * Clamping ivar. If clamping is enabled, the scale is normalized by the
 * Range ivar, and then multiplied by the scale factor. The normalization
 * process includes clamping the scale value between (0,1).
 *
 * @warning
 * Typically this object operates on input data with scalar and/or vector
 * data. However, scalar and/or vector aren't necessary, and it can be used
 * to copy data from a single source to each point. In this case the scale
 * factor can be used to uniformly scale the glyphs.
 *
 * @warning
 * The object uses "vector" data to scale glyphs, orient glyphs, and/or index
 * into a table of glyphs. You can choose to use either the vector or normal
 * data at each input point. Use the method SetVectorModeToUseVector() to use
 * the vector input data, and SetVectorModeToUseNormal() to use the
 * normal input data.
 *
 * @warning
 * If you do use a table of glyphs, make sure to set the Range ivar to make
 * sure the index into the glyph table is computed correctly.
 *
 * @warning
 * You can turn off scaling of the glyphs completely by using the Scaling
 * ivar. You can also turn off scaling due to data (either vector or scalar)
 * by using the SetScaleModeToDataScalingOff() method.
 *
 * @warning
 * You can set what arrays to use for the scalars, vectors, normals, and
 * color scalars by using the SetInputArrayToProcess methods in
 * vtkAlgorithm. The first array is scalars, the next vectors, the next
 * normals and finally color scalars.
 *
 * @sa
 * vtkTensorGlyph
 */

#ifndef vtkGlyph3D_h
#define vtkGlyph3D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_SCALE_BY_SCALAR 0
#define VTK_SCALE_BY_VECTOR 1
#define VTK_SCALE_BY_VECTORCOMPONENTS 2
#define VTK_DATA_SCALING_OFF 3

#define VTK_COLOR_BY_SCALE 0
#define VTK_COLOR_BY_SCALAR 1
#define VTK_COLOR_BY_VECTOR 2

#define VTK_USE_VECTOR 0
#define VTK_USE_NORMAL 1
#define VTK_VECTOR_ROTATION_OFF 2
#define VTK_FOLLOW_CAMERA_DIRECTION 3

#define VTK_INDEXING_OFF 0
#define VTK_INDEXING_BY_SCALAR 1
#define VTK_INDEXING_BY_VECTOR 2

VTK_ABI_NAMESPACE_BEGIN
class vtkTransform;

class VTKFILTERSCORE_EXPORT vtkGlyph3D : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkGlyph3D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with scaling on, scaling mode is by scalar value,
   * scale factor = 1.0, the range is (0,1), orient geometry is on, and
   * orientation is by vector. Clamping and indexing are turned off. No
   * initial sources are defined.
   */
  static vtkGlyph3D* New();

  /**
   * Set the source to use for the glyph.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(vtkPolyData* pd) { this->SetSourceData(0, pd); }

  /**
   * Specify a source object at a specified table location.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(int id, vtkPolyData* pd);

  ///@{
  /**
   * Specify a source object at a specified table location. New style.
   * Source connection is stored in port 1. This method is equivalent
   * to SetInputConnection(1, id, outputPort).
   */
  void SetSourceConnection(int id, vtkAlgorithmOutput* algOutput);
  void SetSourceConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetSourceConnection(0, algOutput);
  }
  ///@}

  /**
   * Get a pointer to a source object at a specified table location.
   */
  vtkPolyData* GetSource(int id = 0);

  ///@{
  /**
   * Turn on/off scaling of source geometry.
   */
  vtkSetMacro(Scaling, vtkTypeBool);
  vtkBooleanMacro(Scaling, vtkTypeBool);
  vtkGetMacro(Scaling, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Either scale by scalar or by vector/normal magnitude.
   */
  vtkSetMacro(ScaleMode, int);
  vtkGetMacro(ScaleMode, int);
  void SetScaleModeToScaleByScalar() { this->SetScaleMode(VTK_SCALE_BY_SCALAR); }
  void SetScaleModeToScaleByVector() { this->SetScaleMode(VTK_SCALE_BY_VECTOR); }
  void SetScaleModeToScaleByVectorComponents()
  {
    this->SetScaleMode(VTK_SCALE_BY_VECTORCOMPONENTS);
  }
  void SetScaleModeToDataScalingOff() { this->SetScaleMode(VTK_DATA_SCALING_OFF); }
  const char* GetScaleModeAsString();
  ///@}

  ///@{
  /**
   * Either color by scale, scalar or by vector/normal magnitude.
   */
  vtkSetMacro(ColorMode, int);
  vtkGetMacro(ColorMode, int);
  void SetColorModeToColorByScale() { this->SetColorMode(VTK_COLOR_BY_SCALE); }
  void SetColorModeToColorByScalar() { this->SetColorMode(VTK_COLOR_BY_SCALAR); }
  void SetColorModeToColorByVector() { this->SetColorMode(VTK_COLOR_BY_VECTOR); }
  const char* GetColorModeAsString();
  ///@}

  ///@{
  /**
   * Specify scale factor to scale object by.
   */
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Specify range to map scalar values into.
   */
  vtkSetVector2Macro(Range, double);
  vtkGetVectorMacro(Range, double, 2);
  ///@}

  ///@{
  /**
   * Turn on/off orienting of input geometry along vector/normal.
   */
  vtkSetMacro(Orient, vtkTypeBool);
  vtkBooleanMacro(Orient, vtkTypeBool);
  vtkGetMacro(Orient, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off clamping of "scalar" values to range. (Scalar value may be
   * vector magnitude if ScaleByVector() is enabled.)
   */
  vtkSetMacro(Clamping, vtkTypeBool);
  vtkBooleanMacro(Clamping, vtkTypeBool);
  vtkGetMacro(Clamping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify whether to use vector or normal to perform vector operations.
   */
  vtkSetMacro(VectorMode, int);
  vtkGetMacro(VectorMode, int);
  void SetVectorModeToUseVector() { this->SetVectorMode(VTK_USE_VECTOR); }
  void SetVectorModeToUseNormal() { this->SetVectorMode(VTK_USE_NORMAL); }
  void SetVectorModeToVectorRotationOff() { this->SetVectorMode(VTK_VECTOR_ROTATION_OFF); }
  void SetVectorModeToFollowCameraDirection() { this->SetVectorMode(VTK_FOLLOW_CAMERA_DIRECTION); }
  const char* GetVectorModeAsString();
  ///@}

  ///@{
  /**
   * Set/Get point position glyphs will face towards. Used if vector mode is
   * VTK_FOLLOW_CAMERA_DIRECTION.
   */
  vtkSetVectorMacro(FollowedCameraPosition, double, 3);
  vtkGetVectorMacro(FollowedCameraPosition, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get glyphs up direction. Used if vector mode is VTK_FOLLOW_CAMERA_DIRECTION.
   */
  vtkSetVectorMacro(FollowedCameraViewUp, double, 3);
  vtkGetVectorMacro(FollowedCameraViewUp, double, 3);
  ///@}

  ///@{
  /**
   * Index into table of sources by scalar, by vector/normal magnitude, or
   * no indexing. If indexing is turned off, then the first source glyph in
   * the table of glyphs is used. Note that indexing mode will only use the
   * InputScalarsSelection array and not the InputColorScalarsSelection
   * as the scalar source if an array is specified.
   */
  vtkSetMacro(IndexMode, int);
  vtkGetMacro(IndexMode, int);
  void SetIndexModeToScalar() { this->SetIndexMode(VTK_INDEXING_BY_SCALAR); }
  void SetIndexModeToVector() { this->SetIndexMode(VTK_INDEXING_BY_VECTOR); }
  void SetIndexModeToOff() { this->SetIndexMode(VTK_INDEXING_OFF); }
  const char* GetIndexModeAsString();
  ///@}

  ///@{
  /**
   * Enable/disable the generation of point ids as part of the output. The
   * point ids are the id of the input generating point. The point ids are
   * stored in the output point field data and named "InputPointIds". Point
   * generation is useful for debugging and pick operations.
   */
  vtkSetMacro(GeneratePointIds, vtkTypeBool);
  vtkGetMacro(GeneratePointIds, vtkTypeBool);
  vtkBooleanMacro(GeneratePointIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the PointIds array if generated. By default the Ids
   * are named "InputPointIds", but this can be changed with this function.
   */
  vtkSetStringMacro(PointIdsName);
  vtkGetStringMacro(PointIdsName);
  ///@}

  ///@{
  /**
   * Enable/disable the generation of cell data as part of the output.
   * The cell data at each cell will match the point data of the input
   * at the glyphed point.
   */
  vtkSetMacro(FillCellData, vtkTypeBool);
  vtkGetMacro(FillCellData, vtkTypeBool);
  vtkBooleanMacro(FillCellData, vtkTypeBool);
  ///@}

  /**
   * This can be overwritten by subclass to return 0 when a point is
   * blanked. Default implementation is to always return 1;
   */
  virtual int IsPointVisible(vtkDataSet*, vtkIdType) { return 1; }

  ///@{
  /**
   * When set, this is use to transform the source polydata before using it to
   * generate the glyph. This is useful if one wanted to reorient the source,
   * for example.
   */
  void SetSourceTransform(vtkTransform*);
  vtkGetObjectMacro(SourceTransform, vtkTransform);
  ///@}

  /**
   * Overridden to include SourceTransform's MTime.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkGlyph3D();
  ~vtkGlyph3D() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  vtkPolyData* GetSource(int idx, vtkInformationVector* sourceInfo);

  ///@{
  /**
   * Method called in RequestData() to do the actual data processing. This will
   * glyph the \c input, filling up the \c output based on the filter
   * parameters.
   */
  virtual bool Execute(vtkDataSet* input, vtkInformationVector* sourceVector, vtkPolyData* output);
  virtual bool Execute(vtkDataSet* input, vtkInformationVector* sourceVector, vtkPolyData* output,
    vtkDataArray* inSScalars, vtkDataArray* inVectors);
  ///@}

  vtkPolyData** Source; // Geometry to copy to each point
  vtkTypeBool Scaling;  // Determine whether scaling of geometry is performed
  int ScaleMode;        // Scale by scalar value or vector magnitude
  int ColorMode;        // new scalars based on scale, scalar or vector
  double ScaleFactor;   // Scale factor to use to scale geometry
  double Range[2];      // Range to use to perform scalar scaling
  int Orient;           // boolean controls whether to "orient" data
  int VectorMode;       // Orient/scale via normal or via vector data
  double
    FollowedCameraPosition[3]; // glyphs face towards this point in VTK_FOLLOW_CAMERA_DIRECTION mode
  double FollowedCameraViewUp[3]; // glyph up direction in VTK_FOLLOW_CAMERA_DIRECTION mode
  vtkTypeBool Clamping;           // whether to clamp scale factor
  int IndexMode;                  // what to use to index into glyph table
  vtkTypeBool GeneratePointIds;   // produce input points ids for each output point
  vtkTypeBool FillCellData;       // whether to fill output cell data
  char* PointIdsName;
  vtkTransform* SourceTransform;
  int OutputPointsPrecision;

private:
  vtkGlyph3D(const vtkGlyph3D&) = delete;
  void operator=(const vtkGlyph3D&) = delete;
};

/**
 * Return the method of scaling as a descriptive character string.
 */
inline const char* vtkGlyph3D::GetScaleModeAsString()
{
  if (this->ScaleMode == VTK_SCALE_BY_SCALAR)
  {
    return "ScaleByScalar";
  }
  else if (this->ScaleMode == VTK_SCALE_BY_VECTOR)
  {
    return "ScaleByVector";
  }
  else
  {
    return "DataScalingOff";
  }
}

/**
 * Return the method of coloring as a descriptive character string.
 */
inline const char* vtkGlyph3D::GetColorModeAsString()
{
  if (this->ColorMode == VTK_COLOR_BY_SCALAR)
  {
    return "ColorByScalar";
  }
  else if (this->ColorMode == VTK_COLOR_BY_VECTOR)
  {
    return "ColorByVector";
  }
  else
  {
    return "ColorByScale";
  }
}

/**
 * Return the vector mode as a character string.
 */
inline const char* vtkGlyph3D::GetVectorModeAsString()
{
  if (this->VectorMode == VTK_USE_VECTOR)
  {
    return "UseVector";
  }
  else if (this->VectorMode == VTK_USE_NORMAL)
  {
    return "UseNormal";
  }
  else if (this->VectorMode == VTK_FOLLOW_CAMERA_DIRECTION)
  {
    return "FollowCameraDirection";
  }
  else
  {
    return "VectorRotationOff";
  }
}

/**
 * Return the index mode as a character string.
 */
inline const char* vtkGlyph3D::GetIndexModeAsString()
{
  if (this->IndexMode == VTK_INDEXING_OFF)
  {
    return "IndexingOff";
  }
  else if (this->IndexMode == VTK_INDEXING_BY_SCALAR)
  {
    return "IndexingByScalar";
  }
  else
  {
    return "IndexingByVector";
  }
}

VTK_ABI_NAMESPACE_END
#endif
