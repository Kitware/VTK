// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTransformToGrid
 * @brief   create a grid for a vtkGridTransform
 *
 * vtkTransformToGrid takes any transform as input and produces a grid
 * for use by a vtkGridTransform.  This can be used, for example, to
 * invert a grid transform, concatenate two grid transforms, or to
 * convert a thin plate spline transform into a grid transform.
 * @sa
 * vtkGridTransform vtkThinPlateSplineTransform vtkAbstractTransform
 */

#ifndef vtkTransformToGrid_h
#define vtkTransformToGrid_h

#include "vtkAlgorithm.h"
#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkImageData.h"           // makes things a bit easier

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractTransform;

class VTKFILTERSHYBRID_EXPORT vtkTransformToGrid : public vtkAlgorithm
{
public:
  static vtkTransformToGrid* New();
  vtkTypeMacro(vtkTransformToGrid, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the transform which will be converted into a grid.
   */
  virtual void SetInput(vtkAbstractTransform*);
  vtkGetObjectMacro(Input, vtkAbstractTransform);
  ///@}

  ///@{
  /**
   * Get/Set the extent of the grid.
   */
  vtkSetVector6Macro(GridExtent, int);
  vtkGetVector6Macro(GridExtent, int);
  ///@}

  ///@{
  /**
   * Get/Set the origin of the grid.
   */
  vtkSetVector3Macro(GridOrigin, double);
  vtkGetVector3Macro(GridOrigin, double);
  ///@}

  ///@{
  /**
   * Get/Set the spacing between samples in the grid.
   */
  vtkSetVector3Macro(GridSpacing, double);
  vtkGetVector3Macro(GridSpacing, double);
  ///@}

  ///@{
  /**
   * Get/Set the scalar type of the grid.  The default is float.
   */
  vtkSetMacro(GridScalarType, int);
  vtkGetMacro(GridScalarType, int);
  void SetGridScalarTypeToDouble() { this->SetGridScalarType(VTK_DOUBLE); }
  void SetGridScalarTypeToFloat() { this->SetGridScalarType(VTK_FLOAT); }
  void SetGridScalarTypeToShort() { this->SetGridScalarType(VTK_SHORT); }
  void SetGridScalarTypeToUnsignedShort() { this->SetGridScalarType(VTK_UNSIGNED_SHORT); }
  void SetGridScalarTypeToUnsignedChar() { this->SetGridScalarType(VTK_UNSIGNED_CHAR); }
  void SetGridScalarTypeToChar() { this->SetGridScalarType(VTK_CHAR); }
  ///@}

  ///@{
  /**
   * Get the scale and shift to convert integer grid elements into
   * real values:  dx = scale*di + shift.  If the grid is of double type,
   * then scale = 1 and shift = 0.
   */
  double GetDisplacementScale()
  {
    this->UpdateShiftScale();
    return this->DisplacementScale;
  }
  double GetDisplacementShift()
  {
    this->UpdateShiftScale();
    return this->DisplacementShift;
  }
  ///@}

  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkImageData* GetOutput();

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkTransformToGrid();
  ~vtkTransformToGrid() override;

  void RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  void RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * Internal method to calculate the shift and scale values which
   * will provide maximum grid precision for a particular integer type.
   */
  void UpdateShiftScale();

  vtkMTimeType GetMTime() override;

  vtkAbstractTransform* Input;

  int GridScalarType;
  int GridExtent[6];
  double GridOrigin[3];
  double GridSpacing[3];

  double DisplacementScale;
  double DisplacementShift;
  vtkTimeStamp ShiftScaleTime;

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkTransformToGrid(const vtkTransformToGrid&) = delete;
  void operator=(const vtkTransformToGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
