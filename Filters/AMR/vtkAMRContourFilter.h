// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRContourFilter
 * @brief   A contour filter for vtkOverlappingAMR data
 *
 * This filters generate a perfectly watertight contour on vtkOverlappingAMR data
 * by creating an interface between non-refined and refined grid of an AMR and running
 * a vtkContourFilter on it.
 *
 * @sa vtkOverlappingAMR vtkAMRInterfaceFilter
 */
#ifndef vtkAMRContourFilter_h
#define vtkAMRContourFilter_h

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkPartitionedDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAMRInterfaceFilter;
class vtkCallbackCommand;
class vtkCartesianGrid;
class vtkCellArray;
class vtkCellData;
class vtkContourFilter;
class vtkDataSet;
class vtkMergePoints;
class vtkOverlappingAMR;
class vtkPointData;
class vtkUnsignedCharArray;
class VTKFILTERSAMR_EXPORT vtkAMRContourFilter : public vtkPartitionedDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkAMRContourFilter, vtkPartitionedDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAMRContourFilter* New();

  ///@{
  /**
   * Methods to set / get contour values, forwarded to the internal vtkContourFilter
   */
  void SetValue(int i, double value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  void SetContourValues(const std::vector<double>& values);
  std::vector<double> GetContourValues();
  ///@}

  /**
   * Modified GetMTime Because we delegate to the internal vtkContourFilter
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the computation of normals.
   * Forwarded to the internal vtkContourFilter.
   */
  void SetComputeNormals(bool val);
  bool GetComputeNormals();
  vtkBooleanMacro(ComputeNormals, bool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of scalars.
   * Forwarded to the internal vtkContourFilter.
   */
  void SetComputeScalars(bool val);
  bool GetComputeScalars();
  vtkBooleanMacro(ComputeScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If this is enabled the output will be triangles
   * otherwise, the output will be the intersection polygon.
   * Forwarded to the internal vtkContourFilter.
   */
  void SetGenerateTriangles(bool val);
  bool GetGenerateTriangles();
  vtkBooleanMacro(GenerateTriangles, bool);
  ///@}

protected:
  vtkAMRContourFilter();
  ~vtkAMRContourFilter() override;

  /**
   * Set input to vtkOverlappingAMR
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Implement the AMR contouring logic, see class documentation for details
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAMRContourFilter(const vtkAMRContourFilter&) = delete;
  void operator=(const vtkAMRContourFilter&) = delete;

  /**
   * Run provided contour filter using provided dataset as input and add at index idx to the
   * provided output
   */
  static bool ContourDataSet(
    vtkContourFilter* contour, vtkDataSet* ds, unsigned int idx, vtkPartitionedDataSet* output);

  /**
   * Remove all empty partitions from the provided output
   */
  static void CleanupOutput(vtkPartitionedDataSet* output);

  ///@{
  /**
   * Progress Handling
   */
  static void InternalProgressCallbackFunction(
    vtkObject* arg, unsigned long, void* clientdata, void*);
  void InternalProgressCallback(vtkAlgorithm* algorithm);
  ///@}

  vtkNew<vtkAMRInterfaceFilter> InternalInterface;
  vtkNew<vtkContourFilter> InternalContour;

  // Progress handling
  vtkNew<vtkCallbackCommand> InternalProgressObserver;
  double ProgressFloor = 0.;
  double ProgressCeiling = 1.;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAMRContourFilter_h
