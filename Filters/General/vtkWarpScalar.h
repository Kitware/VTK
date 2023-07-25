// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWarpScalar
 * @brief   deform geometry with scalar data
 *
 * vtkWarpScalar is a filter that modifies point coordinates by moving
 * points along point normals by the scalar amount times the scale factor.
 * Useful for creating carpet or x-y-z plots.
 *
 * If normals are not present in data, the Normal instance variable will
 * be used as the direction along which to warp the geometry. If normals are
 * present but you would like to use the Normal instance variable, set the
 * UseNormal boolean to true.
 *
 * If XYPlane boolean is set true, then the z-value is considered to be
 * a scalar value (still scaled by scale factor), and the displacement is
 * along the z-axis. If scalars are also present, these are copied through
 * and can be used to color the surface.
 *
 * Note that the filter passes both its point data and cell data to
 * its output, except for normals, since these are distorted by the
 * warping.
 */

#ifndef vtkWarpScalar_h
#define vtkWarpScalar_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkIdTypeArray;
class vtkPointSet;
class vtkUnsignedCharArray;

class VTKFILTERSGENERAL_EXPORT vtkWarpScalar : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information,
   * and printing.
   */
  static vtkWarpScalar* New();
  vtkTypeMacro(vtkWarpScalar, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify value to scale displacement.
   */
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Turn on/off use of user specified normal. If on, data normals
   * will be ignored and instance variable Normal will be used instead.
   */
  vtkSetMacro(UseNormal, vtkTypeBool);
  vtkGetMacro(UseNormal, vtkTypeBool);
  vtkBooleanMacro(UseNormal, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Normal (i.e., direction) along which to warp geometry. Only used
   * if UseNormal boolean set to true or no normals available in data.
   */
  vtkSetVector3Macro(Normal, double);
  vtkGetVectorMacro(Normal, double, 3);
  ///@}

  ///@{
  /**
   * Turn on/off flag specifying that input data is x-y plane. If x-y plane,
   * then the z value is used to warp the surface in the z-axis direction
   * (times the scale factor) and scalars are used to color the surface.
   */
  vtkSetMacro(XYPlane, vtkTypeBool);
  vtkGetMacro(XYPlane, vtkTypeBool);
  vtkBooleanMacro(XYPlane, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points type. By default
   * (DEFAULT_PRECISION) the output type is SINGLE_PRECISION, otherwise it is
   * either SINGLE_PRECISION or DOUBLE_PRECISION as specified by the user.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Set/Get for a property flag that makes the filter include the input data set in the output and
   * connects the output surface to the input surface through "side walls" effectively making a
   * single grid enclosing a volume.
   *
   * In order to use this functionality correctly, the input must be a two dimensional surface and
   * cannot be 3D.
   */
  vtkGetMacro(GenerateEnclosure, bool);
  vtkSetMacro(GenerateEnclosure, bool);
  vtkBooleanMacro(GenerateEnclosure, bool);
  ///@}

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkWarpScalar();
  ~vtkWarpScalar() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Check the topological dimension of the data set (only used when SideWallsActive)
   */
  unsigned int GetInputDimension(vtkDataSet* input);
  /**
   * When GenerateEnclosure is active, construct the cells between the base and warped surfaces
   */
  void BuildSideWalls(vtkPointSet* output, int nInputPoints, vtkUnsignedCharArray* boundaryCells,
    vtkIdTypeArray* boundaryFaceIndexes);
  /**
   * Append the values in the arrays of the array attribute collection to themselves
   */
  void AppendArrays(vtkDataSetAttributes* setData);

  double ScaleFactor;
  vtkTypeBool UseNormal;
  double Normal[3];
  vtkTypeBool XYPlane;
  int OutputPointsPrecision;
  bool GenerateEnclosure = false;

private:
  vtkWarpScalar(const vtkWarpScalar&) = delete;
  void operator=(const vtkWarpScalar&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
