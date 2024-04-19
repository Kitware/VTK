// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAppendDataSets
 * @brief   Appends one or more datasets together into a single output vtkPointSet.
 *
 * vtkAppendDataSets is a filter that appends one of more datasets into a single output
 * point set. The type of the output is set with the OutputDataSetType option. Only inputs
 * that can be converted to the selected output dataset type are appended to the output.
 * By default, the output is vtkUnstructuredGrid, and all input types can be appended to it.
 * If the OutputDataSetType is set to produce vtkPolyData, then only datasets that can be
 * converted to vtkPolyData (i.e., vtkPolyData) are appended to the output.
 *
 * All cells are extracted and appended, but point and cell attributes (i.e., scalars,
 * vectors, normals, field data, etc.) are extracted and appended only if all datasets
 * have the same point and/or cell attributes available. (For example, if one dataset
 * has scalars but another does not, scalars will not be appended.)
 *
 * Points can be merged if MergePoints is set to true. In this case, points are
 * really merged if there are no ghost cells and no global point ids, or if
 * there are global point ids. In the case of the presence of global point ids,
 * the filter exclusively relies on those ids, not checking if points are
 * coincident. It assumes that the global ids were properly set. In the case of
 * the absence of global ids, points within Tolerance are merged.
 *
 * @sa
 * vtkAppendFilter vtkAppendPolyData
 */

#ifndef vtkAppendDataSets_h
#define vtkAppendDataSets_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetCollection;

class VTKFILTERSCORE_EXPORT vtkAppendDataSets : public vtkPointSetAlgorithm
{
public:
  static vtkAppendDataSets* New();
  vtkTypeMacro(vtkAppendDataSets, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set if the filter should merge coincidental points
   * Note: The filter will only merge points if the ghost cell array doesn't exist
   * Defaults to Off
   */
  vtkGetMacro(MergePoints, bool);
  vtkSetMacro(MergePoints, bool);
  vtkBooleanMacro(MergePoints, bool);
  ///@}

  ///@{
  /**
   * Get/Set the tolerance to use to find coincident points when `MergePoints`
   * is `true`. Default is 0.0.
   *
   * This is simply passed on to the internal vtkLocator used to merge points.
   * @sa `vtkLocator::SetTolerance`.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Get/Set whether Tolerance is treated as an absolute or relative tolerance.
   * The default is to treat it as an absolute tolerance. When off, the
   * tolerance is multiplied by the diagonal of the bounding box of the input.
   */
  vtkSetMacro(ToleranceIsAbsolute, bool);
  vtkGetMacro(ToleranceIsAbsolute, bool);
  vtkBooleanMacro(ToleranceIsAbsolute, bool);
  ///@}

  ///@{
  /**
   * Get/Set the output type produced by this filter. Only input datasets compatible with the
   * output type will be merged in the output. For example, if the output type is vtkPolyData, then
   * blocks of type vtkImageData, vtkStructuredGrid, etc. will not be merged - only vtkPolyData
   * can be merged into a vtkPolyData. On the other hand, if the output type is
   * vtkUnstructuredGrid, then blocks of almost any type will be merged in the output.
   * Valid values are VTK_POLY_DATA and VTK_UNSTRUCTURED_GRID defined in vtkType.h.
   * Defaults to VTK_UNSTRUCTURED_GRID.
   */
  vtkSetMacro(OutputDataSetType, int);
  vtkGetMacro(OutputDataSetType, int);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkAppendDataSets();
  ~vtkAppendDataSets() override;

  // Usual data generation method
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // If true we will attempt to merge points. Must also not have
  // ghost cells defined.
  bool MergePoints;

  // Tolerance used for point merging
  double Tolerance;

  // If true, tolerance is used as is. If false, tolerance is multiplied by
  // the diagonal of the bounding box of the input.
  bool ToleranceIsAbsolute;

  // Output data set type.
  int OutputDataSetType;

  // Precision of output points.
  int OutputPointsPrecision;

private:
  vtkAppendDataSets(const vtkAppendDataSets&) = delete;
  void operator=(const vtkAppendDataSets&) = delete;

  // Get all input data sets that have points, cells, or both.
  // Caller must delete the returned vtkDataSetCollection.
  vtkDataSetCollection* GetNonEmptyInputs(vtkInformationVector** inputVector);
};

VTK_ABI_NAMESPACE_END
#endif
