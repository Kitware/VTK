/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplates3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSynchronizedTemplates3D - generate isosurface from structured points

// .SECTION Description
// vtkSynchronizedTemplates3D is a 3D implementation of the synchronized 
// template algorithm. Note that vtkContourFilter will automatically
// use this class when appropriate if vtk was built with patents.

// .SECTION Caveats
// This filter is specialized to 3D images (aka volumes).

// .SECTION See Also
// vtkContourFilter vtkSynchronizedTemplates2D

#ifndef __vtkSynchronizedTemplates3D_h
#define __vtkSynchronizedTemplates3D_h

#include "vtkPolyDataSource.h"
#include "vtkImageData.h"
#include "vtkContourValues.h"
#include "vtkMultiThreader.h"
#include "vtkKitwareContourFilter.h"

class VTK_PATENTED_EXPORT vtkSynchronizedTemplates3D : public vtkPolyDataSource
{
public:
  static vtkSynchronizedTemplates3D *New();

  vtkTypeRevisionMacro(vtkSynchronizedTemplates3D,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the source for the scalar data to contour.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
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
  void SetValue(int i, float value) {this->ContourValues->SetValue(i,value);}

  // Description:
  // Get the ith contour value.
  float GetValue(int i) {return this->ContourValues->GetValue(i);}

  // Description:
  // Get a pointer to an array of contour values. There will be
  // GetNumberOfContours() values in the list.
  float *GetValues() {return this->ContourValues->GetValues();}

  // Description:
  // Fill a supplied list with contour values. There will be
  // GetNumberOfContours() values in the list. Make sure you allocate
  // enough memory to hold the list.
  void GetValues(float *contourValues) {
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
  void GenerateValues(int numContours, float range[2]) {
    this->ContourValues->GenerateValues(numContours, range);}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, float rangeStart, float rangeEnd)
    {this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

  // Description:
  // Needed by templated functions.
  int *GetExecuteExtent() {return this->ExecuteExtent;}
  void ThreadedExecute(vtkImageData *data, int *exExt, int threadId);

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Determines the chunk size fro streaming.  This filter will act like a
  // collector: ask for many input pieces, but generate one output.  Limit is
  // in KBytes
  void SetInputMemoryLimit(unsigned long limit);
  unsigned long GetInputMemoryLimit();  

  // Description:
  // If you want to contour by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}
  
protected:
  vtkSynchronizedTemplates3D();
  ~vtkSynchronizedTemplates3D();

  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  vtkContourValues *ContourValues;

  void Execute();
  void ExecuteInformation();

  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  int ExecuteExtent[6];

  int NumberOfThreads;
  vtkMultiThreader *Threader;
  // temporary outputs
  vtkPolyData *Threads[VTK_MAX_THREADS];

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

private:
  //BTX
  friend class VTK_PATENTED_EXPORT vtkKitwareContourFilter;
  //ETX
  
private:
  vtkSynchronizedTemplates3D(const vtkSynchronizedTemplates3D&);  // Not implemented.
  void operator=(const vtkSynchronizedTemplates3D&);  // Not implemented.
};







// template table.
//BTX

extern int VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_1[];
extern int VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_2[];

//ETX

#endif

