// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkReflectionFilter
 * @brief   reflects a data set across a plane
 *
 * The vtkReflectionFilter reflects a data set across one of the
 * planes formed by the data set's bounding box.
 * Since it converts data sets into unstructured grids, it is not efficient
 * for structured data sets.
 */

#ifndef vtkReflectionFilter_h
#define vtkReflectionFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGrid;
class vtkDataSet;

class VTKFILTERSGENERAL_EXPORT vtkReflectionFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkReflectionFilter* New();

  vtkTypeMacro(vtkReflectionFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ReflectionPlane
  {
    USE_X_MIN = 0,
    USE_Y_MIN = 1,
    USE_Z_MIN = 2,
    USE_X_MAX = 3,
    USE_Y_MAX = 4,
    USE_Z_MAX = 5,
    USE_X = 6,
    USE_Y = 7,
    USE_Z = 8
  };

  ///@{
  /**
   * Set the normal of the plane to use as mirror.
   */
  vtkSetClampMacro(Plane, int, 0, 8);
  vtkGetMacro(Plane, int);
  void SetPlaneToX() { this->SetPlane(USE_X); }
  void SetPlaneToY() { this->SetPlane(USE_Y); }
  void SetPlaneToZ() { this->SetPlane(USE_Z); }
  void SetPlaneToXMin() { this->SetPlane(USE_X_MIN); }
  void SetPlaneToYMin() { this->SetPlane(USE_Y_MIN); }
  void SetPlaneToZMin() { this->SetPlane(USE_Z_MIN); }
  void SetPlaneToXMax() { this->SetPlane(USE_X_MAX); }
  void SetPlaneToYMax() { this->SetPlane(USE_Y_MAX); }
  void SetPlaneToZMax() { this->SetPlane(USE_Z_MAX); }
  ///@}

  ///@{
  /**
   * If the reflection plane is set to X, Y or Z, this variable
   * is use to set the position of the plane.
   */
  vtkSetMacro(Center, double);
  vtkGetMacro(Center, double);
  ///@}

  ///@{
  /**
   * If on (the default), copy the input geometry to the output. If off,
   * the output will only contain the reflection.
   */
  vtkSetMacro(CopyInput, vtkTypeBool);
  vtkGetMacro(CopyInput, vtkTypeBool);
  vtkBooleanMacro(CopyInput, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If off (the default), only Vectors, Normals and Tensors will be flipped.
   * If on, all 3-component data arrays ( considered as 3D vectors),
   * 6-component data arrays (considered as symmetric tensors),
   * 9-component data arrays (considered as tensors ) of signed type will be flipped.
   * All other won't be flipped and will only be copied.
   */
  vtkSetMacro(FlipAllInputArrays, bool);
  vtkGetMacro(FlipAllInputArrays, bool);
  vtkBooleanMacro(FlipAllInputArrays, bool);
  ///@}

protected:
  vtkReflectionFilter();
  ~vtkReflectionFilter() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   * Overridden to create the correct type of output.
   */
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Actual implementation for reflection.
   */
  virtual int RequestDataInternal(vtkDataSet* input, vtkUnstructuredGrid* output, double bounds[6]);

  /**
   * Internal method to compute bounds.
   */
  virtual int ComputeBounds(vtkDataObject* input, double bounds[6]);

  /**
   * Generate new, non-3D cell and return the generated cells id.
   */
  virtual vtkIdType ReflectNon3DCell(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numInputPoints);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  void FlipTuple(double* tuple, int* mirrorDir, int nComp);

  int Plane;
  double Center;
  vtkTypeBool CopyInput;
  bool FlipAllInputArrays;

private:
  vtkReflectionFilter(const vtkReflectionFilter&) = delete;
  void operator=(const vtkReflectionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
