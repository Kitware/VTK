/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearSynchronizedTemplates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRectilinearSynchronizedTemplates - generate isosurface from rectilinear grid

// .SECTION Description
// vtkRectilinearSynchronizedTemplates is a 3D implementation (for rectilinear
// grids) of the synchronized template algorithm. Note that vtkContourFilter
// will automatically use this class when appropriate.

// .SECTION Caveats
// This filter is specialized to rectilinear grids.

// .SECTION See Also
// vtkContourFilter vtkSynchronizedTemplates2D vtkSynchronizedTemplates3D

#ifndef __vtkRectilinearSynchronizedTemplates_h
#define __vtkRectilinearSynchronizedTemplates_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkContourValues.h" // Passes calls through

class vtkRectilinearGrid;
class vtkDataArray;

class VTKFILTERSCORE_EXPORT vtkRectilinearSynchronizedTemplates : public vtkPolyDataAlgorithm
{
public:
  static vtkRectilinearSynchronizedTemplates *New();

  vtkTypeMacro(vtkRectilinearSynchronizedTemplates,vtkPolyDataAlgorithm);
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

  // Description:
  // Set/get which component of the scalar array to contour on; defaults to 0.
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);

 // Description:
  // If this is enabled (by default), the output will be triangles
  // otherwise, the output will be the intersection polygons
  vtkSetMacro(GenerateTriangles,int);
  vtkGetMacro(GenerateTriangles,int);
  vtkBooleanMacro(GenerateTriangles,int);

  // Description:
  // Compute the spacing between this point and its 6 neighbors.  This method
  // needs to be public so it can be accessed from a templated function.
  void ComputeSpacing(vtkRectilinearGrid *data, int i, int j, int k,
                      int extent[6], double spacing[6]);

protected:
  vtkRectilinearSynchronizedTemplates();
  ~vtkRectilinearSynchronizedTemplates();

  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  int GenerateTriangles;

  vtkContourValues *ContourValues;

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int ExecuteExtent[6];

  int ArrayComponent;

  void* GetScalarsForExtent(vtkDataArray *array, int extent[6],
                            vtkRectilinearGrid *input);

private:
  vtkRectilinearSynchronizedTemplates(const vtkRectilinearSynchronizedTemplates&);  // Not implemented.
  void operator=(const vtkRectilinearSynchronizedTemplates&);  // Not implemented.
};

// template table.
//BTX

extern int VTK_RECTILINEAR_SYNCHONIZED_TEMPLATES_TABLE_1[];
extern int VTK_RECTILINEAR_SYNCHONIZED_TEMPLATES_TABLE_2[];

//ETX

#endif
