// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericContourFilter
 * @brief   generate isocontours from input dataset
 *
 * vtkGenericContourFilter is a filter that takes as input any (generic)
 * dataset and generates on output isosurfaces and/or isolines. The exact
 * form of the output depends upon the dimensionality of the input data.
 * Data consisting of 3D cells will generate isosurfaces, data consisting of
 * 2D cells will generate isolines, and data with 1D or 0D cells will
 * generate isopoints. Combinations of output type are possible if the input
 * dimension is mixed.
 *
 * To use this filter you must specify one or more contour values.
 * You can either use the method SetValue() to specify each contour
 * value, or use GenerateValues() to generate a series of evenly
 * spaced contours. You can use ComputeNormalsOn to compute the normals
 * without the need of a vtkPolyDataNormals
 *
 * This filter has been implemented to operate on generic datasets, rather
 * than the typical vtkDataSet (and subclasses). vtkGenericDataSet is a more
 * complex cousin of vtkDataSet, typically consisting of nonlinear,
 * higher-order cells. To process this type of data, generic cells are
 * automatically tessellated into linear cells prior to isocontouring.
 *
 * @sa
 * vtkContourFilter vtkGenericDataSet
 */

#ifndef vtkGenericContourFilter_h
#define vtkGenericContourFilter_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkContourValues;
class vtkIncrementalPointLocator;
class vtkPointData;
class vtkCellData;

class VTKFILTERSGENERIC_EXPORT vtkGenericContourFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkGenericContourFilter, vtkPolyDataAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with initial range (0,1) and single contour value
   * of 0.0.
   */
  static vtkGenericContourFilter* New();

  typedef double PointType[3]; // Arbitrary definition of a point

  ///@{
  /**
   * Methods to set / get contour values.
   */
  void SetValue(int i, float value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* contourValues);
  void SetNumberOfContours(int number);
  vtkIdType GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  ///@}

  /**
   * Modified GetMTime Because we delegate to vtkContourValues
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be
   * processed by filters that modify topology or geometry, it may be
   * wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals, vtkTypeBool);
  vtkGetMacro(ComputeNormals, vtkTypeBool);
  vtkBooleanMacro(ComputeNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of gradients. Gradient computation is
   * fairly expensive in both time and storage. Note that if
   * ComputeNormals is on, gradients will have to be calculated, but
   * will not be stored in the output dataset.  If the output data
   * will be processed by filters that modify topology or geometry, it
   * may be wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeGradients, vtkTypeBool);
  vtkGetMacro(ComputeGradients, vtkTypeBool);
  vtkBooleanMacro(ComputeGradients, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of scalars.
   */
  vtkSetMacro(ComputeScalars, vtkTypeBool);
  vtkGetMacro(ComputeScalars, vtkTypeBool);
  vtkBooleanMacro(ComputeScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  ///@{
  /**
   * If you want to contour by an arbitrary scalar attribute, then set its
   * name here.
   * By default this in nullptr and the filter will use the active scalar array.
   */
  vtkGetStringMacro(InputScalarsSelection);
  virtual void SelectInputScalars(const char* fieldName);
  ///@}

protected:
  vtkGenericContourFilter();
  ~vtkGenericContourFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int, vtkInformation*) override;

  vtkContourValues* ContourValues;
  vtkTypeBool ComputeNormals;
  vtkTypeBool ComputeGradients;
  vtkTypeBool ComputeScalars;
  vtkIncrementalPointLocator* Locator;

  char* InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  // Used internal by vtkGenericAdaptorCell::Contour()
  vtkPointData* InternalPD;
  vtkPointData* SecondaryPD;
  vtkCellData* SecondaryCD;

private:
  vtkGenericContourFilter(const vtkGenericContourFilter&) = delete;
  void operator=(const vtkGenericContourFilter&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
