// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkContourFilter
 * @brief   generate isosurfaces/isolines from scalar values
 *
 * vtkContourFilter is a filter that takes as input any dataset and
 * generates on output isosurfaces and/or isolines. The exact form
 * of the output depends upon the dimensionality of the input data.
 * Data consisting of 3D cells will generate isosurfaces, data
 * consisting of 2D cells will generate isolines, and data with 1D
 * or 0D cells will generate isopoints. Combinations of output type
 * are possible if the input dimension is mixed.
 *
 * To use this filter you must specify one or more contour values.
 * You can either use the method SetValue() to specify each contour
 * value, or use GenerateValues() to generate a series of evenly
 * spaced contours. It is also possible to accelerate the operation of
 * this filter (at the cost of extra memory) by using a
 * vtkScalarTree. A scalar tree is used to quickly locate cells that
 * contain a contour surface. This is especially effective if multiple
 * contours are being extracted. If you want to use a scalar tree,
 * invoke the method UseScalarTreeOn().
 *
 * @warning
 * For unstructured data or structured grids, normals and gradients
 * are not computed. Use vtkPolyDataNormals to compute the surface
 * normals.
 *
 * @sa
 * vtkFlyingEdges3D vtkFlyingEdges2D vtkDiscreteFlyingEdges3D
 * vtkDiscreteFlyingEdges2D vtkMarchingContourFilter vtkMarchingCubes
 * vtkSliceCubes vtkMarchingSquares vtkImageMarchingCubes vtkContour3DLinearGrid
 */

#ifndef vtkContourFilter_h
#define vtkContourFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN

class vtkCallbackCommand;
class vtkContour3DLinearGrid;
class vtkContourGrid;
class vtkFlyingEdges2D;
class vtkFlyingEdges3D;
class vtkGridSynchronizedTemplates3D;
class vtkIncrementalPointLocator;
class vtkRectilinearSynchronizedTemplates;
class vtkScalarTree;
class vtkSynchronizedTemplates2D;
class vtkSynchronizedTemplates3D;

class VTKFILTERSCORE_EXPORT vtkContourFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkContourFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with initial range (0,1) and single contour value
   * of 0.0.
   */
  static vtkContourFilter* New();

  ///@{
  /**
   * Methods to set / get contour values.
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
   * Modified GetMTime Because we delegate to vtkContourValues
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be
   * processed by filters that modify topology or geometry, it may be
   * wise to turn Normals and Gradients off.
   * This setting defaults to On for vtkImageData, vtkRectilinearGrid,
   * vtkStructuredGrid, and vtkUnstructuredGrid inputs.
   * For others, it defaults to the special value -1 which indicates
   * that the caller has made no explicit choice and will result in
   * the normals being computed. This behaviour is a holdover for
   * backwards compatibility and you really should set this to 0 or 1.
   */
  vtkSetMacro(ComputeNormals, int);
  vtkGetMacro(ComputeNormals, int);
  vtkBooleanMacro(ComputeNormals, int);
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
   * Enable the use of a scalar tree to accelerate contour extraction. By
   * default, an instance of vtkSpanSpace is created when needed.
   */
  vtkSetMacro(UseScalarTree, vtkTypeBool);
  vtkGetMacro(UseScalarTree, vtkTypeBool);
  vtkBooleanMacro(UseScalarTree, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable the use of a scalar tree to accelerate contour extraction.
   */
  virtual void SetScalarTree(vtkScalarTree*);
  vtkGetObjectMacro(ScalarTree, vtkScalarTree);
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
   * Set/get which component of the scalar array to contour on; defaults to 0.
   * Currently this feature only works if the input is a vtkImageData.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  ///@}

  ///@{
  /**
   * If this is enabled (by default), the output will be triangles
   * otherwise, the output will be the intersection polygon
   * WARNING: if the contour surface is not planar, the output
   * polygon will not be planar, which might be nice to look at but hard
   * to compute with downstream.
   */
  vtkSetMacro(GenerateTriangles, vtkTypeBool);
  vtkGetMacro(GenerateTriangles, vtkTypeBool);
  vtkBooleanMacro(GenerateTriangles, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Turn on/off fast mode execution. If enabled, fast mode typically runs
   * way faster because the internal algorithm FlyingEdges is multithreaded and the algorithm has
   * performance optimizations, but is does not remove degenerate triangles. FastMode is only
   * meaningful when the input is vtkImageData and GenerateTriangles is on.
   *
   * Default is off.
   */
  vtkSetMacro(FastMode, bool);
  vtkGetMacro(FastMode, bool);
  vtkBooleanMacro(FastMode, bool);
  ///@}

  /**
   * Sets the name of the input array to be used for generating
   * the isosurfaces. This is a convenience method and it calls
   * SetInputArrayToProcess().
   */
  void SetInputArray(const std::string& name)
  {
    this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name.c_str());
  }

protected:
  vtkContourFilter();
  ~vtkContourFilter() override;

  void ReportReferences(vtkGarbageCollector*) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkNew<vtkContourValues> ContourValues;
  int ComputeNormals;
  vtkTypeBool ComputeGradients;
  vtkTypeBool ComputeScalars;
  vtkIncrementalPointLocator* Locator;
  vtkTypeBool UseScalarTree;
  vtkScalarTree* ScalarTree;
  int OutputPointsPrecision;
  int ArrayComponent;
  vtkTypeBool GenerateTriangles;
  bool FastMode;

  vtkNew<vtkContourGrid> ContourGrid;
  vtkNew<vtkContour3DLinearGrid> Contour3DLinearGrid;
  vtkNew<vtkFlyingEdges2D> FlyingEdges2D;
  vtkNew<vtkFlyingEdges3D> FlyingEdges3D;
  vtkNew<vtkGridSynchronizedTemplates3D> GridSynchronizedTemplates;
  vtkNew<vtkRectilinearSynchronizedTemplates> RectilinearSynchronizedTemplates;
  vtkNew<vtkSynchronizedTemplates2D> SynchronizedTemplates2D;
  vtkNew<vtkSynchronizedTemplates3D> SynchronizedTemplates3D;
  vtkNew<vtkCallbackCommand> InternalProgressCallbackCommand;

  static void InternalProgressCallbackFunction(
    vtkObject* caller, unsigned long eid, void* clientData, void* callData);

private:
  vtkContourFilter(const vtkContourFilter&) = delete;
  void operator=(const vtkContourFilter&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void vtkContourFilter::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double vtkContourFilter::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* vtkContourFilter::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void vtkContourFilter::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void vtkContourFilter::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline int vtkContourFilter::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkContourFilter::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkContourFilter::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

/**
 * Convenience method to set all of the contour values at once.
 * Loops over the vector elements and calls SetValue()
 */
inline void vtkContourFilter::SetContourValues(const std::vector<double>& values)
{
  int numContours = static_cast<int>(values.size());
  this->SetNumberOfContours(numContours);
  for (int i = 0; i < numContours; i++)
  {
    this->SetValue(i, values[i]);
  }
}

/**
 * Convenience method to get all of the contour values at once.
 * The returned vector is a copy and cannot be used to modify
 * contour values.
 */
inline std::vector<double> vtkContourFilter::GetContourValues()
{
  std::vector<double> contours;
  int numContours = this->GetNumberOfContours();
  contours.reserve(numContours);
  for (int i = 0; i < numContours; i++)
  {
    contours.push_back(this->GetValue(i));
  }
  return contours;
}

VTK_ABI_NAMESPACE_END
#endif
