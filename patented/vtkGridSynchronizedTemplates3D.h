/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridSynchronizedTemplates3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$



Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

    THIS CLASS IS PATENT PENDING.

    Application of this software for commercial purposes requires 
    a license grant from Kitware. Contact:
        Ken Martin
        Kitware
        469 Clifton Corporate Parkway,
        Clifton Park, NY 12065
        Phone:1-518-371-3971 
    for more information.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

#include "vtkStructuredGridToPolyDataFilter.h"
#include "vtkContourValues.h"
#include "vtkMultiThreader.h"

class VTK_EXPORT vtkGridSynchronizedTemplates3D : public vtkStructuredGridToPolyDataFilter
{
public:
  static vtkGridSynchronizedTemplates3D *New() {
    return new vtkGridSynchronizedTemplates3D;};
  const char *GetClassName() {return "vtkGridSynchronizedTemplates3D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Because we delegate to vtkContourValues
  unsigned long int GetMTime();

  // Description:
  // Set/Get the computation of normals. Normal computation is failrly
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

  //-------- streaming stuff ------------

  // Description:
  // Needed by templated functions.
  int *GetExecuteExtent() {return this->ExecuteExtent;}
  int SplitExtent(int piece, int numPieces, int *ext);
  void ThreadedExecute(int *exExt, int threadId);

  // Description:
  // Part of the streaming protocall.  This method returns the number of chunks
  // required to meet the memory limit of the input.
  int GetNumberOfStreamDivisions();

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // This filter will initiate streaming so that no piece requested
  // from the input will be larger than this value (KiloBytes).
  void SetInputMemoryLimit(long limit);

protected:
  vtkGridSynchronizedTemplates3D();
  ~vtkGridSynchronizedTemplates3D();

  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  vtkContourValues *ContourValues;

  //------------ for streaming --------------
  // default execute method breaks apart the execution and 
  // calls StreamExecute multiple times..
  void Execute();
  void StreamExecuteStart();
  void StreamExecuteEnd();
  // Description:
  // Updates output: EstimatedWholeMemorySize
  void ExecuteInformation();

  // Description:
  // Part of the streaming protocall.  This method sets the UpdateExtent of the 
  // input and sets the ExecuteExtent for this filter.
  int ComputeDivisionExtents(vtkDataObject *output, int idx, int numPieces);

  // Stuff for multithreading
  // this may be pushed into vtkFilter superclass
  int ExecuteExtent[6];
  int MinimumPieceSize[3];
  int NumberOfThreads;
  vtkMultiThreader *Threader;
  // temporary outputs
  vtkPolyData *Threads[VTK_MAX_THREADS];
  void InitializeOutput(int *ext,vtkPolyData *o);

};


#endif

