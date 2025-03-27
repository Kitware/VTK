// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAppendFilter
 * @brief   appends one or more datasets together into a single unstructured grid
 *
 * vtkAppendFilter is a filter that appends one of more datasets into a single
 * unstructured grid. All geometry is extracted and appended, but point
 * attributes (i.e., scalars, vectors, normals, field data, etc.) are extracted
 * and appended only if all datasets have the point attributes available.
 * (For example, if one dataset has scalars but another does not, scalars will
 * not be appended.)
 *
 * You can decide to merge points that are coincident by setting
 * `MergePoints`. If this flag is set, points are merged if they are within
 * `Tolerance` radius. If a point global id array is available (point data named
 * "GlobalPointIds"), then two points are merged if they share the same point global id,
 * without checking for coincident point.
 *
 * @sa
 * vtkAppendPolyData
 */

#ifndef vtkAppendFilter_h
#define vtkAppendFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSetAttributes;
class vtkDataSetCollection;

class VTKFILTERSCORE_EXPORT vtkAppendFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkAppendFilter* New();
  vtkTypeMacro(vtkAppendFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get any input of this filter.
   */
  vtkDataSet* GetInput(int idx);
  vtkDataSet* GetInput() { return this->GetInput(0); }
  ///@}

  ///@{
  /**
   * Get/Set if the filter should merge coincidental points
   * Note: The filter will only merge points if the ghost cell array doesn't exist
   * Defaults to Off
   */
  vtkGetMacro(MergePoints, vtkTypeBool);
  vtkSetMacro(MergePoints, vtkTypeBool);
  vtkBooleanMacro(MergePoints, vtkTypeBool);
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

  /**
   * Remove a dataset from the list of data to append.
   */
  void RemoveInputData(vtkDataSet* in);

  /**
   * Returns a copy of the input array.  Modifications to this list
   * will not be reflected in the actual inputs.
   */
  vtkDataSetCollection* GetInputList();

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkAppendFilter();
  ~vtkAppendFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // list of data sets to append together.
  // Here as a convenience.  It is a copy of the input array.
  vtkDataSetCollection* InputList;

  // If true we will attempt to merge points. Must also not have
  // ghost cells defined.
  vtkTypeBool MergePoints;

  int OutputPointsPrecision;
  double Tolerance;

  // If true, tolerance is used as is. If false, tolerance is multiplied by
  // the diagonal of the bounding box of the input.
  bool ToleranceIsAbsolute;

private:
  vtkAppendFilter(const vtkAppendFilter&) = delete;
  void operator=(const vtkAppendFilter&) = delete;

  // Get all input data sets that have points, cells, or both.
  // Caller must delete the returned vtkDataSetCollection.
  std::vector<vtkSmartPointer<vtkDataSet>> GetNonEmptyInputs(vtkInformationVector** inputVector);

  void AppendArrays(int attributesType, vtkInformationVector** inputVector,
    vtkUnstructuredGrid* output, vtkIdType totalNumberOfElements);
};

VTK_ABI_NAMESPACE_END
#endif
