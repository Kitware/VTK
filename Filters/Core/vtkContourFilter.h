/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkContourFilter is a filter that takes as input any dataset and
// generates on output isosurfaces and/or isolines. The exact form
// of the output depends upon the dimensionality of the input data.
// Data consisting of 3D cells will generate isosurfaces, data
// consisting of 2D cells will generate isolines, and data with 1D
// or 0D cells will generate isopoints. Combinations of output type
// are possible if the input dimension is mixed.
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
// are not computed. Use vtkPolyDataNormals to compute the surface
// normals.

// .SECTION See Also
// vtkMarchingContourFilter vtkMarchingCubes vtkSliceCubes
// vtkMarchingSquares vtkImageMarchingCubes

#ifndef vtkContourFilter_h
#define vtkContourFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Needed for inline methods

class vtkIncrementalPointLocator;
class vtkScalarTree;
class vtkSynchronizedTemplates2D;
class vtkSynchronizedTemplates3D;
class vtkGridSynchronizedTemplates3D;
class vtkRectilinearSynchronizedTemplates;
class vtkCallbackCommand;

class VTKFILTERSCORE_EXPORT vtkContourFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkContourFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkContourFilter *New();

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
  // This setting defaults to On for vtkImageData, vtkRectilinearGrid,
  // vtkStructuredGrid, and vtkUnstructuredGrid inputs, and Off for all others.
  // This default behavior is to preserve the behavior of an older version of
  // this filter, which would ignore this setting for certain inputs.
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
  // Enable the use of a scalar tree to accelerate contour extraction.
  virtual void SetScalarTree(vtkScalarTree*);
  vtkGetObjectMacro(ScalarTree,vtkScalarTree);

  // Description:
  // Set / get a spatial locator for merging points. By default,
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is
  // specified. The locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Description:
  // Set/get which component of the scalar array to contour on; defaults to 0.
  // Currently this feature only works if the input is a vtkImageData.
  void SetArrayComponent( int );
  int  GetArrayComponent();


  // Description:
  // If this is enabled (by default), the output will be triangles
  // otherwise, the output will be the intersection polygon
  // WARNING: if the contour surface is not planar, the output
  // polygon will not be planar, which might be nice to look at but hard
  // to compute with downstream.
  vtkSetMacro(GenerateTriangles,int);
  vtkGetMacro(GenerateTriangles,int);
  vtkBooleanMacro(GenerateTriangles,int);

  // Description:
  // Set/get the desired precision for the output types. See the documentation
  // for the vtkAlgorithm::Precision enum for an explanation of the available
  // precision settings.
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;

protected:
  vtkContourFilter();
  ~vtkContourFilter();

  virtual void ReportReferences(vtkGarbageCollector*);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkContourValues *ContourValues;
  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  vtkIncrementalPointLocator *Locator;
  int UseScalarTree;
  vtkScalarTree *ScalarTree;
  int OutputPointsPrecision;
  int GenerateTriangles;

  vtkSynchronizedTemplates2D *SynchronizedTemplates2D;
  vtkSynchronizedTemplates3D *SynchronizedTemplates3D;
  vtkGridSynchronizedTemplates3D *GridSynchronizedTemplates;
  vtkRectilinearSynchronizedTemplates *RectilinearSynchronizedTemplates;
  vtkCallbackCommand *InternalProgressCallbackCommand;

  static void InternalProgressCallbackFunction(vtkObject *caller,
                                               unsigned long eid,
                                               void *clientData,
                                               void *callData);

private:
  vtkContourFilter(const vtkContourFilter&);  // Not implemented.
  void operator=(const vtkContourFilter&);  // Not implemented.
};

// Description:
// Set a particular contour value at contour number i. The index i ranges
// between 0<=i<NumberOfContours.
inline void vtkContourFilter::SetValue(int i, double value)
{this->ContourValues->SetValue(i,value);}

// Description:
// Get the ith contour value.
inline double vtkContourFilter::GetValue(int i)
{return this->ContourValues->GetValue(i);}

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline double *vtkContourFilter::GetValues()
{return this->ContourValues->GetValues();}

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkContourFilter::GetValues(double *contourValues)
{this->ContourValues->GetValues(contourValues);}

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkContourFilter::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);}

// Description:
// Get the number of contours in the list of contour values.
inline int vtkContourFilter::GetNumberOfContours()
{return this->ContourValues->GetNumberOfContours();}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkContourFilter::GenerateValues(int numContours, double range[2])
{this->ContourValues->GenerateValues(numContours, range);}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkContourFilter::GenerateValues(int numContours, double
                                             rangeStart, double rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}


#endif


