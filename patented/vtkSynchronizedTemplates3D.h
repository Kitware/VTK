/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplates3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$



Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENT PENDING.

    Application of this software for commercial purposes requires 
    a license grant from Kitware. Contact:
        Ken Martin
        Kitware
        469 Clifton Corporate Parkway,
        Clifton Park, NY 12065
        Phone:1-518-371-3971 
    for more information.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_EXPORT vtkSynchronizedTemplates3D : public vtkPolyDataSource
{
public:
  static vtkSynchronizedTemplates3D *New();

  vtkTypeMacro(vtkSynchronizedTemplates3D,vtkPolyDataSource);
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

protected:
  vtkSynchronizedTemplates3D();
  ~vtkSynchronizedTemplates3D();
  vtkSynchronizedTemplates3D(const vtkSynchronizedTemplates3D&) {};
  void operator=(const vtkSynchronizedTemplates3D&) {};

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
  void InitializeOutput(int *ext,vtkPolyData *o);

private:
  //BTX
  friend VTK_EXPORT vtkKitwareContourFilter;
  //ETX
  
};







// template table.
//BTX

extern int VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_1[];
extern int VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_2[];

//ETX

#endif

