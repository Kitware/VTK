/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridSynchronizedTemplates3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGridSynchronizedTemplates3D - generate isosurface from structured grids

// .SECTION Description
// vtkGridSynchronizedTemplates3D is a 3D implementation of the synchronized
// template algorithm.

// .SECTION Caveats
// This filter is specialized to 3D grids.

// .SECTION See Also
// vtkContourFilter vtkSynchronizedTemplates3D

#ifndef __vtkGridSynchronizedTemplates3D_h
#define __vtkGridSynchronizedTemplates3D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkContourValues.h" // Because it passes all the calls to it

class vtkKitwareContourFilter;
class vtkStructuredGrid;

class VTKFILTERSCORE_EXPORT vtkGridSynchronizedTemplates3D : public vtkPolyDataAlgorithm
{
public:
  static vtkGridSynchronizedTemplates3D *New();
  vtkTypeMacro(vtkGridSynchronizedTemplates3D,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Because we delegate to vtkContourValues
  unsigned long int GetMTime();

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
  // If this is enabled (by default), the output will be triangles
  // otherwise, the output will be the intersection polygons
  vtkSetMacro(GenerateTriangles,int);
  vtkGetMacro(GenerateTriangles,int);
  vtkBooleanMacro(GenerateTriangles,int);

  // Description:
  // Set a particular contour value at contour number i. The index i ranges
  // between 0<=i<NumberOfContours.
  void SetValue(int i, double value) {this->ContourValues->SetValue(i,value);}

  // Description:
  // Get the ith contour value.
  double GetValue(int i) {return this->ContourValues->GetValue(i);}

  // Description:
  // Get a pointer to an array of contour values. There will be
  // GetNumberOfContours() values in the list.
  double *GetValues() {return this->ContourValues->GetValues();}

  // Description:
  // Fill a supplied list with contour values. There will be
  // GetNumberOfContours() values in the list. Make sure you allocate
  // enough memory to hold the list.
  void GetValues(double *contourValues) {
    this->ContourValues->GetValues(contourValues);}

  // Description:
  // Set the number of contours to place into the list. You only really
  // need to use this method to reduce list size. The method SetValue()
  // will automatically increase list size as needed.
  void SetNumberOfContours(int number) {
    this->ContourValues->SetNumberOfContours(number);}

  // Description:
  // Get the number of contours in the list of contour values.
  int GetNumberOfContours() {
    return this->ContourValues->GetNumberOfContours();}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double range[2]) {
    this->ContourValues->GenerateValues(numContours, range);}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
    {this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

  // Description:
  // Needed by templated functions.
  int *GetExecuteExtent() {return this->ExecuteExtent;}
  void ThreadedExecute(int *exExt, int threadId, vtkStructuredGrid *input,
                       vtkInformationVector **inVec,
                       vtkInformation *outInfo);

  // Description:
  // This filter will initiate streaming so that no piece requested
  // from the input will be larger than this value (KiloBytes).
  void SetInputMemoryLimit(long limit);

  // Description:
  // Set/get the desired precision for the output types. See the documentation
  // for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
  // the available precision settings.
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);

protected:
  vtkGridSynchronizedTemplates3D();
  ~vtkGridSynchronizedTemplates3D();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  int GenerateTriangles;

  vtkContourValues *ContourValues;

  int MinimumPieceSize[3];
  int ExecuteExtent[6];
  int OutputPointsPrecision;

private:
  vtkGridSynchronizedTemplates3D(const vtkGridSynchronizedTemplates3D&);  // Not implemented.
  void operator=(const vtkGridSynchronizedTemplates3D&);  // Not implemented.
};


#endif
