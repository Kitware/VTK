// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCutter
 * @brief   Cut vtkDataSet with user-specified implicit function
 *
 * vtkCutter is a filter to cut through data using any subclass of
 * vtkImplicitFunction. That is, a polygonal surface is created
 * corresponding to the implicit function F(x,y,z) = value(s), where
 * you can specify one or more values used to cut with.
 *
 * In VTK, cutting means reducing a cell of dimension N to a cut surface
 * of dimension N-1. For example, a tetrahedron when cut by a plane (i.e.,
 * vtkPlane implicit function) will generate triangles. (In comparison,
 * clipping takes a N dimensional cell and creates N dimension primitives.)
 *
 * vtkCutter is generally used to "slice-through" a dataset, generating
 * a surface that can be visualized. It is also possible to use vtkCutter
 * to do a form of volume rendering. vtkCutter does this by generating
 * multiple cut surfaces (usually planes) which are ordered (and rendered)
 * from back-to-front. The surfaces are set translucent to give a
 * volumetric rendering effect.
 *
 * Note that data can be cut using either 1) the scalar values associated
 * with the dataset or 2) an implicit function associated with this class.
 * By default, if an implicit function is set it is used to clip the data
 * set, otherwise the dataset scalars are used to perform the clipping.
 *
 * Note that this class delegates to vtkPlaneCutter whenever possible since
 * it's specialized for planes and it's faster because it's multithreaded, and in some
 * cases also algorithmically faster.
 *
 * @sa
 * vtkImplicitFunction vtkClipPolyData vtkPlaneCutter
 */

#ifndef vtkCutter_h
#define vtkCutter_h

#include "vtkDeprecation.h"       // For VTK_DEPRECATED_IN_9_4_0
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for inline methods

#define VTK_SORT_BY_VALUE 0
#define VTK_SORT_BY_CELL 1

VTK_ABI_NAMESPACE_BEGIN
class vtkGridSynchronizedTemplates3D;
class vtkImplicitFunction;
class vtkIncrementalPointLocator;
class vtkPlaneCutter;
class vtkRectilinearSynchronizedTemplates;
class vtkSynchronizedTemplates3D;
class vtkSynchronizedTemplatesCutter3D;

class VTKFILTERSCORE_EXPORT vtkCutter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCutter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with user-specified implicit function; initial value of 0.0; and
   * generating cut scalars turned off.
   */
  static vtkCutter* New();

  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0<=i<NumberOfContours.
   */
  void SetValue(int i, double value) { this->ContourValues->SetValue(i, value); }

  /**
   * Get the ith contour value.
   */
  double GetValue(int i) { return this->ContourValues->GetValue(i); }

  /**
   * Get a pointer to an array of contour values. There will be
   * GetNumberOfContours() values in the list.
   */
  double* GetValues() { return this->ContourValues->GetValues(); }

  /**
   * Fill a supplied list with contour values. There will be
   * GetNumberOfContours() values in the list. Make sure you allocate
   * enough memory to hold the list.
   */
  void GetValues(double* contourValues) { this->ContourValues->GetValues(contourValues); }

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(int number) { this->ContourValues->SetNumberOfContours(number); }

  /**
   * Get the number of contours in the list of contour values.
   */
  vtkIdType GetNumberOfContours() { return this->ContourValues->GetNumberOfContours(); }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2])
  {
    this->ContourValues->GenerateValues(numContours, range);
  }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
  }

  /**
   * Override GetMTime because we delegate to vtkContourValues and refer to
   * vtkImplicitFunction.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify the implicit function to perform the cutting.
   */
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * If this flag is enabled, then the output scalar values will be
   * interpolated from the implicit function values, and not the input scalar
   * data.
   */
  vtkSetMacro(GenerateCutScalars, vtkTypeBool);
  vtkGetMacro(GenerateCutScalars, vtkTypeBool);
  vtkBooleanMacro(GenerateCutScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If this is enabled (by default), the output will be triangles
   * otherwise, the output will be the intersection polygons
   * WARNING: if the cutting function is not a plane, the output
   * will be 3D polygons, which might be nice to look at but hard
   * to compute with downstream.
   */
  vtkSetMacro(GenerateTriangles, vtkTypeBool);
  vtkGetMacro(GenerateTriangles, vtkTypeBool);
  vtkBooleanMacro(GenerateTriangles, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify a spatial locator for merging points. By default,
   * an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  ///@{
  /**
   * Set the sorting order for the generated polydata. There are two
   * possibilities:
   * Sort by value = 0 - This is the most efficient sort. For each cell,
   * all contour values are processed. This is the default.
   * Sort by cell = 1 - For each contour value, all cells are processed.
   * This order should be used if the extracted polygons must be rendered
   * in a back-to-front or front-to-back order. This is very problem
   * dependent.
   * For most applications, the default order is fine (and faster).

   * Sort by cell is going to have a problem if the input has 2D and 3D cells.
   * Cell data will be scrambled because with
   * vtkPolyData output, verts and lines have lower cell ids than triangles.
   */
  vtkSetClampMacro(SortBy, int, VTK_SORT_BY_VALUE, VTK_SORT_BY_CELL);
  vtkGetMacro(SortBy, int);
  void SetSortByToSortByValue() { this->SetSortBy(VTK_SORT_BY_VALUE); }
  void SetSortByToSortByCell() { this->SetSortBy(VTK_SORT_BY_CELL); }
  const char* GetSortByAsString();
  ///@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Normally I would put this in a different class, but since
   * This is a temporary fix until we convert this class and contour filter
   * to generate unstructured grid output instead of poly data, I am leaving it here.
   */
  VTK_DEPRECATED_IN_9_4_0("This is no longer used. Use vtkCellTypes::GetDimension(type) instead.")
  static void GetCellTypeDimensions(unsigned char* cellTypeDimensions);

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkCutter(vtkImplicitFunction* cf = nullptr);
  ~vtkCutter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  void UnstructuredGridCutter(vtkDataSet* input, vtkPolyData* output);
  void DataSetCutter(vtkDataSet* input, vtkPolyData* output);
  void StructuredPointsCutter(
    vtkDataSet*, vtkPolyData*, vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  void StructuredGridCutter(vtkDataSet*, vtkPolyData*);
  void RectilinearGridCutter(vtkDataSet*, vtkPolyData*);
  vtkImplicitFunction* CutFunction;
  vtkTypeBool GenerateTriangles;

  vtkNew<vtkSynchronizedTemplates3D> SynchronizedTemplates3D;
  vtkNew<vtkSynchronizedTemplatesCutter3D> SynchronizedTemplatesCutter3D;
  vtkNew<vtkGridSynchronizedTemplates3D> GridSynchronizedTemplates;
  vtkNew<vtkRectilinearSynchronizedTemplates> RectilinearSynchronizedTemplates;
  vtkNew<vtkPlaneCutter> PlaneCutter;

  vtkIncrementalPointLocator* Locator;
  int SortBy;
  vtkNew<vtkContourValues> ContourValues;
  vtkTypeBool GenerateCutScalars;
  int OutputPointsPrecision;

  // Garbage collection method
  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkCutter(const vtkCutter&) = delete;
  void operator=(const vtkCutter&) = delete;
};

/**
 * Return the sorting procedure as a descriptive character string.
 */
inline const char* vtkCutter::GetSortByAsString()
{
  if (this->SortBy == VTK_SORT_BY_VALUE)
  {
    return "SortByValue";
  }
  else
  {
    return "SortByCell";
  }
}

VTK_ABI_NAMESPACE_END
#endif
