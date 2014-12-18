/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarchingContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkMarchingContourFilter is a filter that takes as input any dataset and
// generates on output isosurfaces and/or isolines. The exact form
// of the output depends upon the dimensionality of the input data.
// Data consisting of 3D cells will generate isosurfaces, data
// consisting of 2D cells will generate isolines, and data with 1D
// or 0D cells will generate isopoints. Combinations of output type
// are possible if the input dimension is mixed.
//
// This filter will identify special dataset types (e.g., structured
// points) and use the appropriate specialized filter to process the
// data. For examples, if the input dataset type is a volume, this
// filter will create an internal vtkMarchingCubes instance and use
// it. This gives much better performance.
//
// To use this filter you must specify one or more contour values.
// You can either use the method SetValue() to specify each contour
// value, or use GenerateValues() to generate a series of evenly
// spaced contours. It is also possible to accelerate the operation of
// this filter (at the cost of extra memory) by using a
// vtkScalarTree. A scalar tree is used to quickly locate cells that
// contain a contour surface. This is especially effective if multiple
// contours are being extracted. If you want to use a scalar tree,
// invoke the method UseScalarTreeOn().

// .SECTION Caveats
// For unstructured data or structured grids, normals and gradients
// are not computed.  This calculation will be implemented in the
// future. In the mean time, use vtkPolyDataNormals to compute the surface
// normals.

// .SECTION See Also
// vtkMarchingCubes vtkSliceCubes vtkDividingCubes vtkMarchingSquares
// vtkImageMarchingCubes

#ifndef vtkMarchingContourFilter_h
#define vtkMarchingContourFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for direct access to ContourValues

class vtkIncrementalPointLocator;
class vtkScalarTree;

class VTKFILTERSGENERAL_EXPORT vtkMarchingContourFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMarchingContourFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkMarchingContourFilter *New();

  // Description:
  // Methods to set / get contour values.
  void SetValue(int i, double value);
  double GetValue(int i);
  double *GetValues();
  void GetValues(double *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);

  // Description:
  // Modified GetMTime Because we delegate to vtkContourValues
  unsigned long GetMTime();

  // Description:
  // Set/Get the computation of normals. Normal computation is fairly
  // expensive in both time and storage. If the output data will be
  // processed by filters that modify topology or geometry, it may be
  // wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);

  // Description:
  // Set/Get the computation of gradients. Gradient computation is
  // fairly expensive in both time and storage. Note that if
  // ComputeNormals is on, gradients will have to be calculated, but
  // will not be stored in the output dataset.  If the output data
  // will be processed by filters that modify topology or geometry, it
  // may be wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeGradients,int);
  vtkGetMacro(ComputeGradients,int);
  vtkBooleanMacro(ComputeGradients,int);

  // Description:
  // Set/Get the computation of scalars.
  vtkSetMacro(ComputeScalars,int);
  vtkGetMacro(ComputeScalars,int);
  vtkBooleanMacro(ComputeScalars,int);

  // Description:
  // Enable the use of a scalar tree to accelerate contour extraction.
  vtkSetMacro(UseScalarTree,int);
  vtkGetMacro(UseScalarTree,int);
  vtkBooleanMacro(UseScalarTree,int);

  // Description:
  // Set / get a spatial locator for merging points. By default,
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is
  // specified. The locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkMarchingContourFilter();
  ~vtkMarchingContourFilter();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkContourValues *ContourValues;
  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  vtkIncrementalPointLocator *Locator;
  int UseScalarTree;
  vtkScalarTree *ScalarTree;

  //special contouring for structured points
  void StructuredPointsContour(int dim, vtkDataSet *input, vtkPolyData *output);
  //special contouring for image data
  void ImageContour(int dim, vtkDataSet *input, vtkPolyData *output);
  //default if not structured data
  void DataSetContour(vtkDataSet *input, vtkPolyData *output);
private:
  vtkMarchingContourFilter(const vtkMarchingContourFilter&);  // Not implemented.
  void operator=(const vtkMarchingContourFilter&);  // Not implemented.
};

// Description:
// Set a particular contour value at contour number i. The index i ranges
// between 0<=i<NumberOfContours.
inline void vtkMarchingContourFilter::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i,value);
}

// Description:
// Get the ith contour value.
inline double vtkMarchingContourFilter::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline double *vtkMarchingContourFilter::GetValues()
{
  return this->ContourValues->GetValues();
}

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkMarchingContourFilter::GetValues(double *contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkMarchingContourFilter::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

// Description:
// Get the number of contours in the list of contour values.
inline int vtkMarchingContourFilter::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkMarchingContourFilter::GenerateValues(int numContours,
                                                     double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkMarchingContourFilter::GenerateValues(int numContours,
                                                     double rangeStart,
                                                     double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
